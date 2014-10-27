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
#include "packer.h"
#include "unpacker.h"
#include <string>
#include <cstring>

// DataHandle can be copied by using = operator by default. Making of a copy of a handle is thus very easy. 
// This object needs to support serialization and deserialization 
// because runtime often times need to send this DataHandle object to remote node. 
// Before the DataHandle is sent over network, first it needs to be serializaed and then sent. 
// On the remote node, it needs to first figure out the object type, 
// and depending on the object type, it will create the object accordingly, and then deserialize. 
// Then entire serealization and deserialization will be implemented after Mar 1st. 

// FIXME neet to inherit serializable object and that will handle serilization complication.
namespace cd {

class DataHandle : public Serializable {
  friend class CDEntry;
  public:
    enum HandleType { kMemory = 0, kOSFile, kReference, kSource };
  private:
    enum { 
      DATA_PACKER_NODE_ID=0,
      DATA_PACKER_HANDLE_TYPE,
      DATA_PACKER_ADDRESS,
      DATA_PACKER_LEN, 
      DATA_PACKER_FILENAME, 
      DATA_PACKER_REFNAME, 
      DATA_PACKER_REFOFFSET
    };

    HandleType  handle_type_;
		NodeID      node_id_;
    //DRAM
    void*       address_data_;
    uint64_t    len_;
    //FILE
    std::string file_name_;
    //Reference
    std::string ref_name_;
    uint64_t    ref_offset_;

  public: 
    DataHandle() 
      : handle_type_(kMemory), address_data_(0), len_(0), file_name_(), ref_name_(), ref_offset_(0) {} 

    DataHandle(std::string ref_name, uint64_t ref_offset) 
      : handle_type_(kReference), address_data_(0), len_(0), file_name_(), ref_name_(ref_name), ref_offset_(ref_offset) {}

    DataHandle(const char* file_name)
      : handle_type_(kOSFile), address_data_(0), len_(0), file_name_(file_name), ref_name_(), ref_offset_(0) {}

    DataHandle(void* address_data, const uint64_t& len)
      : handle_type_(kMemory), address_data_(address_data), len_(len), file_name_(), ref_name_(), ref_offset_(0) {}

    DataHandle(HandleType handle_type, void* address_data, const uint64_t& len)
      : handle_type_(handle_type), address_data_(address_data), len_(len), file_name_(), ref_name_(), ref_offset_(0) {}

    DataHandle(HandleType handle_type, void* address_data, const uint64_t& len, std::string ref_name, uint64_t ref_offset)
      : handle_type_(handle_type), address_data_(address_data), len_(len), file_name_(), ref_name_(ref_name), ref_offset_(ref_offset) {}

    ~DataHandle() {}

    std::string file_name()    const { return file_name_; }
    std::string ref_name()     const { return ref_name_; }
    uint64_t    ref_offset()   const { return ref_offset_; }
    void*       address_data() const { return address_data_; }
    uint64_t    len()          const { return len_; }
    HandleType  handle_type()  const { return handle_type_; }
    NodeID      node_id()      const { return node_id_; }

    void        set_file_name(const char* file_name)       { file_name_    = std::string(file_name); }
    void        set_ref_name(std::string ref_name)         { ref_name_     = ref_name; }
    void        set_ref_offset(uint64_t ref_offset)        { ref_offset_   = ref_offset; }
    void        set_address_data(const void* address_data) { address_data_ = (void*)address_data; }
    void        set_len(uint64_t len)                      { len_ = len; }
    void        set_handle_type(const HandleType& handle_type) { handle_type_=handle_type; }


    bool operator==(const DataHandle& that) const {
      return (handle_type_ == that.handle_type_) && (node_id_ == that.node_id_) 
             && ( address_data_== that.address_data_) && (len_ == that.len_) 
             && (file_name_ == that.file_name_) && (ref_name_ == that.ref_name_) 
             && (ref_offset_ == that.ref_offset_);
    }
  public: 
    //we need serialize deserialize interface here.
    void *Serialize(uint32_t& len_in_bytes)
    {
      //std::cout << "\nData Handle Serialize\n" << std::endl;
      Packer data_packer;
      uint32_t node_id_packed_len=0;
      void *node_id_packed_p = node_id_.Serialize(node_id_packed_len);
      assert(node_id_packed_len != 0);
      data_packer.Add(DATA_PACKER_NODE_ID, node_id_packed_len, node_id_packed_p);
      data_packer.Add(DATA_PACKER_HANDLE_TYPE, sizeof(HandleType), &handle_type_);
      data_packer.Add(DATA_PACKER_ADDRESS, sizeof(void*), &address_data_);
      //std::cout << "address data is packed : "<< address_data_ << "\n\n" << std::endl; //getchar();
      data_packer.Add(DATA_PACKER_LEN, sizeof(uint64_t), &len_);
      data_packer.Add(DATA_PACKER_FILENAME, file_name_.size()+1, const_cast<char*>(file_name_.c_str())); // string.size() + 1 is for '\0'
      data_packer.Add(DATA_PACKER_REFNAME, ref_name_.size()+1, const_cast<char*>(ref_name_.c_str())); // string.size() + 1 is for '\0'
      data_packer.Add(DATA_PACKER_REFOFFSET, sizeof(uint64_t), &ref_offset_); 
      //std::cout << "\nData Handle Serialize Done\n" << std::endl;
      return data_packer.GetTotalData(len_in_bytes);  
    }
    void Deserialize(void* object) 
    {
      //std::cout << "\nData Handle Deserialize\n" << object << std::endl;
      Unpacker data_unpacker;
      uint32_t return_size;
      uint32_t dwGetID;
      void *node_id_unpacked=0;

      node_id_unpacked = data_unpacker.GetNext((char *)object, dwGetID, return_size);
      //std::cout << "Before Deserialize node_id"<<std::endl;
      node_id_.Deserialize(node_id_unpacked);
      //std::cout << "1st unpacked thing in data_handle : " << node_id_ << ", return size : " << return_size << std::endl;

      handle_type_ = *(HandleType *)data_unpacker.GetNext((char *)object, dwGetID, return_size);
      //std::cout << "2nd unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;

      void *tmp_address_data = data_unpacker.GetNext((char *)object, dwGetID, return_size);
      memcpy(&address_data_, tmp_address_data, sizeof(void *));
      //std::cout << "3rd unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;

      len_ = *(uint64_t *)data_unpacker.GetNext((char *)object, dwGetID, return_size);
      //std::cout << "4th unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;    


      char* unpacked_file_name=0;
      char* unpacked_ref_name=0;
      unpacked_file_name = (char *)data_unpacker.GetNext((char *)object, dwGetID, return_size);
      //std::cout << "5th unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;    

      unpacked_ref_name = (char *)data_unpacker.GetNext((char *)object, dwGetID, return_size);
      //std::cout << "6th unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;    

      file_name_ = unpacked_file_name;
      ref_name_ = unpacked_ref_name;

      ref_offset_ = *(uint64_t *)data_unpacker.GetNext((char *)object, dwGetID, return_size);
      //std::cout << "7th unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;    
    }

    DataHandle& operator=(const DataHandle& that) {
      handle_type_ = that.handle_type();
  		node_id_     = that.node_id();
      address_data_= that.address_data();
      len_         = that.len();
      file_name_   = that.file_name();
      ref_name_    = that.ref_name();
      ref_offset_  = that.ref_offset();
      return *this;
    }

};

std::ostream& operator<<(std::ostream& str, const DataHandle& dh);

}

#endif
