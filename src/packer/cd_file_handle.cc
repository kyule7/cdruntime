#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <unistd.h>
#include <errno.h>
#include "cd_file_handle.h"
#include "buffer_consumer_interface.h"
#include "string.h"
using namespace cd;

PosixFileHandle *PosixFileHandle::fh_ = NULL;

PosixFileHandle::PosixFileHandle(const char *filepath) : FileHandle(filepath) 
{
  struct timeval time;
  gettimeofday(&time, NULL);
  char full_filename[128];
  char base_filename[128];
  char *base_filepath = getenv("CD_BASE_FILEPATH");
  if(base_filepath != NULL) {
    strcpy(base_filename, base_filepath);
  } else {
    strcpy(base_filename, DEFAULT_BASE_FILEPATH);
  }
  sprintf(full_filename, "%s/%s.%ld.%ld", base_filepath, filepath, time.tv_sec, time.tv_usec);
  fdesc_ = open(full_filename, O_CREAT | O_RDWR | O_DIRECT, S_IRUSR | S_IWUSR);
  MYDBG("Opened file : %s\n", full_filename);
  offset_ = 0;
  fh_ = this;
}

PosixFileHandle::~PosixFileHandle(void) 
{
  close(fdesc_);
  fdesc_ = 0;
  fh_ = NULL;
}

FileHandle *PosixFileHandle::Get(const char *filepath) 
{
  if(fh_ == NULL) {
    if(filepath == NULL) {
      fh_ = new PosixFileHandle; 
    } else {
      fh_ = new PosixFileHandle(filepath); 
    }
  }
  return dynamic_cast<FileHandle *>(fh_);
}

void PosixFileHandle::Close(void) 
{
  delete fh_;
  fh_ = NULL;
}

CDErrType PosixFileHandle::Write(uint64_t offset, char *src, uint64_t len)
{
  CDErrType ferr = kOK;
  off_t ret = lseek(fdesc_, offset, SEEK_SET);

  MYDBG("write (%d): %p (%lu) at file offset:%lu (== %lu)\n", 
         fdesc_, src, len, offset, ret);
  //getchar();
  ssize_t written_size = write(fdesc_, src, len);
  offset_ += len;

  // Error Check
  if(ret < 0) {
    perror("write:");
    ferr = kErrorFileSeek;
    ERROR_MESSAGE("Error occurred while seeking file:%ld\n", ret);
  }

  if((uint64_t)written_size != len) {
    perror("write:");
    ferr = kErrorFileWrite;
    ERROR_MESSAGE("Error occurred while writing buffer contents to file: %d %ld\n", fdesc_, written_size);
  }

  return ferr;
}

char *PosixFileHandle::Read(uint64_t len, uint64_t offset)
{
  MYDBG("%lu (file offset:%lu)\n", len, offset);
  //void *ret_ptr = new char[len];
  void *ret_ptr = NULL;
  posix_memalign(&ret_ptr, CHUNK_ALIGNMENT, len);
  ReadTo(ret_ptr, len, offset);
  return (char *)ret_ptr;
}

char *PosixFileHandle::ReadTo(void *dst, uint64_t len, uint64_t offset)
{
  MYDBG("%lu (file offset:%lu)\n", len, offset);
  if(offset != 0) {
    off_t ret = lseek(fdesc_, offset, SEEK_SET);
    if(ret < 0) {
      perror("read:");
      ERROR_MESSAGE("Error occurred while seeking file:%ld\n", ret);
    }
  }

  BufferLock();
  ssize_t read_size = read(fdesc_, dst, len);
  if((uint64_t)read_size < 0) {
    perror("read:");
    ERROR_MESSAGE("Error occurred while reading buffer contents from file: %d %ld != %ld\n", fdesc_, read_size, len);
  }
  BufferUnlock();
  return (char *)dst;
}

void PosixFileHandle::FileSync(void)
{
  fsync(fdesc_);
}

uint64_t PosixFileHandle::GetFileSize(void)
{
  struct stat buf;
  fstat(fdesc_, &buf);
  uint64_t fsize = buf.st_size;
  ASSERT(fsize == offset_);
  return offset_;
  //return fsize - sizeof(MagicStore);
}

uint32_t PosixFileHandle::GetBlkSize(void)
{
  //size_t blockSize;
//  unsigned long blockSize;
//  int ret = ioctl(fdesc_,BLKBSZGET/* BLKSSZGET*/, &blockSize);
//  if( ret != 0 ) { 
//    perror("ioctl:");
//    ERROR_MESSAGE("ioctl error: %d, fdesc:%d, blksize:%lu\n", ret, fdesc_, blockSize);
//  }
  return CHUNK_ALIGNMENT;
}
