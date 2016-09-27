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

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <cstring>
#include "packer.h"
using namespace cd;
#define CD_DEBUG_COND(...)
#define ERROR_MESSAGE(...)

#define DEBUG_OFF_PACKER 1


// It will pack [ID][SIZE][POS]  and data in the separate section.

// In the bigger picture there are [Table] section in the beginning and then [Data] section.  In [Table] section we can see where the data is located.

using std::endl;
using std::cout;
Packer::Packer(bool alloc_flag) 
: table_size_(0), data_size_(0), used_table_size_(0), used_data_size_(0)
{
//  printf("[%s]\n", __func__);
  // What is the rate of growth for the table.
  //There are optimum size, if this is too big then initial effort is too big, if it is too small, then later realloc might happen more often.
  table_grow_unit_ = TABLE_GROW_UNIT;

  // Same but this goes with data section
  data_grow_unit_ = DATA_GROW_UNIT; 

  alloc_flag_=alloc_flag;   // FIXME: Might want to have separate memory allocator of our own. 

  ptr_table_ = NULL;   
  ptr_data_ = NULL;


  uint64_t ret;

  if( alloc_flag_ != 0) {
    ret = CheckAlloc();  // Allocate for the first time
    if( ret == kError) {
      ERROR_MESSAGE("Memory allocation failed in Packer class");
    }
  }
}


Packer::Packer(uint64_t table_grow_unit, uint64_t data_grow_unit, bool alloc_flag) 
: table_size_(0), data_size_(0), used_table_size_(0), used_data_size_(0)
{
//  printf("[%s]\n", __func__);
  table_grow_unit_ = table_grow_unit;
  data_grow_unit_ = data_grow_unit;
  alloc_flag_ = alloc_flag;
  ptr_table_ = NULL;
  ptr_data_ = NULL;
  if(alloc_flag != 0) {
    CheckAlloc();
  }

}



Packer::~Packer()
{
  ClearData(false);  // Delete allocated data.
}



uint64_t Packer::Add(uint64_t id, uint64_t length, const void *ptr_data)
{

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "id : %u, length : %lu, ptr_data : %p\n", id, length, ptr_data);

  // Class user does not need to know the current writing position.	
  return AddData(id , (uint64_t)length ,  used_data_size_, (char *)ptr_data); 
}

uint64_t Packer::Add(const void *ptr_data, uint64_t length, uint64_t id)
{
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "id : %u, length : %lu, ptr_data : %p\n", id, length, ptr_data);

  // Class user does not need to know the current writing position.	
  return AddData(id , (uint64_t)length ,  used_data_size_, (char *)ptr_data); 
}


//TODO: We can put checksum, or ECC at the end of this buffer. We can work on this later not now. 
char *Packer::GetTotalData(uint64_t &total_data_size) 
{
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "GetTotalData\n");

  // If the target to be packed is actually empty, return NULL
  if(used_table_size_ + used_data_size_ == 0) return NULL;

//  char *total_data = new char [used_table_size_ + used_data_size_+ sizeof(uint64_t) ];  // We should not forget the first 4 byte for indicating table size.
  char *total_data = (char *)malloc(used_table_size_ + used_data_size_+ sizeof(uint64_t) );  // We should not forget the first 4 byte for indicating table size.

  uint64_t table_size = (used_table_size_+sizeof(uint64_t));

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "table_size : %u, ptr_table : %p, used_table_size : %u, used_data_size : %u\n", 
					 table_size, (void *)ptr_table_, used_table_size_, used_data_size_);
  
  memcpy(total_data, &table_size, sizeof(uint64_t)); 
  memcpy(total_data + sizeof(uint64_t), ptr_table_, used_table_size_); 
  memcpy(total_data+used_table_size_+sizeof(uint64_t), ptr_data_, used_data_size_); 

  total_data_size = used_table_size_ + used_data_size_+ sizeof(uint64_t);

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "total_data : %p\n", (void *)total_data);

  return total_data;
}

