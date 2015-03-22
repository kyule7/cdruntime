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
#include "cd_path.h"

using namespace cd;
using namespace std;

/// KL
/// dbg is a global variable to be used for debugging purpose.
/// General usage is that it stores any strings, numbers as a string internally,
/// and when dbg.flush() is called, it write the internally stored strings to file,
/// then clean up the internal storage.
#if _DEBUG
cd::DebugBuf cd::dbg;
#endif

/// KL
/// uniquePath is a singleton object per process, which is used for CDPath.
CDPath *CDPath::uniquePath_;

/// KL
/// app_side is a global variable to differentiate application side and CD runtime side.
/// This switch is adopted to turn off runtime logging in CD runtime side,
/// and turn on only in application side.
/// Plus, it enables to control on/off of logging for a specific routine.
bool cd::app_side = true;

/// KL
/// These global variables are used because MAX_TAG_UB is different from MPI libraries.
/// For example, MAX_TAG_UB of MPICH is 29 bits and that of OpenMPI is 31 bits.
/// MPI_TAG is important because it is used to generate an unique message tag in the protocol for preservation-via-reference.
/// It is not possible to set MPI_TAG_UB to some value by application.
/// So, I decided to check the MAX_TAG_UB value from MPI runtime at initialization routine
/// and set the right bit positions in CDID::GenMsgTag(ENTRY_TAG_T tag), CDID::GenMsgTagForSameCD(int msg_type, int task_in_color) functions.
int  cd::max_tag_bit = 0;
int  cd::max_tag_level_bit = 0;
int  cd::max_tag_task_bit  = 0;
int  cd::max_tag_rank_bit  = 0;

int cd::myTaskID = 0;


namespace cd {
// Global functions -------------------------------------------------------
/// KL
CDHandle* CD_Init(int numTask, int myTask)
{
  //GONG
  CDPrologue();
  myTaskID = myTask;

#if _DEBUG
  std::string output_filename("./output/output_");
  output_filename += to_string(static_cast<unsigned long long>(myTaskID));
  dbg.open(output_filename.c_str());
#endif

  Internal::Intialize();

  CD::CDInternalErrT internal_err;
  CDHandle* root_cd_handle = CD::CreateRootCD(NULL, "Root", CDID(CDNameT(0), NodeID(ROOT_COLOR, myTask, ROOT_HEAD_ID, numTask)), kStrict, 0, &internal_err);
  CDPath::GetCDPath()->push_back(root_cd_handle);

#if _PROFILER
  // Profiler-related
  root_cd_handle->profiler_->InitViz();
#endif

#if _DEBUG
  dbg.flush();
#endif

  //GONG
  CDEpilogue();

  return root_cd_handle;
}

void WriteDbgStream(DebugBuf *debugBuf)
{
#if _DEBUG
  dbg.flush();
  dbg.close();

  std::string output_filename("./output/output_app_");
  output_filename += string(to_string(static_cast<unsigned long long>(myTaskID)));
  debugBuf->open(output_filename.c_str());
  debugBuf->flush();
  debugBuf->close();
#endif
}

void CD_Finalize(DebugBuf *debugBuf)
{
  //GONG:
  CDPrologue();


#if _PROFILER
  // Profiler-related  
  CDPath::GetRootCD()->profiler_->FinalizeViz();
#endif

  assert(CDPath::GetCDPath()->size()==1); // There should be only on CD which is root CD
  assert(CDPath::GetCDPath()->back()!=NULL);

  CDPath::GetRootCD()->Destroy();
  Internal::Finalize();
  dbg << "========================= Finalize ["  << myTaskID <<"] ==============================="<< endl;

#if _DEBUG
  if(debugBuf != NULL) WriteDbgStream(debugBuf);
#endif

  CDEpilogue();
}


CDHandle *GetCurrentCD(void) 
{ return CDPath::GetCurrentCD(); }
  
CDHandle *GetRootCD(void)    
{ return CDPath::GetRootCD(); }

CDHandle *GetParentCD(void)
{ return CDPath::GetParentCD(); }

CDHandle *GetParentCD(int current_level)
{ return CDPath::GetParentCD(current_level); }

/// Split the node in three dimension
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
  
