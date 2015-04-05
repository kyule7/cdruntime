/*
Copyright 2014, The University of Texas at Austin 
All rights reserved.

THIS FILE IS PART OF THE CONTAINMENT DOMAINS RUNTIME LIBRARY

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/


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
