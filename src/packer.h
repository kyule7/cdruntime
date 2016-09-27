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

#ifndef _PACKER_H
#define _PACKER_H


// Packer packer;
// packer.Add(ID_NAME, 100, name_ptr);
// packer.Add(ID_NAME, 100, name_ptr);
// packer.Add(ID_NAME, 100, name_ptr);
// char *serialized_data = packer.GetTotalData( total_data_size);
// packer.Add(ID_SOMETHING, 10, something_ptr);
// char *serialized_data = packer.GetTotalData( total_data_size);
#include <stdint.h>
#include "serializable.h"
#define MEGABYTE        1048576
#define GIGABYTE     1073741824 
#define TWO_GIGABYTE 2147483648
#define FOUR_GIGABYTE 4294967296
#define PACKER_ENTRY_SIZE 24
#define PACKER_UNIT_0 8
#define PACKER_UNIT_1 8
#define PACKER_UNIT_2 16
#define TABLE_GROW_UNIT 4096
#define DATA_GROW_UNIT 8388608 // 8KB
namespace cd {
class Packer {
  public:

///@brief Enumerator internally used in Packer.
    enum PackerErrT { kOK =0,         //!< No errors.
                      kError, 
                      kMallocFailed,  //!< Malloc failed.
                      kReallocFailed, //!< Realloc failed.
                      };
    Packer(bool alloc_flag=true);
    Packer(uint64_t table_grow_unit, uint64_t data_grow_unit, bool alloc_flag=true);
    virtual ~Packer();


///@brief Add data to pack in packer data structure.
    virtual uint64_t Add(uint64_t id, uint64_t length, const void *position);
    virtual uint64_t Add(const void *position, uint64_t length, uint64_t id);

///@brief Get total size required for table (metadata) and data.
    virtual char *GetTotalData(uint64_t &total_data_size);

///@brief Grow buffer size internally used in packer.
    virtual void SetBufferGrow( uint64_t table_grow_unit, uint64_t data_grow_unit);
    virtual uint64_t ClearData(bool reuse);
    char *GetDataPtr(void) { return ptr_data_; }
    uint64_t GetDataSize(void) { return used_data_size_; }
  protected:
    uint64_t AddData(uint64_t id, uint64_t length, uint64_t position, char *ptr_data);
    uint64_t CheckAlloc(void);
//    virtual uint64_t CheckRealloc(uint64_t length);
    virtual uint64_t CheckRealloc(uint64_t length, uint64_t required_size=0);
    void WriteWord(char *dst_buffer,uint64_t value);
    virtual void WriteData(char *ptr_data, uint64_t length, uint64_t position);
    void Copy(const Packer& that);
  protected:

    uint64_t table_grow_unit_;  // Growth unit is used when we need to increase the memory allocation size. This scheme is needed because we don't know how much will be needed at the beginning.  
    uint64_t data_grow_unit_;   // This is the same as above but it is for the data section.  (Two section exist) 

    uint64_t table_size_;   //Current table size.
    uint64_t data_size_; 
    uint64_t used_table_size_; 
    uint64_t used_data_size_; 

    bool alloc_flag_;

    char *ptr_table_;
    char *ptr_data_;
  public:
    bool Alloced(void) {
      return alloc_flag_ != false;
    }
    uint64_t used_data_size(void) {
     return used_data_size_;
    } 
};


// Unpacker /////////////////////////////////////////////////


/** \addtogroup utilities Utilities for CD runtime
 *
 *@{
 *
 */

/* @class cd::Unpacker
 * @brief Unpacking data to reconstruct object 
 * Data layout
 * [TableLength][ID][Length][Position][ID][Length][Position][ID][Length][Position]...[DATAChunk]
 * 
 * First 4 byte is the size of the chunk
 * Next following is the ones that describes all data's positions.   
 * ID is identifier
 * Length is the length of the data in bytes, 
 * and Position is the relative position starting from where consqutive data is located
 *
 */ 
class Unpacker {
  public:
    
///@brief Enumerator internally used in Unpacker.
    enum UnpackerErrT { kOK =0,         //!< No errors. 
                        kMallocFailed,  //!< Malloc failed.
                        kReallocFailed, //!< Realloc failed.
                        kNotFound       //!< Could not find the data to unpack.
                        };
    Unpacker();
    virtual ~Unpacker();

///@brief Get actual data in packer data structured from data table.
    void GetAt(const char *src_data, uint64_t find_id, void *target_ptr, uint64_t &return_size, uint64_t &return_id);

///@brief Get actual data in packer data structured from data table.
    char *GetAt(const char *src_data, uint64_t find_id, uint64_t &return_size, uint64_t &dwGetID); 

///@brief Get actual data in packer data structured from data table.
    char *GetAt(const char *src_data, uint64_t find_id, uint64_t &return_size);


///@brief Get actual data in packer data structured from data table.
    uint64_t GetAt(const char *src_data, uint64_t find_id, void *return_data);


///@brief Get next data from data table.
    char *GetNext(char *src_data,  uint64_t &dwGetID, uint64_t &return_size, bool alloc=true, void *dst=0, uint64_t dst_size=0);  

///@brief Get next data from data table.
    void *GetNext(void *str_return_data, void *src_data,  uint64_t &return_id, uint64_t &return_size);

///@brief Peek next data from data table.
    char *PeekNext(char *src_data, uint64_t &return_size, uint64_t &return_id);

///@brief Initialize seek.
    void SeekInit();




  protected:
    uint64_t table_size_;
    uint64_t cur_pos_;  
    uint64_t reading_pos_;

  protected:
    virtual void ReadData(char *str_return_data, char *src_data, uint64_t pos, uint64_t size);
    uint64_t GetWord(const char *src_data);

    uint64_t GetWord(void *src_data);

};

class PackerSerializable : public Serializable {
  public:
    virtual void PreserveObject(Packer *packer)=0;
};
}
#endif
