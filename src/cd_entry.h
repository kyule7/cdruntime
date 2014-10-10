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

#ifndef _CD_ENTRY_H 
#define _CD_ENTRY_H
#include "cd_global.h"
#include "cd.h"
#include "serializable.h"
#include "data_handle.h"
#include "util.h"
#include <stdint.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "unixlog.h"

class cd::CDEntry : public cd::Serializable
{
  friend class cd::CD;
  private:
    enum { 
      ENTRY_PACKER_NAME=0,
//      ENTRY_PACKER_PTRCD,
      ENTRY_PACKER_PRESERVETYPE,
      ENTRY_PACKER_SRC,
      ENTRY_PACKER_DST 
    };
    // need a unique name to do via reference, 
    // this variable can be empty string when this is not needed
    std::string name_;    
    cd::CD*     ptr_cd_;
    DataHandle  src_data_;
    DataHandle  dst_data_;
    cd::CDPreserveT preserve_type_; // already determined according to class 
//		struct tsn_lsn_struct lsn, durable_lsn;

  public:
    enum CDEntryErrT {kOK=0, kOutOfMemory, kFileOpenError};
    
    CDEntry(){}
    CDEntry(const DataHandle& src_data, 
            const DataHandle& dst_data, 
            const char* entry_name) 
    {
      src_data_ = src_data;
      dst_data_ = dst_data;
      if(entry_name == 0) name_.clear();
      else name_ = entry_name;
    }

    ~CDEntry()
    {
      //FIXME we need to delete allocated (preserved) space at some point, should we do it when entry is destroyed?  when is appropriate for now let's do it now.
      if( dst_data_.address_data() != NULL ) {
//        DATA_FREE( dst_data_.address_data() );
      }
 
      if( !dst_data_.file_name_.empty() )  {
//        delete dst_data_.file_name(); 
      }
    }

    // FIXME: currently it is assumed that my cd is always in the same memory space.
    // In the future we might need a distributed CD structure. 
    // For example right now we have one list who handles all the CDEntries for that CD. 
    // But this list might need to be distributed if that CD spans among multiple threads. 
    // If we have that data structure the issue here will be solved anyways. 
    void set_my_cd(cd::CD* ptr_cd) { ptr_cd_ = ptr_cd; }
    cd::CD* ptr_cd() const { return ptr_cd_; }

		CDEntryErrT Delete(void);

  public:
		std::string name() const { return name_.c_str(); }
    bool isViaReference() { return (dst_data_.handle_type() == DataHandle::kReference); }

    CDEntryErrT SaveMem(void);
    CDEntryErrT SaveFile(std::string base, 
                         bool open, 
                         struct tsn_log_struct *log);
    CDEntryErrT Save(void);

    void CloseFile(struct tsn_log_struct *log);


    //FIXME We need another Restore function that would accept offset and length, 
    // so basically it will use dst_data_ as base and then offset and length is used to restore only partial of the original. 
    // This functionality is required for via reference to work. Basically when via reference is doing restoration, 
    // it will first try to find the entry it self by going up and look into parent's directories. 
    // After finding one, it will retrive the dst_data_ and then copy from them.

    CDEntryErrT Restore(bool open, struct tsn_log_struct *log);
    CDEntryErrT Restore(void);

    void *Serialize(uint32_t &len_in_bytes) 
    {

      std::cout << "\nCD Entry Serialize\n" << std::endl;
      Packer entry_packer;
//      uint32_t ptr_cd_packed_len=0;
//      void *ptr_cd_packed_p = ptr_cd_->Serialize(ptr_cd_packed_len);

      uint32_t src_packed_len=0;
      std::cout << "\nsrc Serialize\n" << std::endl;
      void *src_packed_p = src_data_.Serialize(src_packed_len);
      uint32_t dst_packed_len=0;
      std::cout << "\ndst Serialize\n" << std::endl;
      void *dst_packed_p = dst_data_.Serialize(dst_packed_len);
//      assert(ptr_cd_packed_len != 0);
      assert(src_packed_len != 0);
      assert(dst_packed_len != 0);

      entry_packer.Add(ENTRY_PACKER_NAME, name_.size()+1, const_cast<char*>(name_.c_str())); // string.size() + 1 is for '\0'
//      entry_packer.Add(ENTRY_PACKER_PTRCD, ptr_cd_packed_len, ptr_cd_packed_p);

      entry_packer.Add(ENTRY_PACKER_PRESERVETYPE, sizeof(cd::CDPreserveT), &preserve_type_);
      entry_packer.Add(ENTRY_PACKER_SRC, src_packed_len, src_packed_p);
      entry_packer.Add(ENTRY_PACKER_DST, dst_packed_len, dst_packed_p); 
      std::cout << "\nCD Entry Serialize Done\n" << std::endl;

      return entry_packer.GetTotalData(len_in_bytes);  
 
    }

    void Deserialize(void * object)
    {
      
      std::cout << "\nCD Entry Deserialize\nobject : " << object <<std::endl;
      Unpacker entry_unpacker;
      uint32_t return_size=0;
      uint32_t dwGetID=0;
      void *src_unpacked=0;
      void *dst_unpacked=0;

      //char* unpacked_entry_name=0;
      char *unpacked_entry_name = entry_unpacker.GetNext((char *)object, dwGetID, return_size);
      name_ = unpacked_entry_name;
      std::cout << "1st unpacked thing in data_handle : " << name_ << ", return size : " << return_size << std::endl;

      entry_unpacker.GetNext(&preserve_type_, object, dwGetID, return_size);
      std::cout << "2nd unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;

      std::cout << "\nBefore call GetNext for src data handle\tobject : " << object <<std::endl;
      entry_unpacker.GetNext(src_unpacked, object, dwGetID, return_size);
      std::cout << "\nBefore call GetNext for dst data handle\tobject : " << object <<std::endl<<std::endl;
      entry_unpacker.GetNext(dst_unpacked, object, dwGetID, return_size);
      std::cout << "\nBefore call src_data.Deserialize\tobject : " << object <<std::endl;
      src_data_.Deserialize(src_unpacked);
      dst_data_.Deserialize(dst_unpacked);


      std::cout << "3rd unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;
      std::cout << "4th unpacked thing in data_handle : " << dwGetID << ", return size : " << return_size << std::endl;    
    }

  
  

  private:
    DataHandle* GetBuffer(void);
};

#endif

