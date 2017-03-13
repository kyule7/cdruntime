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

#ifndef _SERIALIZABLE_H
#define _SERIALIZABLE_H
/**
 * @file serializable.h
 * @author Jinsuk Chung, Kyushick Lee
 * @date March 2015
 *
 * \brief Abstract base class for serialization.
 * Other classes which need serialization will inherits this class.
 *
 */

#include "cd_global.h"
#include "persistence/data_store.h"
//class packer::CDPacker;
using namespace packer;
namespace cd {

class Serializable {
  public:
    virtual void *Serialize(uint64_t &len_in_bytes)=0;
    virtual void Deserialize(void *object)=0;
};

class PackerSerializable : public Serializable {
  public:
    // PreserveObject must append the table for serialized object to data chunk.
    // It must return the offset of table chunk.
    uint64_t total_size_; 
    uint64_t table_offset_;
    uint64_t table_type_;
    virtual uint64_t PreserveObject(DataStore *packer)=0;
    virtual uint64_t GetTableSize(void)=0;
};

}
#endif
