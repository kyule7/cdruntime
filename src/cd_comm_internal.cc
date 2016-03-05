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

#include "cd_config.h"

#if _MPI_VER
#include "cd_path.h"
#include "cd_global.h"
#include "cd_handle.h"
#include "cd_internal.h"
#include "cd_def_internal.h"
using namespace cd;
using namespace cd::internal;
using namespace std;

int requested_event_count = 0;
int cd::handled_event_count = 0;


NodeID CDHandle::GenNewNodeID(const ColorT &my_color, const int &new_color, const int &new_task, const int &new_head_id)
{
//  dbg<<"new_color : " << new_color <<", new_task: "<<new_task<<", new_node.color(): "<<new_node.color()<<endl;
    CD_DEBUG("[%s] need_reexec??? %d from %u (%s)\n", __func__, CD::need_reexec, CD::reexec_level, ptr_cd_->name_.c_str());

    if(task_size() > 1) {
      CD_DEBUG("[%s] CheckMailBox before calling MPI_Comm_split at level %d (%s)\n", __func__, level(), ptr_cd_->name_.c_str());
      //printf("[%s] CheckMailBox before calling MPI_Comm_split at level %d (%s)\n", __func__, level(), ptr_cd_->name_.c_str());
      MPI_Win_fence(0, ptr_cd_->mailbox_);
//      PMPI_Barrier(color());
    } else {
      CD_DEBUG("[%s] Not synched.....level %d (%s)\n", __func__, level(), ptr_cd_->name_.c_str());
    }
    CheckMailBox();
    
    if(task_size() > 1) {
      CD_DEBUG("[%s 2] CheckMailBox before calling MPI_Comm_split at level %d (%s)\n", __func__, level(), ptr_cd_->name_.c_str());
      //printf("[%s 2] CheckMailBox before calling MPI_Comm_split at level %d (%s)\n", __func__, level(), ptr_cd_->name_.c_str());
      MPI_Win_fence(0, ptr_cd_->mailbox_);
//      PMPI_Barrier(color());
    } else {
      CD_DEBUG("[%s 2] Not synched.....level %d (%s)\n", __func__, level(), ptr_cd_->name_.c_str());
    }
    CheckMailBox();

    CD_DEBUG("[%s] need_reexec? %d from %u (%s)\n", __func__, CD::need_reexec, CD::reexec_level, ptr_cd_->name_.c_str());
    //printf("[%s] need_reexec? %d from %u (%s)\n", __func__, CD::need_reexec, CD::reexec_level, ptr_cd_->name_.c_str());
    if(CD::need_reexec) {
      CD_DEBUG("\n\nReexec (Before calling ptr_cd_->GetCDToRecover()->Recover(false);\n\n");
      CD::GetCDToRecover(this, false)->ptr_cd()->Recover();
//      CD *cd_to_recover = ptr_cd_->GetCDToRecover();
//      cd_to_recover->recoverObj_->Recover(cd_to_recover);
    } else {
      CD_DEBUG("\n\nReexec is false\n");
    }

  NodeID new_node_id(new_head_id);
  PMPI_Comm_split(my_color, new_color, new_task, &(new_node_id.color_));
  PMPI_Comm_size(new_node_id.color_, &(new_node_id.size_));
  PMPI_Comm_rank(new_node_id.color_, &(new_node_id.task_in_color_));
  PMPI_Comm_group(new_node_id.color_, &(new_node_id.task_group_));

  return new_node_id;
}

/// Synchronize the CD object in every task of that CD.
CDErrT CDHandle::Sync(ColorT color) 
{
#if _MPI_VER
  PMPI_Barrier(color);
#endif

  return kOK;
}

