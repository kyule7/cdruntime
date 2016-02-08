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
/**
 * @file cd_entry.h
 * @author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * @date March 2015
 *
 * @brief Containment Domains API v0.2 (C++)
 */
#include "cd_global.h"
#include "cd_def_internal.h"
#include "cd_internal.h"
#include "serializable.h"
#include "data_handle.h"
#include "util.h"
#include "event_handler.h"

namespace cd {
  namespace internal {

/**@class CDEntry
 * @brief Data structure that contains preservation-related information.
 *  This class represent all the required information in the preservation action
 *  such as source/destination information, preservation type, etc.
 *  It is internally managed by CD object.
 */ 
class CDEntry : public Serializable {
  friend class CD;
  friend class HeadCD;
  friend class PFSHandle;
  friend class HandleEntrySend;
  friend class HandleEntrySearch;
  friend std::ostream& operator<<(std::ostream& str, const CDEntry& cd_entry);
  private:
    enum { 
      ENTRY_PACKER_NAME=0,
//      ENTRY_PACKER_PTRCD,
      ENTRY_PACKER_PRESERVETYPE,
      ENTRY_PACKER_SRC,
      ENTRY_PACKER_DST 
    };
    enum CDEntryErrT {
      kOK=0, 
      kOutOfMemory, 
      kFileOpenError, 
      kEntrySearchRemote
    };
    // need a unique name to do via reference, 
    // this variable can be empty string when this is not needed
    ENTRY_TAG_T entry_tag_;    
    CD         *ptr_cd_;
    DataHandle  src_data_;
    DataHandle  dst_data_;
    CD_FLAG_T   preserve_type_; // already determined according to class 

  private:  
    CDEntry(){}
//    CDEntry(const DataHandle  &src_data, 
//            const DataHandle  &dst_data, 
//            const std::string &entry_name) 
//    {
//      src_data_ = src_data;
//      dst_data_ = dst_data;
//      if(entry_name.empty()) 
//        entry_tag_ = 0;
//      else {
//        entry_tag_ = cd_hash(entry_name);
//        tag2str[entry_tag_] = entry_name;
//      }
//    }
    CDEntry(const DataHandle  &src_data, 
            const DataHandle  &dst_data, 
            const std::string &entry_name,
            const CD *ptr_cd,
            const uint32_t &preserve_type) 
      : src_data_(src_data), dst_data_(dst_data)
    {
      ptr_cd_ = const_cast<CD *>(ptr_cd);
      preserve_type_ = preserve_type;
      if(entry_name.empty()) 
        entry_tag_ = 0;
      else {
        entry_tag_ = cd_hash(entry_name);
        tag2str[entry_tag_] = entry_name;
      }
    }

  public:

    ~CDEntry()
    {
      //FIXME we need to delete allocated (preserved) space at some point, should we do it when entry is destroyed?  when is appropriate for now let's do it now.
//      if( dst_data_.address_data() != NULL ) {
////        DATA_FREE( dst_data_.address_data() );
//      }
 
    }
    std::string GetString(void) const;
  private:
    // FIXME: currently it is assumed that my cd is always in the same memory space.
    // In the future we might need a distributed CD structure. 
    // For example right now we have one list who handles all the CDEntries for that CD. 
    // But this list might need to be distributed if that CD spans among multiple threads. 
    // If we have that data structure the issue here will be solved anyways. 
    void set_my_cd(CD* ptr_cd) { ptr_cd_ = ptr_cd; }
    CD* ptr_cd() const { return ptr_cd_; }

    CDEntryErrT Delete(void);

//  public:
    std::string name() const { return tag2str[entry_tag_]; }
    ENTRY_TAG_T name_tag() const { return entry_tag_; }
    bool isViaReference() { return (dst_data_.handle_type() == DataHandle::kReference); }


    CDEntry& operator=(const CDEntry& that) {
      entry_tag_ = that.entry_tag_;    
      src_data_ = that.src_data_;
      dst_data_ = that.dst_data_;
      preserve_type_ = that.preserve_type_;
      return *this;
    }

    bool operator==(const CDEntry& that) const {
      return (entry_tag_ == that.entry_tag_) && (src_data_ == that.src_data_) 
             && (dst_data_ == that.dst_data_) && (preserve_type_ == that.preserve_type_);
    }

    CDEntryErrT SaveMem(void);
    CDEntryErrT SaveFile(void);
    // PFS
    CDEntryErrT SavePFS(void);
    CDEntryErrT Save(void);

    FILE *GetFilePointer(void);

