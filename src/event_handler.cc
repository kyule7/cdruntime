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

#include "cd_global.h"
#include "cd_handle.h"
#include "cd_entry.h"
#include "cd_path.h"
#include "event_handler.h"

using namespace cd;
using namespace std;



void HandleEntrySearch::HandleEvent(void)
{
#if _MPI_VER

  int entry_requester_id = task_id_;
  ENTRY_TAG_T recvBuf[2] = {0,0};

  CD_DEBUG("SIZE : %u at level #%u\n", ptr_cd_->task_size(), ptr_cd_->level());
  CD_DEBUG("[HeadCD::HandleEntrySearch]\ttask_id : %u\n", task_id_);
  CD_DEBUG("[FromRequester] CHECK TAG : %u (Entry Tag: NOT_YET_READY)\n", ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, entry_requester_id));
  CD_DEBUG("CHECK IT OUT : %d --> %u\n", entry_requester_id, ptr_cd_->task_in_color());

//  ptr_cd_->entry_req_.push_back(CommInfo());
  MPI_Status status;

  PMPI_Recv(recvBuf, 
            2, 
            MPI_UNSIGNED_LONG_LONG, 
            entry_requester_id, 
//            MSG_TAG_ENTRY_TAG,
//            11,
            ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, entry_requester_id),
            ptr_cd_->color(), // Entry requester is in the same CD task group. 
            &status);


//  PMPI_Irecv(recvBuf, 
//            2, 
//            MPI_UNSIGNED_LONG_LONG, 
//            entry_requester_id, 
////            MSG_TAG_ENTRY_TAG,
//            ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, entry_requester_id),
//            ptr_cd_->color(), // Entry requester is in the same CD task group. 
//            &(ptr_cd_->entry_req_.back().req_));



  ENTRY_TAG_T &tag_to_search  = recvBuf[0];
  ENTRY_TAG_T &source_task_id = recvBuf[1];
  uint32_t found_level = 0;
  
  CDEntry *target_entry = ptr_cd_->InternalGetEntry(tag_to_search);
  if(target_entry != NULL) { 
    CD_DEBUG("FOUND it at this level #%u\n", ptr_cd_->level());
    found_level = ptr_cd_->level();
  } 
  else {
    CD_DEBUG("NOT FOUND it at this level #%u\n", ptr_cd_->level());
    target_entry = ptr_cd_->SearchEntry(tag_to_search, found_level);
  }


  if(target_entry != NULL) {
    CD_DEBUG("It succeed in finding the entry at this level #%u if head task!\n", found_level);

    // It needs some effort to avoid the naming alias problem of entry tags.
//    ptr_cd_->entry_search_req_[tag_to_search] = CommInfo();

    HeadCD *found_cd = static_cast<HeadCD *>(CDPath::GetCDLevel(found_level)->ptr_cd());

    // It needs some effort to avoid the naming alias problem of entry tags.
  
    // Found the entry!! 
    // Then, send it to the task that has the entry of actual copy
    ENTRY_TAG_T tag_to_search = target_entry->name_tag();
    int target_task_id = target_entry->dst_data_.node_id().task_in_color();

    CD_DEBUG("FOUND %s / %s at level #%u, target task id : %u\n", 
             found_cd->GetCDName().GetString().c_str(), 
             found_cd->GetNodeID().GetString().c_str(), 
             found_level, target_task_id);

    // SetMailBox for entry send to target
    CDEventT entry_send = kEntrySend;

    if(target_task_id != ptr_cd_->task_in_color()) {
      found_cd->SetMailBox(entry_send, target_task_id);
    

    CD_DEBUG("[ToSender] CHECK TAG : %u (Entry Tag: %u)\n",
             found_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, found_cd->task_in_color()),
             tag_to_search);

    ENTRY_TAG_T sendBuf[2] = {tag_to_search, source_task_id};

//    found_cd->entry_search_req_[tag_to_search] = CommInfo();
    found_cd->entry_req_.push_back(CommInfo());

    PMPI_Isend(sendBuf, 
               2, 
               MPI_UNSIGNED_LONG_LONG, 
               target_task_id, 
               found_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, found_cd->task_in_color()),
//               found_cd->GetCDID().GenMsgTag(tag_to_search), 
               found_cd->color(), // Entry sender is also in the same CD task group.
               &(found_cd->entry_req_.back().req_));
