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

#include "cd_handle.h"
#include "cd.h"
#include "cd_entry.h"
#include "cd_id.h"
#include "util.h"
#include "cd_path.h"
#include <assert.h>
#include <utility>
#include <math.h>
#include <fstream>

using namespace cd;
using namespace std;
uint64_t Util::gen_object_id_=0;
CDPath* CDPath::uniquePath_;

#if _DEBUG
std::ostringstream cd::dbg;
#endif

int cd::myTaskID = 0;

static inline void CDPrologue(void) {
  app_side = false;
}

static inline void CDEpilogue(void) {
  app_side = true;
}

// Global functions -------------------------------------------------------
/// CD_Init()
/// Create the data structure that CD object and CDHandle object are going to use.
/// For example, CDEntry object or CDHandle object or CD object itself.
/// These objects are very important data for CDs and 
/// managed separately from the user-level data structure
/// All the meta data objects and preserved data are managed internally.
/// Register Root CD
CDHandle* cd::CD_Init(int numTask, int myRank)
{
  //GONG
  CDPrologue();
  myTaskID = myRank;

#if _MPI_VER
#if _KL
  MPI_Alloc_mem(sizeof(CDFlagT), MPI_INFO_NULL, &(CD::pendingFlag_));

  // Initialize pending flag
  *CD::pendingFlag_ = 0;
#endif
#endif


  CD::CDInternalErrT internal_err;
  CDHandle* root_cd_handle = CD::CreateRootCD(NULL, "Root", CDID(CDNameT(0), NodeID(ROOT_COLOR, myRank, ROOT_HEAD_ID, numTask)), kStrict, 0, &internal_err);
  CDPath::GetCDPath()->push_back(root_cd_handle);

#if _PROFILER
  // Profiler-related
  root_cd_handle->profiler_->InitViz();
#endif

  //GONG
  CDEpilogue();

  return root_cd_handle;
}

void cd::WriteDbgStream(std::ostringstream *oss)
{
#if _DEBUG
  std::string output_filename("./output/output_");
  output_filename = output_filename + string(to_string(myTaskID));
  std::ofstream out_file_cd(output_filename.c_str());

  out_file_cd << dbg.str() << endl;
  out_file_cd.close();

  output_filename.clear();
  output_filename  = "./output/output_app_";
  output_filename = output_filename + string(to_string(myTaskID));
  std::ofstream out_file_app(output_filename.c_str());

  out_file_app << oss->str() << endl;
  out_file_app.close();
#endif
}

void cd::CD_Finalize(std::ostringstream *oss)
{
  //GONG:
  CDPrologue();

  assert(CDPath::GetCDPath()->size()==1); // There should be only on CD which is root CD
  assert(CDPath::GetCDPath()->back()!=NULL);

#if _PROFILER
  // Profiler-related  
  CDPath::GetRootCD()->profiler_->FinalizeViz();
#endif

#if _MPI_VER
#if _KL
//  MPI_Win_free(&CDHandle::pendingWindow_);
  MPI_Free_mem(CD::pendingFlag_);
#endif
#endif
  
//  CDPath::GetRootCD()->Destroy();
#if _DEBUG
  if(oss != NULL) WriteDbgStream(oss);
#endif

  CDEpilogue();
}

// CDHandle Member Methods ------------------------------------------------------------

int SplitCD_3D(const int& my_task_id, 
               const int& my_size, 
               const int& num_children, 
               int& new_color, 
               int& new_task);

CDHandle::CDHandle()
  : ptr_cd_(0), node_id_()
{
#if _PROFILER
  profiler_ = new CDProfiler();
#else
  //profiler_ = new NullProfiler();
#endif
  SplitCD = &SplitCD_3D;

//  if(node_id().size() > 1)
//    MPI_Win_create(pendingFlag_, 1, sizeof(CDFlagT), MPI_INFO_NULL, MPI_COMM_WORLD, &pendingWindow_);
//  else
//    dbg << "KL : size is 1" << endl;
  
}

/// Design decision when we are receiving ptr_cd, do we assume that pointer is usable? 
/// That means is the handle is being created at local? 
/// Perhaps, we should not assume this 
/// as we will sometimes create a handle for an object that is located at remote.. right?
///
/// CDID set up. ptr_cd_ set up. parent_cd_ set up. children_cd_ will be set up later by children's Create call.
/// sibling ID set up, cd_info set up
/// clear children list
/// request to add me as a children to parent (to Head CD object)
CDHandle::CDHandle(CD* ptr_cd, const NodeID& node_id) 
  : ptr_cd_(ptr_cd), node_id_(node_id)
{
  SplitCD = &SplitCD_3D;
#if _PROFILER
  profiler_ = new CDProfiler();
#else
  //profiler_ = new NullProfiler();
#endif


//  if(node_id_.size() > 1)
//    MPI_Win_create(pendingFlag_, 1, sizeof(CDFlagT), MPI_INFO_NULL, MPI_COMM_WORLD, &pendingWindow_);
//  else
//    dbg << "KL : size is 1" << endl;
}

CDHandle::~CDHandle()
{
  // request to delete me in the children list to parent (to Head CD object)
  if(ptr_cd_ != NULL) {
    // We should do this at Destroy(), not creator?
//    RemoveChild(this);
  } 
  else {  // There is no CD for this CDHandle!!

  }

}

void CDHandle::Init(CD* ptr_cd, const NodeID& node_id)
{ 
  ptr_cd_   = ptr_cd;
  node_id_  = node_id;
}

// Non-collective
CDHandle* CDHandle::Create(const char* name, 
                           CDType cd_type, 
                           uint32_t error_name_mask, 
                           uint32_t error_loc_mask, 
                           CDErrT *error )
{
  //GONG
  CDPrologue();
  //CheckMailBox();

  // Create CDHandle for a local node
  // Create a new CDHandle and CD object
  // and populate its data structure correctly (CDID, etc...)
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);
  
  NodeID new_node_id(node_id_.color(), INVALID_TASK_ID, INVALID_HEAD_ID, node_id_.size());
  GetNewNodeID(new_node_id);
  SetHead(new_node_id);


  // Generate CDID
  CDNameT new_cd_name(ptr_cd_->GetCDName(), 1, 0);

  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CD::CDInternalErrT internal_err;
  CDHandle* new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), cd_type, sys_bit_vec, &internal_err);

  CDPath::GetCDPath()->push_back(new_cd_handle);
  
  //GONG
  CDEpilogue();

  return new_cd_handle;
}

