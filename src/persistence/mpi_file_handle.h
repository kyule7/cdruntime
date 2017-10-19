#ifndef _CD_MPI_FILE_HANDLE_H
#define _CD_MPI_FILE_HANDLE_H

#include "cd_file_handle.h"
#include "define.h"
//#define DEFAULT_VIEWSIZE 16384
//#define DEFAULT_VIEWSIZE 512
#define DEFAULT_VIEWSIZE 32768
#define MPI_MAX_ERR_STR 256

#if 1
#include <mpi.h>

// Singleton
namespace packer {

class MPIFileHandle : public FileHandle {
  protected:
    static MPIFileHandle *fh_;
    uint64_t viewsize_;
    MPI_Comm comm_;
    MPI_File fdesc_;
    MPI_Datatype ftype_;
  protected:
    MPIFileHandle(void) : FileHandle(DEFAULT_FILEPATH_MPI) { Init(MPI_COMM_WORLD, DEFAULT_FILEPATH_MPI); }
    MPIFileHandle(const MPI_Comm &comm, const char *filepath) : FileHandle(filepath) { Init(comm, filepath); }
    virtual ~MPIFileHandle(void);
    void Init(const MPI_Comm &comm, const char *filepath);
  public:
    static FileHandle *Get(MPI_Comm comm, const char *filepath=NULL);
    void Close(void);
    virtual CDErrType Write(int64_t offset, char *src, int64_t len, int64_t inc=-1);
    virtual char *Read(int64_t len, int64_t offset=0);
    virtual CDErrType Read(void *dst, int64_t len, int64_t offset=0);
    virtual void FileSync(void);
    virtual void Truncate(uint64_t newsize);
    virtual int64_t GetFileSize(void);
    virtual uint32_t GetBlkSize(void);
};

class LibcFileHandle : public MPIFileHandle {
  protected:
    static LibcFileHandle *fh_;
    LibcFileHandle(void) : MPIFileHandle(MPI_COMM_WORLD, DEFAULT_FILEPATH_LIBC) {
//      printf("LibcFileHandle Created\n");
    }
    LibcFileHandle(const MPI_Comm &comm, const char *filepath) : MPIFileHandle(comm, filepath) {
//      printf("LibcFileHandle Created\n");
    }
  public:
    static FileHandle *Get(MPI_Comm comm, const char *filepath=NULL);
};

}
#endif
#endif
