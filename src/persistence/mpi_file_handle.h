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
    virtual CDErrType Write(uint64_t offset, char *src, uint64_t chunk, int64_t inc=-1);
    virtual char *Read(uint64_t len, uint64_t offset=0);
    virtual char *ReadTo(void *dst, uint64_t len, uint64_t offset=0);
    virtual void FileSync(void);
    virtual void Truncate(uint64_t newsize);
    virtual int64_t GetFileSize(void);
    virtual uint32_t GetBlkSize(void);
};

}
#endif
#endif
