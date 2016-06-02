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

#include "data_handle.h"
#include "util.h"
#include "packer.h"
#include "unpacker.h"
using namespace cd;
using namespace cd::internal;
using namespace std;


DataHandle::DataHandle(void) 
  : handle_type_(kMemory), 
    address_data_(0), len_(0), 
    fp_(NULL), fpos_(0), 
    ref_name_(0), ref_offset_(0), 
    node_id_(-1) 
{ 
  strcpy(file_name_, INIT_FILE_PATH);
}

DataHandle::DataHandle(const DataHandle &that) 
  : handle_type_(that.handle_type_), 
  address_data_(that.address_data_), len_(that.len_), 
    fp_(that.fp_), fpos_(that.fpos_), 
    ref_name_(that.ref_name_), ref_offset_(that.ref_offset_), 
    node_id_(that.node_id_) 
{ 
  strcpy(file_name_, that.file_name_);
}

// DataHandle for preservation to memory
DataHandle::DataHandle(HandleType handle_type, 
                       void *address_data, const uint64_t& len, 
                       const NodeID &node_id)
  : handle_type_(handle_type), 
    address_data_(address_data), len_(len), 
    fp_(NULL), fpos_(0), 
    ref_name_(0), ref_offset_(0), 
    node_id_(node_id)
{ 
  strcpy(file_name_, INIT_FILE_PATH); 
}

// DataHandle for preservation to file system
DataHandle::DataHandle(HandleType handle_type, 
                       void *address_data, const uint64_t& len, 
                       const NodeID &node_id, 
                       const std::string &file_name, FILE *fp, long fpos)
  : handle_type_(handle_type), 
    address_data_(address_data), len_(len), 
    fp_(fp), fpos_(fpos), 
    ref_name_(0), ref_offset_(0), 
    node_id_(node_id)
{ 
  strcpy(file_name_, file_name.c_str()); 
}

// DataHandle for preservation via reference
DataHandle::DataHandle(HandleType handle_type, 
                       void *address_data, const uint64_t& len, 
                       std::string ref_name, uint64_t ref_offset, const NodeID &node_id)
  : handle_type_(handle_type), 
    address_data_(address_data), len_(len), 
    fpos_(0), ref_offset_(ref_offset), 
    node_id_(node_id)
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

ostream& cd::internal::operator<<(ostream& str, const DataHandle& dh)
{
  return str << "\n== Data Handle Information ======="
             << "\nhandle T:\t" << dh.handle_type_  
             << "\nNode ID :\t" << dh.node_id_  
             << "\nAddress :\t" << dh.address_data_
             << "\nlength  :\t" << dh.len_
             << "\nfilename:\t" << dh.file_name_
             << "\nref_name:\t" << dh.ref_name_
             << "\nref_offset:\t"<<dh.ref_offset_
             << "\n==================================\n";
}

string DataHandle::GetString(void) const 
{
  return ( string("\n== Data Handle Information =======")
         + string("\nhandle T:\t") + to_string(handle_type_)
         + string("\nNode ID :\t") + node_id_.GetString()
//         + string("\nAddress :\t") + to_string(static_cast<char *>(address_data_))
         + string("\nlength  :\t") + to_string(len_)
         + string("\nfilename:\t") + string(file_name_)
         + string("\nref_name:\t") + to_string(ref_name_)
         + string("\nref_offset:\t") + to_string(ref_offset_)
         + string("\n==================================\n") );
}


void *DataHandle::Serialize(uint64_t& len_in_bytes)
{
  CD_DEBUG("\nData Handle Serialize\n");

  Packer data_packer;
  uint64_t node_id_packed_len=0;
  void *node_id_packed_p = node_id_.Serialize(node_id_packed_len);
  assert(node_id_packed_len != 0);
  data_packer.Add(DATA_PACKER_NODE_ID, node_id_packed_len, node_id_packed_p);
  data_packer.Add(DATA_PACKER_HANDLE_TYPE, sizeof(HandleType), &handle_type_);
  data_packer.Add(DATA_PACKER_ADDRESS, sizeof(void*), &address_data_);

  CD_DEBUG("address data is packed : %p\n\n", address_data_);

  data_packer.Add(DATA_PACKER_LEN, sizeof(uint64_t), &len_);
  data_packer.Add(DATA_PACKER_FILENAME, sizeof(file_name_), file_name_);
 
//  uint64_t ref_name_key = cd_hash(ref_name);
  data_packer.Add(DATA_PACKER_REFNAME, sizeof(uint64_t), &ref_name_); // string.size() + 1 is for '\0'
  data_packer.Add(DATA_PACKER_REFOFFSET, sizeof(uint64_t), &ref_offset_); 

  CD_DEBUG("\nData Handle Serialize Done\n");

  return data_packer.GetTotalData(len_in_bytes);  
}

void DataHandle::Deserialize(void *object) 
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