void CDHandle::CollectHeadInfoAndEntry(const NodeID &new_node_id) 
{
  // Get the children CD's head information and the total size of entry from each task in the CD.
  int send_buf[2]={0,0};
  int task_count = node_id().size();
  int recv_buf[task_count][2]; 
  if(new_node_id.IsHead()) {
    send_buf[0] = node_id().task_in_color();
  } else {
    send_buf[0] = -1;
  }
  send_buf[1] = ptr_cd()->remote_entry_directory_map_.size();

  PMPI_Allgather(send_buf, 2, MPI_INT, 
                recv_buf, 2, MPI_INT, 
                node_id().color());

  CD_DEBUG("\n================== Remote entry check ===================\n");
  CD_DEBUG("[Before] Check entries in remote entry directory\n");

  for(auto it = ptr_cd()->remote_entry_directory_map_.begin();
           it!= ptr_cd()->remote_entry_directory_map_.end(); ++it) {
    CD_DEBUG("%s\n", it->second->GetString().c_str());

  }
  uint64_t serialized_len_in_bytes=0;

  void *serialized_entry = ptr_cd()->SerializeRemoteEntryDir(serialized_len_in_bytes); 

  // if there is no entries to send to head, just return
  if(serialized_entry == NULL) return;
  
////  int entry_count=0;
//  int recv_counts[task_count];
//  int displs[task_count];
////  int stride = 1196;
//  for(int k=0; k<task_count; k++ ) {
////    displs[k] = k*stride;
//    displs[k] = k*serialized_len_in_bytes;
//    recv_counts[k] = serialized_len_in_bytes;
//  }

//  dbg << "# of entries to gather : " << entry_count << endl;
//  CDEntry *entry_to_deserialize = new CDEntry[entry_count*2];
//  PMPI_Gatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().head(), node_id().color());

  int recv_count = task_count*serialized_len_in_bytes;
//  int recv_count = task_count*stride;
  void *entry_to_deserialize = NULL;

  PMPI_Alloc_mem(recv_count, MPI_INFO_NULL, &entry_to_deserialize);
  MPI_Datatype sType;
  PMPI_Type_contiguous(serialized_len_in_bytes, MPI_BYTE, &sType);
  PMPI_Type_commit(&sType);

//  PMPI_Datatype rType;
//  PMPI_Type_contiguous(recv_count, MPI_BYTE, &rType);
////  PMPI_Type_create_struct(task_count, serialized_len_in_bytes, 0, MPI_BYTE, &rType);
////  PMPI_Type_vector(task_count, serialized_len_in_bytes, serialized_len_in_bytes, MPI_BYTE, &rType);
//  PMPI_Type_commit(&rType);


  CD_DEBUG("\n\nNote : %p, %lu, %d, remote entry dir map size : %lu\n\n", 
           serialized_entry, serialized_len_in_bytes, recv_count, 
           ptr_cd()->remote_entry_directory_map_.size());

//  for(auto it = ptr_cd()->remote_entry_directory_map_.begin(); it!=ptr_cd()->remote_entry_directory_map_.end(); ++it) {
//    dbg << *(it->second) << endl;
//  }
//  char *rbuf = new char[sizeof(int)*task_count*2];

//  PMPI_Gather(&node_id_.task_in_color_, 1, MPI_INT,
//             rbuf, 1, MPI_INT,
//             node_id().head(), node_id().color());
//
//  if(node_id_.task_in_color() == 3) {
//    cout << "entry 3 : " << ptr_cd()->remote_entry_directory_map_.size() << ", head: "<< node_id_.head() << endl;
//  }

//  ptr_cd()->DeserializeRemoteEntryDir(remote_entry_dir, serialized_entry); 
//  dbg << "[Before Gather Entry Check Begin] size : "<< serialized_len_in_bytes << endl;
//  for(auto it = remote_entry_dir.begin(); it!=remote_entry_dir.end(); ++it) {
//    dbg << *(it->second) << endl;
//  }
//  dbg << "[Before Gather Entry Check Ends] size : "<< serialized_len_in_bytes << endl;
//  remote_entry_dir.clear();



//  PMPI_Gather(serialized_entry, 1, sType,
//             entry_to_deserialize, 1, sType,
//             node_id().head(), node_id().color());
//  dbg << "Wait!! " << node_id().size() << endl;
//  for(int i=0; i<task_count; i++) {
//    dbg << "recv_counts["<<i<<"] : " << recv_counts[i] << endl;
//    dbg << "displs["<<i<<"] : " << displs[i] << endl;
//  }
//  PMPI_Allgatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().color());




//  int test_counts[task_count];
//  int test_displs[task_count];
//  for(int k=0; k<task_count; k++ ) {
//    test_displs[k] = k*4;
//    test_counts[k] = 4;
//  }
//  dbg << "Wait!! " << node_id().size() << endl;
//  for(int i=0; i<task_count; i++) {
//    dbg << "test_counts["<<i<<"] : " << test_counts[i] << endl;
//  }
//  for(int i=0; i<task_count; i++) {
//    dbg << "test_displs["<<i<<"] : " << test_displs[i] << endl;
//  }
//  int test_a[4];
//  int test_b[task_count*4];
////  int test_b[task_count][4];
//  test_a[0] = node_id().task_in_color();
//  test_a[1] = test_a[0]+10;
//  test_a[2] = test_a[0]+100;
//  test_a[3] = test_a[0]+1000;
//
//  PMPI_Gatherv(test_a, 4, MPI_INT,
//              test_b, test_counts, test_displs, MPI_INT,
//              node_id().head(), node_id().color());





//  PMPI_Gatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().head(), node_id().color());
//  PMPI_Allgatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().head(), node_id().color());

  PMPI_Gather(serialized_entry, 1, sType,
             entry_to_deserialize, 1, sType,
             node_id_.head(), node_id_.color());

  if(IsHead()) {
    
//    int * test_des = (int *)entry_to_deserialize;
//    char * test_des_char = (char *)entry_to_deserialize;
//    int test_count = serialized_len_in_bytes/4;
//    int tk=0;
//    int tj=0;
//    for(int j=0; j<task_count; j++){
////      dbg << test_des+tk << "-- " << (void *)(test_des_char + tj)<< ", j: "<< j<< endl;
////      for(int i=0; i<test_count;i++) {
////        dbg << *(test_des+i+j*test_count) << " ";
////        tk = i + j*test_count;
////        tj = 4*i + j*serialized_len_in_bytes;
////      }
//        tk = j*test_count;
//        tj = j*serialized_len_in_bytes;
//        dbg << *(test_des+tk) << endl;
//        dbg << *(test_des_char+tj) << endl;
//        cout << test_des+tk << "-- " << (void *)(test_des_char + tj)<< ", j: "<< j<< endl;
//      dbg << endl;
//    }
//    dbg << endl;
////    cout << "1" << endl;
////    for(int i=0; i<task_count; i++) cout << " --- " << ((int *)rbuf)[i];
////    cout << endl;
////    getchar();
////    ptr_cd()->DeserializeRemoteEntryDir(remote_entry_dir, serialized_entry);
//
//    dbg << "===" << serialized_len_in_bytes << endl;


 
    void * temp_ptr = (void *)((char *)entry_to_deserialize+serialized_len_in_bytes-8);

    CD_DEBUG("Check it out : %p -- %p, diff : %p\n", 
             entry_to_deserialize, temp_ptr, (void *)((char *)temp_ptr - (char *)entry_to_deserialize));

    ptr_cd()->DeserializeRemoteEntryDir(ptr_cd()->remote_entry_directory_map_, entry_to_deserialize, task_count, serialized_len_in_bytes); 
//    ptr_cd()->DeserializeRemoteEntryDir(remote_entry_dir, (void *)((char *)entry_to_deserialize + 1196), task_count, serialized_len_in_bytes); 

    CD_DEBUG("\n\n[After] Check entries after deserialization, size : %lu, # of tasks : %u, level : %u\n", 
             ptr_cd()->remote_entry_directory_map_.size(), node_id_.size(), ptr_cd()->GetCDID().level());

//    for(auto it = remote_entry_dir.begin(); it!=remote_entry_dir.end(); ++it) {
//      dbg << *(it->second) << endl;
//    }

    CD_DEBUG("\n\n============================ End of deserialization ===========================\n\n");
  }
  for(auto it=ptr_cd()->remote_entry_directory_map_.begin(); it!=ptr_cd()->remote_entry_directory_map_.end(); ++it) {
    CD_DEBUG("%s\n", it->second->GetString().c_str());
  }  

}  




 
//  }
//  else{
//
//    if(new_node_id.IsHead()) {
//      // Calculate the array of group 
//      PMPI_Group_incl(node_id_.task_group_, head_rank_count, head_rank, &(dynamic_cast<HeadCD*>(ptr_cd)->task_group())); 
//
//    }
//
//  }

