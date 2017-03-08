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
using namespace packer;

std::string FileHandle::basepath(DEFAULT_BASEPATH); 
PosixFileHandle *PosixFileHandle::fh_ = NULL;

FileHandle::FileHandle(const char *filepath) 
  : filepath_(filepath), offset_(0) 
{
//  printf("basepath:%s\n", basepath.c_str()); getchar();
}

PosixFileHandle::PosixFileHandle(const char *filepath) : FileHandle(filepath) 
{
//  printf("posix basepath:%s\n", basepath.c_str()); getchar();
  struct timeval time;
  gettimeofday(&time, NULL);
  char full_filename[128];
  char base_filename[128];
  char *base_filepath = getenv("CD_BASE_FILEPATH");
  if(base_filepath != NULL) {
    strcpy(base_filename, base_filepath);
  } else {
    printf("ASDF############3 %s\n", DEFAULT_BASE_FILEPATH);
    strcpy(base_filename, DEFAULT_BASE_FILEPATH);
  }
  sprintf(full_filename, "%s/%s.%ld.%ld", base_filename, filepath, time.tv_sec, time.tv_usec);
  //fdesc_ = open(full_filename, O_CREAT | O_RDWR | O_DIRECT | O_APPEND, S_IRUSR | S_IWUSR);
  fdesc_ = open(full_filename, O_CREAT | O_RDWR | O_DIRECT, S_IRUSR | S_IWUSR);
  if(fdesc_ < 0) {
    ERROR_MESSAGE_PACKER("ERROR: File open path:%s\n", full_filename);
  }
  MYDBG("Opened file : %s\n", full_filename); //getchar();
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

CDErrType PosixFileHandle::Write(uint64_t offset, char *src, uint64_t len, int64_t inc)
{
  MYDBG("write (%d): %p (%lu) at file offset:%lu\n", 
         fdesc_, src, len, offset); 
//  printf("write (%d): %p (%lu) at file offset:%lx\n", 
//         fdesc_, src, len, offset);
//  getchar();

  CDErrType ferr = kOK;
  //if(offset != offset_) 
  if(1)
  {
    off_t ret = lseek(fdesc_, offset, SEEK_SET);
    // Error Check
    if(ret < 0) {
      perror("lseek:");
      ferr = kErrorFileSeek;
      ERROR_MESSAGE_PACKER("Error occurred while seeking file:%ld\n", ret);
    }
  } else {
    MYDBG("offset:%lu == %lu\n", offset_, offset);
  }


//  uint64_t mask_len = ~(511);
//  len &= mask_len;
//  if((len & 511) > 0) { 
//    len += 512;
//  }
  ssize_t written_size = write(fdesc_, src, len);
  offset_ = (inc >= 0)? offset_ + inc : offset_ + len;
  printf("offset:%lu\n", offset_); //getchar();
  // Error Check
  if((uint64_t)written_size != len) {
    perror("write:");
    ferr = kErrorFileWrite;
    ERROR_MESSAGE_PACKER("Error occurred while writing buffer contents to file: %d %ld == %lu\n", fdesc_, written_size, len);
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
  //if(offset != 0) 
  {
    off_t ret = lseek(fdesc_, offset, SEEK_SET);
    if(ret < 0) {
      perror("read:");
      ERROR_MESSAGE_PACKER("Error occurred while seeking file:%ld\n", ret);
    }
  }

  BufferLock();
  ssize_t ret = read(fdesc_, dst, len);
  if(ret < 0) {
    perror("read:");
    ERROR_MESSAGE_PACKER(
        "Error occurred while reading buffer contents from file: %d %ld != %ld, align:%lu %lu\n",
        fdesc_, ret, len, offset, offset % CHUNK_ALIGNMENT);
  }
  BufferUnlock();
  return (char *)dst;
}

void PosixFileHandle::FileSync(void)
{
  fsync(fdesc_);
}

void PosixFileHandle::Truncate(uint64_t newsize)
{
  MYDBG("%lu\n", newsize); //getchar();
  int ret = ftruncate(fdesc_, newsize); 
  if(ret != 0) {
    perror("ftruncate:");
    ERROR_MESSAGE_PACKER("Error during truncating file:%lu\n", newsize);
  }
}


int64_t PosixFileHandle::GetFileSize(void)
{
//  struct stat buf;
//  fstat(fdesc_, &buf);
//  uint64_t fsize = buf.st_size;
//  PACKER_ASSERT(fsize == offset_);
  return offset_;
//  return offset_ - sizeof(MagicStore);
  //return fsize - sizeof(MagicStore);
}

uint32_t PosixFileHandle::GetBlkSize(void)
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