  int taskID = my_task_id; 
  int sz = num_x*num_y;
  int Z = (double)taskID / sz;
  int tmp = taskID % sz;
  int Y = (double)tmp / num_x;
  int X = tmp % num_x;

  new_color = (int)(X / new_num_x + (Y / new_num_y)*num_children_x + (Z / new_num_z)*(num_children_x * num_children_y));
  new_task  = (int)(X % new_num_x + (Y % new_num_y)*new_num_x      + (Z % new_num_z)*(new_num_x * new_num_y));
  
  return 0;
}

} // namespace cd ends












// CDHandle Member Methods ------------------------------------------------------------

//int SplitCD_3D(const int& my_task_id, 
//               const int& my_size, 
//               const int& num_children, 
//               int& new_color, 
//               int& new_task);

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
//    PMPI_Win_create(pendingFlag_, 1, sizeof(CDFlagT), PMPI_INFO_NULL, PMPI_COMM_WORLD, &pendingWindow_);
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
//    PMPI_Win_create(pendingFlag_, 1, sizeof(CDFlagT), PMPI_INFO_NULL, PMPI_COMM_WORLD, &pendingWindow_);
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

///// Split the node
///// (x,y,z)
///// taskID = x + y * nx + z * (nx*ny)
///// x = taskID % nx
///// y = ( taskID % ny ) - x 
///// z = r / (nx*ny)
//int SplitCD_3D(const int& my_task_id, 
//               const int& my_size, 
//               const int& num_children, 
//               int& new_color, 
//               int& new_task)
//{
//  // Get current task group size
//  int new_size = (int)((double)my_size / num_children);
//  int num_x = round(pow(my_size, 1/3.));
//  int num_y = num_x;
//  int num_z = num_x;
//
//  int num_children_x = 0;
//  if(num_children > 1)       num_children_x = round(pow(num_children, 1/3.));
//  else if(num_children == 1) num_children_x = 1;
//  else assert(0);
//
//  int num_children_y = num_children_x;
//  int num_children_z = num_children_x;
//
//  int new_num_x = num_x / num_children_x;
//  int new_num_y = num_y / num_children_y;
//  int new_num_z = num_z / num_children_z;
//
////  dbg<<"CD : "<< CDPath::GetCurrentCD()->ptr_cd()->name_<<"num_children = "<< num_children 
////     <<", num_x = "<< num_x 
////     <<", new_children_x = "<< num_children_x 
////     <<", new_num_x = "<< new_num_x <<"node size: "<<my_size<<"\n\n"<<endl; //dbgBreak();
//
//  assert(num_x*num_y*num_z == my_size);
//  assert(num_children_x * num_children_y * num_children_z == (int)num_children);
//  assert(new_num_x * new_num_y * new_num_z == new_size);
//  assert(num_x != 0);
//  assert(num_y != 0);
//  assert(num_z != 0);
//  assert(new_num_x != 0);
//  assert(new_num_y != 0);
//  assert(new_num_z != 0);
//  assert(num_children_x != 0);
//  assert(num_children_y != 0);
//  assert(num_children_z != 0);
//  
//  int taskID = my_task_id; // Get current task ID
//  int sz = num_x*num_y;
//  int Z = (double)taskID / sz;
//  int tmp = taskID % sz;
//  int Y = (double)tmp / num_x;
//  int X = tmp % num_x;
//
////  dbg<<"tmp = "<<tmp <<endl;
////  dbg<<"taskID = "<<taskID <<endl;
////  dbg<<"sz = "<<sz<<endl;
////  dbg<<"X = "<<X<<endl;
////  dbg<<"Y = "<<Y<<endl;
////  dbg<<"Z = "<<Z<<endl;
////
////  dbg<<"num_children_x*num_children_y = "<<num_children_x * num_children_y <<endl;
////  dbg<<"new_num_x*new_num_y = "<<new_num_x * new_num_y <<endl;
////  dbg << "(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<")"<< endl; 
//
//  new_color = (int)(X / new_num_x + (Y / new_num_y)*num_children_x + (Z / new_num_z)*(num_children_x * num_children_y));
//  new_task  = (int)(X % new_num_x + (Y % new_num_y)*new_num_x      + (Z % new_num_z)*(new_num_x * new_num_y));
//  
////  dbg << "(color,task,size) = (" << new_color << ","<< new_task << "," << new_size << ") <-- "<<endl;
////       <<"(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<") -- (color,task,size) = (" << GetCurrentCD()->GetColor() << ","<< my_task_id << "," << my_size <<")"
////       << "ZZ : " << round((double)Z / new_num_z)
////       << endl;
////  dbgBreak();
//  return 0;
//}