/*


//  // Calculate the array of displacement and entry_count at each task from recv_buf[oddnum].
//  // Calculate recv count and displacement for entries of each task's remote entries.
//  int entry_size;
//  int entry_count[task_count];
//  int entry_disp[task_count];
//
//  char *serialized_entries;
//  char *entries_to_be_deserialized;
//
//  if(!IsHead()) {
//    // Serialize all the entries in CD.
//    serialized_entries = ptr_cd_->SerializeEntryDir(uint32_t& entry_count);
//  }
//
//  PMPI_Gatherv(serialized_entries, entry_size, MPI_BYTE, 
//              entries_to_be_deserialized, entry_count, entry_disp, MPI_BYTE, 
//              node_id().head(), node_id().color());
//
//  if(IsHead()) {  // Current task is head. It receives children CDs' head info and entry info of every task in the CD.
//    // Deserialize the entries received from the other tasks in current CD's task group.
//    while(ptr < entry_disp[task_count-1] + entry_count[task_count-1]) { // it is the last position
//      ptr_cd_->DeserializeEntryDir(entries_to_be_deserialized + entry_disp[i]);
//    }
//  }
//
//  dbg << "\n\n\nasdfasdfsadf\n\n"<< endl <<endl;
//  dbgBreak();
#endif
#endif






//  if(!IsHead()) {
//    // Serialize all the entries in CD.
//
////    void *buffer = ptr_cd_->SerializeEntryDir(uint32_t& entry_count);
//
//    void *buffer=NULL;
//    Packer entry_dir_packer;
//    uint32_t len_in_bytes=0;
//    uint32_t entry_count=0;
//    void *packed_entry_p=NULL;
//    
//    int task_id=0;
//    if(new_node_id.IsHead()) {
//      task_id = new_node_id.task_in_color();
//    } else {
//      task_id = -1;
//    }
//
//    packed_entry_p = (void *)&task_id;
//    entry_dir_packer.Add(entry_count++, sizeof(int), packed_entry_p);
//  
//    for(auto it=remote_entry_directory_.begin(); it!=remote_entry_directory_.end(); ++it) {
//      uint32_t entry_len=0;
//      packed_entry_p=NULL;
//      if( !it->name().empty() ){
//        // entry_len is output of Serialize 
//        packed_entry_p = it->Serialize(entry_len);
//        entry_dir_packer.Add(entry_count++, entry_len, packed_entry_p); // Add(id, len, datap)
//        len_in_bytes += entry_len;
//      }
//    }
//    
//    buffer = entry_dir_packer.GetTotalData(len_in_bytes);
//
//
//    PMPI_Gather(send_buf, 1, MPI_BYTE, task_id_buffer, 1, MPI_BYTE, node_id().head(), node_id().color());
//  }
//  else {  // Current task is head. It receives children CDs' head info and entry info of every task in the CD.
//    char *recv_buf;
//    PMPI_Gather(send_buf, 1, MPI_BYTE, recv_buf, 1, MPI_BYTE, node_id().head(), node_id().color());
//
//    // Deserialize the entries received from the other tasks in current CD's task group.
//    ptr_cd_->DeserializeEntryDir(recv_buf);
//
//    std::vector<CDEntry> entry_dir;
//    Unpacker entry_dir_unpacker;
//    void *unpacked_entry_p=0;
//    uint32_t dwGetID=0;
//    uint32_t return_size=0;
//    int child_heads[num_children];
//    child_heads[0] = node_id().task_in_color();
//
//
//    while(1) {
//      unpacked_entry_p = entry_dir_unpacker.GetNext((char *)recv_buf, dwGetID, return_size);
//      if(unpacked_entry_p == NULL) break;
//      cd_entry.Deserialize(unpacked_entry_p);
//      entry_dir.push_back(cd_entry);
//    }
//
////    PMPI_Group_incl(node_id().color(), num_children, child_heads, &(dynamic_cast<HeadCD*>(ptr_cd)->task_group()));
//
//  }
//
//
//
//  int task_id_buffer[node_id().size()];
//  int task_id = node_id().task_in_color();
//  int null_id = 0;
//  if(new_node_id.IsHead()) {
//    PMPI_Gather(&task_id, 1, MPI_INT, task_id_buffer, 1, MPI_INT, node_id().head(), node_id().color());
//  }
//  else {
//    PMPI_Gather(&null_id, 1, MPI_INT, task_id_buffer, 1, MPI_INT, node_id().head(), node_id().color());
//  }
//  if(IsHead()) {
//    for(int i=0; i<node_id().size(); ++i) {
//      dbg << "\n\n"<<task_id_buffer[i] <<" ";
//    }
//  }
//  dbg << "\n\n\n\n" << endl; dbgBreak();
////    if(new_node_id.IsHead()) {
////      int child_heads[num_children];
////      // send child_cd_name.rank_in_level_ of heads of children CDs
////      PMPI_Group_incl(node_id().color(), num_children, child_heads, &(dynamic_cast<HeadCD*>(ptr_cd)->task_group()));
////    }
//
////  ptr_cd_->family_mailbox_
//
//

*/