/// Split the node
/// (x,y,z)
/// taskID = x + y * nx + z * (nx*ny)
/// x = taskID % nx
/// y = ( taskID % ny ) - x 
/// z = r / (nx*ny)
int SplitCD_3D(const int& my_task_id, 
               const int& my_size, 
               const int& num_children, 
               int& new_color, 
               int& new_task)
{
  // Get current task group size
  int new_size = (int)((double)my_size / num_children);
  int num_x = round(pow(my_size, 1/3.));
  int num_y = num_x;
  int num_z = num_x;

  int num_children_x = 0;
  if(num_children > 1)       num_children_x = round(pow(num_children, 1/3.));
  else if(num_children == 1) num_children_x = 1;
  else assert(0);

  int num_children_y = num_children_x;
  int num_children_z = num_children_x;

  int new_num_x = num_x / num_children_x;
  int new_num_y = num_y / num_children_y;
  int new_num_z = num_z / num_children_z;

//  dbg<<"CD : "<< CDPath::GetCurrentCD()->ptr_cd()->name_<<"num_children = "<< num_children 
//     <<", num_x = "<< num_x 
//     <<", new_children_x = "<< num_children_x 
//     <<", new_num_x = "<< new_num_x <<"node size: "<<my_size<<"\n\n"<<endl; //dbgBreak();

  assert(num_x*num_y*num_z == my_size);
  assert(num_children_x * num_children_y * num_children_z == (int)num_children);
  assert(new_num_x * new_num_y * new_num_z == new_size);
  assert(num_x != 0);
  assert(num_y != 0);
  assert(num_z != 0);
  assert(new_num_x != 0);
  assert(new_num_y != 0);
  assert(new_num_z != 0);
  assert(num_children_x != 0);
  assert(num_children_y != 0);
  assert(num_children_z != 0);
  
  int taskID = my_task_id; // Get current task ID
  int sz = num_x*num_y;
  int Z = (double)taskID / sz;
  int tmp = taskID % sz;
  int Y = (double)tmp / num_x;
  int X = tmp % num_x;

//  dbg<<"tmp = "<<tmp <<endl;
//  dbg<<"taskID = "<<taskID <<endl;
//  dbg<<"sz = "<<sz<<endl;
//  dbg<<"X = "<<X<<endl;
//  dbg<<"Y = "<<Y<<endl;
//  dbg<<"Z = "<<Z<<endl;
//
//  dbg<<"num_children_x*num_children_y = "<<num_children_x * num_children_y <<endl;
//  dbg<<"new_num_x*new_num_y = "<<new_num_x * new_num_y <<endl;
//  dbg << "(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<")"<< endl; 

  new_color = (int)(X / new_num_x + (Y / new_num_y)*num_children_x + (Z / new_num_z)*(num_children_x * num_children_y));
  new_task  = (int)(X % new_num_x + (Y % new_num_y)*new_num_x      + (Z % new_num_z)*(new_num_x * new_num_y));
  
//  dbg << "(color,task,size) = (" << new_color << ","<< new_task << "," << new_size << ") <-- "<<endl;
//       <<"(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<") -- (color,task,size) = (" << GetCurrentCD()->GetColor() << ","<< my_task_id << "," << my_size <<")"
//       << "ZZ : " << round((double)Z / new_num_z)
//       << endl;
//  dbgBreak();
  return 0;
}


void CDHandle::RegisterSplitMethod(std::function<int(const int& my_task_id,
                                                     const int& my_size,
                                                     const int& num_children,
                                                     int& new_color, 
                                                     int& new_task)> split_func)
{ SplitCD = split_func; }


//CDErrT CDHandle::CD_Comm_Split(int group_id, int color, int key, int* new_group_id)
//{
////1) Get color and task from API
////2) Using MPI_Allreduce(), get table for every color and task
////3) Two contexts are allocated for all the comms to be created.  
////   These same two contexts can be used for all created communicators 
////   since the communicators will not overlap.
////4) If the local process has a color of MPI_UNDEFINED, it can return a NULL comm.
////5) The table entries that match the local process color are sorted by key/rank.
////6) A group is created from the sorted list and a communicator is created with this group and the previously allocated contexts.
//}


CDErrT CDHandle::GetNewNodeID(NodeID& new_node)
{
  CDErrT err = kOK;
  // just set the same as parent.
  new_node = node_id_;
  
  //new_node.init_node_id(node_id_.color(), 0, INVALID_HEAD_ID,1);
  return err;
}

void TestMPIFunc(const ColorT& new_color, const int& color_for_split)
{
/*
  int sendBuf[3] = {1, 2, 3};
  int recvBuf[3] = {0, 0, 0};
//  for(int i=0; i<3; ++i) 
//    sendBuf[i] += color_for_split-1;

  for(int i=0; i<color_for_split*100000; i++) { int a = 5 * 5; } 
  dbg<<"\n-------------------------------------------------------------------\n"<<endl;
  dbg<<new_color<<"[Before Allreduce-----]\nsendBuf : {"<<sendBuf[0]<<", "<<sendBuf[1]<<", "<<sendBuf[2]<<"}"<<endl;
  dbg<<"recvBuf : {"<<recvBuf[0]<<", "<<recvBuf[1]<<", "<<recvBuf[2]<<"}"<<endl;

  MPI_Allreduce(sendBuf, recvBuf, 3, MPI_INT, MPI_SUM, new_color);
//  MPI_Barrier(MPI_COMM_WORLD); 
  for(int i=0; i<color_for_split*100000; i++) { int a = 5 * 5; } 
  dbg<< new_color<<"[After Allreduce-----]\nsendBuf : {"<<sendBuf[0]<<", "<<sendBuf[1]<<", "<<sendBuf[2]<<"}"<<endl;
  dbg<<"recvBuf : {"<<recvBuf[0]<<", "<<recvBuf[1]<<", "<<recvBuf[2]<<"}"<<endl;
  dbg<<"\n-------------------------------------------------------------------\n"<<endl;
  //dbgBreak();
*/
}

