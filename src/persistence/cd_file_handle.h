#ifndef _CD_FILE_HANDLE_H
#define _CD_FILE_HANDLE_H

#include <cstdint>
#include <vector>
#include <string>
#include "define.h"
#define MAX_FILEPATH_SIZE 256
//#define CHUNK_ALIGNMENT 512
//#define CHUNK_ALIGNMENT 256
#define DEFAULT_BASEPATH "./"
#define DEFAULT_FILEPATH_POSIX "posix_filepath"
#define DEFAULT_FILEPATH_AIO   "aio_filepath"
#define DEFAULT_FILEPATH_MPI   "mpi_filepath"
namespace packer {

class DataStore;

//enum {
//  kPosixFile = 0x10,
//  kAIOFile = 0x20,
//  kMPIFile = 0x40
//};

class FileHandle {
  public:
    static std::string basepath; 
  protected:
    std::string filepath_;
    uint64_t offset_;
    FileHandle(const char *filepath=NULL);
    virtual ~FileHandle(void) {}
  public: 
    virtual CDErrType Write(uint64_t offset, char *src, uint64_t chunk, int64_t inc=-1)=0;
    virtual char *Read(uint64_t len, uint64_t offset=0)=0;
    virtual CDErrType Read(void *dst, uint64_t len, uint64_t offset=0)=0;
    virtual void FileSync(void)=0;
    virtual void Truncate(uint64_t newsize)=0;
    virtual int64_t GetFileSize(void)=0;
    virtual uint32_t GetBlkSize(void)=0;
    void SetOffset(uint64_t offset) { offset_ = offset; }
};

// Singleton
class PosixFileHandle : public FileHandle {
    static PosixFileHandle *fh_;
    int fdesc_;
  protected:
    PosixFileHandle(const char *filepath=DEFAULT_FILEPATH_POSIX);
    virtual ~PosixFileHandle(void);
  public:
    static FileHandle *Get(const char *filepath=NULL);
    void Close(void);
    virtual CDErrType Write(uint64_t offset, char *src, uint64_t chunk, int64_t inc=-1);
    virtual char *Read(uint64_t len, uint64_t offset=0);
    virtual CDErrType Read(void *dst, uint64_t len, uint64_t offset=0);
    virtual void FileSync(void);
    virtual void Truncate(uint64_t newsize);
    virtual int64_t GetFileSize(void);
    virtual uint32_t GetBlkSize(void);
};

FileHandle *GetFileHandle(uint32_t ftype=kPosixFile);

// Singleton
//class AIOFileHandle : public FileHandle {
//    static AIOFileHandle *fh_;
//    int fdesc_;
//  public:
//    AIOFileHandle(void) : FileHandle(DEFAULT_FILEPATH_AIO) {}
//    virtual CDErrType Write(uint64_t offset, char *src, uint32_t chunk);
//};
//
//// Singleton
//class MPIFileHandle : public FileHandle {
//    static MPIFileHandle *fh_;
//    MPI_File fdesc_;
//  public:
//    MPIFileHandle(void) : FileHandle(DEFAULT_FILEPATH_MPI) {}
//    virtual CDErrType Write(uint64_t offset, char *src, uint32_t chunk);
//};

//PosixFileHandle *PosixFileHandle::fh_ = NULL;
//AIOFileHandle   *AIOFileHandle::fh_ = NULL;
//MPIFileHandle   *MPIFileHandle::fh_ = NULL;
//std::vector<DataStore *> FileHandle::buf_list_;

// Singleton
//class BufferConsumer {
//    static BufferConsumer *buffer_consumer_;
//    std::vector<DataStore *> buf_list_;
//    BufferConsumer(void);
//    ~BufferConsumer(void);
//  public:
//    static BufferConsumer *Get(void);
//    static void *ConsumeBuffer(void *parm); 
//  public:
//    void InsertBuffer(DataStore *ds);
//    void RemoveBuffer(DataStore *ds);
//    DataStore *GetBuffer(void); 
//    void WriteAll(DataStore *buffer);
//    char *ReadAll(uint64_t len);
//    inline bool IsInvalidBuffer(void) {
//      return ( (active_buffer == NULL) || 
//              ((active_buffer->state) & inv_state != 0) ); 
//    }
//};
//
//static inline BufferConsumer *GetBufferConsumer(void) { return BufferConsumer::Get(); }


} // namespace packer ends
#endif