uint64_t Packer::ClearData(bool reuse)
{

  if(!reuse) { 
    if(ptr_table_ != NULL) {
//      delete [] ptr_table_;
      free(ptr_table_);
    }
    if(ptr_data_ != NULL) {
//      delete [] ptr_data_;
      free(ptr_data_);
    }
    ptr_data_  = NULL;
    ptr_table_ = NULL;
    table_size_ = 0;
    data_size_ = 0;
    table_grow_unit_ = TABLE_GROW_UNIT;
    data_grow_unit_ = DATA_GROW_UNIT; 
  }

  alloc_flag_ = true;

  used_table_size_ = 0;
  used_data_size_ = 0;


  return kOK;

}


void Packer::SetBufferGrow(uint64_t table_grow_unit, uint64_t data_grow_unit) 
{
  table_grow_unit_ = table_grow_unit;
  data_grow_unit_ = data_grow_unit;
}


void Packer::WriteWord(char *dst_buffer,uint64_t value)
{
  if(dst_buffer != NULL ){	
//    dbg << "WriteWord\ndst_buffer : "<< (void *)dst_buffer << ", value : "<< value << endl;
    memcpy(dst_buffer,&value,sizeof(uint64_t) );
  }
}



uint64_t Packer::CheckAlloc() 
{

  if( ptr_table_ == NULL) 
  {
//    printf("ptr_table_:%p, table_grow_unit_:%lu\n", ptr_table_, table_grow_unit_);
//    ptr_table_ = new char[table_grow_unit_];
    ptr_table_ = (char *)malloc(table_grow_unit_);
    //TODO: Check ASSERT(ptr_table_ != NULL);
    if(ptr_table_ == NULL ) return kMallocFailed;
    table_size_ = table_grow_unit_;
  }
  if( ptr_data_ == NULL)
  {
//    ptr_data_ = new char[data_grow_unit_];
    ptr_data_ = (char *)malloc(data_grow_unit_);
    //TODO: Check ASSERT(ptr_data_ != NULL);
    if(ptr_data_ == NULL ) return kMallocFailed;
    data_size_ = data_grow_unit_;
  }

  if( ptr_data_ == NULL || ptr_table_ == NULL)
  {
    return kError;
  }
  return kOK;

}

uint64_t Packer::CheckRealloc(uint64_t length, uint64_t required_size) 
{
//  printf("[%s]\n %lu > %lu\n", __func__, length, data_size_ - used_data_size_); //getchar();
//  printf("%s\n", __func__);// getchar();
  if( used_table_size_ + PACKER_ENTRY_SIZE > table_size_ )
  {
    //printf("%s table\n", __func__);// getchar();
    char *tmp1;

//    tmp1 = new char[ table_size_ + table_grow_unit_ ];
    tmp1 = (char *)malloc( table_size_ + table_grow_unit_ );
    //TODO: check ASSERT( tmp1!=NULL );
    memcpy(tmp1,ptr_table_,table_size_);
//    delete [] ptr_table_;
    free(ptr_table_);

    ptr_table_ = tmp1;

    //TODO: check ASSERT( ptr_table_ != NULL);
    if( ptr_table_ == NULL ) return kReallocFailed;
    table_size_ += table_grow_unit_;

    table_grow_unit_ <<= 2;
    table_grow_unit_ = (table_grow_unit_ < MEGABYTE)? table_grow_unit_ : MEGABYTE;
    //printf("%s table %lu\n", __func__, table_grow_unit_);// getchar();

  }
//  uint64_t left_data = data_size_ - used_data_size_;
//  if(data_size_ > GIGABYTE) { 
//    printf("data size:%lu (used:%lu)\n", data_size_, used_data_size_);// getchar();
//  }
  while( length >= data_size_ - used_data_size_ ) {
    //printf("%s data 1 %lu > %lu (%lu)\n", __func__, used_data_size_+length, data_size_, used_data_size_);//getchar();
    uint64_t new_data_size = data_size_ + length;
    if( data_size_ > TWO_GIGABYTE ) {
      data_grow_unit_ = (length > data_grow_unit_)? length : data_grow_unit_;
      new_data_size = data_size_ + data_grow_unit_;
    } else {
      new_data_size *= 2;
    }

    // doubling
//    new_data_size = (new_data_size > GIGABYTE)? new_data_size*2 : new_data_size + length;
//    data_grow_unit_ += length;
//    new_data_size = (new_data_size > TWO_GIGABYTE)? data_size_ + data_grow_unit_ : new_data_size;
//
//
//    char *tmp2 = new char[new_data_size];/////////////
    char *tmp2 = (char *)malloc(new_data_size);/////////////
//    printf("\n\n\nalloc large [%p - %p] data_size:%lu\n\n\n", tmp2, tmp2+new_data_size, new_data_size); //getchar();

    memcpy(tmp2,ptr_data_,data_size_);
//    delete [] ptr_data_;
    free(ptr_data_);
    ptr_data_ = tmp2;
    data_size_ = new_data_size;

    //TODO: ASSERT( ptr_data_ != NULL);
    assert(ptr_data_);
    if( ptr_data_ == NULL ) return kReallocFailed;
  }
  return kOK;
}

