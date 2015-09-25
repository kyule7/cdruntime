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
#include "cd_pfs.h"
#include "util.h"

using namespace cd;
using namespace cd::internal;
using namespace std;


PFSHandle::PFSHandle( CD* my_cd )
{
	ptr_cd_ = my_cd;
	PFS_file_path_ = "./";
	PFS_chunk_size_ = 1048576;//default chunk is considered to be 1MB.

	Splitter();
	Init( NULL );

	Open_File();
}

PFSHandle::PFSHandle( CD* my_cd, const char* file_path )
{
	ptr_cd_ = my_cd;
	PFS_chunk_size_ = 1048576;//default chunk is considered to be 1MB.
	
	Splitter();
	Init( file_path );
	
	Open_File();
}

PFSHandle::PFSHandle( CD* my_cd, const char* file_path , uint64_t chunk_size )
{
	ptr_cd_ = my_cd;
	PFS_chunk_size_ = chunk_size;
	
	Splitter();
	Init( file_path );

	Open_File();
}	

PFSHandle::PFSHandle( const PFSHandle& that ) 
{ Copy( that ); }

//~PFSHandle::PFSHandle( void ) 
//{ 
//	Close_and_Delete_File(); 
//	MPI_Group_free( &PFS_parallel_file_group_ );
//	MPI_Comm_free( &PFS_parallel_file_communicator_ );
//}

void PFSHandle::Init( const char* file_path )
{
	stringstream temp_file_name;
	temp_file_name << Util::GetUniqueCDFileName( ptr_cd_->GetCDID(), file_path, NULL, kPFS ) + string(".") + to_string(sharing_group_id_) + string(".cd");
	PFS_file_path_ = temp_file_name.str();
	PFS_current_offset_ = PFS_rank_in_file_communicator_ * PFS_chunk_size_;
	PFS_current_chunk_begin_ = PFS_rank_in_file_communicator_ * PFS_chunk_size_;
	PFS_current_chunk_end_ = (PFS_rank_in_file_communicator_ + 1) * PFS_chunk_size_ - 1;
}



void PFSHandle::Reset_File_Pointer( void )
{
	PFS_current_offset_ = PFS_rank_in_file_communicator_ * PFS_chunk_size_;
	PFS_current_chunk_begin_ = PFS_rank_in_file_communicator_ * PFS_chunk_size_;
	PFS_current_chunk_end_ = (PFS_rank_in_file_communicator_ + 1) * PFS_chunk_size_ - 1;
}


void PFSHandle::Copy( const PFSHandle& that )
{
	ptr_cd_ = that.ptr_cd_;
	PFS_file_path_ = that.PFS_file_path_;
	PFS_d_ = that.PFS_d_;
	PFS_current_offset_ = that.PFS_current_offset_;
	PFS_current_chunk_begin_ = that.PFS_current_chunk_begin_;
	PFS_current_chunk_end_ = that.PFS_current_chunk_end_;
	PFS_chunk_size_ = that.PFS_chunk_size_;
	PFS_parallel_file_group_ = that.PFS_parallel_file_group_;
	PFS_parallel_file_communicator_ = that.PFS_parallel_file_communicator_;
	PFS_rank_in_file_communicator_ = that.PFS_rank_in_file_communicator_;
	degree_of_sharing_ = that.degree_of_sharing_;
}
#if _MPI_VER

int PFSHandle::Open_File( void )
{
	//FIXME: Currently no mpi hints are sent to the I/O, this can be tuned in the future.
        if( !(ptr_cd_->log_handle_.IsOpen()) ) ptr_cd_->log_handle_.OpenFilePath();       

	int error_type;
	error_type = MPI_File_open( PFS_parallel_file_communicator_, const_cast<char*>(PFS_file_path_.c_str()), 
							MPI_MODE_CREATE|MPI_MODE_RDWR|MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &(PFS_d_) );
	if( error_type != MPI_SUCCESS )
	{
		//FIXME: this part should be discussed with Kyushick. What error type can be used here?
		std::cout << "File could not be opened!" << std::endl;
		return -1;
	}
	return CDEntry::CDEntryErrT::kOK;
}

int PFSHandle::Close_and_Delete_File( void )
{
	int error_type;
	error_type = MPI_File_close( &PFS_d_ );
	if( error_type != MPI_SUCCESS )
	{
		std::cout << "File could not be closed!" << std::endl;
		return -1;
	}

	return CDEntry::CDEntryErrT::kOK;
}