//               &(found_cd->entry_search_req_[tag_to_search]).req_);
  
    }
    else {
      CD_DEBUG("HandleEntrySearch --> EntrySend (Head): entry was found at %u\n", found_level);
      CD_DEBUG("Size of data to send : %lu (%u)\n", target_entry->dst_data_.len(), tag_to_search);


      char *data_to_send = NULL;
      DataHandle &target = target_entry->dst_data_;
    
      if(target.handle_type() == DataHandle::kMemory) {
        data_to_send = static_cast<char *>(target.address_data());
      }
      else if(target.handle_type() == DataHandle::kOSFile || target.handle_type() == DataHandle::kPFS) {
        data_to_send = new char[target_entry->dst_data_.len()];
        FILE *temp_fp = fopen(target.file_name().c_str(), "r");
        if( temp_fp!= NULL )  {
          if( target.ref_offset() != 0 ) { 
            fseek(temp_fp, target.ref_offset(), SEEK_SET); 
          }
      
          fread(data_to_send, 1, target.len(), temp_fp);
      
          CD_DEBUG("Read data from OS file system\n");
      
          fclose(temp_fp);
        }
        else {
          ERROR_MESSAGE("Error in fopen in HandleEntrySend\n");
        }
      }
      else {
        ERROR_MESSAGE("Unsupported handle type : %d\n", target.handle_type());
      }



    //  ptr_cd_->entry_send_req_[tag_to_search] = CommInfo();
      ptr_cd_->entry_req_.push_back(CommInfo()); //getchar();

      // Should be non-blocking send to avoid deadlock situation. 
      PMPI_Isend(data_to_send, 
                 target.len(), 
                 MPI_BYTE, 
                 source_task_id, 
                 ptr_cd_->cd_id_.GenMsgTag(tag_to_search), 
                 MPI_COMM_WORLD, // could be any task in the whole rank group 
                 &(ptr_cd_->entry_req_.back().req_));  
    //             &(ptr_cd_->entry_send_req_[tag_to_search].req_));  


    if(target.handle_type() == DataHandle::kOSFile || target.handle_type() == DataHandle::kPFS) {
      delete data_to_send;
    }

    }

//    ptr_cd_->event_flag_[entry_requester_id] &= ~kEntrySearch;
//    IncHandledEventCounter();
  }
  else {
    // failed to search the entry at this level of CD.
//    cerr<<"Failed to search the entry at current CD level. Request to upper-level ptr_cd_->head to search the entry. Not implemented yet\n"<<endl;
//    assert(0);

    CD_DEBUG("It failed to find the entry at this level #%u of head task. Request to parent level!\n", ptr_cd_->level());

    CDHandle *parent = CDPath::GetParentCD(ptr_cd_->level());
    CDEventT entry_search = kEntrySearch;
    parent->ptr_cd()->SetMailBox(entry_search);

    CD_DEBUG("CHECK TAG : %u (Entry Tag: %u)\n", 
             parent->ptr_cd()->GetCDID().GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, parent->task_in_color()), 
             tag_to_search);

    parent->ptr_cd()->entry_request_req_[tag_to_search] = CommInfo();
//    parent->ptr_cd()->entry_req_.push_back(CommInfo()); getchar();

    PMPI_Isend(recvBuf, 
               2, 
               MPI_UNSIGNED_LONG_LONG,
               parent->node_id().head(), 
//               MSG_TAG_ENTRY_TAG,
               parent->ptr_cd()->GetCDID().GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, parent->task_in_color()),