void CDHandle::RegisterSplitMethod(std::function<int(const int& my_task_id,
                                                     const int& my_size,
                                                     const int& num_children,
                                                     int& new_color, 
                                                     int& new_task)> split_func)
{ SplitCD = split_func; }


//CDErrT CDHandle::CD_Comm_Split(int group_id, int color, int key, int* new_group_id)
//{
////1) Get color and task from API
////2) Using PMPI_Allreduce(), get table for every color and task
////3) Two contexts are allocated for all the comms to be created.  
////   These same two contexts can be used for all created communicators 
////   since the communicators will not overlap.
////4) If the local process has a color of PMPI_UNDEFINED, it can return a NULL comm.
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

  PMPI_Allreduce(sendBuf, recvBuf, 3, MPI_INT, PMPI_SUM, new_color);
//  PMPI_Barrier(PMPI_COMM_WORLD); 
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
    PMPI_Comm_split(my_color, new_color, new_task, &(new_node.color_));
    PMPI_Comm_size(new_node.color(), &(new_node.size_));
    PMPI_Comm_rank(new_node.color(), &(new_node.task_in_color_));
    PMPI_Comm_group(new_node.color(), &(new_node.task_group_));
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
  CDPrologue();
  cout << "CDHandle::Create " << node_id_ << endl;
  // Create a new CDHandle and CD object
  // and populate its data structure correctly (CDID, etc...)
  //  This is an extremely powerful mechanism for dividing a single communicating group of processes into k subgroups, 
  //  with k chosen implicitly by the user (by the number of colors asserted over all the processes). 
  //  Each resulting communicator will be non-overlapping. 
  //  Such a division could be useful for defining a hierarchy of computations, 
  //  such as for multigrid, or linear algebra.
  //  
  //  Multiple calls to PMPI_COMM_SPLIT can be used to overcome the requirement 
  //  that any call have no overlap of the resulting communicators (each process is of only one color per call). 
  //  In this way, multiple overlapping communication structures can be created. 
  //  Creative use of the color and key in such splitting operations is encouraged.
  //  
  //  Note that, for a fixed color, the keys need not be unique. 
  //  It is PMPI_COMM_SPLIT's responsibility to sort processes in ascending order according to this key, 
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

  cout << "[Before] old: " << node_id_ <<", new: " << new_node_id << endl << endl; //dbgBreak();
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
  cout << "new_color : " << new_color << " in "<< node_id_ << endl;
  CDNameT new_cd_name(ptr_cd_->GetCDName(), num_children, new_color);
  cout<<"~~~~~~~~before create cd obj~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<dbg; //dbgBreak();

//  if(!ptr_cd()->remote_entry_directory_map_.empty())

  dbg << "Remote Entry Dir size:  "<< ptr_cd_->remote_entry_directory_map_.size() << endl;

  CollectHeadInfoAndEntry(new_node_id); 


  cout << "done collect to head" << endl;
  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CD::CDInternalErrT internal_err;
  CDHandle* new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), cd_type, sys_bit_vec, &internal_err);

  cout << "done collect to head" << endl;

  CDPath::GetCDPath()->push_back(new_cd_handle);

//  dbg<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;

  if(err<0) {
    ERROR_MESSAGE("CDHandle::Create failed.\n"); assert(0);
  }

  CDEpilogue();

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
  CDPrologue();
  CDHandle* new_cdh = Create(color, num_children, name, cd_type, error_name_mask, error_loc_mask, error);
  new_cdh->Begin(false, name);
  CDEpilogue();
  return new_cdh;
}

CDErrT CDHandle::Destroy(bool collective)
{
  CDPrologue();

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

   
  CDEpilogue();
  return err;
}