bool CD::TestReqComm(bool is_all_valid)
{
  CD_DEBUG("\nCD::TestReqComm at %s / %s \nentry request req Q size : %lu\n", 
           GetCDName().GetString().c_str(), 
           GetNodeID().GetString().c_str(), 
           entry_request_req_.size());

  is_all_valid = true;
  for(auto it=entry_request_req_.begin(); it!=entry_request_req_.end(); ) {
    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    CD_DEBUG("%d ", it->second.valid_);
    if(it->second.valid_) {
      entry_request_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }
  CD_DEBUG("\nTestReqComm end\n");
  return is_all_valid;
}

bool CD::TestRecvComm(bool is_all_valid)
{
  CD_DEBUG("\nCD::TestRecvComm at %s / %s \nentry recv req Q size : %zu\n", 
           GetCDName().GetString().c_str(), 
           GetNodeID().GetString().c_str(), 
           entry_recv_req_.size());
  is_all_valid = true;
  for(auto it=entry_recv_req_.begin(); it!=entry_recv_req_.end(); ) {
    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    CD_DEBUG("%d ", it->second.valid_);
    if(it->second.valid_) {
      entry_recv_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }
  CD_DEBUG("\nTestRecvComm end\n");
  return is_all_valid;
}


bool CD::TestComm(bool test_until_done)
{
  CD_DEBUG("\nCD::TestComm at %s / %s \nentry req Q size : %lu\nentry request req Q size : %zu\nentry recv req Q size : %zu\n", 
           GetCDName().GetString().c_str(), 
           GetNodeID().GetString().c_str(), 
           entry_req_.size(),
           entry_recv_req_.size(),
           entry_request_req_.size());
  bool is_all_valid = true;
  is_all_valid = TestReqComm(is_all_valid);
  CD_DEBUG("============================");

//
//  for(auto it=entry_recv_req_.begin(); it!=entry_recv_req_.end(); ) {
//    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));
//
//    if(it->second.valid_) {
//      entry_recv_req_.erase(it++);
//    }
//    else {
//      cout << it->second.valid_ << " ";
//      is_all_valid &= it->second.valid_;
//      ++it;
//    }
//  }
//
//  for(auto it=entry_send_req_.begin(); it!=entry_send_req_.end(); ) {
//    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));
//
//    if(it->second.valid_) {
//      entry_send_req_.erase(it++);
//    }
//    else {
//      cout << it->second.valid_ << " ";
//      is_all_valid &= it->second.valid_;
//      ++it;
//    }
//  }


  for(auto it=entry_req_.begin(); it!=entry_req_.end(); ) {

    PMPI_Test(&(it->req_), &(it->valid_), &(it->stat_));

    CD_DEBUG("%d ", it->valid_);
    if(it->valid_) {
      is_all_valid &= it->valid_;
      entry_req_.erase(it++);
    }
    else {
      is_all_valid &= it->valid_;
      ++it;
    }
  }

  CD_DEBUG("\nIs all valid : %d\n==============================\n", is_all_valid);

  return is_all_valid;
}

// ------------------------- Preemption ---------------------------------------------------

CDErrT CD::CheckMailBox(void)
{

  CD::CDInternalErrT ret=kOK;
  int event_count = pendingFlag_;
//  int event_count = *pendingFlag_;
  assert(event_count <= 1024);
  // Reset handled event counter
  //handled_event_count = 0;
  assert(handled_event_count == 0);
  CDHandle *curr_cdh = GetCurrentCD();

//  CD_DEBUG("\n\n=================== Check Mail Box Start [Level #%u], # of pending events : %d ========================\n", level(), event_count);
  
//  CD_DEBUG("Label Check : %s\n", label_.c_str());
  if(event_count == 0) {
//    CD_DEBUG("No event is detected\n");

    //FIXME
//    CD_DEBUG("\n-----------------KYU--------------------------------------------------\n");
//    int temp = handled_event_count;
    InvokeErrorHandler();
//    CD_DEBUG("\nCheck MailBox is done. handled_event_count : %d --> %d, pending events : %d ", 
//             temp, handled_event_count, *pendingFlag_);
  
    DecPendingCounter();
  
//    CD_DEBUG(" --> %d\n", *pendingFlag_);
//    CD_DEBUG("-------------------------------------------------------------------\n");

  }
  else if(event_count > 0) {
    while( curr_cdh != NULL ) { // Terminate when it reaches upto root
      CD_DEBUG("\n---- Internal Check Mail [Level #%u], # of pending events : %d ----\n", curr_cdh->ptr_cd_->level(), event_count);

      if( curr_cdh->node_id_.size() > 1) {
        ret = curr_cdh->ptr_cd_->InternalCheckMailBox();
      }
      else {
        CD_DEBUG("[ReadMailBox] Searching for CD Level having non-single task. Current Level #%u\n", curr_cdh->ptr_cd()->GetCDID().level());
      }
      CD_DEBUG("\n-- level %u ------------------------------------------------------\n\n", curr_cdh->level());

      // If current CD is Root CD and GetParentCD is called, it returns NULL
      CD_DEBUG("ReadMailBox %s / %s at level #%u\n", 
               curr_cdh->ptr_cd()->GetCDName().GetString().c_str(), 
               curr_cdh->node_id_.GetString().c_str(), 
               curr_cdh->ptr_cd()->GetCDID().level());

      CD_DEBUG("Original current CD %s / %s at level #%u\n", 
               GetCDName().GetString().c_str(), 
               GetNodeID().GetString().c_str(), 
               level());

      curr_cdh = GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
    } 

    CD_DEBUG("\n-------------------------------------------------------------------");

    int temp = handled_event_count;
    InvokeErrorHandler();

    CD_DEBUG("\nCheck MailBox is done. handled_event_count : %d --> %d, pending events : %d", 
             temp, handled_event_count, pendingFlag_);
  
    DecPendingCounter();
  
    CD_DEBUG(" --> %d\n", pendingFlag_);
    CD_DEBUG("-------------------------------------------------------------------\n");

  }
  else {
    ERROR_MESSAGE("Pending event counter is less than zero. Something wrong!");
  }
  
//  CD_DEBUG("=================== Check Mail Box Done  [Level #%d] =====================================================\n\n", level());

  return static_cast<CDErrT>(ret);
}


/*
CD::CDInternalErrT CD::InternalCheckMailBox___(void)
{
  CD::CDInternalErrT ret=kOK;
  requested_event_count = *pendingFlag_;


  this->ReadMailBox();


  int event_count = *pendingFlag_;  
  if(requested_event_count != handled_event_count) {
    // There are some events not handled at this point
    ret = requested_event_count - handled_event_count;
    assert(event_count != (requested_event_count-handled_event_count));
  }
  else {
    // All events were handled
    // If every event was handled, event_count must be 0
    ret = 0;
    assert(event_count!=0);
  }

  return ret;
}
*/
CD::CDInternalErrT CD::InternalCheckMailBox(void) 
{
  CDInternalErrT ret = kOK;
  assert(event_flag_);
  CD_DEBUG("\nInternalCheckMailBox\n");
  CD_DEBUG("pending counter : %d\n", pendingFlag_);
  CD_DEBUG("\nTask : %s / %s, Error #%d\n", 
           GetCDName().GetString().c_str(), 
           GetNodeID().GetString().c_str(), 
           *event_flag_);

  // Initialize the resolved flag
  CDFlagT event = *event_flag_;

  // FIXME : possible concurrent bug. it kind of localized event_flag_ and check local variable.
  // but if somebody set event_flag, it is not updated in this event check routine.
  CDEventHandleT resolved = ReadMailBox(event);

  // Reset the event flag after handling the event
  switch(resolved) {
    case CDEventHandleT::kEventNone : 
      // no event
      break;
    case CDEventHandleT::kEventResolved :
      // There was an event, but resolved

      break;
    case CDEventHandleT::kEventPending :
      // There was an event, but could not resolved.
      // Escalation required
      break;
    default :
      break;
  }

  CD_DEBUG("\tResolved? : %d\n", resolved);
  CD_DEBUG("cd event queue size : %lu, handled event count : %d\n", 
           cd_event_.size(), handled_event_count);

  return ret;
}

CD::CDInternalErrT HeadCD::InternalCheckMailBox(void) 
{
  CDEventHandleT resolved = CDEventHandleT::kEventNone;
  CDInternalErrT ret = kOK;
  CDFlagT *event = event_flag_;
  assert(event_flag_);

  CD_DEBUG("\n\nInternalCheckMailBox CHECK IT OUT HEAD\n");
  CD_DEBUG("pending counter : %d\n", pendingFlag_);

  for(int i=0; i<task_size(); i++) {
    CD_DEBUG("\nTask[%d] (%s / %s), Error #%d\n", 
             i, GetCDName().GetString().c_str(), GetNodeID().GetString().c_str(), event[i]);

    // Initialize the resolved flag
    resolved = kEventNone;
    resolved = ReadMailBox(&(event[i]), i);

    // Reset the event flag after handling the event
    switch(resolved) {
      case CDEventHandleT::kEventNone : 
        // no event
        break;
      case CDEventHandleT::kEventResolved :
        // There was an event, but resolved

        break;
      case CDEventHandleT::kEventPending :
        // There was an event, but could not resolved.
        // Escalation required
        break;
      default :
        break;
    }
  }
  CD_DEBUG("\tResolved? : %d\n", resolved);
  CD_DEBUG("cd event queue size : %lu, handled event count : %d\n", 
           cd_event_.size(), handled_event_count);

  return ret;
}

CDEventHandleT CD::ReadMailBox(CDFlagT &event)
{
  CD_DEBUG("\nCD::ReadMailBox(%s)\n", event2str(event).c_str());

  CDEventHandleT ret = CDEventHandleT::kEventResolved;

  if( CHECK_NO_EVENT(event) ) {
    CD_DEBUG("CD Event kNoEvent\t\t\t");
  }
  else {
// Events requested from head to non-head.
// Current task is non-head,
// so, kEntrySearch | kErrorOccurred does not make sense here,
// because non-head task does not have an ability to deal with that.
// kAllResume is also weird here. Imagine that it is just "Resume" when you open the mailbox.
// I am currently "resumed" (my current forward execution), so it does not make sense.

    if( CHECK_EVENT(event, kEntrySearch) || CHECK_EVENT(event, kErrorOccurred) ) {
      assert(0);
    }

    if( CHECK_EVENT(event, kEntrySend) ) {

//#if _FIX
//      HandleEntrySend();
      CD_DEBUG("ENTRY SEND\n");
      cd_event_.push_back(new HandleEntrySend(this));
//  dbg << "CD Event kEntrySend\t\t\t";
//  if(IsHead()) {
//    assert(0);//event_flag_[idx] &= ~kEntrySend;
//  }
//  else {
//    *(event_flag_) &= ~kEntrySend;
//  }
//  IncHandledEventCounter();
//#endif
      CD_DEBUG("CD Event kEntrySend\t\t\t");
    }

    if( CHECK_EVENT(event, kAllPause) ) {
      CD_DEBUG("What?? kAllPause %s\n", GetNodeID().GetString().c_str());
      cd_event_.push_back(new HandleAllPause(this));
    }


    if( CHECK_EVENT(event, kAllReexecute) ) {
      CD_DEBUG("ReadMailBox. push back HandleAllReexecute\n");
      cd_event_.push_back(new HandleAllReexecute(this));
    }

  } 
  return ret;
}


CDEventHandleT HeadCD::ReadMailBox(CDFlagT *p_event, int idx)
{
  CD_DEBUG("CDEventHandleT HeadCD::HandleEvent(%s, %d)\n", event2str(*p_event).c_str(), idx);
  CDEventHandleT ret = CDEventHandleT::kEventResolved;
//  int handle_ret = 0;
  if(idx == head()) {
    if(head() != task_in_color()) 
      assert(0);
    CD_DEBUG("event in head : %d\n", *p_event);
  }

  CDFlagT event = *p_event;
  if( CHECK_NO_EVENT(event) ) {
    CD_DEBUG("CD Event kNoEvent\t\t\t");
  }
  else {
    if( CHECK_EVENT(event, kErrorOccurred) ) {
      CD_DEBUG("Error Occurred at task ID : %d at level #%u\n", idx, level());

      if(error_occurred == true) {
        if(task_size() == 1) 
          assert(0);

        CD_DEBUG("Event kErrorOccurred %d", idx);
        event_flag_[idx] &= ~kErrorOccurred;
        IncHandledEventCounter();
      }
      else {
        if(task_size() == 1) 
          assert(0);

        CD_DEBUG("Event kErrorOccurred %d (Initiator)", idx);
        cd_event_.push_back(new HandleErrorOccurred(this, idx));
        error_occurred = true;
      }
    }

    if( CHECK_EVENT(event, kEntrySearch) ) {

//#if _FIX
      CD_DEBUG("ENTRY SEARCH\n");
//      ENTRY_TAG_T recvBuf[2] = {0,0};
//      bool entry_searched = HandleEntrySearch(idx, recvBuf);
      cd_event_.push_back(new HandleEntrySearch(this, idx));



//    event_flag_[idx] &= ~kEntrySearch;
//    IncHandledEventCounter();
////FIXME_KL
//      if(entry_searched == false) {
//        CDHandle *parent = GetParentCD();
//        CDEventT entry_search = kEntrySearch;
//        parent->SetMailBox(entry_search);
//        PMPI_Isend(recvBuf, 
//                  2, 
//                  MPI_UNSIGNED_LONG_LONG,
//                  parent->node_id().head(), 
//                  parent->ptr_cd()->GetCDID().GenMsgTag(MSG_TAG_ENTRY_TAG), 
//                  parent->node_id().color(),
//                  &(parent->ptr_cd()->entry_request_req_[recvBuf[0]].req_));
//      }
//      else {
//        // Search Entry
////        IncHandledEventCounter();
//      }
//#endif
    }

    if( CHECK_EVENT(event, kAllResume) ) {
      CD_DEBUG("Head ReadMailBox. kAllResume\n");
      cd_event_.push_back(new HandleAllResume(this));


    }
    if( CHECK_EVENT(event, kAllReexecute) ) {
      CD_DEBUG("Handle All Reexecute at %s / %s\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
      cd_event_.push_back(new HandleAllReexecute(this));
    }
// -------------------------------------------------------------------------
// Events requested from non-head to head.
// If current task is head, it means this events is requested from head to head, 
// which does not make sense. (perhaps something like loopback.)
// Currently, I think head can just ignore for checking kEntrySend/kAllPause/kAllReexecute, 
// but I will leave if for now.
    if( CHECK_EVENT(event, kEntrySend) || CHECK_EVENT(event, kAllPause) ) {
      assert(0);
    }

#if 0 
    if( CHECK_EVENT(event, kEntrySend) ) {

      if(!IsHead()) {
        cout << "[event: entry send] This is event for non-head CD.";
//        assert(0);
      }
//      HandleEntrySend();
      cd_event_.push_back(new HandleEntrySend(this));
      dbg << "CD Event kEntrySend\t\t\t";
      
    }

    if( CHECK_EVENT(event, kAllPause) ) {
      cout << "What?? " << GetNodeID() << endl;
      HandleAllPause();


    }

    if( CHECK_EVENT(event, kAllReexecute) ) {

      HandleAllReexecute();


    }
#endif

  } // ReadMailBox ends 
  return ret;
}









#if 1
CDErrT CD::SetMailBox(const CDEventT &event)
{
  CD_DEBUG("\n\n=================== Set Mail Box Start ==========================\n");
  CD_DEBUG("\n[CD::SetMailBox] myTaskID #%d, event : %s, Is it Head? : %d\n", myTaskID, event2str(event).c_str(), IsHead());
  //printf("\n[CD::SetMailBox] myTaskID #%d, event : %s, Is it Head? : %d\n", myTaskID, event2str(event).c_str(), IsHead());

  CD::CDInternalErrT ret=kOK;
//  int val = 1;

  CDHandle *curr_cdh = CDPath::GetCDLevel(level());
//  int head_id = head();
  if(event == CDEventT::kNoEvent) {

    CD_DEBUG("\n=================== Set Mail Box Done ==========================\n");

    return static_cast<CDErrT>(ret);  
  }
//  if(task_size() == 1) {
    // This is the leaf that has just single task in a CD
    CD_DEBUG("[SetMailBox Leaf and have 1 task] size is %u. Check mailbox at the upper level.\n", task_size());

    while( curr_cdh->node_id_.size()==1 ) {
      if(curr_cdh == GetRootCD()) {
        ERROR_MESSAGE("[SetMailBox] there is a single task in the root CD\n");
      }
      CD_DEBUG("[SetMailBox|Searching for CD Level having non-single task] Current Level : %u\n", curr_cdh->ptr_cd()->GetCDID().level());

      // If current CD is Root CD and GetParentCD is called, it returns NULL
      curr_cdh = GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
    }
 
    CD_DEBUG("[SetMailBox] %s / %s at level #%u\n", 
             curr_cdh->ptr_cd()->GetCDName().GetString().c_str(), 
             curr_cdh->node_id_.GetString().c_str(), 
             curr_cdh->ptr_cd()->GetCDID().level());
    CD_DEBUG("Original current CD %s / %s at level #%u\n", 
             GetCDName().GetString().c_str(), 
             GetNodeID().GetString().c_str(), 
             level());

//    head_id = curr_cdh->node_id().head();
//  }

  if(curr_cdh->IsHead()) 
    assert(0);

  CD_DEBUG("Before RemoteSetMailBox\n");

//  curr_cdh->ptr_cd()
  ret = RemoteSetMailBox(curr_cdh->ptr_cd(), event);
  
/*
  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, curr_cdh->ptr_cd()->pendingWindow_);
  if(event != CDEventT::kNoEvent) {
    dbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
    // Increment pending request count at the target task (requestee)
    PMPI_Accumulate(&val, 1, MPI_INT, 
                    head_id, 0, 1, MPI_INT, 
                    MPI_SUM, curr_cdh->ptr_cd()->pendingWindow_);
    dbg << "MPI Accumulate done for tast #"<< head_id <<" (head)"<< endl; //getchar();
  }
  PMPI_Win_unlock(head_id, curr_cdh->ptr_cd()->pendingWindow_);

  CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_;



  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, mailbox);

  // Inform the type of event to be requested
  dbg << "Set event start" << endl; //getchar();
  if(event != CDEventT::kNoEvent) {
    PMPI_Accumulate(&event, 1, MPI_INT, 
                    head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, 
                    MPI_BOR, mailbox);
  }
  dbg << "MPI Accumulate done for task #"<< head_id <<"(head)"<< endl; //getchar();

  PMPI_Win_unlock(head_id, mailbox);
*/

  CD_DEBUG("\n=================== Set Mail Box Done ==========================\n");
  return static_cast<CDErrT>(ret);
}

CD::CDInternalErrT CD::RemoteSetMailBox(CD *curr_cd, const CDEventT &event)
{
  CD_DEBUG("[CD::RemoteSetMailBox] event : %s at %s / %s\n", 
           event2str(event).c_str(), 
           curr_cd->GetCDName().GetString().c_str(), 
           curr_cd->GetNodeID().GetString().c_str());

  CDInternalErrT ret=kOK;

  int head_id = curr_cd->head();
  int val = 1;

  if(event != CDEventT::kNoEvent) {
    PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, curr_cd->pendingWindow_);

    CD_DEBUG("[%s] Set CD Event %s at level #%u. CD Name %s (%s)\n", __func__, event2str(event).c_str(), curr_cd->level(), curr_cd->GetCDName().GetString().c_str(), name_.c_str());

    // Increment pending request count at the target task (requestee)
    PMPI_Accumulate(&val, 1, MPI_INT, 
                    head_id, 0, 1, MPI_INT, 
                    MPI_SUM, curr_cd->pendingWindow_);
    CD_DEBUG("MPI Accumulate (Increment pending counter) done for task #%u (head)\n", head_id);
  
    PMPI_Win_unlock(head_id, curr_cd->pendingWindow_);
  
    CDMailBoxT &mailbox = curr_cd->mailbox_;

    PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, mailbox);
  
    // Inform the type of event to be requested
    PMPI_Accumulate((void *)&event, 1, MPI_INT, 
                    head_id, curr_cd->task_in_color(), 1, MPI_INT, 
                    MPI_BOR, mailbox);
    CD_DEBUG("MPI Accumulate (Set event flag) done for task #%u (head)\n", head_id);

    PMPI_Win_unlock(head_id, mailbox);
  }
  CD_DEBUG("MPI Accumulate done for task #%u (head)\n", head_id);
  CD_DEBUG("[CD::RemoteSetMailBox] Done.\n");

  return ret;
}


CDErrT HeadCD::SetMailBox(const CDEventT &event, int task_id)
{
  CD_DEBUG("\n\n=================== Set Mail Box (%s, %d) Start! myTaskID #%d at %s level #%u ==========================\n", event2str(event).c_str(), task_id, myTaskID, name_.c_str(), level());
 
  if(event == CDEventT::kErrorOccurred || event == CDEventT::kEntrySearch) {
    ERROR_MESSAGE("[HeadCD::SetMailBox(event, task_id)] Error, the event argument is something wrong. event: %s\n", event2str(event).c_str());
  }

  if(task_size() == 1) {
    ERROR_MESSAGE("[HeadCD::SetMailBox(event, task_id)] Error, the # of task should be greater than 1 at this routine. # of tasks in current CD task size: %d\n", task_size());
  }

  CDErrT ret=CDErrT::kOK;
  int val = 1;
  if(event == CDEventT::kNoEvent) {
    // There is no vent to set.
    CD_DEBUG("No event to set\n");
  }
  else {
    if(task_id != task_in_color()) {
 
      // Increment pending request count at the target task (requestee)
      if(event != CDEventT::kNoEvent) {
        PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_id, 0, pendingWindow_);
        CD_DEBUG("Set CD Event %s at level #%u. CD Name : %s\n", event2str(event).c_str(), level(), GetCDName().GetString().c_str());
        CD_DEBUG("Accumulate event at %d\n", task_id);
  
        PMPI_Accumulate(&val, 1, MPI_INT, 
                       task_id, 0, 1, MPI_INT, 
                       MPI_SUM, pendingWindow_);
        CD_DEBUG("PMPI_Accumulate done for task #%d\n", task_id);

        PMPI_Win_unlock(task_id, pendingWindow_);
    
        CD_DEBUG("Finished to increment the pending counter at task #%d\n", task_id);
    
        if(task_id == task_in_color()) { 
          CD_DEBUG("after accumulate --> pending counter : %d\n", pendingFlag_);
        }
        
        // Inform the type of event to be requested
        PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_id, 0, mailbox_);

        CD_DEBUG("Set event start\n");

        PMPI_Accumulate((void *)&event, 1, MPI_INT, 
                       task_id, 0, 1, MPI_INT, 
                       MPI_BOR, mailbox_);
        PMPI_Win_unlock(task_id, mailbox_);
      }

      CD_DEBUG("PMPI_Accumulate done for task #%d\n", task_id);
  
      if(task_id == task_in_color()) { 
        CD_DEBUG("after accumulate --> event : %s\n", event2str(event_flag_[task_id]).c_str());
      }
    
    
    }
    else {
      // If the task to set event is the head itself,
      // it does not increase pending counter and set event flag through RDMA,
      // but it locally increase the counter and directly register event handler.
      // The reason for this is that the head task's CheckMailBox is scheduled before non-head task's,
      // and after head's CheckMailBox associated with SetMailBox for some events such as kErrorOccurred,
      // (kErrorOccurred at head tast associates kAllReexecute for all task in the CD currently)
      // head CD does not check mail box to invoke kAllRexecute for itself. 
      // Therefore, it locally register the event handler right away, 
      // and all the tasks including head task can reexecute after the CheckMailBox.
      CD_DEBUG("\nEven though it calls SetMailBox(%s, %d == %u), locally set/handle events\n", 
               event2str(event).c_str(), task_id, task_in_color());

      CDInternalErrT cd_ret = LocalSetMailBox(this, event);
      ret = static_cast<CDErrT>(cd_ret);
    }
  }

  CD_DEBUG("\n=================== Set Mail Box Done ==========================\n");

  return ret;
}


