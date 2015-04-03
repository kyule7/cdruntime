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

  dbg << "SIZE :" << ptr_cd_->task_size() << ", LEVEL : " << ptr_cd_->level() << endl;
  dbg << "[HeadCD::HandleEntrySearch\t" << task_id_ << endl;
  dbg << "[FromRequester] CHECK TAG : " << ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, entry_requester_id)
      << " (Entry Tag: " << "NOT_YET_READY" << ")" << endl;
  dbg << "CHECK IT OUT : " << entry_requester_id << " --> " << ptr_cd_->task_in_color() << endl;
  dbg.flush(); 
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
  int found_level = 0;
  
  CDEntry *target_entry = ptr_cd_->InternalGetEntry(tag_to_search);
  if(target_entry != NULL) { 
    dbg << "FOUND it at this level! " << ptr_cd_->level() << endl;
    found_level = ptr_cd_->level();
  } 
  else {
    dbg << "NOT FOUND it at this level! " << ptr_cd_->level() << endl;
    target_entry = ptr_cd_->SearchEntry(tag_to_search, found_level);
  }


  if(target_entry != NULL) {
    dbg << "It succeed in finding the entry at this level #"<<found_level<<" of head task!" << endl;
    // It needs some effort to avoid the naming alias problem of entry tags.
//    ptr_cd_->entry_search_req_[tag_to_search] = CommInfo();

    HeadCD *found_cd = static_cast<HeadCD *>(CDPath::GetCDLevel(found_level)->ptr_cd());
    // It needs some effort to avoid the naming alias problem of entry tags.
  
    // Found the entry!! 
    // Then, send it to the task that has the entry of actual copy
    ENTRY_TAG_T tag_to_search = target_entry->name_tag();
    int target_task_id = target_entry->dst_data_.node_id().task_in_color();
    dbg << "FOUND " << found_cd->GetCDName() << " " << found_cd->GetNodeID() << ", found level: " << found_level << ", check it out: "<< target_task_id<<endl;

    // SetMailBox for entry send to target
    CDEventT entry_send = kEntrySend;

    if(target_task_id != ptr_cd_->task_in_color()) {
      found_cd->SetMailBox(entry_send, target_task_id);
    

    dbg << "[ToSender] CHECK TAG : " << found_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, found_cd->task_in_color())
        << " (Entry Tag: " << tag_to_search << ")" << endl;
    dbg.flush();
    ENTRY_TAG_T sendBuf[2] = {tag_to_search, source_task_id};
//    found_cd->entry_search_req_[tag_to_search] = CommInfo();
    found_cd->entry_req_.push_back(CommInfo()); //getchar();
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
      dbg << "HandleEntrySearch --> EntrySend (Head): entry was found at " << found_level << endl;
      dbg << "Size of data to send : " << target_entry->dst_data_.len() << " (" << tag_to_search << ")" << endl;
      dbg.flush();
    //  ptr_cd_->entry_send_req_[tag_to_search] = CommInfo();
      ptr_cd_->entry_req_.push_back(CommInfo()); //getchar();
      // Should be non-blocking send to avoid deadlock situation. 
      PMPI_Isend(target_entry->dst_data_.address_data(), 
                 target_entry->dst_data_.len(), 
                 MPI_BYTE, 
                 source_task_id, 
                 ptr_cd_->cd_id_.GenMsgTag(tag_to_search), 
                 MPI_COMM_WORLD, // could be any task in the whole rank group 
                 &(ptr_cd_->entry_req_.back().req_));  
    //             &(ptr_cd_->entry_send_req_[tag_to_search].req_));  

    }



//    ptr_cd_->event_flag_[entry_requester_id] &= ~kEntrySearch;
//    IncHandledEventCounter();
  }
  else {
    // failed to search the entry at this level of CD.
//    cerr<<"Failed to search the entry at current CD level. Request to upper-level ptr_cd_->head to search the entry. Not implemented yet\n"<<endl;
//    assert(0);

    dbg << "It failed to find the entry at this level #"<<ptr_cd_->level()<< " of head task. Request to parent level!" << endl;

    CDHandle *parent = CDPath::GetParentCD(ptr_cd_->level());
    CDEventT entry_search = kEntrySearch;
    parent->ptr_cd()->SetMailBox(entry_search);
    dbg << "CHECK TAG : " << parent->ptr_cd()->GetCDID().GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, parent->task_in_color()) 
        <<" (Entry Tag: "<< tag_to_search<< ")"<< endl;
    dbg.flush();
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

  dbg << "CD::HandleEntrySend\n" << endl;
  dbg << "CHECK TAG : " << ptr_cd_->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, ptr_cd_->head()) << endl;
  dbg.flush();
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
//  dbg << "Received Entry Tag : " << tag_to_search << " = " << tag2str[tag_to_search] << endl; 

  dbg.flush();

  // Find entry
  if(tag_to_search == INIT_TAG_VALUE) {
    cerr << "Entry sender failed to receive the entry tag to search for the right entry with. CD Level " << ptr_cd_->level() << endl;
    assert(0);
  }

