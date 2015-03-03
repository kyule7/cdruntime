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

#if _MPI_VER
#if _KL

#include "cd_handle.h"
#include "cd.h"
#include "cd_global.h"
using namespace cd;
using namespace std;


//  if(IsHead()) {
//    // Create window to get head info of children CDs.
//    MPI_Win_create();
//  }
//  else {
//    MPI_Win_create();
//  }

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

  MPI_Allgather(send_buf, 2, MPI_INTEGER, 
                recv_buf, 2, MPI_INTEGER, 
                node_id().color());

  dbg << "\n==================Remote entry check ===================\n" << endl;
  for(int k=0; k<task_count; ++k ) {
    dbg <<recv_buf[k][0] << " " << recv_buf[k][1] << endl;
  }
  dbg << "\n" << endl; dbgBreak();

//  uint32_t node_id_len=0;
//  dbg << "=========== Check Ser ==============" << endl;
//  dbg <<"[Before packed] :\t"<< node_id_ << endl << endl;
//  void *ser_node_id = node_id_.Serialize(node_id_len);
//  NodeID unpacked_node_id;
//  unpacked_node_id.Deserialize(ser_node_id);
//  dbg <<"[After unpacked] :\t"<< unpacked_node_id << ", len : "<< node_id_len<< endl << endl;

//  for(std::list<CDEntry*>::iterator it = entry_directory_.begin();
//  
//  for(int i=0; i<send_buf[1]; i++) {
//    void *serialized_entry = Serialize(entry_len);
//  }

  dbg << "[Before] Check entries in remote entry directory" << endl;
  for(auto it = ptr_cd()->remote_entry_directory_map_.begin();
           it!= ptr_cd()->remote_entry_directory_map_.end(); ++it) {
    dbg << *(it->second) << endl;

  }
  uint32_t serialized_len_in_bytes=0;
//  std::map<uint64_t, CDEntry*> remote_entry_dir;

  void *serialized_entry = ptr_cd()->SerializeRemoteEntryDir(serialized_len_in_bytes); 
  // if there is no entries to send to head, just return
  if(serialized_entry == NULL) return;
  
//  int entry_count=0;
  int recv_counts[task_count];
  int displs[task_count];
//  int stride = 1196;
  for(int k=0; k<task_count; k++ ) {
//    displs[k] = k*stride;
    displs[k] = k*serialized_len_in_bytes;
    recv_counts[k] = serialized_len_in_bytes;
  }
//  dbg << "# of entries to gather : " << entry_count << endl;
//  CDEntry *entry_to_deserialize = new CDEntry[entry_count*2];
//  MPI_Gatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().head(), node_id().color());

  int recv_count = task_count*serialized_len_in_bytes;
//  int recv_count = task_count*stride;
  void *entry_to_deserialize = NULL;
//  int alloc_err=0;
//  if(IsHead())  {
//    alloc_err= 
  MPI_Alloc_mem(recv_count, MPI_INFO_NULL, &entry_to_deserialize);
//  }
  MPI_Datatype sType;
  MPI_Type_contiguous(serialized_len_in_bytes, MPI_BYTE, &sType);
  MPI_Type_commit(&sType);

//  MPI_Datatype rType;
//  MPI_Type_contiguous(recv_count, MPI_BYTE, &rType);
////  MPI_Type_create_struct(task_count, serialized_len_in_bytes, 0, MPI_BYTE, &rType);
////  MPI_Type_vector(task_count, serialized_len_in_bytes, serialized_len_in_bytes, MPI_BYTE, &rType);
//  MPI_Type_commit(&rType);


  dbg << "\n\nNote : " << serialized_entry << ", " << serialized_len_in_bytes << ", " << recv_count << ", remote entry dir map size : " << ptr_cd()->remote_entry_directory_map_.size() << "======= "<< endl << endl;
//  for(auto it = ptr_cd()->remote_entry_directory_map_.begin(); it!=ptr_cd()->remote_entry_directory_map_.end(); ++it) {
//    dbg << *(it->second) << endl;
//  }
//  char *rbuf = new char[sizeof(int)*task_count*2];

//  MPI_Gather(&node_id_.task_in_color_, 1, MPI_INT,
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



//  MPI_Gather(serialized_entry, 1, sType,
//             entry_to_deserialize, 1, sType,
//             node_id().head(), node_id().color());
  dbg << "Wait!! " << node_id().size() << endl;
  for(int i=0; i<task_count; i++) {
    dbg << "recv_counts["<<i<<"] : " << recv_counts[i] << endl;
    dbg << "displs["<<i<<"] : " << displs[i] << endl;
  }
//  MPI_Allgatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().color());

  int test_counts[task_count];
  int test_displs[task_count];
  for(int k=0; k<task_count; k++ ) {
    test_displs[k] = k*4;
    test_counts[k] = 4;
  }
  dbg << "Wait!! " << node_id().size() << endl;
  for(int i=0; i<task_count; i++) {
    dbg << "test_counts["<<i<<"] : " << test_counts[i] << endl;
  }
  for(int i=0; i<task_count; i++) {
    dbg << "test_displs["<<i<<"] : " << test_displs[i] << endl;
  }
  int test_a[4];
  int test_b[task_count*4];
//  int test_b[task_count][4];
  test_a[0] = node_id().task_in_color();
  test_a[1] = test_a[0]+10;
  test_a[2] = test_a[0]+100;
  test_a[3] = test_a[0]+1000;

  MPI_Gatherv(test_a, 4, MPI_INT,
              test_b, test_counts, test_displs, MPI_INT,
              node_id().head(), node_id().color());