CDErrT CDHandle::GetNewNodeID(const ColorT& my_color, const int& new_color, const int& new_task, NodeID& new_node)
{
#if _MPI_VER
    CDErrT err = kOK;
//    dbg<<"new_color : " << new_color <<", new_task: "<<new_task<<", new_node.color(): "<<new_node.color()<<endl;
    MPI_Comm_split(my_color, new_color, new_task, &(new_node.color_));
    MPI_Comm_size(new_node.color(), &(new_node.size_));
    MPI_Comm_rank(new_node.color(), &(new_node.task_in_color_));
    MPI_Comm_group(new_node.color(), &(new_node.task_group_));
//    Sync();
//    TestMPIFunc(node_id_.color(), node_id_.task_in_color());
//    Sync();
//    for(int i=0; i<new_color*100000; i++) { int a = 5 * 5; } 
//    if(new_color == 0) 
//      dbg<<"\n--------PRE DONE-----------------------------------------------------------\n\n\n\n\n\n\n\n\n"<<endl;
//
//    TestMPIFunc(new_node.color(), new_color);
//
//    Sync();
//    for(int i=0; i<new_color*100000; i++) { int a = 5 * 5; } 
//    if(new_color == 0) 
//      dbg<<"\n--------DONE-----------------------------------------------------------\n\n\n\n\n\n\n\n\n"<<endl;
    return err;

#elif _PGAS_VER
    CDErrT err = kOK;
    int size = new_node.size();
//    assert(size == new_node.size());
    return err;
#else
    return kOK;
#endif
}


// Collective
CDHandle* CDHandle::Create(const ColorT& color,
                           uint32_t  num_children,
                           const char* name, 
                           CDType cd_type, 
                           uint32_t error_name_mask, 
                           uint32_t error_loc_mask, 
                           CDErrT *error )
{
  // Create a new CDHandle and CD object
  // and populate its data structure correctly (CDID, etc...)
  //  This is an extremely powerful mechanism for dividing a single communicating group of processes into k subgroups, 
  //  with k chosen implicitly by the user (by the number of colors asserted over all the processes). 
  //  Each resulting communicator will be non-overlapping. 
  //  Such a division could be useful for defining a hierarchy of computations, 
  //  such as for multigrid, or linear algebra.
  //  
  //  Multiple calls to MPI_COMM_SPLIT can be used to overcome the requirement 
  //  that any call have no overlap of the resulting communicators (each process is of only one color per call). 
  //  In this way, multiple overlapping communication structures can be created. 
  //  Creative use of the color and key in such splitting operations is encouraged.
  //  
  //  Note that, for a fixed color, the keys need not be unique. 
  //  It is MPI_COMM_SPLIT's responsibility to sort processes in ascending order according to this key, 
  //  and to break ties in a consistent way. 
  //  If all the keys are specified in the same way, 
  //  then all the processes in a given color will have the relative rank order 
  //  as they did in their parent group. 
  //  (In general, they will have different ranks.)
  //  
  //  Essentially, making the key value zero for all processes of a given color means that 
  //  one doesn't really care about the rank-order of the processes in the new communicator.  

  // Create CDHandle for multiple tasks (MPI rank or threads)
  dbg << "check mode : " << ptr_cd()->cd_exec_mode_ << " at level " << ptr_cd()->level() << endl;

  //CheckMailBox();


//  Sync();
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);
  int new_size = (int)((double)node_id_.size() / num_children);
  int new_color=0, new_task = 0;
  int err=0;

  ColorT new_comm;
  NodeID new_node_id(new_comm, INVALID_TASK_ID, INVALID_HEAD_ID, new_size);

//  dbg << "[Before] old: " << node_id_ <<", new: " << new_node_id << endl << endl; //dbgBreak();
  if(num_children > 1) {
    dbg<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl; //dbgBreak();
    cout << "task id: "<<node_id_.task_in_color() <<", size: " << node_id_.size() <<", num_child: "<<num_children<< endl;
    err = SplitCD(node_id_.task_in_color(), node_id_.size(), num_children, new_color, new_task);
    dbg<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl; //dbgBreak();
    cout << "before new_color : " << new_color << endl;

    err = GetNewNodeID(node_id_.color(), new_color, new_task, new_node_id);
    cout << "new_color : " << new_color << endl;
    assert(new_size == new_node_id.size());

  }
  else if(num_children == 1) {
    err = GetNewNodeID(new_node_id);
  }
  else {
    ERROR_MESSAGE("Number of children to create is wrong.\n");
  }

//  Sync();
//  for(int i=0; i<node_id_.task_in_color()*100000; i++) { int a = 5 * 5; } 
//  dbg << "[After] old: " << node_id_ <<", new: " << new_node_id << endl << endl; //dbgBreak();

  SetHead(new_node_id);
  // Generate CDID
  cout << "new_color : " << new_color << endl;
  CDNameT new_cd_name(ptr_cd_->GetCDName(), num_children, new_color);
//  dbg<<"~~~~~~~~before create cd obj~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<dbg; //dbgBreak();

//  if(!ptr_cd()->remote_entry_directory_map_.empty())
    CollectHeadInfoAndEntry(new_node_id); 

  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CD::CDInternalErrT internal_err;
  CDHandle* new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), cd_type, sys_bit_vec, &internal_err);


  CDPath::GetCDPath()->push_back(new_cd_handle);

//  dbg<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;

  if(err<0) {
    ERROR_MESSAGE("CDHandle::Create failed.\n"); assert(0);
  }

  return new_cd_handle;
}


// Collective
CDHandle* CDHandle::Create(uint32_t num_children, 
                           const char* name, 
                           CDType cd_type, 
                           uint32_t error_name_mask, 
                           uint32_t error_loc_mask, 
                           CDErrT *error )
{
  return Create(GetNodeID(), num_children, name, cd_type, error_name_mask, error_loc_mask, error);
}


CDHandle* CDHandle::CreateAndBegin(const ColorT& color, 
                                   uint32_t num_children, 
                                   const char* name, 
                                   CDType cd_type, 
                                   uint32_t error_name_mask, 
                                   uint32_t error_loc_mask, 
                                   CDErrT *error )
{
  CDHandle* new_cdh = Create(color, num_children, name, cd_type, error_name_mask, error_loc_mask, error);
  new_cdh->Begin(false, name);

  return new_cdh;
}

