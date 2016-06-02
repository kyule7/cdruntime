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
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCuDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCuDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCuDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include "cd_config.h"
#include "cd_entry.h"
#include "cd_path.h"
#include "cd_def_internal.h"
#include "cd_def_debug.h"
//#include "cd_internal.h"
#include "serializable.h"
#include "packer.h"
#include "unpacker.h"
#include "util.h"
#include "event_handler.h"
#include <fcntl.h>
#include <unistd.h>

using namespace cd;
using namespace cd::internal;
using namespace std;

CDEntry::CDEntryErrT CDEntry::Delete(void)
{
  // Do something

  if( dst_data_.handle_type() == DataHandle::kMemory ) {
    DATA_FREE( dst_data_.address_data() );
    CD_DEBUG("free preserved data\n"); 
  }
  else if( dst_data_.handle_type() == DataHandle::kOSFile )  {

#if _PRV_FILE_ERASED
//    char cmd[256];
//    char backup_dir[256];
//    sprintf(backup_dir, "mkdir backup_results");
//    int ret = system(backup_dir);
//    sprintf(cmd, "mv %s ./backup_results/", dst_data_.file_name_);
//    ret = system(cmd);
//#else
//    char cmd[256];
//    sprintf(cmd, "rm -f %s", dst_data_.file_name_);
//    int ret = system(cmd);
#endif

//    if( ret == -1 ) {
//      ERROR_MESSAGE("Failed to create a directory for preservation data (SSD)");
//    }
  }

  return CDEntry::CDEntryErrT::kOK;
}



CDEntry::CDEntryErrT CDEntry::Save(void) 
{
  uint32_t prv_medium = MASK_MEDIUM(preserve_type_);

  if(CHECK_PRV_TYPE(preserve_type_, kSerdes) == false) {
    if( CHECK_PRV_TYPE(prv_medium, kDRAM) ) {
      return SaveMem();
    } else if( (CHECK_PRV_TYPE(prv_medium, kHDD)) || (CHECK_PRV_TYPE(prv_medium,kSSD)) ) {
      return SaveFile();
    } else if( CHECK_PRV_TYPE(prv_medium, kPFS)) {
      return SavePFS();
    } else {
      ERROR_MESSAGE("wrong preserve medium : %u\n", prv_medium);
    }
  }
  else {
    CD_DEBUG("[%s] SERDES\n", __func__);
    return CDEntryErrT::kOK;
  }
}

CDEntry::CDEntryErrT CDEntry::SaveMem(void)
{
  CD_DEBUG("%s\n", __func__);
  if(dst_data_.address_data() == NULL) {
    void *allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));

    // FIXME: Jinsuk we might want our own memory manager 
    // since we don't want to call malloc everytime we want small amount of additional memory. 
    dst_data_.set_address_data(allocated_space);
  }
  memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len());

  CD_DEBUG("Saved Data %s : %d at %p (Memory)\n", 
          tag2str[entry_tag_].c_str(), 
          *(reinterpret_cast<int*>(dst_data_.address_data())), 
          dst_data_.address_data()); 

  if(false) { // Is there a way to check if memcpy is done well?
    ERROR_MESSAGE("Not enough memory.");
    return kOutOfMemory; 
  }


  return kOK;
}

CDEntry::CDEntryErrT CDEntry::SaveFile(void)
{
  CD_DEBUG("Write to file %s\n", GetFileName()); 
//  printf("%s\n", __func__);
  // Get file name to write if it is currently NULL
//  char* str; 
//  if( !strcmp(dst_data_.file_name_, INIT_FILE_PATH) ) {
//    // assume cd_id_ is unique (sequential cds will have different cd_id) 
//    // and address data is also unique by natural
//    const char* data_name;
//    str = new char[30];
//    if(entry_tag_ == 0) {
//      sprintf(str, "%d", rand());
//      data_name = str;
//    }
//    else  {
//      data_name = tag2str[entry_tag_].c_str();
//    }
//
//    const char *file_name = Util::GetUniqueCDFileName(ptr_cd_->GetCDID(), base_.c_str(), data_name).c_str();
//    strcpy(dst_data_.file_name_, file_name); 
//  }
  dst_data_.set_file_name(GetFileName());
  FILE *fp = GetFilePointer();
  if( fp != NULL ) {
    fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);

    CD_DEBUG("Write to a file. \n");

    return kOK;
  }
  else 
    return kFileOpenError;
  
}




