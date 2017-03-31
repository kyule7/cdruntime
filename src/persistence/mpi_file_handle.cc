#include "mpi_file_handle.h"
#include "packer_prof.h"
#include <sys/time.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
//#include <linux/fs.h>
//#include <unistd.h>
//#include <errno.h>
//#include "cd_file_handle.h"
#include "buffer_consumer_interface.h"
#include <string.h> // strcpy

using namespace packer;
//packer::Time packer::time_mpiio_write("mpiio_write"); 
//packer::Time packer::time_mpiio_read("mpiio_read"); 
//packer::Time packer::time_mpiio_seek("mpiio_seek"); 

MPIFileHandle *MPIFileHandle::fh_ = NULL;
char err_str[MPI_MAX_ERR_STR];
int err_len = 0, err_class = 0;

inline void CheckError(int err) 
{
  if(err != MPI_SUCCESS) {
    MPI_Error_class(err, &err_class);
    MPI_Error_string(err, err_str, &err_len);
    ERROR_MESSAGE_PACKER("%s\n", err_str);
    assert(0);
  }
}
//MPIFileHandle::MPIFileHandle(const MPI_Comm &comm, const char *filepath) : FileHandle(filepath) 
void MPIFileHandle::Init(const MPI_Comm &comm, const char *filepath)
{
  int rank=0, commsize=0;
  struct timeval time;
  gettimeofday(&time, NULL);

  MPI_Comm_dup(comm, &comm_);
  MPI_Comm_rank(comm_, &rank);
  MPI_Comm_size(comm_, &commsize);
  // For error handling,
  MPI_Errhandler_set(comm_, MPI_ERRORS_RETURN);

  char full_filename[128];
  char base_filename[128];
  char *base_filepath = getenv("CD_BASE_FILEPATH");
  if(base_filepath != NULL) {
    strcpy(base_filename, base_filepath);
  } else {
    strcpy(base_filename, DEFAULT_BASE_FILEPATH);
  }

  //viewsize_ = DEFAULT_VIEWSIZE;
  viewsize_ = 0;
  if(viewsize_ != 0) { // N-to-1 file write
    if(rank == 0) {
      sprintf(full_filename, "%s/%s.%ld.%ld", base_filename, filepath, time.tv_sec % 100, time.tv_usec % 100);
      PMPI_Bcast(&full_filename, 128, MPI_CHAR, 0, comm_);
    } else {
      PMPI_Bcast(&full_filename, 128, MPI_CHAR, 0, comm_);
    }
    CheckError( MPI_File_open(comm_, full_filename, MPI_MODE_CREATE|MPI_MODE_RDWR, MPI_INFO_NULL, &fdesc_) );
  } else { // N-to-N file write
    sprintf(full_filename, "%s/%s.%d", base_filename, filepath, rank);//time.tv_sec % 100, time.tv_usec % 100);
    CheckError( MPI_File_open(comm_, full_filename, MPI_MODE_CREATE|MPI_MODE_RDWR, MPI_INFO_NULL, &fdesc_) );
//  fdesc_ = open(full_filename, O_CREAT | O_RDWR | O_DIRECT, S_IRUSR | S_IWUSR);
//  MPI_Aint lb = 0, extent = (commsize)*viewsize_;
  }

  if(viewsize_ != 0) {
    // Set up datatype for file view
    MPI_Datatype contig = MPI_BYTE;
  //  MPI_Type_contiguous(viewsize_, contig, &ftype_);
    
  
    MPI_Type_contiguous(viewsize_, MPI_BYTE, &contig);
    MPI_Type_create_resized(contig, 0, (commsize)*viewsize_, &ftype_);
  
  //  MPI_Type_vector(2, viewsize_, viewsize_*commsize, MPI_BYTE, &ftype_);
  
    MPI_Type_commit(&ftype_);
  
    MPI_Info info = MPI_INFO_NULL;
    uint64_t view_offset = rank * viewsize_;

    CheckError( MPI_File_set_view(fdesc_, view_offset, MPI_BYTE, ftype_, "native", info) );
    MYDBG("[%d/%d] Opened file%d : %s, viewsize:%lu, view_offset:%lu, chunk:%lu\n", 
        rank, commsize, rank, full_filename, viewsize_, view_offset, (commsize)*viewsize_);
  } else {
    MYDBG("[%d/%d] Opened file%d : %s, viewsize:%lu, chunk:%lu\n", 
        rank, commsize, rank, full_filename, viewsize_, (commsize)*viewsize_);
  }
  
}

