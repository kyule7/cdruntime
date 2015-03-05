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
  int entry_requester_id = task_id_;
  ENTRY_TAG_T recvBuf[2] = {0,0};

  dbg << "HeadCD::HandleEntrySearch\t";

  MPI_Status status;
  MPI_Recv(recvBuf, 
           2, 
           MPI_UNSIGNED_LONG_LONG, 
           entry_requester_id, 
           MSG_TAG_ENTRY_TAG, 
           ptr_cd_->color(), // Entry requester is in the same CD task group. 
           &status);

  ENTRY_TAG_T &tag_to_search  = recvBuf[0];
  ENTRY_TAG_T &source_task_id = recvBuf[1];
  int found_level = 0;

  CDEntry *target_entry = ptr_cd_->SearchEntry(tag_to_search, found_level);
  
  if(target_entry != NULL) {
    // It needs some effort to avoid the naming alias problem of entry tags.
    ptr_cd_->entry_search_req_[tag_to_search] = CommInfo();
  
    // Found the entry!! 
    // Then, send it to the task that has the entry of actual copy
    ENTRY_TAG_T tag_to_search = target_entry->name_tag();
    int target_task_id = target_entry->dst_data_.node_id().task_in_color();

    // SetMailBox for entry send to target
    CDEventT entry_send = kEntrySend;
    ptr_cd_->SetMailBox(entry_send, target_task_id);

    ENTRY_TAG_T sendBuf[2] = {tag_to_search, source_task_id};
    MPI_Isend(sendBuf, 
              2, 
              MPI_UNSIGNED_LONG_LONG, 
              target_task_id, 
              ptr_cd_->GetCDID().GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG),
              ptr_cd_->color(), // Entry sender is also in the same CD task group.
              &(ptr_cd_->entry_search_req_[tag_to_search]).req_);
  
    ptr_cd_->event_flag_[entry_requester_id] &= ~kEntrySearch;
    handled_event_count++;
  }
  else {
    // failed to search the entry at this level of CD.
//    cerr<<"Failed to search the entry at current CD level. Request to upper-level ptr_cd_->head to search the entry. Not implemented yet\n"<<endl;
//    assert(0);

    CDHandle *parent = CDPath::GetParentCD(ptr_cd_->level());
    CDEventT entry_search = kEntrySearch;
    parent->ptr_cd()->SetMailBox(entry_search);
    MPI_Isend(recvBuf, 
              2, 
              MPI_UNSIGNED_LONG_LONG,
              parent->node_id().head(), 
              parent->ptr_cd()->GetCDID().GenMsgTag(MSG_TAG_ENTRY_TAG), 
              parent->node_id().color(),
              &(parent->ptr_cd()->entry_request_req_[recvBuf[0]].req_));

    ptr_cd_->event_flag_[entry_requester_id] &= ~kEntrySearch;
    handled_event_count++;
  }


}

void HandleEntrySend::HandleEvent(void)
{
  dbg << "CD::HandleEntrySend\n" << endl;

  MPI_Status status;
  ENTRY_TAG_T recvBuf[2] = {INIT_TAG_VALUE, INIT_ENTRY_SRC};
  MPI_Recv(recvBuf, 
           2, 
           MPI_UNSIGNED_LONG_LONG, 
           ptr_cd_->head(), 
           ptr_cd_->GetCDID().GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG),
           ptr_cd_->color(), // Entry searcher is in the same CD task gruop (ptr_cd_->head) 
           &status);

  ENTRY_TAG_T &tag_to_search = recvBuf[0];
  int entry_source_task_id  = static_cast<int>(recvBuf[1]);
  dbg << "Received tag : " << tag_to_search << " = " << tag2str[tag_to_search] << endl; 

  // Find entry
  if(tag_to_search == INIT_TAG_VALUE) {
    cerr << "Entry sender failed to receive the entry tag to search for the right entry with. CD Level " << ptr_cd_->level() << endl;
    assert(0);
  }

