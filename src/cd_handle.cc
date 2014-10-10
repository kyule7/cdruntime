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

using namespace cd;
using namespace std;
uint64_t Util::gen_object_id_=0;
CDPath* CDPath::uniquePath_;
//int cd::myTaskID = 0;

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
//  myTaskID = myRank;
//  CDHandle* root_cd = new CDHandle(NULL, "Root", NodeID(ROOT_COLOR, myRank, numTask, 0), kStrict, 0);
  CDHandle* root_cd_handle = CD::CreateRootCD(NULL, "Root", CDID(CDNameT(0), NodeID(ROOT_COLOR, myRank, ROOT_HEAD_ID, numTask)), kStrict, 0);
  CDPath::GetCDPath()->push_back(root_cd_handle);

#if _PROFILER
  // Profiler-related  
  root_cd_handle->profiler_->InitViz();
#endif

  return root_cd_handle;
}


void cd::CD_Finalize(void)
{
  assert(CDPath::GetCDPath()->size()==1); // There should be only on CD which is root CD
  assert(CDPath::GetCDPath()->back()!=NULL);

#if _PROFILER
  // Profiler-related  
  CDPath::GetRootCD()->profiler_->FinalizeViz();
#endif

  CDPath::GetRootCD()->Destroy();
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
//#else
//  profiler_ = new NullProfiler();
#endif
  SplitCD = &SplitCD_3D;
  
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
//#else
//  profiler_ = new NullProfiler();
#endif
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
  // Create CDHandle for a local node
  // Create a new CDHandle and CD object
  // and populate its data structure correctly (CDID, etc...)
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);
  
  NodeID new_node_id(node_id_.color(), INVALID_TASK_ID, INVALID_HEAD_ID, node_id_.size());
  GetNewNodeID(new_node_id);
  SetHead(new_node_id);


  // Generate CDID
  CDNameT new_cd_name(ptr_cd_->GetCDName());
  new_cd_name.IncLevel();
  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CDHandle* new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), cd_type, sys_bit_vec);

  CDPath::GetCDPath()->push_back(new_cd_handle);

//  cout<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;
  
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

//  cout<<"CD : "<< ptr_cd()->name_<<"num_children = "<< num_children 
//  <<", num_x = "<< num_x 
//  <<", new_children_x = "<< num_children_x 
//  <<", new_num_x = "<< new_num_x <<"node size: "<<my_size<<"\n\n"<<endl; //getchar();

//  cout<<"split check"<<endl;
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

  cout<<"tmp = "<<tmp <<endl;
  cout<<"taskID = "<<taskID <<endl;
  cout<<"sz = "<<sz<<endl;
  cout<<"X = "<<X<<endl;
  cout<<"Y = "<<Y<<endl;
  cout<<"Z = "<<Z<<endl;

  cout<<"num_children_x*num_children_y = "<<num_children_x * num_children_y <<endl;
  cout<<"new_num_x*new_num_y = "<<new_num_x * new_num_y <<endl;
  cout << "(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<")"<< endl; 

  new_color = (int)(X / new_num_x + (Y / new_num_y)*num_children_x + (Z / new_num_z)*(num_children_x * num_children_y));
  new_task  = (int)(X % new_num_x + (Y % new_num_y)*new_num_x      + (Z % new_num_z)*(new_num_x * new_num_y));
  
  cout << "(color,task,size) = (" << new_color << ","<< new_task << "," << new_size << ") <-- "<<endl;
//       <<"(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<") -- (color,task,size) = (" << GetCurrentCD()->GetColor() << ","<< my_task_id << "," << my_size <<")"
//       << "ZZ : " << round((double)Z / new_num_z)
//       << endl;
//  getchar();
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
  new_node.init_node_id(node_id_.color(), 0, INVALID_HEAD_ID,1);
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
  cout<<"\n-------------------------------------------------------------------\n"<<endl;
  cout<<new_color<<"[Before Allreduce-----]\nsendBuf : {"<<sendBuf[0]<<", "<<sendBuf[1]<<", "<<sendBuf[2]<<"}"<<endl;
  cout<<"recvBuf : {"<<recvBuf[0]<<", "<<recvBuf[1]<<", "<<recvBuf[2]<<"}"<<endl;

  MPI_Allreduce(sendBuf, recvBuf, 3, MPI_INT, MPI_SUM, new_color);
//  MPI_Barrier(MPI_COMM_WORLD); 
  for(int i=0; i<color_for_split*100000; i++) { int a = 5 * 5; } 
  cout<< new_color<<"[After Allreduce-----]\nsendBuf : {"<<sendBuf[0]<<", "<<sendBuf[1]<<", "<<sendBuf[2]<<"}"<<endl;
  cout<<"recvBuf : {"<<recvBuf[0]<<", "<<recvBuf[1]<<", "<<recvBuf[2]<<"}"<<endl;
  cout<<"\n-------------------------------------------------------------------\n"<<endl;
  //getchar();
*/
}