MPIFileHandle::~MPIFileHandle(void) 
{
  //getchar();
  MPI_File_close(&fdesc_);
  MPI_Comm_free(&comm_);
  fdesc_ = MPI_FILE_NULL;
  comm_ = MPI_COMM_NULL;
  fh_ = NULL;
}

//FileHandle *MPIFileHandle::Get(const MPI_Comm &comm, const char *filepath) 
FileHandle *MPIFileHandle::Get(MPI_Comm comm, const char *filepath)
{
  if(fh_ == NULL) {
    if(filepath == NULL) {
      fh_ = new MPIFileHandle; 
    } else {
      fh_ = new MPIFileHandle(comm, filepath); 
    }
  }
  return dynamic_cast<FileHandle *>(fh_);
}

void MPIFileHandle::Close(void) 
{
  delete fh_;
  fh_ = NULL;
}

CDErrType MPIFileHandle::Write(int64_t offset, char *src, int64_t len, int64_t inc)
{
  CDErrType ferr = kOK;
  PACKER_ASSERT(offset >= 0);
  PACKER_ASSERT(len > 0);
  MPI_Status status;
//  CheckError( MPI_File_write(fdesc_, src, len, MPI_BYTE, &status) );
  time_mpiio_write.Begin();
  CheckError( MPI_File_write_at(fdesc_, offset, src, len, MPI_BYTE, &status) );
  time_mpiio_write.End(len);
  offset_ = (inc >= 0)? offset_ + inc : offset_ + len;
  MYDBG("[%d] MPI Write offset:%ld, %ld, src:%p, len:%ld\n", fdesc_, offset, offset_, src, len);
  return ferr;
}

char *MPIFileHandle::Read(int64_t len, int64_t offset)
{
  MYDBG("%ld (file offset:%ld)\n", len, offset);
  //void *ret_ptr = new char[len];
  void *ret_ptr = NULL;
  posix_memalign(&ret_ptr, CHUNK_ALIGNMENT, len);
  Read(ret_ptr, len, offset);
  return (char *)ret_ptr;
}

CDErrType MPIFileHandle::Read(void *dst, int64_t len, int64_t offset)
{
  MYDBG("%ld (file offset:%ld)\n", len, offset);
  PACKER_ASSERT(offset >= 0);
  PACKER_ASSERT(len >= 0);
  CDErrType ret = kOK;
  MPI_Status status;
 // int rank;
 // MPI_Comm_rank(comm_, &rank);
//  uint64_t view_offset = rank * viewsize_;
  BufferLock();
//  MPI_Info info;
//  CheckError( MPI_File_set_view(fdesc_, view_offset, MPI_BYTE, ftype_, "native", MPI_INFO_NULL) );
  time_mpiio_read.Begin();
  CheckError( MPI_File_read_at(fdesc_, offset, dst, len, MPI_BYTE, &status) );
  time_mpiio_read.End(len);
  BufferUnlock();
  return ret;
}

void MPIFileHandle::FileSync(void)
{
  MPI_File_sync(fdesc_);
}

void MPIFileHandle::Truncate(uint64_t newsize)
{
  // STUB: No support for MPI file truncate
}

int64_t MPIFileHandle::GetFileSize(void)
{
//  struct stat buf;
//  fstat(fdesc_, &buf);
//  off_t fsize = buf.st_size;
  return offset_;
  //return fsize - sizeof(MagicStore);
}

uint32_t MPIFileHandle::GetBlkSize(void)
{
  //size_t blockSize;
//  unsigned long blockSize;
//  int ret = ioctl(fdesc_,BLKBSZGET/* BLKSSZGET*/, &blockSize);
//  if( ret != 0 ) { 
//    perror("ioctl:");
//    ERROR_MESSAGE_PACKER("ioctl error: %d, fdesc:%d, blksize:%lu\n", ret, fdesc_, blockSize);
//  }
  return CHUNK_ALIGNMENT;
}