int PFSHandle::Splitter( void )
{
	MPI_Group original_group;
	MPI_Comm_group( ptr_cd_->color(), &original_group );
		
	int original_comm_size = ptr_cd_->GetCDID().task_count();
	
	int cd_sharing_degree = log2( CD_SHARING_DEGREE ) - 1;
	if( getenv("CD_PFS_PRSV_SHARE_DGR") )
	{
		cd_sharing_degree = log2( atoi( getenv("CD_PFS_PRSV_SHARE_DGR") ) ) - 1;
	}
	degree_of_sharing_ = pow( 2, ( int ) log2( original_comm_size ) - cd_sharing_degree );
	degree_of_sharing_ = degree_of_sharing_ <= 0 ? 1 : degree_of_sharing_;

	int base_rank = (int)( ptr_cd_->GetCDID().task_in_color() / degree_of_sharing_ );		
	sharing_group_id_ = base_rank;
	
	int* selected_ranks;
	if( original_comm_size % degree_of_sharing_ == 0 )
	{
		selected_ranks = new int[ degree_of_sharing_ ];
		for( int i = 0; i < degree_of_sharing_; i++ )
		{
			selected_ranks[ i ] = base_rank * degree_of_sharing_ + i;
		}
	}
	else 
	{
		if( base_rank != (int) (original_comm_size / degree_of_sharing_) )
		{
			selected_ranks = new int[ degree_of_sharing_ ];
			for( int i = 0; i < degree_of_sharing_; i++ )
			{
				selected_ranks[ i ] = base_rank * degree_of_sharing_ + i;
			}
		}
		else
		{
			int temp = original_comm_size % degree_of_sharing_;
			selected_ranks = new int[ temp ];
			for( int i = 0; i < temp; i++ )
			{
				selected_ranks[ i ] = base_rank * degree_of_sharing_ + i;
			}
			degree_of_sharing_ = temp;
		}
	}
	
	MPI_Group_incl( original_group, degree_of_sharing_, selected_ranks, &PFS_parallel_file_group_ );
	MPI_Comm_create( ptr_cd_->GetCDID().color(), PFS_parallel_file_group_, &PFS_parallel_file_communicator_ );
	
	MPI_Comm_rank( PFS_parallel_file_communicator_, &PFS_rank_in_file_communicator_ );
	
	return 0;
}





COMMLIB_Offset PFSHandle::Write( void* buffer, uint64_t buffer_len)
{
	MPI_Status write_status;
	COMMLIB_Offset start_location_in_file = PFS_current_offset_;
	uint64_t length_to_preserve = buffer_len;
	uint64_t currently_preserved = 0;

	while( length_to_preserve > 0 )
	{
		if( length_to_preserve >= static_cast<uint64_t>((char *)PFS_current_chunk_end_ - (char *)PFS_current_offset_ + 1) )
		{
			length_to_preserve -= (PFS_current_chunk_end_ - PFS_current_offset_ + 1);
			MPI_File_write_at( PFS_d_, PFS_current_offset_, (char *)buffer + currently_preserved, 
			                   PFS_current_chunk_end_ - PFS_current_offset_ + 1, MPI_BYTE, &write_status );
			currently_preserved += (PFS_current_chunk_end_ - PFS_current_offset_ + 1);
			PFS_current_chunk_begin_ = PFS_current_chunk_begin_ + (degree_of_sharing_ * PFS_chunk_size_);
			PFS_current_chunk_end_ = PFS_current_chunk_end_ + (degree_of_sharing_ * PFS_chunk_size_);
			PFS_current_offset_ = PFS_current_chunk_begin_;
		}
		else
		{
			MPI_File_write_at( PFS_d_, PFS_current_offset_, (char *)buffer + currently_preserved, length_to_preserve, MPI_BYTE, &write_status );
			PFS_current_offset_ += length_to_preserve;
			currently_preserved += length_to_preserve;
			length_to_preserve = 0;
		}
	}

	return start_location_in_file;
}

uint64_t PFSHandle::Read_at( void* buffer, uint64_t buffer_len, COMMLIB_Offset read_from )
{
	MPI_Status read_status;
    COMMLIB_Offset chunk_begin = (read_from / PFS_chunk_size_) * PFS_chunk_size_;
    COMMLIB_Offset chunk_end = chunk_begin + PFS_chunk_size_ - 1;

    uint64_t length_to_restore = buffer_len;
    uint64_t currently_restored = 0;

    COMMLIB_Offset file_pointer = read_from;
  
    while( length_to_restore > 0 )
    {
      if( length_to_restore >= static_cast<uint64_t>((char *)chunk_end - (char *)file_pointer + 1) )
      {
        length_to_restore -= (chunk_end - file_pointer + 1);
        MPI_File_read_at( PFS_d_, file_pointer, (char *)buffer + currently_restored, 
                                            chunk_end - file_pointer + 1, MPI_BYTE, &read_status );
        currently_restored += (chunk_end - file_pointer + 1);

        chunk_begin = chunk_begin + ( degree_of_sharing_ * PFS_chunk_size_ );
        chunk_end = chunk_end + ( degree_of_sharing_ * PFS_chunk_size_ );
        file_pointer = chunk_begin;
      }
      else
      {
        MPI_File_read_at( PFS_d_, file_pointer, (char *)buffer + currently_restored, length_to_restore, MPI_BYTE, &read_status );
        currently_restored += length_to_restore;
        file_pointer += length_to_restore;
        length_to_restore = 0;
      }
    }

    return currently_restored;
}


#else

int PFSHandle::Open_File( void )
{
  assert(0);
	return CDEntry::CDEntryErrT::kOK;
}

int PFSHandle::Close_and_Delete_File( void )
{
  assert(0);
	return CDEntry::CDEntryErrT::kOK;
}



int PFSHandle::Splitter( void )
{
	assert(0);
	return 0;
}




COMMLIB_Offset PFSHandle::Write( void* buffer, uint64_t buffer_len)
{
  assert(0);
	return 0;
}

uint64_t PFSHandle::Read_at( void* buffer, uint64_t buffer_len, COMMLIB_Offset read_from )
{
    assert(0);
    return 0;
}


#endif
