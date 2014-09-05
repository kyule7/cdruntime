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


#define UNITSIZE 12  // One unit of data is consist of 12 bytes.


// It will pack [ID][SIZE][POS]  and data in the separate section.

// In the bigger picture there are [Table] section in the beginning and then [Data] section.  In [Table] section we can see where the data is located.

using namespace cd;
  Packer::Packer() 
: table_size_(0), data_size_(0), used_table_size_(0), used_data_size_(0)
{
  table_grow_unit_ = 2048; // What is the rate of growth for the table.  There are optimum size, if this is too big then initial effort is too big, if it is too small, then later realloc might happen more often.
  data_grow_unit_ = 2048;  // Same but this goes with data section

  alloc_flag_=0;   // FIXME: Might want to have separate memory allocator of our own. 

  ptr_table_ = NULL;   
  ptr_data_ = NULL;


  unsigned long ret;

  if( alloc_flag_ == 0)
  {

    ret = CheckAlloc();  // Allocate for the first time

    if( ret == kError)
      ERROR_MESSAGE("Memory allocation failed in Packer class");
  }
}



  Packer::Packer(unsigned long table_grow_unit, unsigned long data_grow_unit) 
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



unsigned int Packer::Add(unsigned long id, unsigned long length, const void *ptr_data)
{
  // Class user does not need to know the current writing position.	
  return AddData(id , length ,  used_data_size_, (char *)ptr_data); 
}


//TODO: We can put checksum, or ECC at the end of this buffer. We can work on this later not now. 
char *Packer::GetTotalData(unsigned long &total_data_size) 
{
  char *total_data = new char [used_table_size_ + used_data_size_+ sizeof(unsigned long) ];  // We should not forget the first 4 byte for indicating table size.

  unsigned long table_size = (used_table_size_+sizeof(unsigned long));

  memcpy(total_data, &table_size,sizeof(unsigned long) ); 
  memcpy(total_data+sizeof(unsigned long),ptr_table_,used_table_size_); 
  memcpy(total_data+used_table_size_+sizeof(unsigned long), ptr_data_, used_data_size_); 

  total_data_size = used_table_size_ + used_data_size_+ sizeof(unsigned long);

  return total_data;
}



unsigned int Packer::ClearData()
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



void Packer::SetBufferGrow(unsigned long table_grow_unit, unsigned long data_grow_unit) 
{
  table_grow_unit_ = table_grow_unit;
  data_grow_unit_ = data_grow_unit;

}



void Packer::WriteWord(char *dst_buffer,unsigned long value)
{
  if(dst_buffer != NULL )	
    memcpy(dst_buffer,&value,sizeof(unsigned long) );
}



unsigned int Packer::CheckAlloc() 
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

unsigned int Packer::CheckRealloc(unsigned long length) 
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
void Packer::WriteData(char *ptr_data, unsigned long length, unsigned long position)
{
  if( ptr_data != NULL ) 	
    memcpy( ptr_data_ + position, ptr_data, length );
}

unsigned int Packer::AddData(unsigned long id, unsigned long length, unsigned long position, char *ptr_data)
{
  unsigned long ret;


  if ( (ret = CheckRealloc(length)) != kOK ) 
  {
    return ret;  
  }


  WriteWord(ptr_table_ + used_table_size_, id); 
  WriteWord(ptr_table_ + used_table_size_ + 4, length); 
  WriteWord(ptr_table_ + used_table_size_ + 8, position); 



  used_table_size_ += UNITSIZE;    
  WriteData(ptr_data,length,position); 

  used_data_size_ += length; 

  return kOK;

}