CDEntry::CDEntryErrT CDEntry::SavePFS(void)
{
  CD_DEBUG("%s\n", __func__);
  //First we should check for PFS file => I think we have checked it before calling this function (not sure).
  //PMPI_Status preserve_status;//This variable can be used to non-blocking writes to PFS. By checking this variable we can understand whether write has been finished or not.
  //PMPI_File_get_position( ptr_cd_->PFS_d_, &(dst_data_.parallel_file_offset_));

  // Dynamic Chunk Interleave
  dst_data_.parallel_file_offset_ = ptr_cd_->pfs_handle_->Write( src_data_.address_data(), src_data_.len() );

  return kOK;
}


// -----------------------------------------------------------------------------------------------
void CDEntry::RequestEntrySearch(void)
{
#if _MPI_VER
  CD_DEBUG("\nCDEntry::RequestEntrySearch\n");
  
  CDHandle *curr_cdh = CDPath::GetCDLevel(ptr_cd_->level());
  curr_cdh = CDPath::GetCoarseCD(curr_cdh);

  CD_DEBUG("task size: %u, finding entry at level #%u from level #%u", 
            curr_cdh->task_size(), curr_cdh->level(), ptr_cd_->level());

  CD *curr_cd = curr_cdh->ptr_cd();

  // SetMailbox
  curr_cd->SetMailBox(kEntrySearch);

  ENTRY_TAG_T tag_to_search = dst_data_.ref_name_tag();

  // Tag should contain CDID+entry_tag
  CD_DEBUG("[ToHead] CHECK TAG : %u (Entry Tag: %u)\n", 
          curr_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, curr_cd->task_in_color()), tag_to_search);

  ENTRY_TAG_T sendBuf[2] = {tag_to_search, static_cast<ENTRY_TAG_T>(myTaskID)};

  CD_DEBUG("sendBuf[0] : %u, sendBuf[1] : %u \nCHECK IT OUT : %u --> %u", 
           sendBuf[0], sendBuf[1], curr_cd->task_in_color(), curr_cd->head());

  curr_cd->entry_request_req_[tag_to_search] = CommInfo();
//  curr_cd->entry_req_.push_back(CommInfo()); getchar();

  PMPI_Isend(sendBuf, 
             2, 
             MPI_UNSIGNED_LONG_LONG,
             curr_cd->head(), 
             curr_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, curr_cd->task_in_color()),
             curr_cd->color(),
             &(curr_cd->entry_request_req_[tag_to_search].req_));
//             &(curr_cd->entry_req_.back().req_));

  CD_DEBUG("[FromAny] CHECK TAG : %u (Entry Tag: %u)\n", curr_cd->GetCDID().GenMsgTag(tag_to_search), tag_to_search);
  CD_DEBUG("Size of data to receive : %lu (%u)\n", src_data_.len(), tag_to_search);

    ptr_cd()->entry_recv_req_[tag_to_search] = CommInfo();
//  curr_cd->entry_req_.push_back(CommInfo()); //getchar();

  // Receive the preserved data to restore the data in application memory
  PMPI_Irecv(src_data_.address_data(), 
             src_data_.len(), 
             MPI_BYTE, 
             MPI_ANY_SOURCE, 
             curr_cd->cd_id_.GenMsgTag(tag_to_search), 
             MPI_COMM_WORLD, 
             &(curr_cd->entry_recv_req_[tag_to_search].req_));  
//             &(curr_cd->entry_req_.back().req_));  

#endif

}



