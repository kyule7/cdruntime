#ifndef _CD_PFS_H
#define _CD_PFS_H

#include <iostream>
#include <string>
#include <math.h>
#include <sstream>
#include "cd_global.h"
#include "cd_def_internal.h" 
#include "cd.h"

#if _MPI_VER
#include <mpi.h>
#define CommFree MPI_Comm_free
#define CommGroupFree MPI_Group_free
#else
#define CommFree free
#define CommGroupFree free
#endif
namespace cd {

class PFSHandle {
	friend class cd::CD;
	friend class cd::HeadCD;
	friend class cd::CDEntry;
private:
	CD *ptr_cd_;

	std::string PFS_file_path_;
	COMMLIB_File PFS_d_;

	COMMLIB_Offset PFS_current_offset_;
	COMMLIB_Offset PFS_current_chunk_begin_;
	COMMLIB_Offset PFS_current_chunk_end_;
	uint64_t PFS_chunk_size_;

	CommGroupT PFS_parallel_file_group_;
	ColorT PFS_parallel_file_communicator_;
	int PFS_rank_in_file_communicator_;

	int degree_of_sharing_;
	int sharing_group_id_;

	PFSHandle( CD* my_cd );
	PFSHandle( CD* my_cd, const char* file_path );
	PFSHandle( CD* my_cd, const char* file_path , uint64_t chunk_size );
	PFSHandle( const PFSHandle& that ) ;
	~PFSHandle( void ) 
  { 
  	Close_and_Delete_File(); 
  	CommGroupFree( &PFS_parallel_file_group_ );
  	CommFree( &PFS_parallel_file_communicator_ );
  }

	void Reset_File_Pointer( void );
	COMMLIB_Offset Write( void*, uint64_t );
	uint64_t Read_at( void*, uint64_t, COMMLIB_Offset );

private:
	int Open_File( void );
	int Close_and_Delete_File( void );
	void Copy( const PFSHandle& that );
	int Splitter( void );
	void Init( const char* file_path );
};


} // namespace ends
#endif
