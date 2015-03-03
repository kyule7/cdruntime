#ifndef _CD_PFS_H
#define _CD_PFS_H

#include <iostream>
#include <string>
#include <mpi.h>
#include <math.h>
#include <sstream>
#include "cd_global.h"
#include "cd.h"

namespace cd {
class CD_Parallel_IO_Manager {
	friend class cd::CD;
private:
	CD *ptr_cd_;

	std::string PFS_file_path_;
	MPI_File PFS_d_;

	MPI_Offset PFS_current_offset_;
	MPI_Offset PFS_current_chunk_begin_;
	MPI_Offset PFS_current_chunk_end_;
	uint64_t PFS_chunk_size_;

	MPI_Group PFS_parallel_file_group_;
	MPI_Comm PFS_parallel_file_communicator_;
	int PFS_rank_in_file_communicator_;

	int degree_of_sharing_;
	int sharing_group_id_;

public:
	CD_Parallel_IO_Manager( CD* my_cd );
	CD_Parallel_IO_Manager( CD* my_cd, const char* file_path );
	CD_Parallel_IO_Manager( CD* my_cd, const char* file_path , uint64_t chunk_size );
	CD_Parallel_IO_Manager( const CD_Parallel_IO_Manager& that ) ;
	~CD_Parallel_IO_Manager( void ) 
  { 
  	Close_and_Delete_File(); 
  	MPI_Group_free( &PFS_parallel_file_group_ );
  	MPI_Comm_free( &PFS_parallel_file_communicator_ );
  }

	void Reset_File_Pointer( void );
	MPI_Offset Write( void*, uint64_t );
	uint64_t Read_at( void*, uint64_t, MPI_Offset );
private:
	int Open_File( void );
	int Close_and_Delete_File( void );
	void Copy( const CD_Parallel_IO_Manager& that );
	int Splitter( void );
	void Init( const char* file_path );
};


} // namespace ends
#endif