//  CDEntry *entry = InternalGetEntry(tag_to_search);
  int found_level=ptr_cd_->level();


  CDEntry *entry = ptr_cd_->InternalGetEntry(tag_to_search);
  if(entry != NULL) { 
    dbg << "FOUND it at this level! " << ptr_cd_->level() << endl;
    found_level = ptr_cd_->level();
  } 
  else {
    dbg << "NOT FOUND it at this level! " << ptr_cd_->level() << endl;
    entry = ptr_cd_->SearchEntry(tag_to_search, found_level);
  }



//  CDEntry *entry = ptr_cd_->SearchEntry(tag_to_search, found_level);
  
  //CDHandle *cdh = CDPath::GetParentCD(ptr_cd_->level());
  CDHandle *cdh = CDPath::GetCDLevel(ptr_cd_->level());
  
  for(auto it=cdh->ptr_cd()->remote_entry_directory_map_.begin(); it!=cdh->ptr_cd()->remote_entry_directory_map_.end(); ++it) {
    dbg << it->first << " (" << tag2str[it->first] << ") - " << *(it->second) << endl;
    dbg << "--------------------- level : "<<cdh->level()<< " --------------------------------" << endl;
  }

  if(entry == NULL) {
    dbg << "The received tag ["<< tag_to_search << "] does not work in "<< ptr_cd_->GetNodeID() <<". CD Level " << ptr_cd_->level() << endl; dbg.flush();
    cerr << "The received tag ["<< tag_to_search << "] does not work in "<< ptr_cd_->GetNodeID() <<". CD Level " << ptr_cd_->level() << endl;
    assert(0);
  }
  else
    dbg << "HandleEntrySend: entry was found at " << found_level << endl;
  dbg.flush();
//  ptr_cd_->entry_send_req_[tag_to_search] = CommInfo();
  ptr_cd_->entry_req_.push_back(CommInfo()); //getchar();
  // Should be non-blocking send to avoid deadlock situation. 
  PMPI_Isend(entry->dst_data_.address_data(), 
             entry->dst_data_.len(), 
             MPI_BYTE, 
             entry_source_task_id, 
             ptr_cd_->GetCDID().GenMsgTag(tag_to_search), 
             MPI_COMM_WORLD, // could be any task in the whole rank group 
             &(ptr_cd_->entry_req_.back().req_));  
//             &(ptr_cd_->entry_send_req_[tag_to_search].req_));  

  dbg << "CD Event kEntrySend\t\t\t";
  if(ptr_cd_->IsHead()) {
    ptr_cd_->event_flag_[ptr_cd_->task_in_color()] &= ~kEntrySend;
  }
  else {
    *(ptr_cd_->event_flag_) &= ~kEntrySend;
  }
  IncHandledEventCounter();

#endif
}



void HandleErrorOccurred::HandleEvent(void)
{
#if _MPI_VER


  if(ptr_cd_->task_size() == 1) return;
  int task_id = task_id_;
  dbg << "\n== HandleErrorOccurred::HandleEvent\n"<<endl;;

  CDEventT all_reexecute = kAllReexecute;
  for(int i=0; i<ptr_cd_->task_size(); i++) {
    dbg << "\nSetMailBox(kAllRexecute) for error occurred of task #" << i <<endl;;
    ptr_cd_->SetMailBox(all_reexecute, i);
  }

  // Resume
  dbg << "\n== HandleErrorOccurred::HandleEvent HeadCD Event kErrorOccurred is handled!!\n"<<endl;;
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

  dbg << "CD Event kAllResume\t\t\t";

    *(ptr_cd_->event_flag_) &= ~kAllResume;

  IncHandledEventCounter();

//  PMPI_Barrier(ptr_cd_->color());
  cout << "\n\n[Barrier] HandleAllResume::HandleEvent "<< ptr_cd_->GetCDName() <<" / " << ptr_cd_->GetNodeID() << "\n\n" << endl; //getchar();
  dbg << "Barrier resolved" << endl;

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

  dbg << "CD Event kAllReexecute\t\t";
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