CDErrT CDHandle::Begin(bool collective, const char* label)
{
  CDPrologue();
  assert(ptr_cd_ != 0);
  CDErrT err = ptr_cd_->Begin(collective, label);

  dbg << "Do Barrier at CDHandle::Begin at level [" << ptr_cd_->level() << "], is it reexec? " << ptr_cd_->num_reexecution_ << endl;
  
  if(node_id_.size() > 1) {
    PMPI_Barrier(node_id_.color());
    cout << "\n\n[Barrier] CDHandle::Begin - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
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

  CDEpilogue();
  return err;
}

CDErrT CDHandle::Complete(bool collective, bool update_preservations)
{
  CDPrologue();

  if(IsHead()) {
    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      cout << "\n\n[Barrier] CDHandle::Complete 1 (Head) - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }
    CheckMailBox();

  }
  else {
    CheckMailBox();
    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      cout << "\n\n[Barrier] CDHandle::Complete 1 - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }
  }



//  CheckMailBox();
  if(IsHead()) {
    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      cout << "\n\n[Barrier] CDHandle::Complete 2 (Head) - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }
    CheckMailBox();

  }
  else {
    CheckMailBox();
    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      cout << "\n\n[Barrier] CDHandle::Complete 2 - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }
  }
  if(node_id_.size() > 1) {
    PMPI_Barrier(node_id_.color());
    cout << "\n\n[Barrier] CDHandle::Complete 3 --- "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; 
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
  
  CDEpilogue();
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
  CDPrologue();
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
  CDEpilogue();
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
  CDPrologue();
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
  CDPrologue();
  return kError;
}


char *CDHandle::GetName(void) const
{
  if(ptr_cd_ != NULL)
    return const_cast<char*>(ptr_cd_->name_.c_str());  
  else
    assert(0);
}

CDID     &CDHandle::GetCDID(void)       { return ptr_cd_->GetCDID(); }
CDNameT  &CDHandle::GetCDName(void)     { return ptr_cd_->GetCDName(); }
ColorT   CDHandle::GetNodeID(void)     { return node_id_.color(); }
NodeID   &CDHandle::node_id(void)       { return node_id_; }

CD       *CDHandle::ptr_cd(void)       const { return ptr_cd_; }
void     CDHandle::SetCD(CD* ptr_cd)         { ptr_cd_=ptr_cd; }

uint32_t CDHandle::level(void)         const { return ptr_cd_->GetCDName().level(); }
uint32_t CDHandle::rank_in_level(void) const { return ptr_cd_->GetCDName().rank_in_level(); }
uint32_t CDHandle::sibling_num(void)   const { return ptr_cd_->GetCDName().size(); }

ColorT  CDHandle::color(void)          const { return node_id_.color(); }
int     CDHandle::task_in_color(void)  const { return node_id_.task_in_color(); }
int     CDHandle::head(void)           const { return node_id_.head(); }
int     CDHandle::task_size(void)      const { return node_id_.size(); }

// FIXME
bool CDHandle::IsHead(void) const { return node_id_.IsHead(); }

// FIXME
// For now task_id_==0 is always Head which is not good!
// head is always task id 0 for now
void CDHandle::SetHead(NodeID& new_node_id) { new_node_id.set_head(0); }

int       CDHandle::GetSeqID(void)     const { return ptr_cd_->GetCDID().sequential_id(); }
CDHandle *CDHandle::GetParent(void)    const { return CDPath::GetParentCD(ptr_cd_->GetCDName().level()); }

bool CDHandle::operator==(const CDHandle &other) const 
{
  bool ptr_cd = (other.ptr_cd() == ptr_cd_);
  bool color  = (other.node_id_.color()  == node_id_.color());
  bool task   = (other.node_id_.task_in_color()   == node_id_.task_in_color());
  bool size   = (other.node_id_.size()   == node_id_.size());
  return (ptr_cd && color && task && size);
}


/// Synchronize the CD object in every task of that CD.
CDErrT CDHandle::Sync() 
{
  CDErrT err = INITIAL_ERR_VAL;

#if _MPI_VER
  int mpi_err = PMPI_Barrier(node_id_.color());
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
  CDPrologue();
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
  CDEpilogue();
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
  CDPrologue();
  //CheckMailBox();
  dbg << "DETECT check mode : " << ptr_cd()->cd_exec_mode_ << " at level " << ptr_cd()->level() << endl;
  std::vector<SysErrT> ret_prepare;
  CDEventT event = CDEventT::kNoEvent;

  if(ptr_cd_->num_reexecution_ == 0) {
    if(true) {
    
    //  dbg << "Detect at Level : " << ptr_cd()->GetCDID().level() << ", my node : " << node_id() << endl; getchar();
    
      if(ptr_cd()->GetCDID().level() == 2) {
//        if(myTaskID == 3 ) {
//          event = CDEventT::kErrorOccurred;
//        }
      }
      else if(ptr_cd()->GetCDID().level() == 1) {
        switch(myTaskID) {
          case 0:
            break;
          case 1:
            break;
          case 2:
            break;
          case 3:
            break;
          case 4:
            break;
          case 5:
            break;
          case 6:
//            event = CDEventT::kErrorOccurred;
            break;
          case 7:
            break;
          default : break; 
        }
      }
      else if(ptr_cd()->GetCDID().level() == 2) {
        switch(myTaskID) {
          case 0:
            break;
          case 1:
//            event = CDEventT::kErrorOccurred;
            break;
          case 2:
            break;
          case 3:
//            event = CDEventT::kErrorOccurred;
            break;
          case 4:
            break;
          case 5:
            break;
          case 6:
            break;
          case 7:
//            event = CDEventT::kErrorOccurred;
            break;
          default : break;
        }
      }
      else {

      }
    }
  }
//  while( !(ptr_cd()->TestComm()) ) {
//    // FIXME : Becareful! CheckMailBox() has some collectives.
//    CheckMailBox();
//  }

  PMPI_Barrier(node_id_.color());

  if(IsHead()) { 

    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      dbg << "\n\n[Barrier] CDHandle::Detect 1 (Head) - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }
    SetMailBox(event);

  }
  else {

    SetMailBox(event);
    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      dbg << "\n\n[Barrier] CDHandle::Detect 1 - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }

  }




  if(IsHead()) { 

    CheckMailBox();
    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      cout << "\n\n[Barrier] CDHandle::Detect 2 (Head) - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }
  }
  else {

    if(node_id_.size() > 1) {
      PMPI_Barrier(node_id_.color());
      cout << "\n\n[Barrier] CDHandle::Detect 2 - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
    }
    CheckMailBox();

  }


//  if(IsHead() && ptr_cd()->GetCDID().level() == 1) {
//    ptr_cd_->HandleAllResume(this);
//    cout <<"============ Resumed ================" << endl;
//  }



  CDEpilogue();
  return ret_prepare;

}

CDErrT CDHandle::RegisterRecovery (uint32_t error_name_mask, uint32_t error_loc_mask, RecoverObject *recover_object)
{
  CDPrologue();
  if( IsHead() ) {
    // STUB
  }
  else {
    // It is at remote node so do something for that.
  }
  CDEpilogue();
  return kOK;
}

CDErrT CDHandle::RegisterDetection (uint32_t system_name_mask, uint32_t system_loc_mask)
{
  CDPrologue();
  if( IsHead() ) {
    // STUB
  }
  else {
    // It is at remote node so do something for that.

  }
  CDEpilogue();

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
  ENTRY_TAG_T entry_tag = cd_hash(entry_name);
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
  CDPrologue();
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
  CDEpilogue();
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
//  if(node_id_.size() > 1) {
//    PMPI_Barrier(node_id_.color());
//    cout << "\n\n[Barrier] CDHandle::CheckMailBox - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
//  }
  CDErrT cd_err = ptr_cd()->CheckMailBox();

//  if(node_id_.size() > 1) {
//    PMPI_Barrier(node_id_.color());
//    cout << "\n\n[Barrier] CDHandle::CheckMailBox - "<< ptr_cd_->GetCDName() << " / " << node_id_ << "\n\n" << endl; //getchar();
//  }
//  cout << "==============================================================" << endl;
//
//  cd_err = ptr_cd()->CheckMailBox();
  return cd_err;
}

CDErrT CDHandle::SetMailBox(CDEventT &event)
{
  return ptr_cd()->SetMailBox(event);
}

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



