#ifndef _CD_MPI_FILE_HANDLE_INTERFACE_H
#define _CD_MPI_FILE_HANDLE_INTERFACE_H
#include "cd_file_handle.h"
#include "mpi_file_handle.h"
//#include "new_file_handle.h"
namespace packer {

FileHandle *GetFileHandle(uint32_t ftype)
{
//  printf("GetFileHandle %u\n", ftype);
  switch(ftype) {
    case kPosixFile:
      return PosixFileHandle::Get();
//    case kAIOFile:
//      return AIOFileHandle::Get();
    case kMPIFile:
      return MPIFileHandle::Get(MPI_COMM_WORLD, DEFAULT_FILEPATH_MPI);
    default:
      return NULL;
  }
}

}
#endif