CDErrT CDHandle::Destroy(bool collective)
{
  CDErrT err;
 
  if ( collective ) {
    //  Sync();
  }

  // It could be a local execution or a remote one.
  // These two should be atomic 

  if(this != CDPath::GetRootCD()) { 

    // Mark that this is destroyed
    // this->parent_->is_child_destroyed = true;

  } 
  else {

  }

  if(CDPath::GetCDPath()->size() > 1) {
    CDPath::GetParentCD()->RemoveChild(this);
  }

#if _PROFILER
  dbg << "calling finish profiler" <<endl;
  //if(ptr_cd()->cd_exec_mode_ == 0) { 
    profiler_->FinishProfile();
  //}
#endif

  assert(CDPath::GetCDPath()->size()>0);
  assert(CDPath::GetCDPath()->back() != NULL);

  err = ptr_cd()->Destroy();
//  delete ptr_cd_;
  delete CDPath::GetCDPath()->back();
  CDPath::GetCDPath()->pop_back();

   

  return err;
}


CDErrT CDHandle::Begin(bool collective, const char* label)
{
  assert(ptr_cd_ != 0);
  CDErrT err = ptr_cd_->Begin(collective, label);

  dbg << "Do Barrier at CDHandle::Begin at level [" << ptr_cd_->level() << "], is it reexec? " << ptr_cd_->num_reexecution_ << endl;
  
  if(node_id_.size() > 1) {
    MPI_Barrier(node_id_.color());
    cout << "\n\n[Barrier] CDHandle::Begin - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; getchar();
  }
  //CheckMailBox();

//  dbg << jmp_buffer_ << endl; dbgBreak();
  //TODO It is illegal to call a collective Begin() on a CD that was created without a collective Create()??
  if ( collective ) {
    // Sync();
  }

#if _PROFILER
  // Profile-related
  dbg << "calling get profile" <<endl; //dbgBreak();
//  if(ptr_cd()->cd_exec_mode_ == 0) { 
    if(label == NULL) label = "INITIAL_LABEL";
    dbg << "label "<< label <<endl; //dbgBreak();
    profiler_->GetProfile(label);
//  }
#endif


  return err;
}

CDErrT CDHandle::Complete(bool collective, bool update_preservations)
{
  //CheckMailBox();
  if(node_id_.size() > 1) {
    MPI_Barrier(node_id_.color());
    cout << "\n\n[Barrier] CDHandle::Complete - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; getchar();
  }
  // Call internal Complete routine
  assert(ptr_cd_ != 0);

#if _PROFILER
  dbg << "calling collect profile" <<endl; //dbgBreak();
  // Profile-related
//  if(ptr_cd()->cd_exec_mode_ == 0) { 
    profiler_->CollectProfile();
//  }
#endif

//FIXME
  CDErrT ret=INITIAL_ERR_VAL;

  // Profile will be acquired inside CD::Complete()
  ret = ptr_cd_->Complete();


  if ( collective == true ) {

    //int sync_ret = Sync();
    // sync_ret check, go to error routine if it fails.

  }
  else {  // Non-collective Begin()


  }

  return ret;
}