CDErrT CDHandle::GetNewNodeID(const ColorT& my_color, const int& new_color, const int& new_task, NodeID& new_node)
{
#if _MPI_VER
    CDErrT err = kOK;
//    cout<<"new_color : " << new_color <<", new_task: "<<new_task<<", new_node.color(): "<<new_node.color()<<endl;
    MPI_Comm_split(my_color, new_color, new_task, &(new_node.color_));
    MPI_Comm_size(new_node.color(), &(new_node.size_));
    MPI_Comm_rank(new_node.color(), &(new_node.task_in_color_));
//    Sync();
//    TestMPIFunc(node_id_.color(), node_id_.task_in_color());
//    Sync();
//    for(int i=0; i<new_color*100000; i++) { int a = 5 * 5; } 
//    if(new_color == 0) 
//      cout<<"\n--------PRE DONE-----------------------------------------------------------\n\n\n\n\n\n\n\n\n"<<endl;
//
//    TestMPIFunc(new_node.color(), new_color);
//
//    Sync();
//    for(int i=0; i<new_color*100000; i++) { int a = 5 * 5; } 
//    if(new_color == 0) 
//      cout<<"\n--------DONE-----------------------------------------------------------\n\n\n\n\n\n\n\n\n"<<endl;
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

//  Sync();
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);
  int new_size = (int)((double)node_id_.size() / num_children);
  int new_color, new_task = 0;
  int err=0;

  ColorT new_comm;
  NodeID new_node_id(new_comm, INVALID_TASK_ID, INVALID_HEAD_ID, new_size);

//  cout << "[Before] old: " << node_id_ <<", new: " << new_node_id << endl << endl; //getchar();
  if(num_children > 1) {
    err = SplitCD(node_id_.task_in_color(), node_id_.size(), num_children, new_color, new_task);
    err = GetNewNodeID(node_id_.color(), new_color, new_task, new_node_id);
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
//  cout << "[After] old: " << node_id_ <<", new: " << new_node_id << endl << endl; //getchar();

  SetHead(new_node_id);

  // Generate CDID
  CDNameT new_cd_name(ptr_cd_->GetCDName(), num_children, new_color);

  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CDHandle* new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), cd_type, sys_bit_vec);

  CDPath::GetCDPath()->push_back(new_cd_handle);

//  cout<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;

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
  profiler_->FinishProfile();
#endif

  assert(CDPath::GetCDPath()->size()>0);
  assert(CDPath::GetCDPath()->back() != NULL);

  err = ptr_cd_->Destroy();
  CDPath::GetCDPath()->pop_back();

   

  return err;
}


CDErrT CDHandle::Begin(bool collective, const char* label)
{

  //TODO It is illegal to call a collective Begin() on a CD that was created without a collective Create()??
  if ( collective ) {
    // Sync();
  }

#if _PROFILER
  // Profile-related
  profiler_->GetProfile();
#endif

  assert(ptr_cd_ != 0);
  CDErrT err = ptr_cd_->Begin(collective, label);

  return err;
}

CDErrT CDHandle::Complete(bool collective, bool update_preservations)
{

  // Call internal Complete routine
  assert(ptr_cd_ != 0);

#if _PROFILER
  // Profile-related
  profiler_->CollectProfile();
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
                          CDPreserveT preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject *regen_object, 
                          PreserveUseT data_usage)
{


  /// Preserve meta-data
  /// Accumulated volume of data to be preserved for Sequential CDs. 
  /// It will be averaged out with the number of seq. CDs.
#if _PROFILER
  profiler_->GetPrvData(data_ptr, len, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
#endif

//  if( IsHead() ) {
    assert(ptr_cd_ != 0);
    return ptr_cd_->Preserve(data_ptr, len, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
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
                          CDPreserveT preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject *regen_object, 
                          PreserveUseT data_usage )
{
  if( IsHead() ) {
    assert(ptr_cd_ != 0);
    // TODO CDEvent object need to be handled separately, 
    // this is essentially shared object among multiple nodes.
    return ptr_cd_->Preserve(data_ptr, len, preserve_mask, 
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
//  cout<<"In SetHead, Newly born CDHandle's Task# is "<<task<<endl;

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
  if( IsHead() ) {
    assert(ptr_cd_ != 0);
    return ptr_cd_->Assert(test_true); 
  }
  else {
    assert(ptr_cd_ != 0);
    return ptr_cd_->Assert(test_true); 
    // It is at remote node so do something for that.
  }

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
  std::vector<SysErrT> ret_prepare;

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

CDEntry* CDHandle::InternalGetEntry(std::string entry_name)
{
  return ptr_cd_->InternalGetEntry(entry_name);
  //FIXME need some way to ask to accomplish this functionality...  // Remote request
}

void CDHandle::Recover (uint32_t error_name_mask, 
                        uint32_t error_loc_mask, 
                        std::vector<SysErrT> errors)
{
  // STUB
}

CDErrT CDHandle::SetPGASType (void *data_ptr, uint64_t len, CDPGASUsageT region_type)
{
   // STUB
  return kOK;
}

int CDHandle::ctxt_prv_mode()
{
  if( IsHead() ) {
    return (int)ptr_cd_->ctxt_prv_mode_;
  }
  else {
    //FIXME: need to get the flag from remote
  }

  return 0;
}

void CDHandle::CommitPreserveBuff()
{
  if( IsHead() ) {
    if( ptr_cd_->ctxt_prv_mode_ == CD::kExcludeStack) {
        memcpy(ptr_cd_->jump_buffer_, jump_buffer_, sizeof(jmp_buf));
     }
     else {
        ptr_cd_->ctxt_ = this->ctxt_;
     }
 
  } 
  else {
    //FIXME: need to transfer the buffers to remote
  }
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

    return &ptr_cd_->jump_buffer_;
  }
  else {
    //FIXME: need to get the flag from remote

  }
} 
*/



