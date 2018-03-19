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
#include <unistd.h>
#include <string.h> // strcpy
#include <math.h>
using namespace packer;
//packer::Time packer::time_mpiio_write("mpiio_write"); 
//packer::Time packer::time_mpiio_read("mpiio_read"); 
//packer::Time packer::time_mpiio_seek("mpiio_seek"); 

MPIFileHandle *MPIFileHandle::fh_ = NULL;
LibcFileHandle *LibcFileHandle::fh_ = NULL;
char err_str[MPI_MAX_ERR_STR];
int err_len = 0, err_class = 0;
//int packer::packerTaskID = 0;
//int packer::packerTaskSize = 0;

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
  viewsize_ = 0;
  if(viewsize_ != 0) {
    MPI_Comm_dup(comm, &comm_);
  } else {
    MPI_Comm_dup(MPI_COMM_SELF, &comm_);
  }
  MPI_Comm_dup(comm, &comm_);
  MPI_Comm_rank(comm_, &rank);
  MPI_Comm_size(comm_, &commsize);
  packerTaskID = rank;
  packerTaskSize = commsize;
  // For error handling,
  MPI_Errhandler_set(comm_, MPI_ERRORS_RETURN);

  char base_filename[MAX_FILEPATH_SIZE];
  if(packerTaskID == 0) {
    char *base_filepath = getenv("CD_BASE_FILEPATH");
    if(base_filepath != NULL) {
      sprintf(base_filename, "%s-%ld-%ld", 
          base_filepath, time.tv_sec % 100, time.tv_usec % 100);
      //strcpy(base_filename, base_filepath);
    } else {
      sprintf(base_filename, "%s-%ld-%ld", 
          DEFAULT_BASE_FILEPATH "/global" , time.tv_sec % 100, time.tv_usec % 100);
      //strcpy(base_filename, DEFAULT_BASE_FILEPATH "/global");
    }
  }
  PMPI_Bcast(base_filename, MAX_FILEPATH_SIZE, MPI_CHAR, 0, comm_);
  if(packerTaskID == 0) {
    MakeFileDir(base_filename);
  }
  //viewsize_ = DEFAULT_VIEWSIZE;
  char full_filename[MAX_FILEPATH_SIZE];
  viewsize_ = 0;
  if(viewsize_ != 0) { // N-to-1 file write
    if(rank == 0) {
      sprintf(full_filename, "%s/%s.%ld.%ld", base_filename, filepath, time.tv_sec % 100, time.tv_usec % 100);
      PMPI_Bcast(&full_filename, MAX_FILEPATH_SIZE, MPI_CHAR, 0, comm_);
    } else {
      PMPI_Bcast(&full_filename, MAX_FILEPATH_SIZE, MPI_CHAR, 0, comm_);
    }
    CheckError( MPI_File_open(comm_, full_filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fdesc_) );
  } else { // N-to-N file write
    sprintf(full_filename, "%s/%s.%d", base_filename, filepath, rank);//time.tv_sec % 100, time.tv_usec % 100);
    //CheckError( MPI_File_open(comm_, full_filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fdesc_) );
    CheckError( MPI_File_open(MPI_COMM_SELF, full_filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fdesc_) );
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
  
  filepath_ = full_filename;
}

MPIFileHandle::~MPIFileHandle(void) 
{
  //getchar();
  MPI_File_close(&fdesc_);
  MPI_Comm_free(&comm_);
  unlink(filepath_.c_str());
  fdesc_ = MPI_FILE_NULL;
  comm_ = MPI_COMM_NULL;
  fh_ = NULL;
}

//FileHandle *MPIFileHandle::Get(const MPI_Comm &comm, const char *filepath) 
FileHandle *MPIFileHandle::Get(MPI_Comm comm, const char *filepath)
{
  if(fh_ == NULL) {
    if(strcmp(filepath, "") == 0) {
      fh_ = new MPIFileHandle; 
    } else {
      fh_ = new MPIFileHandle(comm, filepath); 
    }
  }
  return dynamic_cast<FileHandle *>(fh_);
}