// Bring data handle from parents
CDEntry::CDEntryErrT CDEntry::Restore(void) {
  
  CD_DEBUG("CDEntry::Restore at level: %u\n", ptr_cd_->level());

  bool local_found = false;
  DataHandle *buffer = NULL;
  CDEntry *entry = NULL;

  if( dst_data_.handle_type() == DataHandle::kReference ) {  
    // Restoration from reference
    CD_DEBUG("[kReference] Ref name: %s\n", dst_data_.ref_name().c_str());
    
    uint32_t found_level = 0;
    ENTRY_TAG_T tag_to_search = dst_data_.ref_name_tag();
    entry = ptr_cd()->SearchEntry(tag_to_search, found_level);

    // Remote case
    if(entry != NULL) {  // Local case
      local_found = true;
      // FIXME MPI_Group
      if(entry->dst_data_.node_id() == CDPath::GetCDLevel(found_level)->node_id()) {

        CD_DEBUG("\n[CDEntry::Restore] Succeed in finding the entry locally!!\n");
        buffer = &(entry->dst_data_);
    
        buffer->set_ref_offset(dst_data_.ref_offset() );   
    
        // length could be different in case we want to describe only part of the preserved entry.
        buffer->set_len(dst_data_.len());     
  
//        CD_DEBUG("addr %p (dst), addr %p (buffer)\n[Buffer] ref name : %s, value: %d\n", 
//                 entry->dst_data_.address_data(), buffer->address_data(), 
//                 buffer->ref_name().c_str(), *(reinterpret_cast<int*>(buffer->address_data())));
      }
      else {

#if _MPI_VER
        // Entry was found at this task because the current task is head.
        // So, it should request EntrySend to the task that holds the actual data copy for preservation.
        CD_DEBUG("\nIt succeed in finding the entry at this level #%u of head task!\n", found_level);
        HeadCD *found_cd = static_cast<HeadCD *>(CDPath::GetCDLevel(found_level)->ptr_cd());
        // It needs some effort to avoid the naming alias problem of entry tags.
      
        // Found the entry!! 
        // Then, send it to the task that has the entry of actual copy
        uint32_t target_task_id = entry->dst_data_.node_id().task_in_color();
        
        CD_DEBUG("Before SetMailBox(kEntrySend)\n");

        // SetMailBox for entry send to target
        CDEventT entry_send = kEntrySend;
        found_cd->SetMailBox(entry_send, target_task_id);
    
        CD_DEBUG("CHECK TAG : %u (Entry Tag : %u)\nCHECK IT OUT : %u --> %u at %s\n", 
                 found_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, found_cd->task_in_color()), tag_to_search, 
                 target_task_id, found_cd->task_in_color(), found_cd->GetCDName().GetString().c_str());
        
        CD_DEBUG("FOUND %s, %s at %u, target task ID : %u\n", 
                  found_cd->GetCDName().GetString().c_str(), 
                 found_cd->GetNodeID().GetString().c_str(), 
                 found_level, target_task_id);

        ENTRY_TAG_T sendBuf[2] = {tag_to_search, static_cast<ENTRY_TAG_T>(myTaskID)};

        found_cd->entry_req_.push_back(CommInfo()); //getchar();
//        found_cd->entry_search_req_[tag_to_search] = CommInfo();

        PMPI_Isend(sendBuf, 
                   2, 
                   MPI_UNSIGNED_LONG_LONG, 
                   target_task_id, 
                   found_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, found_cd->task_in_color()),
                   found_cd->color(), // Entry sender is also in the same CD task group.
                   &(found_cd->entry_req_.back().req_));
//                   &(found_cd->entry_search_req_[tag_to_search]).req_);

//        CDHandle *curr_cdh = CDPath::GetCurrentCD();
//        while( curr_cdh->node_id().size()==1 ) {
//          if(curr_cdh == CDPath::GetRootCD()) {
//            dbg << "[SetMailBox] there is a single task in the root CD" << endl;
//            assert(0);
//          }
//          // If current CD is Root CD and GetParentCD is called, it returns NULL
//          curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
//        } 
//        CDHandle *cdh = CDPath::GetAncestorLevel(found_level);
//        if(!cdh->IsHead()) assert(0);
//        HeadCD *headcd = static_cast<HeadCD *>(cdh->ptr_cd());
//        CDEventT entry_send = kEntrySend;
//        headcd->SetMailBox(entry_send, entry->dst_data_.node_id().task_in_color());
//        dbg << "[CDEntry::Restore] Can't find the entry local. Remote Case!!" << endl;


        CD_DEBUG("[CDEntry::Restore] Can't find the entry local. Remote Case!!\n");


#endif
      }
    }
    else { // Remote case
      entry = NULL;
      CD_DEBUG("[CDEntry::Restore] Can't find the entry local. Remote Case!!\n");
    } 
  }
  else {  
    // Restoration from memory or file system. Just use the current dst_data_ for it.
    CD_DEBUG("[kCopy] buffer addr : %p\n", buffer);
 
    buffer = &dst_data_;
//    dst_data_.address_data();
  }

  CD_DEBUG("[Restore] Done. Entry : %p\n", entry);

  return InternalRestore(buffer, local_found);
}

