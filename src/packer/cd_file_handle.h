#ifndef _CD_FILE_HANDLE_H
#define _CD_FILE_HANDLE_H

#include <cstdint>
#include <vector>
#include <string>
#include "define.h"
#define MAX_FILEPATH_SIZE 256
//#define CHUNK_ALIGNMENT 512
#define CHUNK_ALIGNMENT 4096
#define DEFAULT_BASE_FILEPATH "./"
#define DEFAULT_FILEPATH_POSIX "posix_filepath"
#define DEFAULT_FILEPATH_AIO   "aio_filepath"
#define DEFAULT_FILEPATH_MPI   "mpi_filepath"
namespace cd {

class DataStore;

//enum {
//  kPosixFile = 0x10,
//  kAIOFile = 0x20,
//  kMPIFile = 0x40
//};

class FileHandle {
  protected: 
    std::string filepath_;
    FileHandle(const char *filepath=NULL) : filepath_(filepath) {}
    virtual ~FileHandle(void) {}
  public: 
    virtual CDErrType Write(uint64_t offset, char *src, uint64_t chunk)=0;
    virtual char *Read(uint64_t len, uint64_t offset=0)=0;
    virtual char *ReadTo(void *dst, uint64_t len, uint64_t offset=0)=0;
    virtual void FileSync(void)=0;
    virtual uint64_t GetFileSize(void)=0;
    virtual uint32_t GetBlkSize(void)=0;
};

// Singleton
class PosixFileHandle : public FileHandle {
    static PosixFileHandle *fh_;
    int fdesc_;
    uint64_t offset_;
  protected:
    PosixFileHandle(const char *filepath=DEFAULT_FILEPATH_POSIX);
    virtual ~PosixFileHandle(void);
  public:
    static FileHandle *Get(const char *filepath=NULL);
    void Close(void);
    virtual CDErrType Write(uint64_t offset, char *src, uint64_t chunk);
    virtual char *Read(uint64_t len, uint64_t offset=0);
    virtual char *ReadTo(void *dst, uint64_t len, uint64_t offset=0);
    virtual void FileSync(void);
    virtual uint64_t GetFileSize(void);
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


} // namespace cd ends
#endif