//               parent->ptr_cd()->GetCDID().GenMsgTag(MSG_TAG_ENTRY_TAG), 
               parent->node_id().color(),
//               &(parent->ptr_cd()->entry_req_.back().req_));
               &(parent->ptr_cd()->entry_request_req_[tag_to_search].req_));

//    ptr_cd_->event_flag_[entry_requester_id] &= ~kEntrySearch;
//    IncHandledEventCounter();

  }

  ptr_cd_->event_flag_[entry_requester_id] &= ~kEntrySearch;
  IncHandledEventCounter();

#endif

}

void HandleEntrySend::HandleEvent(void)
{
#if _MPI_VER

  CD_DEBUG("CD::HandleEntrySend\n");
  CD_DEBUG("CHECK TAG : %u\n", ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, ptr_cd_->head()));

  ENTRY_TAG_T recvBuf[2] = {INIT_TAG_VALUE, INIT_ENTRY_SRC};
//  ptr_cd_->entry_req_.push_back(CommInfo());

  MPI_Status status;

  PMPI_Recv(recvBuf, 
            2, 
            MPI_UNSIGNED_LONG_LONG, 
            ptr_cd_->head(), 
            ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, ptr_cd_->head()),
            ptr_cd_->color(), // Entry searcher is in the same CD task gruop (ptr_cd_->head) 
            &status);
/*
  PMPI_Irecv(recvBuf, 
             2, 
             MPI_UNSIGNED_LONG_LONG, 
             ptr_cd_->head(), 
             ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, ptr_cd_->head()),
             ptr_cd_->color(), // Entry searcher is in the same CD task gruop (ptr_cd_->head) 
             &(ptr_cd_->entry_req_.back().req_));
*/

  ENTRY_TAG_T &tag_to_search = recvBuf[0];

  int entry_source_task_id  = static_cast<int>(recvBuf[1]);
  CD_DEBUG("Received Entry Tag : %u == %s\n", tag_to_search, tag2str[tag_to_search].c_str());

  // Find entry
  if(tag_to_search == INIT_TAG_VALUE) {
    ERROR_MESSAGE("Entry sender failed to receive the entry tag to search for the right entry with. CD Level #%u\n", ptr_cd_->level());
  }

//  CDEntry *entry = InternalGetEntry(tag_to_search);
  uint32_t found_level=ptr_cd_->level();


  CDEntry *entry = ptr_cd_->InternalGetEntry(tag_to_search);
  if(entry != NULL) { 
    CD_DEBUG("FOUND it at this level #%u!\n", ptr_cd_->level());

    found_level = ptr_cd_->level();
  } 
  else {
    CD_DEBUG("NOT FOUND it at this level #%u!\n", ptr_cd_->level());

    entry = ptr_cd_->SearchEntry(tag_to_search, found_level);
  }