CDEntry::CDEntryErrT CDEntry::InternalRestore(DataHandle *buffer, bool local_found)
{
  CD_DEBUG("[CDEntry::InternalRestore] ");

  // Handles preservation via reference for remote copy.
  if(buffer == NULL) {

#if _MPI_VER

    CD_DEBUG("Preservation via reference for remote copy.\n");
    
    // Add this entry for receiving preserved data remotely.
    ENTRY_TAG_T tag_to_search = dst_data_.ref_name_tag();

    if(local_found == false) {
      CD_DEBUG("Failed to find the entry locally. Request head to bring the entry.\n"); 
//      printf("Failed to find the entry locally. Request head to bring the entry. %s\n",ptr_cd()->GetCDName().GetString().c_str()); 
      RequestEntrySearch();
    } 
    else {
      // Receive the preserved data to restore the data in application memory
      CD_DEBUG("Found entry locally, but need to bring the entry (Entry Tag: %u, Msg Tag: %u) from remote task in the same CD\n", 
               tag_to_search, ptr_cd()->GetCDID().GenMsgTag(tag_to_search));
      CD_DEBUG("CHECK TAG : %u --> %u\n", tag_to_search, ptr_cd()->GetCDID().GenMsgTag(tag_to_search));

      ptr_cd()->entry_recv_req_[tag_to_search] = CommInfo();
//      ptr_cd()->entry_req_.push_back(CommInfo()); //getchar(); //src_data_.address_data());

      PMPI_Irecv(src_data_.address_data(), 
                 src_data_.len(), 
                 MPI_BYTE, 
                 MPI_ANY_SOURCE, 
                 ptr_cd()->GetCDID().GenMsgTag(tag_to_search), 
                 MPI_COMM_WORLD, 
                 &(ptr_cd()->entry_recv_req_[tag_to_search].req_));  
//                 &(ptr_cd()->entry_req_.back().req_));  
    }



    CD_DEBUG("[RequestEntrhSearch] Done\n");

#endif
    return kEntrySearchRemote;
  }
  else { // Preservation via copy/ref local 
    if(buffer->handle_type() == DataHandle::kMemory)  {
      CD_DEBUG("Local Case -> kMemory\n");
  
      //FIXME we need to distinguish whether this request is on Remote or local for both 
      // when using kOSFile or kMemory and do appropriate operations..
  
      if(src_data_.address_data() != NULL) {
        CD_DEBUG("%lu (src) - %lu (buffer), offset : %lu\n", src_data_.len(), buffer->len(), buffer->ref_offset());
        CD_DEBUG("%p - %p\n", src_data_.address_data(), buffer->address_data());
        assert( src_data_.len() == buffer->len() );

        if( !CHECK_PRV_TYPE(preserve_type_, kSerdes) ) { 
          memcpy(src_data_.address_data(), (char *)buffer->address_data()+(buffer->ref_offset()), buffer->len());
        } else {
          CD_DEBUG("Deserialize in CDEntry\n");
          (static_cast<Serializable *>(src_data_.address_data_))->Deserialize(dst_data_.address_data_);
        }
  
        CD_DEBUG("Succeeds in memcpy %lu size data. %p --> %p\n", dst_data_.len(), dst_data_.address_data(), src_data_.address_data());
      }
      else {
        ERROR_MESSAGE("Not enough memory.\n");
      }
  
    }
    else if(buffer->handle_type() == DataHandle::kOSFile)  {
      CD_DEBUG("Local Case -> kOSFile\n");

      //FIXME we need to collect file writes and write as a chunk. We don't want to have too many files per one CD.   
//      FILE *fp = buffer->fp_;
      FILE *fp = fopen(buffer->file_name_, "r");
      int fd = fileno(fp);
      bool file_opened = false;
      if(fcntl(fd, F_GETFD) == -1) {
//        if(errno == EBADF) { // fd is not an open file descriptor
//          fp = fdopen(fd, "r");
//        }
//        else { 
//          fp = fopen(buffer->file_name_, "r"); 
//        }
        CD_DEBUG("filename:%s\n", buffer->file_name_);
        fp = fopen(buffer->file_name_, "r"); 
        file_opened = true;
      }

      // Now, fp should be valid pointer
      if( fp!= NULL )  {
        CD_DEBUG("filename:%s\n", buffer->file_name_);
        fseek(fp, buffer->GetOffset() , SEEK_SET); 
        fread(src_data_.address_data(), 1, src_data_.len(), fp);
        if(file_opened == true) {
          fclose(fp);
          file_opened = false;
        } else {
          fclose(fp);
        }
        CD_DEBUG("Read data from OS file system\n");

        return CDEntryErrT::kOK;
      }
      else {
        CD_DEBUG("[WARNING] File Descriptor is invalid during restoration from %s. %s", buffer->file_name_, strerror(errno));
        return CDEntryErrT::kFileOpenError;
      }
    } 
    else {
      ERROR_MESSAGE("Unsupported data handle type. Handle Type : %d\n", buffer->handle_type());
    }
  }

  return kOK;
  //Direction is from dst_data_ to src_data_

}



