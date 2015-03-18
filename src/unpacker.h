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

#ifndef _UNPACKER_H
#define _UNPACKER_H
/**
 * @file unpacker.h
 * @author Jinsuk Chung, Kyushick Lee
 * @date March 2015
 *
 * \brief Packing object for serialization
 *
 */
#include "cd_global.h"


/** \addtogroup utilities Utilities for CD runtime
 *
 *@{
 *
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
class cd::Unpacker    
{
  public:

    enum UnpackerErrT {kOK =0, kMallocFailed, kReallocFailed, kNotFound};
    Unpacker();
    virtual ~Unpacker();


    char *GetAt(const char *src_data, uint32_t find_id, uint32_t &return_size, uint32_t &dwGetID); 
    char *GetAt(const char *src_data, uint32_t find_id, uint32_t &return_size);



    uint32_t GetAt(const char *src_data, uint32_t find_id, void *return_data);


    char *GetNext(char *src_data,  uint32_t &dwGetID, uint32_t &return_size);  

    void *GetNext(void *str_return_data, void *src_data,  uint32_t &return_id, uint32_t &return_size);
    void SeekInit();




  private:
    uint32_t table_size_;
    uint32_t cur_pos_;  
    uint32_t reading_pos_;

  private:
    uint32_t GetWord(const char *src_data);

    uint32_t GetWord(void *src_data);

};

/** @} */ // End group utilities
 


#endif 
