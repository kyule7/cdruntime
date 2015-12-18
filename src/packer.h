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

/**
 * @file packer.h
 * @author Jinsuk Chung, Kyushick Lee
 * @date March 2015
 *
 * \brief Packing object for serialization
 *
 */
#include "cd_global.h"
#include "cd_def_internal.h" 
#include "cstdint"

// Packer packer;
// packer.Add(ID_NAME, 100, name_ptr);
// packer.Add(ID_NAME, 100, name_ptr);
// packer.Add(ID_NAME, 100, name_ptr);
// char *serialized_data = packer.GetTotalData( total_data_size);
// packer.Add(ID_SOMETHING, 10, something_ptr);
// char *serialized_data = packer.GetTotalData( total_data_size);


/** \addtogroup utilities Utilities for CD runtime
 *
 *@{
 *
 */

/**@class cd::Packer
 * @brief Packing object for serialization 
 *
 *
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

class cd::Packer  
{
  public:

///@brief Enumerator internally used in Packer.
    enum PackerErrT { kOK =0,         //!< No errors. 
                      kMallocFailed,  //!< Malloc failed.
                      kReallocFailed, //!< Realloc failed.
                      };
    Packer();
    Packer(uint32_t table_grow_unit, uint32_t data_grow_unit);
    virtual ~Packer();


///@brief Add data to pack in packer data structure.
    virtual uint32_t Add(uint32_t id, uint32_t length, const void *position);

///@brief Get total size required for table (metadata) and data.
    virtual char *GetTotalData(uint32_t &total_data_size);

///@brief Grow buffer size internally used in packer.
    virtual void SetBufferGrow( uint32_t table_grow_unit, uint32_t data_grow_unit);



  private:
    uint32_t ClearData();
    uint32_t AddData(uint32_t id, uint32_t length, uint32_t position, char *ptr_data);
    uint32_t CheckAlloc();
    uint32_t CheckRealloc(uint32_t length);
    void WriteWord(char *dst_buffer,uint32_t value);
    void WriteData(char *ptr_data, uint32_t length, uint32_t position);



  private:

    uint32_t table_grow_unit_;  // Growth unit is used when we need to increase the memory allocation size. This scheme is needed because we don't know how much will be needed at the beginning.  
    uint32_t data_grow_unit_;   // This is the same as above but it is for the data section.  (Two section exist) 

    uint32_t table_size_;   //Current table size.
    uint32_t data_size_; 
    uint32_t used_table_size_; 
    uint32_t used_data_size_; 

    uint32_t alloc_flag_;

    char *ptr_table_;
    char *ptr_data_;
};


/** @} */ // End group utilities
#endif
