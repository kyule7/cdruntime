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

#ifndef _DATA_HANDLE_H 
#define _DATA_HANDLE_H
#include "cd_global.h"
#include "node_id.h"
#include "serializable.h"
#include <string>
// DataHandle can be copied by using = operator by default. Making of a copy of a handle is thus very easy. 
// This object needs to support serialization and deserialization 
// because runtime often times need to send this DataHandle object to remote node. 
// Before the DataHandle is sent over network, first it needs to be serializaed and then sent. 
// On the remote node, it needs to first figure out the object type, 
// and depending on the object type, it will create the object accordingly, and then deserialize. 
// Then entire serealization and deserialization will be implemented after Mar 1st. 

// FIXME neet to inherit serializable object and that will handle serilization complication.
class cd::DataHandle : public cd::Serializable {
  public:
    enum HandleType { kMemory = 0, kOSFile, kReference };
  private: 
    HandleType  handle_type_;
		NodeID      node_id_;
    //DRAM
    void*       address_data_;
    uint64_t    len_;
    //FILE
    char*       file_name_;
    //Reference
    std::string ref_name_;
    uint64_t    ref_offset_;

  public: 
    DataHandle() 
      : handle_type_(kMemory), address_data_(0), len_(0), file_name_(0), ref_name_("INITIAL_DATAHANDLE"), ref_offset_(0) {} 

    DataHandle(std::string ref_name, uint64_t ref_offset) 
      : handle_type_(kReference), address_data_(0), len_(0), file_name_(0), ref_name_(ref_name), ref_offset_(ref_offset)
    { 
//      ref_name_ = ref_name.c_str(); 
//      ref_offset_ = ref_offset;
    }

    DataHandle(char* file_name)
      : handle_type_(kOSFile), address_data_(0), len_(0), file_name_(file_name), ref_name_("INITIAL_DATAHANDLE"), ref_offset_(0) {}

    DataHandle(void* address_data, const uint64_t& len)
      : handle_type_(kMemory), address_data_(address_data), len_(len), file_name_(0), ref_name_("INITIAL_DATAHANDLE"), ref_offset_(0) {}

    ~DataHandle() {}

    char*       file_name()    { return file_name_; }
    std::string ref_name()     { return ref_name_; }
    uint64_t    ref_offset()   { return ref_offset_; }
    void*       address_data() { return address_data_; }
    uint64_t    len()          { return len_; }
    HandleType  handle_type()  { return handle_type_; }

    void        set_file_name(const char* file_name)       { file_name_ = const_cast<char*>(file_name); }
    void        set_ref_name(std::string ref_name)         { ref_name_ = ref_name; }
    void        set_ref_offset(uint64_t ref_offset)        { ref_offset_ = ref_offset; }
    void        set_address_data(const void* address_data) { address_data_ = (void*)address_data; }
    void        set_len(uint64_t len)                      { len_ = len; }
    void        set_handle_type(const HandleType& handle_type) { handle_type_=handle_type; }

  public: 
    //we need serialize deserialize interface here.
    virtual void * Serialize(uint64_t *len_in_bytes)
    {
      //STUB
      return 0;  
    }
    virtual void Deserialize(void * object) 
    {
      //STUB
      return;
    }
};


#endif