    //FIXME We need another Restore function that would accept offset and length, 
    // so basically it will use dst_data_ as base and then offset and length is used to restore only partial of the original. 
    // This functionality is required for via reference to work. Basically when via reference is doing restoration, 
    // it will first try to find the entry it self by going up and look into parent's directories. 
    // After finding one, it will retrive the dst_data_ and then copy from them.

    CDEntryErrT Restore(void);
    CDEntryErrT InternalRestore(DataHandle *buffer, bool local_found=false);

    CDEntryErrT Restore(Serializable *serdes);


    void *Serialize(uint64_t &len_in_bytes) 
    {

      CD_DEBUG("\nCD Entry Serialize\n");
      Packer entry_packer;
//      uint32_t ptr_cd_packed_len=0;
//      void *ptr_cd_packed_p = ptr_cd_->Serialize(ptr_cd_packed_len);

      uint64_t src_packed_len=0;
      CD_DEBUG("\nsrc Serialize\n");
      void *src_packed_p = src_data_.Serialize(src_packed_len);
      uint64_t dst_packed_len=0;
      CD_DEBUG("\ndst Serialize\n");
      void *dst_packed_p = dst_data_.Serialize(dst_packed_len);
//      assert(ptr_cd_packed_len != 0);
      assert(src_packed_len != 0);
      assert(dst_packed_len != 0);

      CD_DEBUG("packed entry_entry_tag_ is : %u \n", entry_tag_);

      ENTRY_TAG_T str_key = entry_tag_;
      entry_packer.Add(ENTRY_PACKER_NAME, sizeof(str_key), &str_key); 
//      entry_packer.Add(ENTRY_PACKER_PTRCD, ptr_cd_packed_len, ptr_cd_packed_p);
      
      CD_DEBUG("packed preserve_type_ is : %d \n", preserve_type_);

      entry_packer.Add(ENTRY_PACKER_PRESERVETYPE, sizeof(CD_FLAG_T), &preserve_type_);

      CD_DEBUG("packed src_packed_ is : %p\n", src_packed_p);

      entry_packer.Add(ENTRY_PACKER_SRC, src_packed_len, src_packed_p);
      entry_packer.Add(ENTRY_PACKER_DST, dst_packed_len, dst_packed_p); 

      CD_DEBUG("\nCD Entry Serialize Done\n");

      return entry_packer.GetTotalData(len_in_bytes);  
 
    }

    void Deserialize(void *object)
    {
      CD_DEBUG("\nCD Entry Deserialize\nobject : %p\n", object);

      Unpacker entry_unpacker;
      uint32_t return_size=0;
      uint32_t dwGetID=0;
      void *src_unpacked=0;
      void *dst_unpacked=0;

      entry_tag_ = *(ENTRY_TAG_T *)entry_unpacker.GetNext((char *)object, dwGetID, return_size);

      CD_DEBUG("unpacked entry_entry_tag_ is : %u <-> %s\n", entry_tag_, tag2str[entry_tag_].c_str());
      CD_DEBUG("1st unpacked thing in data_handle : %u, return size : %u\n", entry_tag_, return_size);

      preserve_type_ = *(CD_FLAG_T *)entry_unpacker.GetNext((char *)object, dwGetID, return_size);

      CD_DEBUG("unpacked preserve_type_ is : %d\n", preserve_type_);
      CD_DEBUG("2nd unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);

      CD_DEBUG("Before call GetNext for src data handle object : %p\n", object);
      src_unpacked = entry_unpacker.GetNext((char *)object, dwGetID, return_size);
      CD_DEBUG("After call GetNext for dst data handle object : %p\n", object);

      CD_DEBUG("src_unpacked is : %p\n", src_unpacked);
      CD_DEBUG("3rd unpacked thing in data_handle : %u, return size: %u\n", dwGetID, return_size);


      dst_unpacked = entry_unpacker.GetNext((char *)object, dwGetID, return_size);
      CD_DEBUG("Before call src_data.Deserialize object : %p\n", object);
      CD_DEBUG("dst_unpacked is : %p\n", dst_unpacked);
      CD_DEBUG("4th unpacked thing in data_handle : %u, return size : %u\n", dwGetID, return_size);

      src_data_.Deserialize(src_unpacked);
      dst_data_.Deserialize(dst_unpacked);
      ptr_cd_ = NULL;
    }

  
  

  private:
//    DataHandle* GetBuffer(void);
    void RequestEntrySearch(void);
};


  } // namespace internal ends
} // namespace cd ends
#endif