CDErrT HeadCD::SetMailBox(const CDEventT &event)
{
  // If the task to set event is the head itself,
  // it does not increase pending counter and set event flag through RDMA,
  // but it locally increase the counter and directly register event handler.
  // The reason for this is that the head task's CheckMailBox is scheduled before non-head task's,
  // and after head's CheckMailBox associated with SetMailBox for some events such as kErrorOccurred,
  // (kErrorOccurred at head tast associates kAllReexecute for all task in the CD currently)
  // head CD does not check mail box to invoke kAllRexecute for itself. 
  // Therefore, it locally register the event handler right away, 
  // and all the tasks including head task can reexecute after the CheckMailBox.
  CD_DEBUG("\n[HeadCD::SetMailBox] event %s at level #%u --------\n", event2str(event).c_str(), level());
  //printf("\n[HeadCD::SetMailBox] event %s at level #%u --------\n", event2str(event).c_str(), level());

  CDInternalErrT ret = kOK;

  CDHandle *curr_cdh = CDPath::GetCDLevel(level());
  while( curr_cdh->task_size() == 1 ) {
    if(curr_cdh == GetRootCD()) {
      ERROR_MESSAGE("[SetMailBox] there is a single task in the root CD\n");
    }
    // If current CD is Root CD and GetParentCD is called, it returns NULL
    curr_cdh = GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
  } 
//  if(curr_cdh->IsHead()) assert(0);


  if(curr_cdh->IsHead()) {
    ret = LocalSetMailBox(static_cast<HeadCD *>(curr_cdh->ptr_cd()), event);
  }
  else {
    ret = RemoteSetMailBox(curr_cdh->ptr_cd(), event);
  }

/*
  CDErrT ret=CDErrT::kOK;
  if(event != kNoEvent) {
    if(curr_cdh->task_in_color() != head()) assert(0);
  
    if(event == kAllResume || event == kAllPause || event == kEntrySearch || event == kEntrySend) { 
      assert(0);
    }
//  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_in_color(), 0, pendingWindow_);
    (*pendingFlag_)++;
//  PMPI_Win_unlock(task_in_color(), pendingWindow_);
    if( CHECK_EVENT(event, kErrorOccurred) ) {
        dbg << "kErrorOccurred in HeadCD::SetMailBox" << endl; //getchar();
        curr_cdh->ptr_cd()->cd_event_.push_back(new HandleErrorOccurred(static_cast<HeadCD *>(curr_cdh->ptr_cd())));
//        curr_cdh->ptr_cd()->cd_event_.push_back(new HandleAllReexecute(this));
        // it is not needed to register here. I will be registered by HandleErrorOccurred functor.
    }
    if( CHECK_EVENT(event, kAllReexecute) ) {
        dbg << "kAllRexecute in HeadCD::SetMailBox" << endl; //getchar();
        assert(curr_cdh->ptr_cd() == this);
        curr_cdh->ptr_cd()->cd_event_.push_back(new HandleAllReexecute(curr_cdh->ptr_cd()));
    }
  
  }
*/
  CD_DEBUG("\n------------------------------------------------------------\n");

  return static_cast<CDErrT>(ret);
}

