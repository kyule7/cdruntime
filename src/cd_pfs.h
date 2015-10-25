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

#include "cd_features.h"
#include "cd_global.h"
#include "cd_def_internal.h" 
#include "cd_internal.h"

#if CD_PROFILER_ENABLED

#include <mpi.h>
#define CommFree MPI_Comm_free
#define CommGroupFree MPI_Group_free

#else

#define CommFree free
#define CommGroupFree free

#endif

namespace cd {
  namespace internal {

/** 
 * @brief This class is designed for Parallel File System (PFS) operations using MPI-I/O.
 * In CDs, one of the preservation mediums is considered to be the PFS so in this case, 
 * CDs use this class for preservation to PFS. 
 */
   
/**
 * @brief In order to decrease pressure on each file and the meta-data server for 
 * parallel files, this class creates some internal communicators from the tasks
 * inside the CD and each file is shared among the ranks inside that local communicator.
 * Number of new local communicators depends on the total number of tasks in CD.
 * Basically, number of files grows logarithmically while number of ranks in CD 
 * increases, and consequently, number of ranks which operator on the same shared-file
 * would be less while total number of files is also manageable.
 */

class PFSHandle {
	friend class CD;
	friend class HeadCD;
	friend class CDEntry;
private:
	CD *ptr_cd_; //pointer to owner CD

	std::string PFS_file_path_;  //string pointing to the target path.
	COMMLIB_File PFS_d_; //parallel file desctiptor.

	COMMLIB_Offset PFS_current_offset_; //this offset points to the location which the next data would be written to.
	COMMLIB_Offset PFS_current_chunk_begin_; //in a shared file, each rank operates on specific chunks. This variable and the next
						 //one define boundaries of the operating chunk.
	COMMLIB_Offset PFS_current_chunk_end_;
	uint64_t PFS_chunk_size_; //This value can be tunned. This represents the chunk dedicated to each task in the 
				  //shared mode. When there is only one file operating on the file, this variable does not have impac

	CommGroupT PFS_parallel_file_group_; //local group of ranks operating on the file (sharing the file).
	ColorT PFS_parallel_file_communicator_; //local communicator associated with the local group.
	int PFS_rank_in_file_communicator_; //local PFS rank used for calculating next possible chunk for each rank.

	int degree_of_sharing_; //Depending on number of ranks in the CD, this value represents number of ranks which share a single file.
	int sharing_group_id_; //each local group has a local id starting from 0. This is used for naming the shared parallel file.

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
       /**
  	* @brief This function is used for "CD_Complete". Whenever CD completes its operation, 
  	* by calling this function, file pointer is returned to its appropriate location so that the same file
  	* can be used again. 
  	*/
	void Reset_File_Pointer( void );
	COMMLIB_Offset Write( void*, uint64_t ); //This is a basic function, It writes the input data.

       /**
  	* @brief For getting back the data, i.e. reading it from the file, it is important to have the 
  	* starting point of data in the file. The third argument is the start point of data whenever it
  	* was saved to the file. This element is saved in CD_Entry.
  	*/
	uint64_t Read_at( void*, uint64_t, COMMLIB_Offset );

private:
       /**
  	* @brief Open file is called whenever this object is created.
  	* Opening the file uses the MPI_MODE_DELETE_ON_CLOSE, so whenever the next function
  	* is called for closing the file, it automatically deletes the file. This happens 
  	* at the point which CD is destroyed (CD_Destroy).
  	*/
	int Open_File( void );

	int Close_and_Delete_File( void );
	void Copy( const PFSHandle& that );

       /**
  	* @brief This function is used for creating the local communicators. Basically, it gets the 
  	* number of tasks in a CD and based on that it increases number of operating tasks/file in a 
  	* exponential manner.
  	*/
	int Splitter( void );
	void Init( const char* file_path );
};


  } // namespace internal ends
} // namespace cd ends
#endif
