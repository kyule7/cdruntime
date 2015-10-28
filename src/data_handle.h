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

/**
 * @file data_handle.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * \brief Data handle
 *
 * Description on data handle
 */

#include "cd_global.h"
#include "node_id.h"
#include "serializable.h"
#include "packer.h"
#include "unpacker.h"
#include <string>
#include <cstring>


namespace cd {
  namespace internal {

/**
  * @brief Information of data regarding preservation
  *
  * DataHandle can be copied by using = operator by default. Making of a copy of a handle is thus very easy. 
  * This object needs to support serialization and deserialization 
  * because runtime often times need to send this DataHandle object to remote node. 
  * Before the DataHandle is sent over network, first it needs to be serializaed and then sent. 
  * On the remote node, it needs to first figure out the object type, 
  * and depending on the object type, it will create the object accordingly, and then deserialize. 
  * 
  * 
  */ 
class DataHandle : public Serializable {
  friend class CDEntry;
  friend class CD;
  friend class HeadCD;
  friend std::ostream& operator<<(std::ostream& str, const DataHandle& dh);
  public:
    enum HandleType { 
      kMemory=0, 
      kOSFile, 
      kReference, 
      kSource, 
      kPFS 
    };
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
    //DRAM
    void *address_data_;
    uint64_t len_;

    //FILE
    char file_name_[MAX_FILE_PATH]; // This is matched with CDFileHandle::unique_filename_
    FILE *fp_; // This is matched with CDFileHandle::fp_
    long fpos_;

    //Parallel Filesystem
    COMMLIB_Offset  parallel_file_offset_;

    //Reference
    ENTRY_TAG_T ref_name_;
    uint64_t ref_offset_;
    NodeID node_id_;

  private:
    DataHandle(void) 
      : handle_type_(kMemory), address_data_(0), len_(0), fp_(NULL), fpos_(0), ref_name_(0), ref_offset_(0) 
    { 
      strcpy(file_name_, INIT_FILE_PATH);
    }

//    DataHandle(std::string ref_name, uint64_t ref_offset, 
//               const NodeID &node_id) 
//      : handle_type_(kReference), address_data_(0), len_(0), ref_offset_(ref_offset) 
//    { 
//      strcpy(file_name_, INIT_FILE_PATH); 
//      ref_name_ = cd_hash(ref_name); 
//      tag2str[ref_name_] = ref_name; 
//      node_id_ = node_id; 
//    }
//
//    DataHandle(const char* file_name, 
//               const NodeID &node_id)
//      : handle_type_(kOSFile), address_data_(0), len_(0), ref_name_(0), ref_offset_(0) 
//    { 
//      strcpy(file_name_, file_name); 
//      node_id_ = node_id; 
//    }
//
//    DataHandle(void* address_data, const uint64_t& len, 
//               const NodeID &node_id)
//      : handle_type_(kMemory), address_data_(address_data), len_(len), ref_name_(0), ref_offset_(0) 
//    { 
//      strcpy(file_name_, INIT_FILE_PATH); 
//      node_id_ = node_id; 
//    }

    // DataHandle for preservation to memory
    DataHandle(HandleType handle_type, 
               void* address_data, const uint64_t& len, 
               const NodeID &node_id)
      : handle_type_(handle_type), address_data_(address_data), len_(len), fp_(NULL), fpos_(0), ref_name_(0), ref_offset_(0)
    { 
      strcpy(file_name_, INIT_FILE_PATH); 
      node_id_ = node_id; 
    }

    // DataHandle for preservation to file system
    DataHandle(HandleType handle_type, 
               void* address_data, const uint64_t& len, 
               const NodeID &node_id, const std::string &file_name, FILE *fp=NULL, long fpos=0)
      : handle_type_(handle_type), address_data_(address_data), len_(len), fp_(fp), fpos_(fpos), ref_name_(0), ref_offset_(0)
    { 
      strcpy(file_name_, file_name.c_str()); 
      node_id_ = node_id; 
    }

    // DataHandle for preservation via reference
    DataHandle(HandleType handle_type, 
               void* address_data, const uint64_t& len, 
               std::string ref_name, uint64_t ref_offset)
      : handle_type_(handle_type), address_data_(address_data), len_(len), fpos_(0), ref_offset_(ref_offset) 
    { 
      // There is no NodeID passed to newly created DataHandle object.
      // The reason for this is that we do not track the destinatino information when we preserve,
      // but we track it in the restoration routine.
      // Basically, it removes the overhead at failure-free execuation, 
      // but reqires more overhead in reexecution time which is rare.
      strcpy(file_name_, INIT_FILE_PATH); 
      ref_name_ = cd_hash(ref_name); 
      tag2str[ref_name_] = ref_name; 
    }

    ~DataHandle() {}