CD::CDInternalErrT HeadCD::LocalSetMailBox(HeadCD *curr_cd, const CDEventT &event)
{
  CD_DEBUG("HeadCD::LocalSetMailBox Event : %s at %s level #%u\n", event2str(event).c_str(), name_.c_str(), level());
  CDInternalErrT ret=kOK;

  if(event != kNoEvent) {
    if(curr_cd->task_in_color() != head()) 
      assert(0);
  
    if(event == kAllResume || event == kAllPause  || event == kEntrySearch ) {//|| event == kEntrySend) {
      ERROR_MESSAGE("HeadCD::LocalSetMailBox -> Event: %s\n", event2str(event).c_str());
    }

    IncPendingCounter();

    if( CHECK_EVENT(event, kErrorOccurred) ) {
        CD_DEBUG("kErrorOccurred in HeadCD::SetMailBox\n");

        curr_cd->cd_event_.push_back(new HandleErrorOccurred(curr_cd));
        //FIXME
//        EventHandler *err_handler = new HandleErrorOccurred(curr_cd);
//        err_handler->HandleEvent();
//        IncHandledEventCounter();
//        delete err_handler;
        error_occurred = true;
        // it is not needed to register here. I will be registered by HandleErrorOccurred functor.
    }
    if( CHECK_EVENT(event, kAllReexecute) ) {
        CD_DEBUG("kAllRexecute in HeadCD::SetMailBox\n");
        assert(curr_cd == this);

        curr_cd->cd_event_.push_back(new HandleAllReexecute(curr_cd));
    }
    if( CHECK_EVENT(event, kEntrySend) ) {
        CD_DEBUG("kEntrySend in HeadCD::SetMailBox\n");
        assert(curr_cd == this);
        assert(0);
        curr_cd->cd_event_.push_back(new HandleEntrySend(curr_cd));
    }
  }
  return ret;
}