void Packer::WriteData(char *ptr_data, uint64_t length, uint64_t position)
{
//  printf("[Packer::%s] %p, %lu(len), %lu(pos)\n", __func__, ptr_data, length, position); //getchar();
  if( ptr_data != NULL ) {	
    memcpy( ptr_data_ + position, ptr_data, length );

//    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Written Data is %d\n", *(int*)(ptr_data_ + position));
  }
}

void Packer::Copy(const Packer& that) {
  table_grow_unit_ = that.table_grow_unit_;
  data_grow_unit_ = that.data_grow_unit_;
  table_size_ = that.table_size_;   
  data_size_ = that.data_size_; 
  used_table_size_ = that.used_table_size_; 
  used_data_size_ = that.used_data_size_; 
  alloc_flag_ = that.alloc_flag_;
  ptr_table_ = that.ptr_table_;
  ptr_data_ = that.ptr_data_;        
}




uint64_t Packer::AddData(uint64_t id, uint64_t length, uint64_t position, char *ptr_data)
{
  uint64_t ret;

//  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");
//  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Packer::AddData] Before CheckRealloc ptr_table : %p\n", (void *)ptr_table_);

  if ( (ret = CheckRealloc(length)) != kOK ) 
  {
    assert(0);
    return ret;  
  }

//  CD_DEBUG_COND(DEBUG_OFF_PACKER, "Before write word, ptr_table : %p\n", (void *)ptr_table_);

  WriteWord(ptr_table_ + used_table_size_, id); 
  WriteWord(ptr_table_ + used_table_size_ + PACKER_UNIT_1, length); 
  WriteWord(ptr_table_ + used_table_size_ + PACKER_UNIT_2, position); 

//  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Get Info from table] id : %u (%p), length : %u (%p), position : %u (%p), ptr_data : %p\n",
//				  id, (void*)(ptr_table_ + used_table_size_), 
//					length, (void*)(ptr_table_ + used_table_size_ + PACKER_UNIT_1), 
//					position, (void*)(ptr_table_ + used_table_size_ + PACKER_UNIT_2), 
//					(void*)ptr_data);
//  CD_DEBUG_COND(DEBUG_OFF_PACKER, "Bring data from %p to %p\n", (void *)ptr_data, (void *)(ptr_data_ + position));
//  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");

  used_table_size_ += PACKER_ENTRY_SIZE;    
  WriteData(ptr_data,length,position); 

  used_data_size_ += length; 
//  printf("used_data_size:%lu / %lu\n", used_data_size_, data_size_);
  return kOK;
}

// Unpacker /////////////////////////////////////////////////

Unpacker::Unpacker(void)
:table_size_(0), cur_pos_(PACKER_UNIT_1), reading_pos_(0)
{


}

Unpacker::~Unpacker()
{


}