//  CDEntry *entry = InternalGetEntry(tag_to_search);
  int found_level=ptr_cd_->level();
  CDEntry *entry = ptr_cd_->SearchEntry(tag_to_search, found_level);

  if(entry == NULL) {
    cerr << "The received tag ["<< tag_to_search << "] does not work in "<< ptr_cd_->GetCDID().node_id() <<". CD Level " << ptr_cd_->level() << endl;
    assert(0);
  }
  else
    dbg << "HandleEntrySend: entry was found at " << found_level << endl;

  ptr_cd_->entry_send_req_[tag_to_search] = CommInfo();
  // Should be non-blocking send to avoid deadlock situation. 
  MPI_Isend(entry->dst_data_.address_data(), 
            entry->dst_data_.len(), 
            MPI_BYTE, 
            entry_source_task_id, 
            ptr_cd_->GetCDID().GenMsgTag(tag_to_search), 
            MPI_COMM_WORLD, // could be any task in the whole rank group 
            &(ptr_cd_->entry_send_req_[tag_to_search].req_));  

  dbg << "CD Event kEntrySend\t\t\t";
  *(ptr_cd_->event_flag_) &= ~kEntrySend;
  handled_event_count++;
}



void HandleErrorOccurred::HandleEvent(void)
{
  if(ptr_cd_->task_size() == 1) return;
  int task_id = task_id_;
#if 1
  // Pause all the tasks in current CD, first.
  for(int i=0; i<ptr_cd_->task_size(); i++) {
    if(i == ptr_cd_->head()) continue;
    CDEventT all_pause = kAllPause;
    ptr_cd_->SetMailBox(all_pause, i);
  }

  // Make decision for the error handling
  bool error_handled = false;
  if(error_handled) {
    // Resume

    dbg << "HeadCD Event kErrorOccurred";
    ptr_cd_->event_flag_[task_id] &= ~kErrorOccurred;
//    handled_event_count++;


    dbg << "Barrier resolved" << endl;
    MPI_Barrier(ptr_cd_->color());
    cout << "\n\n[Barrier] HandleErrorOccurred::HandleEvent, normal "<< ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() << "\n\n" << endl; //getchar();
    dbg << "Barrier resolved" << endl;
    // reset this flag for the next error
    // This will be invoked after CheckMailBox is done, 
    // so it is safe to reset here.
    ptr_cd_->error_occurred = false;
  }
  else {
    for(int i=0; i<ptr_cd_->task_size(); i++) {
      if(i == ptr_cd_->head()) continue;
      CDEventT all_reexecute = kAllReexecute;
      ptr_cd_->SetMailBox(all_reexecute, i);
    }

    // Resume
    dbg << "HeadCD Event kErrorOccurred";
    ptr_cd_->event_flag_[task_id] &= ~kErrorOccurred;

    dbg << "Barrier resolved" << endl;
    MPI_Barrier(ptr_cd_->color()); // for succeeding MPI_Barrier after CheckMailBox
    MPI_Barrier(ptr_cd_->color());
    cout << "\n\n[Barrier] HandleErrorOccurred::HandleEvent, reexec "<< ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() <<"\n\n" << endl; //getchar();
    dbg << "Barrier resolved" << endl;
    // reset this flag for the next error
    // This will be invoked after CheckMailBox is done, 
    // so it is safe to reset here.
    ptr_cd_->error_occurred = false;


    ptr_cd_->need_reexec = true;
  }
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
  if(ptr_cd_->task_size() == 1) return;

  dbg << "CD Event kAllResume\t\t\t";
  ptr_cd_->event_flag_[ptr_cd_->head()] &= ~kAllResume;
  handled_event_count++;

  MPI_Barrier(ptr_cd_->color());
  cout << "\n\n[Barrier] HandleAllResume::HandleEvent "<< ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() << "\n\n" << endl; //getchar();
  dbg << "Barrier resolved" << endl;
}

void HandleAllPause::HandleEvent(void)
{
  if(ptr_cd_->task_size() == 1) return;
  
  dbg << "CD Event kAllPause\t\t\t";
  *(ptr_cd_->event_flag_) &= ~kAllPause;
  handled_event_count++;
  dbg << "Barrier resolved" << endl;
  MPI_Barrier(ptr_cd_->color());
  cout << "\n\n[Barrier] HandleAllPause::HandleEvent " << ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() << "\n\n" << endl;

  dbg << "Barrier resolved" << endl;

  if( CHECK_EVENT(*(ptr_cd_->event_flag_), kAllReexecute) ) {
    ptr_cd_->need_reexec = true;
  }
}

void HandleAllReexecute::HandleEvent(void)
{
  if(ptr_cd_->task_size() == 1) return;

  dbg << "CD Event kAllReexecute\t\t";
  *(ptr_cd_->event_flag_) &= ~kAllReexecute;
  handled_event_count++;

  ptr_cd_->need_reexec = true;
}
