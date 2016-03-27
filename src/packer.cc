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


#include "packer.h"
#include <string.h>


#define DEBUG_OFF_PACKER 1
#define UNITSIZE 12  // One unit of data is consist of 12 bytes.


// It will pack [ID][SIZE][POS]  and data in the separate section.

// In the bigger picture there are [Table] section in the beginning and then [Data] section.  In [Table] section we can see where the data is located.

using namespace cd;
using std::endl;
using std::cout;
Packer::Packer() 
: table_size_(0), data_size_(0), used_table_size_(0), used_data_size_(0)
{
  table_grow_unit_ = 2048; // What is the rate of growth for the table.  There are optimum size, if this is too big then initial effort is too big, if it is too small, then later realloc might happen more often.
  data_grow_unit_ = 2048;  // Same but this goes with data section

  alloc_flag_=0;   // FIXME: Might want to have separate memory allocator of our own. 

  ptr_table_ = NULL;   
  ptr_data_ = NULL;


  uint32_t ret;

  if( alloc_flag_ == 0)
  {

    ret = CheckAlloc();  // Allocate for the first time

    if( ret == kError)
      ERROR_MESSAGE("Memory allocation failed in Packer class");
  }
}


Packer::Packer(uint32_t table_grow_unit, uint32_t data_grow_unit) 
: table_size_(0), data_size_(0), used_table_size_(0), used_data_size_(0)
{
  table_grow_unit_ = table_grow_unit;
  data_grow_unit_ = data_grow_unit;
  alloc_flag_=0;
  ptr_table_ = NULL;
  ptr_data_ = NULL;
  CheckAlloc();

}



Packer::~Packer()
{
  ClearData();  // Delete allocated data.
}



uint32_t Packer::Add(uint32_t id, uint64_t length, const void *ptr_data)
{

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "id : %u, length : %lu, ptr_data : %p\n", id, length, ptr_data);

  // Class user does not need to know the current writing position.	
  return AddData(id , (uint32_t)length ,  used_data_size_, (char *)ptr_data); 
}


//TODO: We can put checksum, or ECC at the end of this buffer. We can work on this later not now. 
char *Packer::GetTotalData(uint64_t &total_data_size) 
{
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "GetTotalData\n");

  // If the target to be packed is actually empty, return NULL
  if(used_table_size_ + used_data_size_ == 0) return NULL;

  char *total_data = new char [used_table_size_ + used_data_size_+ sizeof(uint32_t) ];  // We should not forget the first 4 byte for indicating table size.

  uint32_t table_size = (used_table_size_+sizeof(uint32_t));

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "table_size : %u, ptr_table : %p, used_table_size : %u, used_data_size : %u\n", 
					 table_size, (void *)ptr_table_, used_table_size_, used_data_size_);
  
  memcpy(total_data, &table_size,sizeof(uint32_t) ); 
  memcpy(total_data+sizeof(uint32_t),ptr_table_,used_table_size_); 
  memcpy(total_data+used_table_size_+sizeof(uint32_t), ptr_data_, used_data_size_); 

  total_data_size = used_table_size_ + used_data_size_+ sizeof(uint32_t);

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "total_data : %p\n", (void *)total_data);

  return total_data;
}


uint32_t Packer::ClearData()
{

  if(ptr_table_ != NULL)
    delete [] ptr_table_;
  if(ptr_data_ != NULL)
    delete [] ptr_data_;


  ptr_table_ = NULL;
  ptr_data_  = NULL;
  alloc_flag_=0;

  table_size_ = 0;
  data_size_ = 0;
  used_table_size_ = 0;
  used_data_size_ = 0;


  return kOK;

}


void Packer::SetBufferGrow(uint32_t table_grow_unit, uint32_t data_grow_unit) 
{
  table_grow_unit_ = table_grow_unit;
  data_grow_unit_ = data_grow_unit;
}


void Packer::WriteWord(char *dst_buffer,uint32_t value)
{
  if(dst_buffer != NULL ){	
//    dbg << "WriteWord\ndst_buffer : "<< (void *)dst_buffer << ", value : "<< value << endl;
    memcpy(dst_buffer,&value,sizeof(uint32_t) );
  }
}