void Unpacker::GetAt(const char *src_data, uint64_t find_id, void *target_ptr, uint64_t &return_size, uint64_t &return_id)
{
  uint64_t id=-1, size, pos;

  reading_pos_=0;
  table_size_ = GetWord(src_data + reading_pos_);
  reading_pos_ += PACKER_UNIT_1;

  while ( reading_pos_ < table_size_ )
  {
    id = GetWord( src_data + reading_pos_ );
    if( id == find_id ) {
      size = GetWord(src_data + reading_pos_ + PACKER_UNIT_1); // FIXME: Currently assumed uint64_t is 4 bytes long.
      pos  = GetWord(src_data + reading_pos_ + PACKER_UNIT_2);
      if(target_ptr == NULL) {
//        target_ptr = new char[size];
        target_ptr = (char *)malloc(size);
      }

      memcpy(target_ptr, src_data+table_size_+pos, size); 
      return_id = id;
      return_size = size;

      break;
    }
    else {
      reading_pos_ = reading_pos_ + PACKER_ENTRY_SIZE;
    }
  }

}

char *Unpacker::GetAt(const char *src_data, uint64_t find_id, uint64_t &return_size, uint64_t &return_id)
{
  char *str_return_data=NULL;
  uint64_t id=-1, size, pos;

  reading_pos_=0;
  table_size_ = GetWord(src_data + reading_pos_);
  reading_pos_ += PACKER_UNIT_1;

  while ( reading_pos_ < table_size_ )
  {
    id = GetWord( src_data + reading_pos_ );
    if( id == find_id ) {
      size = GetWord(src_data + reading_pos_ + PACKER_UNIT_1); // FIXME: Currently assumed uint64_t is 4 bytes long.
      pos  = GetWord(src_data + reading_pos_ + PACKER_UNIT_2);
//      str_return_data = new char[size];
      str_return_data = (char *)malloc(size);
      memcpy(str_return_data, src_data+table_size_+pos, size); 
      return_id = id;
      return_size = size;

      break;
    }
    else {
      reading_pos_ = reading_pos_ + PACKER_ENTRY_SIZE;
    }
  }

  if( id == find_id ) {
    return str_return_data;
  }
  else {
    return NULL;
  }
}

uint64_t Unpacker::GetAt(const char *src_data, uint64_t find_id, void *return_data)
{
  uint64_t id=-1;
  char *pData;
  uint64_t return_size;
  pData = GetAt(src_data, find_id, return_size, id);
  if( pData != NULL)
  {
    memcpy(return_data,pData,return_size);
//    delete [] pData;
    free(pData);
    return kOK;
  }
  else
  {
    return kNotFound;
  }
}

char *Unpacker::GetAt(const char *src_data, uint64_t find_id, uint64_t &return_size)
{
  uint64_t id=-1;
  return GetAt(src_data,find_id,return_size,id);
}

void Unpacker::ReadData(char *str_return_data, char *src_data, uint64_t pos, uint64_t size)
{
//  printf("[Unpacker::%s] %p %p %lu (pos) %lu (len)\n", __func__, str_return_data, src_data, pos, size); //getchar();
  if( str_return_data != NULL && src_data != NULL ) {	
    memcpy(str_return_data, src_data + table_size_ + pos, size); 
  } else {
    assert(0);
  }
}

char *Unpacker::GetNext(char *src_data,  uint64_t &return_id, uint64_t &return_size, bool alloc, void *dst, uint64_t dst_size)
{
  char *str_return_data;
  uint64_t id, size, pos;

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetNext] src_data : %p, return id : %u, return size : %u\n", (void *)src_data, return_id, return_size);

  table_size_ = GetWord(src_data);
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "table size : %u\n", table_size_);

  if( cur_pos_ < table_size_ ) {
    id   = GetWord( src_data + cur_pos_ );
    size = GetWord( src_data + cur_pos_ + PACKER_UNIT_1);
    pos  = GetWord( src_data + cur_pos_ + PACKER_UNIT_2);

    if(alloc) {
//      str_return_data = new char[size];
      str_return_data = (char *)malloc(size);
    } else {
      str_return_data = (char *)dst;
      if(dst_size != size) {
        CD_DEBUG_COND(DEBUG_OFF_PACKER, "dst_size : %lu, size from packer : %d\n", dst_size, size);
        assert(0);
      }
      assert(dst_size == size);
    }
    
    CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Get Info from table] id : %u (%p), size : %u (%p), pos : %u (%p)\n",
             id, (void *)((char *)src_data + cur_pos_),
             size, (void *)((char *)src_data + cur_pos_+ PACKER_UNIT_1),
             pos, (void *)((char *)src_data + cur_pos_+ PACKER_UNIT_2));

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Bring data from %p to %p\n", (void *)((char *)src_data+table_size_+pos), (void *)str_return_data);