#else
/*
CDErrT CD::SetMailBox(CDEventT &event)
{
  dbg << "\n\n=================== Set Mail Box Start ==========================\n" << endl;
  dbg << "\nSetMailBox " << myTaskID << ", event : " << event << ", Is it Head? "<< IsHead() <<endl;
  CD::CDInternalErrT ret=kOK;
  int val = 1;

  if(task_size() > 1) {
    dbg << "KL : [SetMailBox] size is " << task_size() << ". Check mailbox at the upper level." << endl;
  
    PMPI_Win_fence(0, pendingWindow_);
    if(event != CDEventT::kNoEvent) {
//      PMPI_Win_fence(0, pendingWindow_);

      dbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
      // Increment pending request count at the target task (requestee)
      PMPI_Accumulate(&val, 1, MPI_INT, 
                     head(), 0, 1, MPI_INT, 
                     MPI_SUM, pendingWindow_);
      dbg << "MPI Accumulate done" << endl; //getchar();

//      PMPI_Win_fence(0, pendingWindow_);
    }
    PMPI_Win_fence(0, pendingWindow_);

    CDMailBoxT &mailbox = mailbox_;  

    PMPI_Win_fence(0, mailbox);
    if(event != CDEventT::kNoEvent) {
      // Inform the type of event to be requested
      PMPI_Accumulate(&event, 1, MPI_INT, 
                     head(), task_in_color(), 1, MPI_INT, 
                     MPI_BOR, mailbox);

//    PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
      dbg << "PMPI_Accumulate done" << endl; //getchar();
    }
  
//    switch(event) {
//      case CDEventT::kNoEvent :
//        dbg << "Set CD Event kNoEvent" << endl;
//        break;
//      case CDEventT::kErrorOccurred :
//        // Inform the type of event to be requested
//        PMPI_Accumulate(&event, 1, MPI_INT, node_id_.head(), node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
////        PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//        dbg << "PMPI_Put done" << endl; getchar();
//        break;
//      case CDEventT::kAllPause :
//        dbg << "Set CD Event kAllPause" << endl;
//  
//        break;
//      case CDEventT::kAllResume :
//  
//        break;
//      case CDEventT::kEntrySearch :
//  
//        break;
//      case CDEventT::kEntrySend :
//  
//        break;
//      case CDEventT::kAllReexecute :
//  
//        break;
//      default:
//  
//        break;
//    }
  
    PMPI_Win_fence(0, mailbox);
  
  }
  else {
    // This is the leaf that has just single task in a CD
    dbg << "KL : [SetMailBox Leaf and have 1 task] size is " << task_size() << ". Check mailbox at the upper level." << endl;
    CDHandle *curr_cdh = GetCurrentCD();

    while( curr_cdh->node_id_.size()==1 ) {
      if(curr_cdh == GetRootCD()) {
        dbg << "[SetMailBox] there is a single task in the root CD" << endl;
        assert(0);
      }
      dbg << "[SetMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
      // If current CD is Root CD and GetParentCD is called, it returns NULL
      curr_cdh = GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
    } 
    dbg << "[SetMailBox] " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
        << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;


    int parent_head_id = curr_cdh->node_id().head();

    PMPI_Win_lock(PMPI_LOCK_EXCLUSIVE, parent_head_id, 0, curr_cdh->ptr_cd()->pendingWindow_);
    if(event != CDEventT::kNoEvent) {
      dbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
      // Increment pending request count at the target task (requestee)
      PMPI_Accumulate(&val, 1, MPI_INT, parent_head_id, 
                     0, 1, MPI_INT, 
                     MPI_SUM, curr_cdh->ptr_cd()->pendingWindow_);
      dbg << "MPI Accumulate done" << endl; //getchar();
    }
    PMPI_Win_unlock(parent_head_id, curr_cdh->ptr_cd()->pendingWindow_);

    CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_;

    PMPI_Win_lock(PMPI_LOCK_EXCLUSIVE, parent_head_id, 0, mailbox);
    // Inform the type of event to be requested
    dbg << "Set event start" << endl; //getchar();
    if(event != CDEventT::kNoEvent) {
      PMPI_Accumulate(&event, 1, MPI_INT, parent_head_id, 
                     curr_cdh->node_id_.task_in_color(), 1, MPI_INT, 
                     MPI_BOR, mailbox);
//    PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
    }
    dbg << "PMPI_Accumulate done" << endl; //getchar();

//    switch(event) {
//      case CDEventT::kNoEvent :
//        dbg << "Set CD Event kNoEvent" << endl;
//        break;
//      case CDEventT::kErrorOccurred :
//        // Inform the type of event to be requested
//        dbg << "Set event start" << endl; getchar();
//        PMPI_Accumulate(&event, 1, MPI_INT, parent_head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
////        PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//        dbg << "PMPI_Accumulate done" << endl; getchar();
//        break;
//      case CDEventT::kAllPause :
//        dbg << "Set CD Event kAllPause" << endl;
//  
//        break;
//      case CDEventT::kAllResume :
//  
//        break;
//      case CDEventT::kEntrySearch :
//  
//        break;
//      case CDEventT::kEntrySend :
//  
//        break;
//      case CDEventT::kAllReexecute :
//  
//        break;
//      default:
//  
//        break;
//    }
    PMPI_Win_unlock(parent_head_id, mailbox);
  }
  dbg << "\n================================================================\n" << endl;

  return static_cast<CDErrT>(ret);
}
*/
#endif