CDErrT CDHandle::Preserve(void *data_ptr, 
                          uint64_t len, 
                          uint32_t preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject *regen_object, 
                          PreserveUseT data_usage)
{
  //CheckMailBox();


  /// Preserve meta-data
  /// Accumulated volume of data to be preserved for Sequential CDs. 
  /// It will be averaged out with the number of seq. CDs.
#if _PROFILER
  if(ptr_cd()->cd_exec_mode_ == 0) { 
//    profiler_->GetPrvData(data_ptr, len, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
  }
#endif

//  if( IsHead() ) {
    assert(ptr_cd_ != 0);
    return ptr_cd()->Preserve(data_ptr, len, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
//  }
//  else {
//    // It is at remote node so do something for that.
//
//  }

  return kError;
}

CDErrT CDHandle::Preserve(CDEvent &cd_event, 
                          void *data_ptr, 
                          uint64_t len, 
                          uint32_t preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject *regen_object, 
                          PreserveUseT data_usage )
{
  //CheckMailBox();
  if( IsHead() ) {
    assert(ptr_cd_ != 0);
    // TODO CDEvent object need to be handled separately, 
    // this is essentially shared object among multiple nodes.
    return ptr_cd()->Preserve(data_ptr, len, preserve_mask, 
                             my_name, ref_name, ref_offset,  
                             regen_object, data_usage );
  }
  else {
    // It is at remote node so do something for that.
  }

  return kError;
}


CD*     CDHandle::ptr_cd(void)    const { return ptr_cd_;         }
NodeID& CDHandle::node_id(void)    { return node_id_;        }
void    CDHandle::SetCD(CD* ptr_cd){ ptr_cd_=ptr_cd;         }
ColorT  CDHandle::GetNodeID(void)  const { return node_id_.color(); }
int     CDHandle::GetTaskID(void)  const{ return node_id_.task_in_color();  }
int     CDHandle::GetTaskSize(void) const{ return node_id_.size();  }

int CDHandle::GetSeqID() const 
{ return ptr_cd_->GetCDID().sequential_id(); }

bool CDHandle::operator==(const CDHandle &other) const 
{
  bool ptr_cd = (other.ptr_cd() == ptr_cd_);
  bool color  = (other.node_id_.color()  == node_id_.color());
  bool task   = (other.node_id_.task_in_color()   == node_id_.task_in_color());
  bool size   = (other.node_id_.size()   == node_id_.size());
  return (ptr_cd && color && task && size);
}

char* CDHandle::GetName(void) const
{
  if(ptr_cd_ != NULL)
    return const_cast<char*>(ptr_cd_->name_.c_str());  
  else
    assert(0);
}

// FIXME
bool CDHandle::IsHead(void) const
{
  return node_id_.IsHead();
}

// FIXME
// For now task_id_==0 is always Head which is not good!
void CDHandle::SetHead(NodeID& new_node_id)
{
//  dbg<<"In SetHead, Newly born CDHandle's Task# is "<<task<<endl;

  // head is always task id 0 for now
  new_node_id.set_head(0);
}

/// Synchronize the CD object in every task of that CD.
CDErrT CDHandle::Sync() 
{
  CDErrT err = INITIAL_ERR_VAL;

#if _MPI_VER
  int mpi_err = MPI_Barrier(node_id_.color());
#else
  int mpi_err = 1;
#endif

  if(mpi_err != 0) { 
    err = kOK; 
  }
  return err;
}

CDErrT CDHandle::Stop()
{ return ptr_cd_->Stop(); }

CDErrT CDHandle::AddChild(CDHandle* cd_child)
{
  CDErrT err=INITIAL_ERR_VAL;
  ptr_cd_->AddChild(cd_child);
  return err;
}

CDErrT CDHandle::RemoveChild(CDHandle* cd_child)  
{
  CDErrT err=INITIAL_ERR_VAL;
  ptr_cd_->RemoveChild(cd_child);
  return err;
}

bool CDHandle::IsLocalObject()
{
/* RELEASE
  // Check whether we already have this current node info, if not then get one.
  if( current_task_id_ == -1 || current_process_id_ == -1 ) {
    current_task_id_    = cd::Util::GetCurrentTaskID(); 
    current_process_id_ = cd::Util::GetCurrentProcessID();
  }

  if( current_task_id_ == task_id_ && current_process_id_ == process_id_ )
    return true;
  else 
    return false;
*/
return true;
}


CDErrT CDHandle::CDAssert (bool test_true, const SysErrT *error_to_report)
{
//  if( IsHead() ) {
    assert(ptr_cd_ != 0);
#if _PROFILER
//    if(!test_true) {
//      profiler_->ClearSightObj();
//    }
#endif
    return ptr_cd_->Assert(test_true);
//  }
//  else {
//    assert(ptr_cd_ != 0);
//    return ptr_cd_->Assert(test_true); 
//    // It is at remote node so do something for that.
//  }

  return kOK;
}

CDErrT CDHandle::CDAssertFail (bool test_true, const SysErrT *error_to_report)
{
  if( IsHead() ) {

  }
  else {
    // It is at remote node so do something for that.
  }

  return kOK;
}

CDErrT CDHandle::CDAssertNotify(bool test_true, const SysErrT *error_to_report)
{
  if( IsHead() ) {
    // STUB
  }
  else {
    // It is at remote node so do something for that.
  }

  return kOK;
}

std::vector<SysErrT> CDHandle::Detect(CDErrT *err_ret_val)
{
  //CheckMailBox();
  dbg << "DETECT check mode : " << ptr_cd()->cd_exec_mode_ << " at level " << ptr_cd()->level() << endl;
  std::vector<SysErrT> ret_prepare;
  CDEventT event = CDEventT::kNoEvent;

//if(ptr_cd_->num_reexecution_ == 0) {
if(false) {

//  dbg << "Detect at Level : " << ptr_cd()->GetCDID().level() << ", my node : " << node_id() << endl; getchar();

  if(ptr_cd()->GetCDID().level() == 0) {
    if(myTaskID == 3 ) {
      event = CDEventT::kErrorOccurred;
    }
//    if(myTaskID == 6 ) {
//      event = CDEventT::kEntrySearch;
//    }
  }

  if(ptr_cd()->GetCDID().level() == 1) {
//    if(myTaskID == 7 ) {
//      event = CDEventT::kErrorOccurred;
//    }
    if(myTaskID == 5 ) {
      event = CDEventT::kErrorOccurred;
    }
  }
  if(ptr_cd()->GetCDID().level() == 1) {
    if(myTaskID == 1 ) {
      event = CDEventT::kErrorOccurred;
    }
//    if(myTaskID == 2 ) {
//      event = CDEventT::kEntrySearch;
//    }
  }
  if(ptr_cd()->GetCDID().level() == 1) {
    if(myTaskID == 2 ) {
      event = CDEventT::kErrorOccurred;
    }
//    if(myTaskID == 5 ) {
//      event = CDEventT::kEntrySearch;
//    }
  }
}


  SetMailBox(event);
  CheckMailBox();
//  CheckMailBox();
//  if(IsHead() && ptr_cd()->GetCDID().level() == 1) {
//    ptr_cd_->HandleAllResume(this);
//    cout <<"============ Resumed ================" << endl;
//  }


  return ret_prepare;

}

CDErrT CDHandle::RegisterRecovery (uint32_t error_name_mask, uint32_t error_loc_mask, RecoverObject *recover_object)
{
  if( IsHead() ) {
    // STUB
  }
  else {
    // It is at remote node so do something for that.
  }
  return kOK;
}

CDErrT CDHandle::RegisterDetection (uint32_t system_name_mask, uint32_t system_loc_mask)
{
  if( IsHead() ) {
    // STUB
  }
  else {
    // It is at remote node so do something for that.

  }

  return kOK;
}

float CDHandle::GetErrorProbability (SysErrT error_type, uint32_t error_num)
{

  return 0;
}

float CDHandle::RequireErrorProbability (SysErrT error_type, uint32_t error_num, float probability, bool fail_over)
{


  return 0;
}

CDEntry *CDHandle::InternalGetEntry(std::string entry_name)
{
  ENTRY_TAG_T entry_tag = str_hash(entry_name);
  tag2str[entry_tag] = entry_tag;
  return ptr_cd_->InternalGetEntry(entry_tag);
  //FIXME need some way to ask to accomplish this functionality...  // Remote request
}

void CDHandle::Recover (uint32_t error_name_mask, 
                        uint32_t error_loc_mask, 
                        std::vector<SysErrT> errors)
{
  // STUB
}

//CDErrT CDHandle::SetPGASType (void *data_ptr, uint64_t len, CDPGASUsageT region_type)
//{
//   // STUB
//  return kOK;
//}

int CDHandle::ctxt_prv_mode()
{
  return (int)ptr_cd_->ctxt_prv_mode_;
}

void CDHandle::CommitPreserveBuff()
{
//  if(ptr_cd_->cd_exec_mode_ ==CD::kExecution){
  if( ptr_cd_->ctxt_prv_mode_ == CD::kExcludeStack) {
//  dbg << "Commit jmp buffer!" << endl; dbgBreak();
//  dbg << "cdh: " << jmp_buffer_ << ", cd: " << ptr_cd_->jmp_buffer_ << ", size: "<< sizeof(jmp_buf) << endl; dbgBreak();
    memcpy(ptr_cd_->jmp_buffer_, jmp_buffer_, sizeof(jmp_buf));
    ptr_cd_->jmp_val_ = jmp_val_;
//  dbg << "cdh: " << jmp_buffer_ << ", cd: " << ptr_cd_->jmp_buffer_ << endl; dbgBreak();
  }
  else {
    ptr_cd_->ctxt_ = this->ctxt_;
  }
//  }
}


uint64_t CDHandle::SetSystemBitVector(uint64_t error_name_mask, uint64_t error_loc_mask)
{
  uint64_t sys_bit_vec = 0;
  if(error_name_mask == 0) {

  }
  else {

  }

  if(error_loc_mask == 0) {

  }
  else {

  }
  return sys_bit_vec;
}







CDErrT CDHandle::CheckMailBox(void)
{
  // FIXME
  if(node_id_.size() > 1) {
    MPI_Barrier(node_id_.color());
    cout << "\n\n[Barrier] CDHandle::CheckMailBox - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; getchar();
  }
  CDErrT cd_err = ptr_cd()->CheckMailBox();

  if(node_id_.size() > 1) {
    MPI_Barrier(node_id_.color());
    cout << "\n\n[Barrier] CDHandle::CheckMailBox - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; getchar();
  }
  cout << "==============================================================" << endl;

  cd_err = ptr_cd()->CheckMailBox();
  return cd_err;
}

CDErrT CDHandle::SetMailBox(CDEventT &event)
{
  return ptr_cd()->SetMailBox(event);
}

////int CDHandle::CheckMailBox(void)
////{
////  int ret=0;
////  dbg << "\n\n=================== Check Mail Box Start [Level " << ptr_cd()->GetCDID().level() <<"] ========================" << endl;
////  if(node_id_.size() > 1) {
////    int event_count = *pendingFlag_;
////    dbg << "\nCheckMailBox " << myTaskID << ", # of events : " << event_count << ", Is it Head? "<< IsHead() <<endl;
////    ReadMailBoxFromRemote();
////    if(event_count > 0) {
////      dbg << "An event is detected" << endl;
////      dbg << "Task : " << myTaskID << ", Level : " << ptr_cd()->GetCDID().level() <<" CD Name : " << ptr_cd()->GetCDName()<< ", Node ID: " <<node_id_<<endl;
////  
////      ret = InternalCheckMailBox();
////  
////    } 
////    else {
////      dbg << "No event is detected" << endl;
////    }
////  }
////  else {
////    dbg << "KL : [CheckMailBox] size is " << node_id_.size() << endl;
////    ReadMailBoxFromRemote();
////  }
////  dbg << "\n===================================================================\n" << endl;
////
////  return ret;
////}
////
////int requested_event_count = 0;
////int handled_event_count = 0;
////
////int CDHandle::InternalCheckMailBox(void)
////{
////  int ret=0;
////  requested_event_count = *pendingFlag_;
////  handled_event_count = 0;
////
////  CDHandle *curr_cdh = this;
////
//////  ReadMailBoxFromRemote();
////
////	while( curr_cdh != NULL ) {
////    // If this task is head at the current CD level, read mailbox
////    if(curr_cdh->IsHead()) {
////      ret = curr_cdh->ReadMailBox();
////    }
////    dbg << "[Searching for CD Level where current task is head] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
////    // Termination condition
////    // It finished every requests, and does not need to look at mailbox any more
////    if(requested_event_count == handled_event_count) {
////      dbg<<"I finished handling events!!"<<endl;
////      break;
////    }
////    else {
////      // If current CD is Root CD and GetParentCD is called, it returns NULL
////		  curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
////    }
////
////	} 
////
/////*
////  int event_count = *pendingFlag_;  
////  if(requested_event_count != handled_event_count) {
////    // There are some events not handled at this point
////    ret = requested_event_count - handled_event_count;
////    assert(event_count != (requested_event_count-handled_event_count));
////  }
////  else {
////    // All events were handled
////    // If every event was handled, event_count must be 0
////    ret = 0;
////    assert(event_count!=0);
////  }
////*/
////  return ret;
////}
////
////int CDHandle::ReadMailBox(void) 
////{
////  CDEventHandleT resolved = CDEventHandleT::kEventNone;
////  CDFlagT *event = ptr_cd()->event_flag();
////  assert(event);
////
////  for(int i=0; i<node_id_.size(); i++) {
////
////
////    dbg << "Task["<< i <<"] (" << ptr_cd()->GetCDName() << " / " << node_id_ 
////        << "), Error #" << event[i] << "\t";
////
////    // Initialize the resolved flag
////    resolved = kEventNone;
////    resolved = HandleEvent(&(event[i]), i);
////
////    // Reset the event flag after handling the event
////    switch(resolved) {
////      case CDEventHandleT::kEventNone : 
////        // no event
////        break;
////      case CDEventHandleT::kEventResolved :
////        // There was an event, but resolved
////
////        //(*pendingFlag_)--;
////        handled_event_count--;
////        break;
////      case CDEventHandleT::kEventPending :
////        // There was an event, but could not resolved.
////        // Escalation required
////        break;
////      default :
////        break;
////    }
////    dbg << "\tResolved? : " << resolved << endl;
////  }
////
////  return resolved;
////}
////
////int CDHandle::ReadMailBoxFromRemote(void)
////{
////  int ret = 0;
////  CDHandle *curr_cdh = this;
////	while( curr_cdh->node_id_.size()==1 ) {
////    if(curr_cdh == CDPath::GetRootCD()) {
////      dbg << "[ReadMailBoxFromRemote] there is a single task in the root CD" << endl;
////      assert(0);
////    }
////    dbg << "[ReadMailBoxFromRemote|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
////    // If current CD is Root CD and GetParentCD is called, it returns NULL
////		curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
////	} 
////  dbg << "ReadMailBoxFromRemote " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
////      << "\nOriginal CD " << ptr_cd()->GetCDName() << " / " << node_id_ << " at level : " << ptr_cd()->GetCDID().level() << "\n" << endl;
////  CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_; 
//// 
////  MPI_Group task_group;
////  MPI_Comm_group(curr_cdh->node_id_.color(), &task_group);
////
////  CDFlagT myEvent = 0;
////  if(curr_cdh->IsHead() == false) {
////    // This is not head, so needs to bring its mailbox from head
////    // Check leaf's status first
//////    MPI_Win_start(task_group, 0, mailbox);
////    MPI_Win_fence(0, mailbox);
////    MPI_Get(&myEvent, 1, MPI_INT, curr_cdh->node_id_.head(), curr_cdh->node_id_.task_in_color(), 1, MPI_INT, mailbox);
////    dbg << "Complete event # " << myEvent << endl;
////    MPI_Win_fence(0, mailbox);
////    
//////    MPI_Win_complete(mailbox);
////
//////    dbg << "Task "<< myTaskID <<" (" << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ 
//////        << ") got an error #" << myEvent << " (";
//////    CDEventHandleT ret_handled = HandleEvent(&myEvent);
//////
//////    switch( ret_handled ) {
//////      case CDEventHandleT::kEventNone : 
//////        // no event
//////        break;
//////      case CDEventHandleT::kEventResolved :
//////        // There was an event, but resolved
//////
//////        //(*pendingFlag_)--;
//////        handled_event_count--;
//////        break;
//////      case CDEventHandleT::kEventPending :
//////        // There was an event, but could not resolved.
//////        // Escalation required
//////        break;
//////      default :
//////        break;
//////    }
//////    dbg << "),\t\tResolved? : " << ret_handled << endl;
////
////  }
////  else {
////    // This task is head
//////    MPI_Win_post(task_group, 0, mailbox);
////    MPI_Win_fence(0, mailbox);
////    dbg << "Epoch for getting mailbox for each task " << curr_cdh->IsHead() << ", node id: " << curr_cdh->node_id_ << endl;
////    myEvent = (curr_cdh->ptr_cd()->event_flag())[curr_cdh->node_id_.task_in_color()];
////    MPI_Win_fence(0, mailbox);
//////    MPI_Win_wait(mailbox);
////  }
////
////  dbg << "Task "<< myTaskID <<" (" << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ 
////      << "), Error #" << myEvent << "\t";
////  CDEventHandleT ret_handled = HandleEvent(&myEvent);
////
////  switch( ret_handled ) {
////    case CDEventHandleT::kEventNone : 
////      // no event
////      break;
////    case CDEventHandleT::kEventResolved :
////      // There was an event, but resolved
////
////      //(*pendingFlag_)--;
////      handled_event_count--;
////      break;
////    case CDEventHandleT::kEventPending :
////      // There was an event, but could not resolved.
////      // Escalation required
////      break;
////    default :
////      break;
////  }
////  dbg << "\tResolved? : " << ret_handled << endl;
////
////  return ret;
////}
////
////
////CDEventHandleT CDHandle::HandleEvent(CDFlagT *p_event, int idx)
////{
////  CDEventHandleT ret = CDEventHandleT::kEventResolved;
////  int handle_ret = 0;
////  CDFlagT event = *p_event;
////  if( CHECK_NO_EVENT(event) ) {
////    dbg << "CD Event kNoEvent\t\t\t";
////  }
////  else {
////    int handled_event_count=0;
////    if( CHECK_EVENT(event, kErrorOccurred) ) {
////
////      ptr_cd_->HandleErrorOccurred(this);
////
////      handled_event_count++;
////    }
////
////    if( CHECK_EVENT(event, kEntrySearch) ) {
////      if(IsHead()) {
////        cout << "[event: entry search] This is event for head CD.";
//////        assert(0);
////      }
////#if _FIX
////      ENTRY_TAG_T recvBuf[2] = {0,0};
////      bool entry_searched = false; 
////      if( IsHead() ) {
////        entry_searched = ptr_cd()->HandleEntrySearch(this, idx, recvBuf);
////      }
////      else {
////        
//////        ptr_head_cd()->HandleEntrySearch(this, idx, recvBuf);
////      }
////      if(entry_searched == false) {
////        CDHandle *parent = CDPath::GetParentCD();
////        CDEventT entry_search = kEntrySearch;
////        parent->SetMailBox(entry_search);
////        MPI_Isend(recvBuf, 
////                  2, 
////                  MPI_UNSIGNED_LONG_LONG,
////                  parent->node_id().head(), 
////                  parent->ptr_cd()->GetCDID().GenMsgTag(MSG_TAG_ENTRY_TAG), 
////                  parent->node_id().color(),
////                  &(parent->ptr_cd()->entry_request_req_[recvBuf[0]].req_));
////      }
////#endif
////      // Search Entry
////      handled_event_count++;
////    }
////
////    if( CHECK_EVENT(event, kEntrySend) ) {
////
////#if _FIX
////      if(!IsHead()) {
////        cout << "[event: entry send] This is event for non-head CD.";
//////        assert(0);
////      }
////      ptr_cd_->HandleEntrySend(this);
////#endif
////      dbg << "CD Event kEntrySend\t\t\t";
////      handled_event_count++;
////      
////    }
////
////    if( CHECK_EVENT(event, kAllPause) ) {
////      cout << "What?? " << node_id_ << endl;
////      ptr_cd_->HandleAllPause(this);
////
////      handled_event_count++;
////
////    }
////
////    if( CHECK_EVENT(event, kAllResume) ) {
////
////      ptr_cd_->HandleAllResume(this);
////
////      handled_event_count++;
////
////    }
////
////    if( CHECK_EVENT(event, kAllReexecute) ) {
////
////      ptr_cd_->HandleAllReexecute(this);
////
////      handled_event_count++;
////
////    }
////    
////    (*pendingFlag_) -= handled_event_count;
////  } 
////  return ret;
////}
////
////
////int CDHandle::SetMailBox(CDEventT &event)
////{
////  dbg << "\n\n=================== Set Mail Box Start ==========================\n" << endl;
////  dbg << "\nSetMailBox " << myTaskID << ", event : " << event << ", Is it Head? "<< IsHead() <<endl;
////  int ret=0;
////  int val = 1;
////  if(node_id_.size() > 1) {
////    dbg << "KL : [SetMailBox] size is " << node_id_.size() << ". Check mailbox at the upper level." << endl;
////  
////    MPI_Win_fence(0, pendingWindow_);
////    if(event != CDEventT::kNoEvent) {
//////      MPI_Win_fence(0, pendingWindow_);
////      dbg << "Set CD Event kErrorOccurred. Level : " << ptr_cd()->GetCDID().level() <<" CD Name : " << ptr_cd()->GetCDName()<< endl;
////      // Increment pending request count at the target task (requestee)
////      MPI_Accumulate(&val, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, MPI_SUM, pendingWindow_);
////      dbg << "MPI Accumulate done" << endl; getchar();
//////      MPI_Win_fence(0, pendingWindow_);
////    }
////    MPI_Win_fence(0, pendingWindow_);
////
////    CDMailBoxT &mailbox = ptr_cd()->mailbox_;  
////    MPI_Win_fence(0, mailbox);
////    if(event != CDEventT::kNoEvent) {
////      // Inform the type of event to be requested
////      MPI_Accumulate(&event, 1, MPI_INT, node_id_.head(), node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
//////    MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
////      dbg << "MPI_Put done" << endl; getchar();
////    }
////  
//////    switch(event) {
//////      case CDEventT::kNoEvent :
//////        dbg << "Set CD Event kNoEvent" << endl;
//////        break;
//////      case CDEventT::kErrorOccurred :
//////        // Inform the type of event to be requested
//////        MPI_Accumulate(&event, 1, MPI_INT, node_id_.head(), node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
////////        MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//////        dbg << "MPI_Put done" << endl; getchar();
//////        break;
//////      case CDEventT::kAllPause :
//////        dbg << "Set CD Event kAllPause" << endl;
//////  
//////        break;
//////      case CDEventT::kAllResume :
//////  
//////        break;
//////      case CDEventT::kEntrySearch :
//////  
//////        break;
//////      case CDEventT::kEntrySend :
//////  
//////        break;
//////      case CDEventT::kAllReexecute :
//////  
//////        break;
//////      default:
//////  
//////        break;
//////    }
////  
////    MPI_Win_fence(0, mailbox);
////  
////  }
////  else {
////    // This is the leaf that has just single task in a CD
////    dbg << "KL : [SetMailBox Leaf and have 1 task] size is " << node_id_.size() << ". Check mailbox at the upper level." << endl;
////    CDHandle *curr_cdh = this;
////
////  	while( curr_cdh->node_id_.size()==1 ) {
////      if(curr_cdh == CDPath::GetRootCD()) {
////        dbg << "[SetMailBox] there is a single task in the root CD" << endl;
////        assert(0);
////      }
////      dbg << "[SetMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
////      // If current CD is Root CD and GetParentCD is called, it returns NULL
////  		curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
////  	} 
////    dbg << "[SetMailBox] " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
////        << "\nOriginal CD " << ptr_cd()->GetCDName() << " / " << node_id_ << " at level : " << ptr_cd()->GetCDID().level() << "\n" << endl;
////
////
////    int parent_head_id = curr_cdh->node_id().head();
////    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, parent_head_id, 0, curr_cdh->pendingWindow_);
////    if(event != CDEventT::kNoEvent) {
////      dbg << "Set CD Event kErrorOccurred. Level : " << ptr_cd()->GetCDID().level() <<" CD Name : " << ptr_cd()->GetCDName()<< endl;
////      // Increment pending request count at the target task (requestee)
////      MPI_Accumulate(&val, 1, MPI_INT, parent_head_id, 0, 1, MPI_INT, MPI_SUM, curr_cdh->pendingWindow_);
////      dbg << "MPI Accumulate done" << endl; getchar();
////    }
////    MPI_Win_unlock(parent_head_id, curr_cdh->pendingWindow_);
////
////
////    CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_;
////
////    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, parent_head_id, 0, mailbox);
////    // Inform the type of event to be requested
////    dbg << "Set event start" << endl; getchar();
////    if(event != CDEventT::kNoEvent) {
////      MPI_Accumulate(&event, 1, MPI_INT, parent_head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
//////    MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
////    }
////    dbg << "MPI_Accumulate done" << endl; getchar();
////
//////    switch(event) {
//////      case CDEventT::kNoEvent :
//////        dbg << "Set CD Event kNoEvent" << endl;
//////        break;
//////      case CDEventT::kErrorOccurred :
//////        // Inform the type of event to be requested
//////        dbg << "Set event start" << endl; getchar();
//////        MPI_Accumulate(&event, 1, MPI_INT, parent_head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
////////        MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//////        dbg << "MPI_Accumulate done" << endl; getchar();
//////        break;
//////      case CDEventT::kAllPause :
//////        dbg << "Set CD Event kAllPause" << endl;
//////  
//////        break;
//////      case CDEventT::kAllResume :
//////  
//////        break;
//////      case CDEventT::kEntrySearch :
//////  
//////        break;
//////      case CDEventT::kEntrySend :
//////  
//////        break;
//////      case CDEventT::kAllReexecute :
//////  
//////        break;
//////      default:
//////  
//////        break;
//////    }
////    MPI_Win_unlock(parent_head_id, mailbox);
////  }
////  dbg << "\n================================================================\n" << endl;
////
////  return ret;
////}
////
////
////char *CDHandle::GenTag(const char* tag)
////{
////  Tag tag_gen;
////  CDNameT cd_name = ptr_cd()->GetCDName();
////  tag_gen << tag << node_id_.task_in_color_ <<'-'<<cd_name.level()<<'-'<<cd_name.rank_in_level();
//////  cout << const_cast<char*>(tag_gen.str().c_str()) << endl; getchar();
////  return const_cast<char*>(tag_gen.str().c_str());
////}
/*
ucontext_t* CDHandle::context()
{
  if( IsHead() ) {

    return &ptr_cd_->ctxt_;
  }
  else {
    //FIXME: need to get the flag from remote

  }
}

jmp_buf* CDHandle::jump_buffer()
{
  if( IsHead() ) {

    return &ptr_cd_->jmp_buffer_;
  }
  else {
    //FIXME: need to get the flag from remote

  }
} 
*/