void *CDEntry::Serialize(uint64_t &len_in_bytes) 
{

  CD_DEBUG("\nCD Entry Serialize\n");
  Packer entry_packer;
// uint32_t ptr_cd_packed_len=0;
// void *ptr_cd_packed_p = ptr_cd_->Serialize(ptr_cd_packed_len);

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

void CDEntry::Deserialize(void *object)
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


FILE *CDEntry::GetFilePointer(void)
{
  return ptr_cd_->file_handle_.fp_;
}

char *CDEntry::GetFileName(void)
{
  return ptr_cd_->file_handle_.unique_filename_;
}

ostream& cd::internal::operator<<(ostream& str, const CDEntry& cd_entry)
{
  return str << "\n== CD Entry Information ================"
             << "\nSource      :\t" << cd_entry.src_data_  
             << "\nDestination :\t" << cd_entry.dst_data_
             << "\nEntry Tag   :\t" << cd_entry.entry_tag_
             << "\nEntry Name  :\t" << tag2str[cd_entry.entry_tag_]
             << "\n========================================\n";
}

string CDEntry::GetString(void) const 
{
  return ( string("\n== CD Entry Information ================")
         + string("\nSource      :\t") + src_data_.GetString() 
         + string("\nDestination :\t") + dst_data_.GetString()
         + string("\nEntry Tag   :\t") + to_string(entry_tag_)
         + string("\nEntry Name  :\t") + tag2str[entry_tag_]
         + string("\n========================================\n") );
}

/*
CDEntry::CDEntryErrT CDEntry::Save(void)
{
  //Direction is from src_data_ to dst_data_

  //FIXME Jinsuk: Just for now we assume everything works on memory
  // What we need to do is that distinguish the medium type and then do operations appropriately

  //FIXME we need to distinguish whether this request is on Remote or local for both when using kOSFile or kMemory and do appropriate operations..
  //FIXME we need to distinguish whether this is to a reference....
  if( dst_data_.handle_type() == DataHandle::kMemory ) {
    if(dst_data_.address_data() == NULL) {

      void *allocated_space;

      // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 
      allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));  

      dst_data_.set_address_data(allocated_space);

    }

    if(dst_data_.address_data() != NULL) {
      memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len()); 
      CD_DEBUG("Succeeds in memcpy %lu size data. %p --> %p\n", dst_data_.len(), dst_data_.address_data(), src_data_.address_data());
    }
    else {
      ERROR_MESSAGE("Not enough memory.");
      return kOutOfMemory;
    }

  }
  else if( dst_data_.handle_type() == DataHandle::kOSFile ) {
    if( !strcmp(dst_data_.file_name_, INIT_FILE_PATH) ) {
//      char *cd_file = new char[MAX_FILE_PATH];
//      cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); 




      // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
//      dst_data_.file_name_ = Util::GetUniqueCDFileName(ptr_cd_->GetCDID(), "kOSFILE");
    }

    // FIXME  Jinsuk: we need to collect file writes and write as a chunk. 
    // We don't want to have too many files per one CD. So perhaps one file per chunk is okay.

    FILE *fp;
    //char filepath[1024]; 
    //Util::GetCDFilePath(cd_id_, src_data.address_data(), filepath); 
    // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
    fp = fopen(dst_data_.file_name_, "w");
    if( fp!= NULL ) {
      fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);
      fclose(fp);
      return kOK;
    }
    else return kFileOpenError;
  }


  return kOK;

}
*/


