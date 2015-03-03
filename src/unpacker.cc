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
#include <string.h>


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
  char *str_return_data;
  uint32_t id, size, pos;

  reading_pos_=0;
  table_size_ = GetWord(src_data + reading_pos_);
  reading_pos_+=4;

  while ( reading_pos_ < table_size_ )
  {
    id = GetWord( src_data + reading_pos_ );
    if( id == find_id )  

    {
      size = GetWord(src_data + reading_pos_ + 4); // FIXME: Currently assumed uint32_t is 4 bytes long.
      pos  = GetWord(src_data + reading_pos_ + 8);
      str_return_data = new char[size];
      memcpy(str_return_data, src_data+table_size_+pos, size); 
      return_id = id;
      return_size = size;

      break;
    }
    else
    {
      reading_pos_ = reading_pos_ + 12;
    }
  }
  if( id == find_id ) 

  {
    return str_return_data;
  }
  else 
  {
    return NULL;
  }
}

uint32_t Unpacker::GetAt(const char *src_data, uint32_t find_id, void *return_data)
{
  uint32_t id=0;
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
  uint32_t id;
  return GetAt(src_data,find_id,return_size,id);
}

char *Unpacker::GetNext(char *src_data,  uint32_t &return_id, uint32_t &return_size)
{
  char *str_return_data;
  uint32_t id, size, pos;

    dbg << "==========================================================\nUnpacker::GetNext(char *src_data,  uint32_t &return_id, uint32_t &return_size)"<<endl;
//    dbg << "src_data : " << (void *)src_data << ", return_id : " << return_id << ", return_size : "<< return_size << endl; dbgBreak(); 

  table_size_ = GetWord(src_data);
  dbg << "table size : " << table_size_ << endl;

  if( cur_pos_ < table_size_ ) {
    id   = GetWord( src_data + cur_pos_ );
    size = GetWord( src_data + cur_pos_ + 4);
    pos  = GetWord( src_data + cur_pos_ + 8);
    str_return_data = new char[size];
    
    dbg << "[Get Info from table] id: "<< id << " ("<< (void *)((char *)src_data + cur_pos_) <<"), size : " << size << " (" <<  (void *)((char *)src_data + cur_pos_+4) <<"), pos : " << pos << " ("<< (void *)((char *)src_data + cur_pos_+8)<<")"<<endl; 
    dbg << "Bring data from " << (void *)((char *)src_data+table_size_+pos) << " to "<< (void *)str_return_data << endl;  

    memcpy(str_return_data, src_data+table_size_+pos, size); 

    dbg << " Read Data is "<< (char *)str_return_data << endl;
 
    return_id = id;
    return_size = size;
    cur_pos_ +=12;

    dbg << "=========================================================="<<endl; //dbgBreak();

    return str_return_data;
  }
  else
    return NULL;
}

void *Unpacker::GetNext(void *str_return_data, void *src_data,  uint32_t &return_id, uint32_t &return_size)
{
  
    dbg << "==========================================================\nUnpacker::GetNext"<<endl;
//    dbg << "str_return_data: "<< str_return_data << ", src_data : " << src_data << ", return_id : " << return_id << ", return_size : "<< return_size << endl; dbgBreak(); 
  uint32_t id, size, pos;


  table_size_ = GetWord(src_data);

  if( cur_pos_ < table_size_ )
  {
    id   = GetWord( (char *)src_data + cur_pos_ );
    size = GetWord( (char *)src_data + cur_pos_ + 4 );
    pos  = GetWord( (char *)src_data + cur_pos_ + 8 );

    dbg << "[Get Info from table] id: "<< id << " ("<< (void *)((char *)src_data + cur_pos_) <<"), size : " << size 
        << " (" <<  (void *)((char *)src_data + cur_pos_+4) <<"), pos : " << pos 
        << " ("<< (void *)((char *)src_data + cur_pos_+8)<<")"<<endl; 
    dbg << "Bring data from " << (void *)((char *)src_data+table_size_+pos) << " to "<< str_return_data << endl;  

    memcpy(str_return_data, (char *)src_data+table_size_+pos, size); 

    dbg << " Read Data is "<< (char *)str_return_data << endl;
 
    return_id = id;
    return_size = size;
    cur_pos_ +=12;
    dbg << "=========================================================="<<endl; //dbgBreak();

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
//  dbg << "uint32_t Unpacker::GetWord(void *src_data) "<< (void *)src_data << endl;
  uint32_t return_value;
  memcpy( &return_value, src_data, sizeof(uint32_t) );
  return return_value;
}

uint32_t Unpacker::GetWord(const char *src_data)
{
//  dbg << "uint32_t Unpacker::GetWord(const char *src_data) " << (void *)src_data <<  endl;
  uint32_t return_value;
  memcpy( &return_value, src_data, sizeof(uint32_t) );
  return return_value;
}