//    memcpy(str_return_data, src_data+table_size_+pos, size); 
    ReadData(str_return_data, src_data, pos, size); 

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Read Data is %s", (char *)str_return_data);
 
    return_id = id;
    return_size = size;
    cur_pos_ += PACKER_ENTRY_SIZE;

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");

    return str_return_data;
  }
  else
    return NULL;
}

void *Unpacker::GetNext(void *str_return_data, void *src_data,  uint64_t &return_id, uint64_t &return_size)
{
  
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetNext] str_return_data: %p, src_data : %p, return_id : %u, return_size : %u\n",
           str_return_data, src_data, return_id, return_size);
  
  uint64_t id, size, pos;

  table_size_ = GetWord(src_data);

  if( cur_pos_ < table_size_ )
  {
    id   = GetWord( (char *)src_data + cur_pos_ );
    size = GetWord( (char *)src_data + cur_pos_ + PACKER_UNIT_1 );
    pos  = GetWord( (char *)src_data + cur_pos_ + PACKER_UNIT_2 );

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Get Info from table] id: %u (%p), size : %u (%p), pos : %u (%p)\n",
             id, (void *)((char *)src_data + cur_pos_),
             size, (void *)((char *)src_data + cur_pos_ + PACKER_UNIT_1),
             pos, (void *)((char *)src_data + cur_pos_ + PACKER_UNIT_2));

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Bring data from %p to %p\n", (void *)((char *)src_data+table_size_+pos), str_return_data);

    memcpy(str_return_data, (char *)src_data+table_size_+pos, size); 

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Read Data is %s\n", (char *)str_return_data);
 
    return_id = id;
    return_size = size;
    cur_pos_ += PACKER_ENTRY_SIZE;

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");

    return str_return_data;
  }
  else
    return NULL;
}

char *Unpacker::PeekNext(char *src_data, uint64_t &return_size, uint64_t &return_id)
{
  char *str_return_data = NULL;
  uint64_t id, size, pos;
  if(src_data == 0) return NULL;
//  assert(src_data != 0);

  table_size_ = GetWord(src_data);
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "table size : %u\n", table_size_);

  if( cur_pos_ < table_size_ ) {
    id   = GetWord( src_data + cur_pos_ );
    size = GetWord( src_data + cur_pos_ + PACKER_UNIT_1);
    pos  = GetWord( src_data + cur_pos_ + PACKER_UNIT_2);
    
    CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Get Info from table] id : %u (%p), size : %u (%p), pos : %u (%p)\n",
             id, (void *)((char *)src_data + cur_pos_),
             size, (void *)((char *)src_data + cur_pos_+ PACKER_UNIT_1),
             pos, (void *)((char *)src_data + cur_pos_+ PACKER_UNIT_2));

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Bring data from %p to %p\n", (void *)((char *)src_data+table_size_+pos), (void *)str_return_data);

    str_return_data = src_data + table_size_ + pos;

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Read Data is %s", (char *)str_return_data);
 
    return_size = size;
    return_id = id;
    cur_pos_ += PACKER_ENTRY_SIZE;

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");

  }
  return str_return_data;
}


void Unpacker::SeekInit()
{
  cur_pos_ = PACKER_UNIT_0;
}

uint64_t Unpacker::GetWord(void *src_data)
{
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetWord] src_data : %p\n", (void *)src_data);
  uint64_t return_value;
  memcpy( &return_value, src_data, sizeof(uint64_t) );
  return return_value;
}

uint64_t Unpacker::GetWord(const char *src_data)
{
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetWord] src_data : %p\n", (void *)src_data);
  uint64_t return_value;
  memcpy( &return_value, src_data, sizeof(uint64_t) );
  return return_value;
}