uint32_t Packer::CheckAlloc() 
{

  if( ptr_table_ == NULL) 
  {

    ptr_table_ = new char[table_grow_unit_];
    //TODO: Check ASSERT(ptr_table_ != NULL);
    if(ptr_table_ == NULL ) return kMallocFailed;
    table_size_ = table_grow_unit_;
    alloc_flag_++;
  }
  if( ptr_data_ == NULL)
  {
    ptr_data_ = new char[data_grow_unit_];
    //TODO: Check ASSERT(ptr_data_ != NULL);
    if(ptr_data_ == NULL ) return kMallocFailed;
    data_size_ = data_grow_unit_;
    alloc_flag_++;
  }

  if( ptr_data_ == NULL || ptr_table_ == NULL)
  {
    return kError;
  }
  return kOK;

}

uint32_t Packer::CheckRealloc(uint32_t length) 
{

  if( used_table_size_ + UNITSIZE > table_size_ )
  {
    char *tmp1;

    tmp1 = new char[ table_size_ + table_grow_unit_ ];
    //TODO: check ASSERT( tmp1!=NULL );
    memcpy(tmp1,ptr_table_,table_size_);
    delete [] ptr_table_;

    ptr_table_ = tmp1;

    //TODO: check ASSERT( ptr_table_ != NULL);
    if( ptr_table_ == NULL ) return kReallocFailed;
    table_size_ += table_grow_unit_;



  }
  if( used_data_size_ + length > data_size_ )
  {
    char *tmp2;

    if( used_data_size_ + length < used_data_size_+data_grow_unit_ ) 
    { // if data fits within the current memory allocation. 
      tmp2 = new char[data_size_ + data_grow_unit_];/////////////
      memcpy(tmp2,ptr_data_,data_size_);
      delete [] ptr_data_;
      ptr_data_ = tmp2;
      //TODO: ASSERT( ptr_data_ != NULL);
      if( ptr_data_ == NULL ) return kReallocFailed;
      data_size_ += data_grow_unit_;
    }
    else // if data has grown bigger than the currently allocated memory region, get reallocation with bigger size 
    {	
      tmp2 = new char[data_size_ + length];
      memcpy(tmp2,ptr_data_,data_size_);
      delete [] ptr_data_;
      ptr_data_ =tmp2;
      //TODO: ASSERT( ptr_data_ != NULL);
      if( ptr_data_ == NULL) return kReallocFailed;
      data_size_+=length;
    }

  }
  return kOK;
}

void Packer::WriteData(char *ptr_data, uint32_t length, uint32_t position)
{
  if( ptr_data != NULL ) {	
    memcpy( ptr_data_ + position, ptr_data, length );

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Written Data is %d\n", *(int*)(ptr_data_ + position));
  }
}






uint32_t Packer::AddData(uint32_t id, uint32_t length, uint32_t position, char *ptr_data)
{
  uint32_t ret;

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Packer::AddData] Before CheckRealloc ptr_table : %p\n", (void *)ptr_table_);

  if ( (ret = CheckRealloc(length)) != kOK ) 
  {
    return ret;  
  }

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "Before write word, ptr_table : %p\n", (void *)ptr_table_);

  WriteWord(ptr_table_ + used_table_size_, id); 
  WriteWord(ptr_table_ + used_table_size_ + 4, length); 
  WriteWord(ptr_table_ + used_table_size_ + 8, position); 

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Get Info from table] id : %u (%p), length : %u (%p), position : %u (%p), ptr_data : %p\n",
				  id, (void*)(ptr_table_ + used_table_size_), 
					length, (void*)(ptr_table_ + used_table_size_ + 4), 
					position, (void*)(ptr_table_ + used_table_size_ + 8), 
					(void*)ptr_data);
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "Bring data from %p to %p\n", (void *)ptr_data, (void *)(ptr_data_ + position));
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");

  used_table_size_ += UNITSIZE;    
  WriteData(ptr_data,length,position); 

  used_data_size_ += length; 

  return kOK;
}

