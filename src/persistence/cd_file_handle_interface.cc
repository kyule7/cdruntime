#ifndef _CD_MPI_FILE_HANDLE_INTERFACE_H
#define _CD_MPI_FILE_HANDLE_INTERFACE_H
#include "cd_file_handle.h"
#include "mpi_file_handle.h"
//#include "new_file_handle.h"

packer::FileHandle *packer::GetFileHandle(uint32_t ftype)
{
//  printf("GetFileHandle %u\n", ftype);
  switch(ftype) {
    case kPosixFile:
      return PosixFileHandle::Get();
//    case kAIOFile:
//      return AIOFileHandle::Get();
    case kMPIFile:
      return MPIFileHandle::Get(MPI_COMM_WORLD, DEFAULT_FILEPATH_MPI);
    case kLibcFile:
      return LibcFileHandle::Get(MPI_COMM_WORLD, DEFAULT_FILEPATH_LIBC);
    default:
      return NULL;
  }
}

void packer::DeleteFiles(void)
{
  PosixFileHandle::Close();
  MPIFileHandle::Close();
  LibcFileHandle::Close();
}

#endif