    void        set_file_name(const char *file_name)       { strcpy(file_name_, file_name); }
    void        set_ref_name(std::string ref_name)         { ref_name_     = cd_hash(ref_name); tag2str[ref_name_] = ref_name;}
    void        set_ref_offset(uint64_t ref_offset)        { ref_offset_   = ref_offset; }
    void        set_address_data(const void *address_data) { address_data_ = (void *)address_data; }
    void        set_len(uint64_t len)                      { len_ = len; }
    void        set_handle_type(const HandleType& handle_type) { handle_type_ = handle_type; }
public:
    std::string file_name(void)    const { return file_name_; }
    std::string ref_name(void)     const { return tag2str[ref_name_]; }
    ENTRY_TAG_T ref_name_tag(void) const { return ref_name_; }
    uint64_t    ref_offset(void)   const { return ref_offset_; }
    void*       address_data(void) const { return address_data_; }
    uint64_t    len(void)          const { return len_; }
    HandleType  handle_type(void)  const { return handle_type_; }
    NodeID      node_id(void)      const { return node_id_; }


    bool operator==(const DataHandle& that) const {
      return (handle_type_ == that.handle_type_) && (node_id_ == that.node_id_) 
             && ( address_data_== that.address_data_) && (len_ == that.len_) 
             && !strcmp(file_name_, that.file_name_) && (ref_name_ == that.ref_name_) 
             && (ref_offset_ == that.ref_offset_);
    }

  std::string GetString(void) const;
  uint64_t GetOffset(void) const {
    return fpos_ + ref_offset_;
  }
private:
//  public: 
    //we need serialize deserialize interface here.
    void *Serialize(uint32_t& len_in_bytes)
    {
      CD_DEBUG("\nData Handle Serialize\n");

      Packer data_packer;
      uint32_t node_id_packed_len=0;
      void *node_id_packed_p = node_id_.Serialize(node_id_packed_len);
      assert(node_id_packed_len != 0);
      data_packer.Add(DATA_PACKER_NODE_ID, node_id_packed_len, node_id_packed_p);
      data_packer.Add(DATA_PACKER_HANDLE_TYPE, sizeof(HandleType), &handle_type_);
      data_packer.Add(DATA_PACKER_ADDRESS, sizeof(void*), &address_data_);

      CD_DEBUG("address data is packed : %p\n\n", address_data_);

      data_packer.Add(DATA_PACKER_LEN, sizeof(uint64_t), &len_);
      data_packer.Add(DATA_PACKER_FILENAME, sizeof(file_name_), file_name_);
 
//      uint64_t ref_name_key = cd_hash(ref_name);
      data_packer.Add(DATA_PACKER_REFNAME, sizeof(uint64_t), &ref_name_); // string.size() + 1 is for '\0'
      data_packer.Add(DATA_PACKER_REFOFFSET, sizeof(uint64_t), &ref_offset_); 

      CD_DEBUG("\nData Handle Serialize Done\n");

      return data_packer.GetTotalData(len_in_bytes);  
    }
    void Deserialize(void* object) 
    {
      CD_DEBUG("\nData Handle Deserialize %p\n", object);

      Unpacker data_unpacker;
      uint32_t return_size;
      uint32_t dwGetID;

      void *node_id_unpacked=0;
      node_id_unpacked = data_unpacker.GetNext((char *)object, dwGetID, return_size);

      CD_DEBUG("Before Deserialize node_id\n");

      node_id_.Deserialize(node_id_unpacked);
      
      CD_DEBUG("1st unpacked thing in data_handle : %s, return size : %u\n", node_id_.GetString().c_str(), return_size);

      handle_type_ = *(HandleType *)data_unpacker.GetNext((char *)object, dwGetID, return_size);

      CD_DEBUG("2nd unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);

      void *tmp_address_data = data_unpacker.GetNext((char *)object, dwGetID, return_size);
      memcpy(&address_data_, tmp_address_data, sizeof(void *));

      CD_DEBUG("3rd unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);

      len_ = *(uint64_t *)data_unpacker.GetNext((char *)object, dwGetID, return_size);

      CD_DEBUG("4th unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);

      char *file_name_p = (char *)data_unpacker.GetNext((char *)object, dwGetID, return_size);
      strcpy(file_name_, file_name_p);

      CD_DEBUG("5th unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);

      ref_name_ = *(uint64_t *)data_unpacker.GetNext((char *)object, dwGetID, return_size);

      CD_DEBUG("6th unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);

      ref_offset_ = *(uint64_t *)data_unpacker.GetNext((char *)object, dwGetID, return_size);

      CD_DEBUG("7th unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);
    }

    DataHandle& operator=(const DataHandle& that) {
      handle_type_ = that.handle_type_;
      node_id_     = that.node_id_;
      address_data_= that.address_data_;
      len_         = that.len();
      strcpy(file_name_, that.file_name_);
      fp_ = that.fp_;
      fpos_ = that.fpos_;
      ref_name_    = that.ref_name_;
      ref_offset_  = that.ref_offset_;
      return *this;
    }

};

std::ostream& operator<<(std::ostream& str, const DataHandle& dh);

  } // namespace internal ends
} // namespace cd ends

#endif