void CD::DecPendingCounter(void)
{
  CD *rootcd = CDPath::GetRootCD()->ptr_cd();
  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, rootcd->task_in_color(), 0, rootcd->pendingWindow_);
  (pendingFlag_) -= handled_event_count;
  PMPI_Win_unlock(rootcd->task_in_color(), rootcd->pendingWindow_);
  // Initialize handled_event_count;
  handled_event_count = 0;
}

void CD::IncPendingCounter(void)
{
  CD *rootcd = CDPath::GetRootCD()->ptr_cd();
  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, rootcd->task_in_color(), 0, rootcd->pendingWindow_);
  (pendingFlag_) += 1;
  PMPI_Win_unlock(rootcd->task_in_color(), rootcd->pendingWindow_);
}



int CD::BlockUntilValid(MPI_Request *request, MPI_Status *status) {
  int flag = 0, ret = 0;
  while(1) {
    ret = PMPI_Test(request, &flag, status);
    if(flag != 0) 
      return ret;
    else {
      assert(need_reexec == false); // should be false at this point.
      CheckMailBox();
      if(need_reexec) { // This could be set inside CD::CheckMailBox()
        CD_DEBUG("\n[%s] Reexec is true, %u->%u, %s %s\n\n", 
            __func__, level(), reexec_level, label_.c_str(), cd_id_.node_id_.GetString().c_str());

//        GetCDToRecover(GetCurrentCD(), false)->ptr_cd()->Recover();
        break;
      } else {
        CD_DEBUG("[%s] Reexec is false, %u->%u, %s %s\n", 
            __func__, level(), reexec_level, label_.c_str(), cd_id_.node_id_.GetString().c_str());
      }
    }
  }
  GetCDToRecover(GetCurrentCD(), false)->ptr_cd()->Recover();
  return 0;
}



// TODO
bool CD::CheckIntraCDMsg(int target_id, MPI_Group &target_group)
{
  int global_rank_id = -1;
  int local_rank_id = -1;
  return false;
//  printf("target_id %d, group %p\n", target_id, &target_group);
  // Translate user group's rank ID to MPI_COMM_WORLD
  int status = MPI_Group_translate_ranks(target_group, 1, &target_id, cd::whole_group, &global_rank_id);
  if(status != MPI_SUCCESS) {
    // error
  }
  int size = task_size();
  int group_ranks[size];
  int result_ranks[size];
  for(int i=0; i<size; i++) {
    group_ranks[i] = i;
  }

//  MPI_Group g = group();
//  if(g == 0) {
//    MPI_Comm_group(color(), &g);
//  }
//  assert(g);
//  cout << "group : " << group() << " group rank : " << group_ranks[size-1] << " whole group : " << cd::whole_group << endl;

  // Translate task IDs of CD to MPI_COMM_WORLD    
  status = MPI_Group_translate_ranks(group(), size, group_ranks, cd::whole_group, result_ranks);

//  printf("\n\nRank #%d ----------------------------\n", myTaskID); 
//  for(int i=0; i<size; i++) {
//    printf("%d->%d\n", group_ranks[i], result_ranks[i]);
//  }
//  printf("\n\n"); 

  bool found = false;
  for(int i=0; i<size; i++) {
    if(result_ranks[i] == global_rank_id) { 
      found = true; 
      break; 
    }
  }

//  assert(g);
//  status = MPI_Group_translate_ranks(cd::whole_group, 1, &global_rank_id, group(), &local_rank_id);
//  if(status != MPI_SUCCESS) {
//    // this case the task is not in the current group
//    // it should return false
//  }

  //printf("Translate rank_id = %d->%d->%d at %s, found? %d\n\n", 
  //    target_id, global_rank_id, local_rank_id, GetCDID().GetString().c_str(), found);
//  if(target_id != local_rank_id) //getchar();
  return found;
}

//bool CD::CheckSubGroup(MPI_Comm &target)
//{
//  bool subset = false;
//  int target_size;
//  MPI_Comm_size(target, &target_size);
//  MPI_Comm_size(target, &target_size);
//  int target_ranks[target_size];
//  int ranks_from_target[target_size];
//  int ranks_from_cd[size()];
//  // Gather IDs
//  if( target_size <= size() ) {
//    MPI_Gather()
//    status = MPI_Group_translate_ranks(target_group, target_size, target_ranks, whole_group, ranks_from_target);
//    status = MPI_Group_translate_ranks(group(), size(), group_ranks, whole_group, ranks_from_cd);
//
//    // compare ranks_from_cd and ranks_from_target
//
//  } // else subset is false
//  
//  return subset;
//}

#endif