//  CDEntry *entry = ptr_cd_->SearchEntry(tag_to_search, found_level);
  
  //CDHandle *cdh = CDPath::GetParentCD(ptr_cd_->level());
  CDHandle *cdh = CDPath::GetCDLevel(ptr_cd_->level());
  
  for(auto it=cdh->ptr_cd()->remote_entry_directory_map_.begin(); it!=cdh->ptr_cd()->remote_entry_directory_map_.end(); ++it) {
    CD_DEBUG("%lu (%s) - %s\n)\n", it->first, tag2str[it->first].c_str(), it->second->GetString().c_str());
    CD_DEBUG("--------------------- level : %u --------------------------------", cdh->level());
  }

  if(entry == NULL) {
    ERROR_MESSAGE("The received tag [%u] does not work in %s. CD Level %u.\n",
                   tag_to_search, ptr_cd_->GetNodeID().GetString().c_str(), ptr_cd_->level());
  }
  else {
    CD_DEBUG("HandleEntrySend: entry was found at level #%u.\n", found_level);

    char *data_to_send = NULL;
    DataHandle &target = entry->dst_data_;

    if(target.handle_type() == DataHandle::kMemory) {
      data_to_send = static_cast<char *>(target.address_data());
    }
    else if(target.handle_type() == DataHandle::kOSFile || target.handle_type() == DataHandle::kPFS) {
      data_to_send = new char[entry->dst_data_.len()];
      FILE *temp_fp = fopen(target.file_name().c_str(), "r");
      if( temp_fp!= NULL )  {
        if( target.ref_offset() != 0 ) { 
          fseek(temp_fp, target.ref_offset(), SEEK_SET); 
        }
  
        fread(data_to_send, 1, target.len(), temp_fp);
  
        CD_DEBUG("Read data from OS file system\n");
  
        fclose(temp_fp);
      }
      else {
        ERROR_MESSAGE("Error in fopen in HandleEntrySend\n");
      }
    }
    else {
      ERROR_MESSAGE("Unsupported handle type : %d\n", target.handle_type());
    }
    
//    ptr_cd_->entry_send_req_[tag_to_search] = CommInfo();
    ptr_cd_->entry_req_.push_back(CommInfo()); //getchar();
      
    // Should be non-blocking send to avoid deadlock situation. 
    PMPI_Isend(data_to_send, 
               target.len(), 
               MPI_BYTE, 
               entry_source_task_id, 
               ptr_cd_->GetCDID().GenMsgTag(tag_to_search), 
               MPI_COMM_WORLD, // could be any task in the whole rank group 
               &(ptr_cd_->entry_req_.back().req_));  
//               &(ptr_cd_->entry_send_req_[tag_to_search].req_));  
  
    if(target.handle_type() == DataHandle::kOSFile || target.handle_type() == DataHandle::kPFS) {
      delete data_to_send;
    }
    CD_DEBUG("CD Event kEntrySend\t\t\t");
  
    if(ptr_cd_->IsHead()) {
      ptr_cd_->event_flag_[ptr_cd_->task_in_color()] &= ~kEntrySend;
    }
    else {
      *(ptr_cd_->event_flag_) &= ~kEntrySend;
    }
    IncHandledEventCounter();
  }
#endif
}



void HandleErrorOccurred::HandleEvent(void)
{
#if _MPI_VER

  if(ptr_cd_->task_size() == 1) return;

  int task_id = task_id_;
  CD_DEBUG("\n[HandleErrorOccurred::HandleEvent]\n");

  CDEventT all_reexecute = kAllReexecute;
  for(int i=0; i<ptr_cd_->task_size(); i++) {
    CD_DEBUG("SetMailBox(kAllRexecute) for error occurred of task #%d\n", i);
    ptr_cd_->SetMailBox(all_reexecute, i);
  }

  // Resume
  CD_DEBUG("\n== HandleErrorOccurred::HandleEvent HeadCD Event kErrorOccurred is handled!!\n");
  ptr_cd_->event_flag_[task_id] &= ~kErrorOccurred;
  IncHandledEventCounter();

  // reset this flag for the next error
  // This will be invoked after CheckMailBox is done, 
  // so it is safe to reset here.
  ptr_cd_->error_occurred = false;

#endif
}

//void AllPause::HandleEvent(void)
//{
//  // Pause all the tasks in current CD, first.
//  for(int i=0; i<ptr_cd_->task_size(); i++) {
//    if(i == ptr_cd_->head()) continue;
//    CDEventT all_pause = kAllPause;
//    ptr_cd_->SetMailBox(all_pause, i);
//  }
//
//}

void HandleAllResume::HandleEvent(void)
{
#if _MPI_VER


  if(ptr_cd_->task_size() == 1) return;

  CD_DEBUG("CD Event kAllResume\t\t\t");

    *(ptr_cd_->event_flag_) &= ~kAllResume;

  IncHandledEventCounter();

//  PMPI_Barrier(ptr_cd_->color());
//  cout << "\n\n[Barrier] HandleAllResume::HandleEvent "<< ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() << "\n\n" << endl; //getchar();

  CD_DEBUG("Barrier resolved\n");

#endif
}

