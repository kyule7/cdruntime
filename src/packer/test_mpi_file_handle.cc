#include "mpi_file_handle.h"
#include "test_common.h"
#include "buffer_consumer_interface.h"
#include "ckpt_time_hist.h"
#include <unistd.h>
using namespace cd;

FILE *dbgfp = NULL;
#ifdef _DEBUG_ENABLED
std::map<pthread_t, unsigned int> tid2str;
int indent_cnt = 0; 
#endif
pthread_cond_t  cd::full  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cd::empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cd::mutex = PTHREAD_MUTEX_INITIALIZER;

Singleton st;
uint32_t ckpt_id = 0;

class TestPosixFileHandle : public PosixFileHandle {
public:
  TestPosixFileHandle(void) : PosixFileHandle(DEFAULT_FILEPATH_POSIX) {
  
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    char dbgfile[32];
    sprintf(dbgfile, "/scratch/03341/kyushick/dbgfile.%d", rank);
    dbgfp = fopen(dbgfile, "w");
  }
  virtual ~TestPosixFileHandle() {
    fclose(dbgfp);
  }

  //void Test1(uint32_t elemsize=262144, uint32_t chunksize=1024) {
  void Test1(uint32_t elemsize=131072, uint32_t chunksize=1024) {
    
    printf("\n\n\n=================== %s ==(%u,%u)==================\n\n", __func__, elemsize, chunksize);
    uint64_t totsize = elemsize * chunksize;
    uint64_t len_in_byte = totsize * sizeof(uint32_t);
    void *ptr = NULL;
    posix_memalign(&ptr, 512, len_in_byte);
    uint32_t *myData = (uint32_t *)ptr;
    //uint32_t *myData = new uint32_t[totsize];
    
    int rank = 0, commsize = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &commsize);
  
    fprintf(dbgfp, "[%d/%d] elemsize:%u, chunksize:%u\n", 
        rank, commsize, elemsize, chunksize);
    
  
    for(uint32_t cnt=0; cnt<1; cnt++) {
      // Generate data
      for(uint64_t i=0; i<totsize; i++) {
        myData[i] = i + totsize*rank;
      }
  
      int offset = 0;
  
      MPI_Barrier(MPI_COMM_WORLD);
      st.BeginTimer();
      Write(offset, (char *)myData, len_in_byte);
      FileSync();
      st.EndTimerPhase(0, ckpt_id);
      st.FinishTimer(ckpt_id);
      ckpt_id++;
  
      // Mimic some computation
      sleep(3);
  
      // This is for correctness
      char *preserved = Read(len_in_byte, offset);
      //offset+=len_in_byte;
  
      if(CompareResult((char *)myData, preserved, len_in_byte) == 0) {
        printf("\n\n#################### Success ## %lu ###################\n\n", GetFileSize()); 
        //getchar();
      } else {
        printf("Failed\n"); assert(0);
      }
      free(preserved);
  
    }
  
    printf("Before delete\n");
    delete myData;
    printf("After  delete\n");
  }

};

class TestMPIFileHandle : public MPIFileHandle {
public:
  TestMPIFileHandle(void) {
  
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    char dbgfile[32];
    sprintf(dbgfile, "/scratch/03341/kyushick/dbgfile.%d", rank);
    dbgfp = fopen(dbgfile, "w");
  }
  virtual ~TestMPIFileHandle() {
    fclose(dbgfp);
  }

  void Test1(uint32_t elemsize=131072, uint32_t chunksize=1024) {
  //void Test1(uint32_t elemsize=262144, uint32_t chunksize=1024) {
    
    printf("\n\n\n=================== %s ==(%u,%u)==================\n\n", __func__, elemsize, chunksize);
    uint64_t totsize = elemsize * chunksize;
    uint64_t len_in_byte = totsize * sizeof(uint32_t);
    void *ptr = NULL;
    posix_memalign(&ptr, 512, len_in_byte);
    uint32_t *myData = (uint32_t *)ptr;
    //uint32_t *myData = new uint32_t[totsize];
    
    int rank = 0, commsize = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &commsize);
  
    fprintf(dbgfp, "[%d/%d] viewsize:%lu, view_offset:%lu, chunk:%lu\n", 
        rank, commsize, viewsize_, rank * viewsize_, (commsize)*viewsize_);
    
  
    for(uint32_t cnt=0; cnt<64; cnt++) {
      // Generate data
      for(uint64_t i=0; i<totsize; i++) {
        myData[i] = i + totsize*rank;
      }
  
      int offset = 0;
  
      MPI_Barrier(comm_);
      st.BeginTimer();
      Write(offset, (char *)myData, len_in_byte);
      FileSync();
      st.EndTimerPhase(0, ckpt_id);
      st.FinishTimer(ckpt_id);
      ckpt_id++;
  
      // Mimic some computation
      sleep(3);
  
      // This is for correctness
      char *preserved = Read(len_in_byte, offset);
      //offset+=len_in_byte;
  
      if(CompareResult((char *)myData, preserved, len_in_byte) == 0) {
        printf("\n\n#################### Success ## %lu ###################\n\n", GetFileSize()); 
        //getchar();
      } else {
        printf("Failed\n"); assert(0);
      }
      free(preserved);
  
    }
  
    printf("Before delete\n");
    delete myData;
    printf("After  delete\n");
  }

};

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  int rank=0, commsize=0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &commsize);

  st.Initialize(rank, commsize);

  TestMPIFileHandle *filehandle = new TestMPIFileHandle;
//  TestPosixFileHandle *filehandle = new TestPosixFileHandle;

  filehandle->Test1();
  delete filehandle;

  st.Finalize();

  MPI_Finalize();
  return 0;
}