void MPIFileHandle::Close(void) 
{
  if(fh_ != NULL) {
    delete fh_;
  }
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
//  CheckError( MPI_File_sync(fdesc_) );
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
  PACKER_ASSERT_STR(offset >= 0 && len >= 0, "MPIFileHandle::Read Error: %ld (file offset:%ld)\n", len, offset);
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

FileHandle *LibcFileHandle::Get(MPI_Comm comm, const char *filepath)
{
  if(fh_ == NULL) {
    if(strcmp(filepath, "") == 0) {
      fh_ = new LibcFileHandle; 
    } else {
      fh_ = new LibcFileHandle(comm, filepath); 
    }
  }
  return dynamic_cast<FileHandle *>(fh_);
}

void LibcFileHandle::Close(void) 
{
  if(fh_ != NULL) {
    delete fh_;
  }
}

void Time::GatherBW(void) 
{
  const int prof_elems = 9;
  double sendbuf[9] = { 
                         packer::time_copy.GetBW(), 
                         packer::time_write.GetBW(), 
                         packer::time_read.GetBW(), 
                         packer::time_posix_write.GetBW(), 
                         packer::time_posix_read.GetBW(), 
                         packer::time_posix_seek.GetBW(), 
                         packer::time_mpiio_write.GetBW(), 
                         packer::time_mpiio_read.GetBW(), 
                         packer::time_mpiio_seek.GetBW()
  };
  double recvbufmax[prof_elems] = { 0, 0, 0, 0 };
  double recvbufmin[prof_elems] = { 0, 0, 0, 0 };
  double recvbufavg[prof_elems] = { 0, 0, 0, 0 };
  double recvbufstd[prof_elems] = { 0, 0, 0, 0 };
  double sendbufstd[prof_elems];
  for(int i=0; i<prof_elems; i++) { sendbufstd[i] = sendbuf[i] * sendbuf[i]; }
  MPI_Reduce(sendbuf, recvbufmax, prof_elems, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(sendbuf, recvbufmin, prof_elems, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(sendbuf, recvbufavg, prof_elems, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(sendbufstd, recvbufstd, prof_elems, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  for(int i=0; i<prof_elems; i++) { 
    recvbufavg[i] /= packerTaskSize; // avg
    recvbufstd[i] /= packerTaskSize; // avg of 2nd momentum
    recvbufstd[i] =  sqrt(recvbufstd[i] - recvbufavg[i] * recvbufavg[i]);
  }
  packer::time_copy.UpdateBW(       recvbufavg[0], recvbufstd[0], recvbufmin[0], recvbufmax[0]); 
  packer::time_write.UpdateBW(      recvbufavg[1], recvbufstd[1], recvbufmin[1], recvbufmax[1]); 
  packer::time_read.UpdateBW(       recvbufavg[2], recvbufstd[2], recvbufmin[2], recvbufmax[2]); 
  packer::time_posix_write.UpdateBW(recvbufavg[3], recvbufstd[3], recvbufmin[3], recvbufmax[3]); 
  packer::time_posix_read.UpdateBW( recvbufavg[4], recvbufstd[4], recvbufmin[4], recvbufmax[4]); 
  packer::time_posix_seek.UpdateBW( recvbufavg[5], recvbufstd[5], recvbufmin[5], recvbufmax[5]); 
  packer::time_mpiio_write.UpdateBW(recvbufavg[6], recvbufstd[6], recvbufmin[6], recvbufmax[6]); 
  packer::time_mpiio_read.UpdateBW( recvbufavg[7], recvbufstd[7], recvbufmin[7], recvbufmax[7]); 
  packer::time_mpiio_seek.UpdateBW( recvbufavg[8], recvbufstd[8], recvbufmin[8], recvbufmax[8]);
}