//  MPI_Gatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().head(), node_id().color());
//  MPI_Allgatherv(serialized_entry, serialized_len_in_bytes, MPI_BYTE,
//              entry_to_deserialize, recv_counts, displs, MPI_BYTE,
//              node_id().head(), node_id().color());

  MPI_Gather(serialized_entry, 1, sType,
             entry_to_deserialize, 1, sType,
             node_id_.head(), node_id_.color());

  if(IsHead()) {
    
    for(int i=0; i<task_count; i++) {
      dbg<<endl;
      for(int j=0; j<4; j++) {
        dbg << "test_b["<<4*i+j << "] : " << *(test_b+4*i+j) << endl;
//        dbg << "test_b["<<i<<"]["<<j << "] : " << test_b[i][j] << endl;
      }
    }

    int * test_des = (int *)entry_to_deserialize;
    char * test_des_char = (char *)entry_to_deserialize;
    int test_count = serialized_len_in_bytes/4;
//    int abc = serialized_len_in_bytes;
    int aaa=1;
    dbg << "Check it out----------------------------------"<< test_des << " " << test_des+aaa << "--" << (void *)test_des_char << " " <<(void *)(test_des_char+aaa) << endl;
    int tk=0;
    int tj=0;
    for(int j=0; j<task_count; j++){
//      dbg << test_des+tk << "-- " << (void *)(test_des_char + tj)<< ", j: "<< j<< endl;
//      for(int i=0; i<test_count;i++) {
//        dbg << *(test_des+i+j*test_count) << " ";
//        tk = i + j*test_count;
//        tj = 4*i + j*serialized_len_in_bytes;
//      }
        tk = j*test_count;
        tj = j*serialized_len_in_bytes;
        dbg << *(test_des+tk) << endl;
        dbg << *(test_des_char+tj) << endl;
        cout << test_des+tk << "-- " << (void *)(test_des_char + tj)<< ", j: "<< j<< endl;
      dbg << endl;
    }
    dbg << endl;
//    cout << "1" << endl;
//    for(int i=0; i<task_count; i++) cout << " --- " << ((int *)rbuf)[i];
//    cout << endl;
//    getchar();
//    ptr_cd()->DeserializeRemoteEntryDir(remote_entry_dir, serialized_entry);

    dbg << "===" << serialized_len_in_bytes << endl; 
    void * temp_ptr = (void *)((char *)entry_to_deserialize+serialized_len_in_bytes-8);
    dbg << "Check it out: "<< entry_to_deserialize << " -- " << temp_ptr << ", diff : " << (void *)((char *)temp_ptr - (char *)entry_to_deserialize) << endl;
 
    ptr_cd()->DeserializeRemoteEntryDir(ptr_cd()->remote_entry_directory_map_, entry_to_deserialize, task_count, serialized_len_in_bytes); 
//    ptr_cd()->DeserializeRemoteEntryDir(remote_entry_dir, (void *)((char *)entry_to_deserialize + 1196), task_count, serialized_len_in_bytes); 
    dbg << "\n\n[After] Check entries after deserialization " << ", size : "<< ptr_cd()->remote_entry_directory_map_.size() << ", # of tasks : " << node_id_.size() << ", level: "<< ptr_cd()->GetCDID().level() << endl<<endl;
//    for(auto it = remote_entry_dir.begin(); it!=remote_entry_dir.end(); ++it) {
//      dbg << *(it->second) << endl;
//    }
    dbg << "\n\nEnd of deserialization =========================================\n" << endl;
  }
  for(auto it=ptr_cd()->remote_entry_directory_map_.begin(); it!=ptr_cd()->remote_entry_directory_map_.end(); ++it) {
    dbg << (*it->second) << endl;
  }  

}  


















 
//  }
//  else{
//
//    if(new_node_id.IsHead()) {
//      // Calculate the array of group 
//      MPI_Group_incl(node_id_.task_group_, head_rank_count, head_rank, &(dynamic_cast<HeadCD*>(ptr_cd)->task_group())); 
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
//  MPI_Gatherv(serialized_entries, entry_size, MPI_BYTE, 
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
//    MPI_Gather(send_buf, 1, MPI_BYTE, task_id_buffer, 1, MPI_BYTE, node_id().head(), node_id().color());
//  }
//  else {  // Current task is head. It receives children CDs' head info and entry info of every task in the CD.
//    char *recv_buf;
//    MPI_Gather(send_buf, 1, MPI_BYTE, recv_buf, 1, MPI_BYTE, node_id().head(), node_id().color());
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
////    MPI_Group_incl(node_id().color(), num_children, child_heads, &(dynamic_cast<HeadCD*>(ptr_cd)->task_group()));
//
//  }
//
//
//
//  int task_id_buffer[node_id().size()];
//  int task_id = node_id().task_in_color();
//  int null_id = 0;
//  if(new_node_id.IsHead()) {
//    MPI_Gather(&task_id, 1, MPI_INTEGER, task_id_buffer, 1, MPI_INTEGER, node_id().head(), node_id().color());
//  }
//  else {
//    MPI_Gather(&null_id, 1, MPI_INTEGER, task_id_buffer, 1, MPI_INTEGER, node_id().head(), node_id().color());
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
////      MPI_Group_incl(node_id().color(), num_children, child_heads, &(dynamic_cast<HeadCD*>(ptr_cd)->task_group()));
////    }
//
////  ptr_cd_->family_mailbox_
//
//

*/
#endif
#endif


