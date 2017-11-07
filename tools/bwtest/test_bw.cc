#include "mpitimer.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
//#define _MPI_TEST
#define DEFAULT_FILEPATH "./file.out"
#define MAX_CHUNKSIZE 1073741824 // 1GB
//#define MAX_CHUNKSIZE 134217728 // 1GB
static char err_str[512];
static int err_len = 0, err_class = 0;
static int myRank = 0;
static int numRank = 0;
static int viewsize = 0;
//static int viewsize = DEFAULT_VIEWSIZE;

static inline 
void CheckError(int err) 
{
  if(err != MPI_SUCCESS) {
    MPI_Error_class(err, &err_class);
    MPI_Error_string(err, err_str, &err_len);
    printf("%s\n", err_str);
    assert(0);
  }
}

#ifdef _MPI_TEST
typedef MPI_File FileType;
#else
typedef int FileType;
#endif

void OpenFile(FileType *fdesc, const char *filepath)
{
#ifdef _MPI_TEST
  // For error handling
  MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  if(viewsize == 0) { // N-to-N file write
    struct timeval time;
    gettimeofday(&time, NULL);
    char full_filepath[256];
    sprintf(full_filepath, "%s.mpi.%d.%d.%d", filepath, time.tv_sec % 100, time.tv_usec % 100, myRank);
    CheckError( MPI_File_open(MPI_COMM_SELF, full_filepath, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, fdesc) );
  } else { // N-to-1 file write
    struct timeval time;
    gettimeofday(&time, NULL);
    char full_filepath[256];
    sprintf(full_filepath, "%s.mpi.%d.%d", filepath, time.tv_sec % 100, time.tv_usec % 100);
    CheckError( MPI_File_open(MPI_COMM_WORLD, full_filepath, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, fdesc) );

    MPI_Datatype ftype;
    // Set up datatype for file view
    MPI_Datatype contig = MPI_BYTE;
  //  MPI_Type_contiguous(viewsize, contig, &ftype);
    
    MPI_Type_contiguous(viewsize, MPI_BYTE, &contig);
    MPI_Type_create_resized(contig, 0, (numRank)*viewsize, &ftype);
  
  //  MPI_Type_vector(2, viewsize, viewsize*numRank, MPI_BYTE, &ftype);
  
    MPI_Type_commit(&ftype);
    MPI_Info info = MPI_INFO_NULL;
    uint64_t view_offset = myRank * viewsize;
    CheckError( MPI_File_set_view(*fdesc, view_offset, MPI_BYTE, ftype, "native", info) );
  }
  printf("[%d/%d] Opened file : %s, viewsize:%lu, chunk:%lu\n", 
        myRank, numRank, filepath, viewsize, (numRank)*viewsize);
#else
  struct timeval time;
  gettimeofday(&time, NULL);
  char full_filepath[256];
  sprintf(full_filepath, "%s.posix.%d.%d.%d", filepath, time.tv_sec % 100, time.tv_usec % 100, myRank);
  *fdesc = open(full_filepath, O_CREAT | O_RDWR | O_DIRECT, S_IRUSR | S_IWUSR);
#endif 
}

void CloseFile(FileType *fdesc) {
#ifdef _MPI_TEST
  MPI_File_close(fdesc);
  *fdesc = MPI_FILE_NULL;
#else
  close(*fdesc);
  *fdesc = -1;
#endif

}

void WriteFile(FileType *fdesc, uint64_t chunksize, int iter)
{

  MPITimer mpi_timer;
  InitializeMPITimer(&mpi_timer, 1);
#ifdef _MPI_TEST
  MPI_Status status;
#endif
  for(; chunksize <= MAX_CHUNKSIZE; chunksize *= 4) {
    uint64_t offset = 0;
#ifdef _MPI_TEST
    uint32_t *src = (uint32_t *)malloc(chunksize);
#else
    void *tmp = NULL;
    int ret = posix_memalign(&tmp, 1048576, chunksize);
    uint32_t *src = (uint32_t *)tmp;
    assert(chunksize % 1048576 == 0);
    if(ret != 0) { perror("posix_memalign"); }
#endif
    int numElem = chunksize / sizeof(uint32_t);
    
    for(int i=0; i<numElem; i++) {
      src[i] = i % 64;
    }
  
    for(int i=0; i<iter; i++, offset+=chunksize) {
      BeginTimer(&mpi_timer);
#ifdef _MPI_TEST
      CheckError( MPI_File_write_at(*fdesc, offset, src, chunksize, MPI_BYTE, &status) );
      CheckError( MPI_File_sync(*fdesc) );
#else
      ssize_t written_size = write(*fdesc, src, chunksize);
      if((int64_t)written_size != chunksize) {
        perror("write:");
        printf("Error occurred while writing buffer contents to file: %d %ld == %ld\n", 
            *fdesc, written_size, chunksize);
        assert(0);
      }

#endif
      EndTimer(&mpi_timer, 0);
      AdvanceTimer(&mpi_timer);
    }
  
    GatherTimer(&mpi_timer, iter, chunksize);
    ReInitMPITimer(&mpi_timer);
    free(src);
  }
  FinalizeMPITimer(&mpi_timer);
}

int main(int argc, char *argv[]) {
  int iter = 100;
  int chunksize = 33554432;
  int inc = chunksize;
  char filepath[256];
  memset(filepath, '\0', 256);
  
  if(argc == 1) {
    strcpy(filepath, DEFAULT_FILEPATH);
  } else if(argc == 2) {
    strcpy(filepath, argv[1]);
  } else if(argc == 3) {
    strcpy(filepath, argv[1]);
    iter = atoi(argv[2]);
  } else if(argc == 4) {
    strcpy(filepath, argv[1]);
    iter = atoi(argv[2]);
    chunksize = atoi(argv[3]);
  } else if(argc == 5) {
    strcpy(filepath, argv[1]);
    iter = atoi(argv[2]);
    chunksize = atoi(argv[3]);
    inc = atoi(argv[4]);
  } 
  printf("Iteration:%d, Chunksize:%d, Inc:%d, Filepath: %s\n",
      iter, chunksize, inc, filepath);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  MPI_Comm_size(MPI_COMM_WORLD, &numRank);

  FileType fdesc;
  OpenFile(&fdesc, filepath);

  WriteFile(&fdesc, chunksize, iter);

  CloseFile(&fdesc);

  MPI_Finalize();
}