void HandleAllPause::HandleEvent(void)
{
#if _MPI_VER

#if 0
  if(ptr_cd_->task_size() == 1) return;
  
  dbg << "CD Event kAllPause\t\t\t";
  *(ptr_cd_->event_flag_) &= ~kAllPause;
  IncHandledEventCounter();
  dbg << "Barrier resolved" << endl;
  PMPI_Barrier(ptr_cd_->color());
  cout << "\n\n[Barrier] HandleAllPause::HandleEvent " << ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() << "\n\n" << endl;

  dbg << "Barrier resolved" << endl;

  if( CHECK_EVENT(*(ptr_cd_->event_flag_), kAllReexecute) ) {
    ptr_cd_->need_reexec = true;
  }
#else
  *(ptr_cd_->event_flag_) &= ~kAllPause;
  IncHandledEventCounter();
#endif

#endif
}

void HandleAllReexecute::HandleEvent(void)
{
#if _MPI_VER

#if 1
  if(ptr_cd_->task_size() == 1) assert(0);

  CD_DEBUG("CD Event kAllReexecute\t\t");
  *(ptr_cd_->event_flag_) &= ~kAllReexecute;
  IncHandledEventCounter();

  ptr_cd_->need_reexec = true;
  ptr_cd_->reexec_level = ptr_cd_->level();
#else
  *(ptr_cd_->event_flag_) &= ~kAllReexecute;
  IncHandledEventCounter();

#endif

#endif
}

















/*
void HandleErrorOccurred::HandleEvent(void)
{
  if(ptr_cd_->task_size() == 1) return;
  int task_id = task_id_;

  // Pause all the tasks in current CD, first.
  for(int i=0; i<ptr_cd_->task_size(); i++) {
    if(i == ptr_cd_->task_in_color()) continue;
    CDEventT all_pause = kAllPause;
    ptr_cd_->SetMailBox(all_pause, i);
  }

  // Make decision for the error handling
  bool error_handled = true;
  if(error_handled) {
    // Resume

    dbg << "HeadCD Event kErrorOccurred";
    ptr_cd_->event_flag_[task_id] &= ~kErrorOccurred;


//    dbg << "Barrier resolved" << endl;
//    PMPI_Barrier(ptr_cd_->color());
//    cout << "\n\n[Barrier] HandleErrorOccurred::HandleEvent, normal "<< ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() << "\n\n" << endl; //getchar();
//    dbg << "Barrier resolved" << endl;

    // reset this flag for the next error
    // This will be invoked after CheckMailBox is done, 
    // so it is safe to reset here.
    ptr_cd_->error_occurred = false;
  }
  else {

    for(int i=0; i<ptr_cd_->task_size(); i++) {
      if(i == ptr_cd_->task_in_color()) continue;
      CDEventT all_reexecute = kAllReexecute;
      ptr_cd_->SetMailBox(all_reexecute, i);
    }

    // Resume
    dbg << "HeadCD Event kErrorOccurred";
    ptr_cd_->event_flag_[task_id] &= ~kErrorOccurred;
    IncHandledEventCounter();

    // reset this flag for the next error
    // This will be invoked after CheckMailBox is done, 
    // so it is safe to reset here.
    ptr_cd_->error_occurred = false;
  }
}
*/

/*
//void AllPause::HandleEvent(void)
//{
//  // Pause all the tasks in current CD, first.
//  for(int i=0; i<ptr_cd_->task_size(); i++) {
//    if(i == ptr_cd_->head()) continue;
//    CDEventT all_pause = kAllPause;
//    ptr_cd_->SetMailBox(all_pause, i);
//  }
//
//}

*/
