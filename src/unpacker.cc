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

#include "unpacker.h"
#include "cd_def_internal.h" 
#include <cstring>

#define DEBUG_OFF_PACKER 1

using namespace cd;
using std::endl;
using std::cout;

Unpacker::Unpacker()
:table_size_(0), cur_pos_(4), reading_pos_(0)
{


}

Unpacker::~Unpacker()
{


}

char *Unpacker::GetAt(const char *src_data, uint32_t find_id, uint32_t &return_size, uint32_t &return_id)
{
  char *str_return_data=NULL;
  uint32_t id=-1, size, pos;

  reading_pos_=0;
  table_size_ = GetWord(src_data + reading_pos_);
  reading_pos_+=4;

  while ( reading_pos_ < table_size_ )
  {
    id = GetWord( src_data + reading_pos_ );
    if( id == find_id ) {
      size = GetWord(src_data + reading_pos_ + 4); // FIXME: Currently assumed uint32_t is 4 bytes long.
      pos  = GetWord(src_data + reading_pos_ + 8);
      str_return_data = new char[size];
      memcpy(str_return_data, src_data+table_size_+pos, size); 
      return_id = id;
      return_size = size;

      break;
    }
    else {
      reading_pos_ = reading_pos_ + 12;
    }
  }

  if( id == find_id ) {
    return str_return_data;
  }
  else {
    return NULL;
  }
}

uint32_t Unpacker::GetAt(const char *src_data, uint32_t find_id, void *return_data)
{
  uint32_t id=-1;
  char *pData;
  uint32_t return_size;
  pData = GetAt(src_data, find_id, return_size, id);
  if( pData != NULL)
  {
    memcpy(return_data,pData,return_size);
    delete [] pData;
    return kOK;
  }
  else
  {
    return kNotFound;
  }
}

char *Unpacker::GetAt(const char *src_data, uint32_t find_id, uint32_t &return_size)
{
  uint32_t id=-1;
  return GetAt(src_data,find_id,return_size,id);
}

char *Unpacker::GetNext(char *src_data,  uint32_t &return_id, uint32_t &return_size, bool alloc, void *dst, uint64_t dst_size)
{
  char *str_return_data;
  uint32_t id, size, pos;

  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetNext] src_data : %p, return id : %u, return size : %u\n", (void *)src_data, return_id, return_size);

  table_size_ = GetWord(src_data);
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "table size : %u\n", table_size_);

  if( cur_pos_ < table_size_ ) {
    id   = GetWord( src_data + cur_pos_ );
    size = GetWord( src_data + cur_pos_ + 4);
    pos  = GetWord( src_data + cur_pos_ + 8);

    if(alloc)
      str_return_data = new char[size];
    else {
      str_return_data = (char *)dst;
      if(dst_size != size) {
        CD_DEBUG_COND(DEBUG_OFF_PACKER, "dst_size : %lu, size from packer : %d\n", dst_size, size);
        assert(0);
      }
      assert(dst_size == size);
    }
    
    CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Get Info from table] id : %u (%p), size : %u (%p), pos : %u (%p)\n",
             id, (void *)((char *)src_data + cur_pos_),
             size, (void *)((char *)src_data + cur_pos_+4),
             pos, (void *)((char *)src_data + cur_pos_+8));

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Bring data from %p to %p\n", (void *)((char *)src_data+table_size_+pos), (void *)str_return_data);

    memcpy(str_return_data, src_data+table_size_+pos, size); 

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Read Data is %s", (char *)str_return_data);
 
    return_id = id;
    return_size = size;
    cur_pos_ +=12;

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");

    return str_return_data;
  }
  else
    return NULL;
}

void *Unpacker::GetNext(void *str_return_data, void *src_data,  uint32_t &return_id, uint32_t &return_size)
{
  
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetNext] str_return_data: %p, src_data : %p, return_id : %u, return_size : %u\n",
           str_return_data, src_data, return_id, return_size);
  
  uint32_t id, size, pos;

  table_size_ = GetWord(src_data);

  if( cur_pos_ < table_size_ )
  {
    id   = GetWord( (char *)src_data + cur_pos_ );
    size = GetWord( (char *)src_data + cur_pos_ + 4 );
    pos  = GetWord( (char *)src_data + cur_pos_ + 8 );

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Get Info from table] id: %u (%p), size : %u (%p), pos : %u (%p)\n",
             id, (void *)((char *)src_data + cur_pos_),
             size, (void *)((char *)src_data + cur_pos_+4),
             pos, (void *)((char *)src_data + cur_pos_+8));

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Bring data from %p to %p\n", (void *)((char *)src_data+table_size_+pos), str_return_data);

    memcpy(str_return_data, (char *)src_data+table_size_+pos, size); 

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "Read Data is %s\n", (char *)str_return_data);
 
    return_id = id;
    return_size = size;
    cur_pos_ +=12;

    CD_DEBUG_COND(DEBUG_OFF_PACKER, "==========================================================\n");

    return str_return_data;
  }
  else
    return NULL;
}


void Unpacker::SeekInit()
{
  cur_pos_ = 4;
}

uint32_t Unpacker::GetWord(void *src_data)
{
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetWord] src_data : %p\n", (void *)src_data);
  uint32_t return_value;
  memcpy( &return_value, src_data, sizeof(uint32_t) );
  return return_value;
}

uint32_t Unpacker::GetWord(const char *src_data)
{
  CD_DEBUG_COND(DEBUG_OFF_PACKER, "[Unpacker::GetWord] src_data : %p\n", (void *)src_data);
  uint32_t return_value;
  memcpy( &return_value, src_data, sizeof(uint32_t) );
  return return_value;
}


