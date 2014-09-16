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
//#include "node_id.h"
//#include "cd_name_t.h"
#include "util.h"
#include "cd_path.h"

#include <assert.h>
#include <utility>
#include <math.h>

#if _MPI_VER
#include <mpi.h>
#endif

using namespace cd;
//using namespace cd::CDPath;

using namespace std;

std::map<uint64_t, int> cd::nodeMap;  // Unique CD Node ID - Communicator
CDPath* CDPath::uniquePath_;
//std::vector<CDHandle*>  cd::CDPath;
//CDPath* CDPath::uniquePath_;

std::vector<MasterCD*>  cd::MasterCDPath;
bool cd::is_visualized = false;
int cd::myTaskID = 0;
int cd::status = 0;

// Global functions -------------------------------------------------------
ostream& cd::operator<<(ostream& str, const NodeID& node_id)
{
  return str << '(' 
             << node_id.color_ << ", " 
             << node_id.task_ << "/" << node_id.size_ 
             << ')';
}

void cd::SetStatus(int flag)
{ cd::status = flag; }

//CDHandle* cd::GetCurrentCD(void) 
//{ return cd::CDPath.back(); }
//
//CDHandle* cd::GetRootCD(void)    
//{ return cd::CDPath.front(); }
//
//CDHandle* cd::GetParentCD(const CDID& cd_id)
//{ return CDPath.at(cd_id.level_ - 1); }


/// CD_Init()
/// Create the data structure that CD object and CDHandle object are going to use.
/// For example, CDEntry object or CDHandle object or CD object itself.
/// These objects are very important data for CDs and 
/// managed separately from the user-level data structure
/// All the meta data objects and preserved data are managed internally.
/// Register Root CD
CDHandle* cd::CD_Init(int numTask, int myRank)
{
  myTaskID = myRank; 
  CDHandle* root_cd = new CDHandle(NULL, "Root", NodeID(ROOT_COLOR, myRank, numTask, 0), kStrict, 0);
  CDPath::GetCDPath()->push_back(root_cd);

#if _PROFILER
  // Profiler-related  
  root_cd->profiler_->InitViz();
#endif

  return root_cd;

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

CDHandle::CDHandle()
  : ptr_cd_(0), node_id_()
{
#if _PROFILER
  profiler_ = new CDProfiler();
#else
  profiler_ = new NullProfiler();
#endif

  IsMaster_ = false;
  
}

/// Design decision when we are receiving ptr_cd, do we assume that pointer is usable? 
/// That means is the handle is being created at local? 
/// Perhaps, we should not assume this 
/// as we will sometimes create a handle for an object that is located at remote.. right?
///
/// CDID set up. ptr_cd_ set up. parent_cd_ set up. children_cd_ will be set up later by children's Create call.
/// sibling ID set up, cd_info set up
/// clear children list
/// request to add me as a children to parent (to MASTER CD object)
CDHandle::CDHandle( CDHandle* parent, 
                    const char* name, 
                    const NodeID& node_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector)
{

#if _PROFILER
  profiler_ = new CDProfiler();
#else
  profiler_ = new NullProfiler();
#endif

  node_id_ = node_id;
  SetMaster(node_id_.task_);

  if(parent != NULL) { 
    // Generate CDID
    CDID new_cd_id(parent->ptr_cd_->GetCDID().level_ + 1, node_id_);

    /// Create CD object with new CDID
    if( !IsMaster() ) {
      ptr_cd_  = new CD(parent, name, new_cd_id, cd_type, sys_bit_vector);
    }
    else {
      MasterCD* ptr_cd  = new MasterCD(parent, name, new_cd_id, cd_type, sys_bit_vector);
//    MasterCD* ptr_cd  = (MasterCD*)DATA_MALLOC(sizeof(MasterCD));
//    ptr_cd->Initialize(NULL, name, new_cd_id, cd_type, sys_bit_vector);
      MasterCDPath.push_back(ptr_cd);
      ptr_cd_ = ptr_cd;
//      if(is_visualized == false){
//        ptr_cd->InitViz();
//        is_visualized = true;
//      }
    }
  }
  else { // Root CD
    // Generate CDID
    CDID new_cd_id(0, node_id_);

    /// Create CD object with new CDID
    if( !IsMaster() ) {
      ptr_cd_  = new CD(parent, name, new_cd_id, cd_type, sys_bit_vector);
    }
    else {
      MasterCD* ptr_cd  = new MasterCD(parent, name, new_cd_id, cd_type, sys_bit_vector);
//    MasterCD* ptr_cd  = (MasterCD*)DATA_MALLOC(sizeof(MasterCD));
//    ptr_cd->Initialize(NULL, name, new_cd_id, cd_type, sys_bit_vector);
      MasterCDPath.push_back(ptr_cd);
      ptr_cd_ = ptr_cd;
//      if(is_visualized == false){
//        ptr_cd->InitViz();
//        is_visualized = true;
//      }
    }
  }

  cout<<"\nCD Node is created : "<<node_id_<<"\n"<<endl;
  assert(ptr_cd_ != 0);
}

CDHandle::CDHandle( CDHandle* parent, 
                    const char* name, 
                    NodeID&& node_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector)
{
#if _PROFILER
  profiler_ = new CDProfiler();
#else
  profiler_ = new NullProfiler();
#endif

  node_id_ = std::move(node_id);
  SetMaster(node_id_.task_);

  if(parent != NULL) { 
    // Generate CDID
    CDID new_cd_id(parent->ptr_cd_->GetCDID().level_ + 1, node_id_);

    /// Create CD object with new CDID
    if( !IsMaster() ) {
      ptr_cd_  = new CD(parent, name, new_cd_id, cd_type, sys_bit_vector);
    }
    else {
      MasterCD* ptr_cd  = new MasterCD(parent, name, new_cd_id, cd_type, sys_bit_vector);
//    MasterCD* ptr_cd  = (MasterCD*)DATA_MALLOC(sizeof(MasterCD));
//    ptr_cd->Initialize(NULL, name, new_cd_id, cd_type, sys_bit_vector);
      MasterCDPath.push_back(ptr_cd);
      ptr_cd_ = ptr_cd;
//      if(is_visualized == false){
//        ptr_cd->InitViz();
//        is_visualized = true;
//      }
    }
  }
  else { // Root CD
    // Generate CDID
    CDID new_cd_id(0, node_id_);

    /// Create CD object with new CDID
    if( !IsMaster() ) {
      ptr_cd_  = new CD(parent, name, new_cd_id, cd_type, sys_bit_vector);
    }
    else {
      MasterCD* ptr_cd  = new MasterCD(parent, name, new_cd_id, cd_type, sys_bit_vector);
      assert(ptr_cd != NULL);
//      MasterCD* ptr_cd  = (MasterCD*)DATA_MALLOC(sizeof(MasterCD));
//      ptr_cd->Initialize(NULL, name, new_cd_id, cd_type, sys_bit_vector);
      MasterCDPath.push_back(ptr_cd);
      ptr_cd_ = ptr_cd;
//      if(is_visualized == false){
//        ptr_cd->InitViz();
//        is_visualized = true;
//      }
    }
  }

  cout<<"\nCD Node is created : "<<node_id_<<"\n"<<endl;
  cout<<"ptr_cd "<<ptr_cd_<<endl;
  assert(ptr_cd_ != 0);
}

CDHandle::~CDHandle()
{
  // request to delete me in the children list to parent (to MASTER CD object)
  if(ptr_cd_ != NULL) {
    // We should do this at Destroy(), not creator?
//    RemoveChild(this);
  } 
  else {  // There is no CD for this CDHandle!!

  }

}

void CDHandle::Init(CD* ptr_cd, NodeID node_id)
{ 
  // CDHandle *parent
  ptr_cd_    = ptr_cd;
  node_id_  = node_id;
}

// Non-collective
CDHandle* CDHandle::Create( const char* name, 
                            CDType type, 
                            uint32_t error_name_mask, 
                            uint32_t error_loc_mask, 
                            CDErrT *error )
{
  // Create CDHandle for a local node
  // Create a new CDHandle and CD object
  // and populate its data structure correctly (CDID, etc...)
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);

  CDHandle* new_cd = new CDHandle(this, name, node_id_, type, sys_bit_vec);

  CDPath::GetCDPath()->push_back(new_cd);
//  cout<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;;
  // Request to add me as a child to my parent
  // It could be a local execution or a remote one. 
  ptr_cd_->AddChild(this);

  
  return new_cd;
}

/// Split the node
/// (x,y,z)
/// taskID = x + y * nx + z * (nx*ny)
/// x = taskID % nx
/// y = ( taskID % ny ) - x 
/// z = r / (nx*ny)
int CDHandle::SplitCD(int my_size, int num_children, int& new_color, int& new_task)
{

  int new_size = (int)((double)my_size / num_children);
  int num_x = round(pow(node_id_.size_, 1/3.));
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

  cout<<"CD : "<< ptr_cd()->name_<<"num_children = "<< num_children 
  <<", num_x = "<< num_x 
  <<", new_children_x = "<< num_children_x 
  <<", new_num_x = "<< new_num_x <<"node size: "<<node_id_.size_<<"\n\n"<<endl; //getchar();

//  cout<<"split check"<<endl;
  assert(num_x*num_y*num_z == node_id_.size_);
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
//  uint64_t num_y = (uint64_t)pow(new_node.size_, 1/3.);
//  uint64_t num_z = (uint64_t)pow(new_node.size_, 1/3.);
  
  int taskID = GetTaskID();
  int sz = num_x*num_y;
  int Z = taskID / sz;
  int tmp = taskID % sz;
  int Y = tmp / num_x;
  int X = tmp % num_x;


//  cout<<"num_children_x*num_children_y = "<<num_children_x * num_children_y <<endl;
//  cout<<"new_num_x*new_num_y = "<<new_num_x * new_num_y <<endl;
  cout << "(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<")"<< endl; 

  new_color = (int)(X / new_num_x + (Y / new_num_y)*num_children_x + (Z / new_num_z)*(num_children_x * num_children_y));
  new_task  = (int)(X % new_num_x + (Y % new_num_y)*new_num_x      + (Z % new_num_z)*(new_num_x * new_num_y));
  
//  cout << "(color,task,size) = (" << new_node.color_ << ","<< new_node.task_ << "," << new_node.size_ <<") <-- "
//       <<"(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<") -- (color,task,size) = (" << node_id_.color_ << ","<< node_id_.task_ << "," << node_id_.size_ <<")"
//       << "ZZ : " << round((double)Z / new_num_z)
//       << endl;
//  getchar();

  return 0;
}


CDErrT CDHandle::GetNewNodeID(NodeID& new_node)
{
  CDErrT err = kOK;
  // just set the same as parent.
  new_node.color_ = node_id_.color_;
  new_node.task_  = 0;
  new_node.size_  = 1;
  return err;
}

CDErrT CDHandle::GetNewNodeID(int my_color, int new_color, int new_task, NodeID& new_node)
{
#if _MPI_VER
    CDErrT err = kOK;
    int size = new_node.size_;
    MPI_Comm_split(my_color, new_color, new_task, &(new_node.color_));
    MPI_Comm_size(new_node.color_, &(new_node.size_));
    MPI_Comm_rank(new_node.color_, &(new_node.task_));
  
    assert(size == new_node.size_);
    return err;
#elif _PGAS_VER
    CDErrT err = kOK;
    int size = new_node.size_;
  //  err = MPI_Comm_split(my_color, new_color, new_task, &(new_node.color_));
  //  err = MPI_Comm_size(new_node.color_, &(new_node.size_));
  //  err = MPI_Comm_rank(new_node.color_, &(new_node.task_));
  
  //  assert(size == new_node.size_);
    return err;
#else
    return kOK;
#endif
}
// Collective
CDHandle* CDHandle::Create( int color, 
                            uint32_t num_children, 
                            const char* name, 
                            CDType type, 
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
  int new_size = (int)((double)node_id_.size_ / num_children);
  int new_color, new_task = 0;
  int err=0;

//  Sync(); cout << "[Before] old: " << node_id_ <<", new: " << new_node << endl << endl; //getchar();

  NodeID new_node(INITIAL_COLOR, 0, new_size, 0);
  if(num_children > 1) {
    err = SplitCD(node_id_.size_, num_children, new_color, new_task);
//  cout << "new_color: "<< new_color <<", new_task: " << new_task << endl << endl; //getchar();
    err = GetNewNodeID(node_id_.color_, new_color, new_task, new_node);
  }
  else if(num_children == 1) {
    err = GetNewNodeID(new_node);
  }
  else {
    ERROR_MESSAGE("Number of children to create is wrong.\n");
  }

//  Sync();
//  for(int i=0; i<node_id_.task_*100000; i++) { int a = 5 * 5; } 
//  cout << "[After] old: " << node_id_ <<", new: " << new_node << endl << endl; //getchar();

  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CDHandle* new_cd = new CDHandle(this, name, new_node, type, sys_bit_vec);

  CDPath::GetCDPath()->push_back(new_cd);
//  cout<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;

  // Request to add me as a child to my parent
  // It could be a local execution or a remote one. 
  ptr_cd_->AddChild(this);

  if(err<0) {
    ERROR_MESSAGE("CDHandle::Create failed.\n");
  }

  return new_cd;

}

// Collective
CDHandle* CDHandle::Create (uint32_t  numchildren,
                            const char* name, 
                            CDType type, 
                            uint32_t error_name_mask, 
                            uint32_t error_loc_mask, 
                            CDErrT *error )
{
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);

  NodeID new_node_tmp, new_node;
  SetColorAndTask(new_node_tmp, numchildren);

//  MPI_Comm_split(node_id_.color_, new_node_tmp.color_, new_node_tmp.task_, &(new_node.color_));
//  MPI_Comm_size(new_node.color_, &(new_node.size_));
//  MPI_Comm_rank(new_node.color_, &(new_node.task_));
  CDHandle* new_cd = new CDHandle(this, name, new_node, type, sys_bit_vec);
  CDPath::GetCDPath()->push_back(new_cd);
//  cout<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;;

  ptr_cd_->AddChild(this);

  return new_cd;

}

void CDHandle::SetColorAndTask(NodeID& new_node, const int& numchildren) 
{
  // mytask = a*x+b;
  // a : color
  // x : children node size (# of tasks in it)
  // b : task ID of each children node
  for(int i = 0; i < numchildren; ++i) {
    if( i * node_id_.size_/numchildren <= node_id_.task_ && 
        node_id_.task_ < (i + 1)* node_id_.size_/numchildren) {
      new_node.color_ = i;
      new_node.task_  = node_id_.task_ % numchildren;
      new_node.size_  = node_id_.size_ / numchildren;
      break;
    }
  }
}

CDHandle* CDHandle::CreateAndBegin( uint32_t color, 
                                    uint32_t num_tasks_in_color, 
                                    const char* name, 
                                    CDType type, 
                                    uint32_t error_name_mask, 
                                    uint32_t error_loc_mask, 
                                    CDErrT *error )
{

  return NULL;
}

CDErrT CDHandle::Destroy (bool collective)
{
  CDErrT err;
 
  if ( collective ) {
//    if( Sync() != 0 ) {
    if( true ) {
//      printf("------------------ Sync Success in Destroy %d ------------------\n", node_id_.task_);
    }
    else {
//      printf("------------------ Sync Failure in Destroy %d ------------------\n", node_id_.task_);
    }
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
//    GetParentCD(ptr_cd_->GetCDID())->RemoveChild(this);
    CDPath::GetParentCD()->RemoveChild(this);
  }

#if _PROFILER
  profiler_->FinishProfile();
#endif

  assert(CDPath::GetCDPath()->size()>0);
  assert(CDPath::GetCDPath()->back() != NULL);
//  assert(MasterCDPath.size()>0);
//  assert(MasterCDPath.back() != NULL);

  if(IsMaster()) {
    MasterCDPath.pop_back();
  }
  

  err = ptr_cd_->Destroy();
  CDPath::GetCDPath()->pop_back();

   

  return err;
}


CDErrT CDHandle::Begin (bool collective, const char* label)
{
  SetStatus(1);


  //TODO It is illegal to call a collective Begin() on a CD that was created without a collective Create()??
  if ( collective ) {
    Sync();
//    if( ret != 0 ) {
    if( true ) {
      //printf("------------------ Sync Success in Begin %d (%d) ------------------\n", node_id_.task_, ret);
      //getchar();
    }
    else {
      //printf("------------------ Sync Failure in Begin %d (%d) ------------------\n", node_id_.task_, ret);
      //getchar();
      //exit(-1);
    }
  }
//  cout << label << endl;
  cout<<11111<<endl;
  getchar();

#if _PROFILER
  // Profile-related
  profiler_->GetProfile();
#endif

  assert(ptr_cd_ != 0);
  cout<<22222<<"  "<<ptr_cd_<<"   "<<ptr_cd_->GetCDID().object_id_<<endl; getchar();
  CDErrT err = ptr_cd_->Begin(collective, label);
  return err;
}

CDErrT CDHandle::Complete (bool collective, bool update_preservations)
{
  SetStatus(0);

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

CDErrT CDHandle::Preserve ( void *data_ptr, 
                            uint64_t len, 
                            CDPreserveT preserve_mask, 
                            const char *my_name, 
                            const char *ref_name, 
                            uint64_t ref_offset, 
                            const RegenObject *regen_object, 
                            PreserveUseT data_usage )
{


  /// Preserve meta-data
  /// Accumulated volume of data to be preserved for Sequential CDs. 
  /// It will be averaged out with the number of seq. CDs.
#if _PROFILER
  profiler_->GetPrvData(data_ptr, len, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
#endif

  if( IsMaster() ) {
    assert(ptr_cd_ != 0);
    return ptr_cd_->Preserve(data_ptr, len, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
  }
  else {
    // It is at remote node so do something for that.

  }

  return kError;
}

CDErrT CDHandle::Preserve ( CDEvent &cd_event, 
                            void *data_ptr, 
                            uint64_t len, 
                            CDPreserveT preserve_mask, 
                            const char *my_name, 
                            const char *ref_name, 
                            uint64_t ref_offset, 
                            const RegenObject *regen_object, 
                            PreserveUseT data_usage )
{
  if( IsMaster() ) {
    assert(ptr_cd_ != 0);
    // TODO CDEvent object need to be handled separately, 
    // this is essentially shared object among multiple nodes.
    return ptr_cd_->Preserve( data_ptr, len, preserve_mask, 
                              my_name, ref_name, ref_offset, 
                              regen_object, data_usage );
  }
  else {
    // It is at remote node so do something for that.
  }

  return kError;
}


CD*     CDHandle::ptr_cd(void)     { return ptr_cd_;         }
NodeID& CDHandle::node_id(void)    { return node_id_;        }
void    CDHandle::SetCD(CD* ptr_cd){ ptr_cd_=ptr_cd;         }
int&    CDHandle::GetNodeID(void)  { return node_id_.color_; }
int     CDHandle::GetTaskID(void)  { return node_id_.task_;  }
int     CDHandle::GetTaskSize(void){ return node_id_.size_;  }

int CDHandle::GetSeqID() 
{ return ptr_cd_->GetCDID().sequential_id_; }

bool CDHandle::operator==(const CDHandle &other) const 
{
  bool ptr_cd = (other.ptr_cd_ == this->ptr_cd_);
  bool color  = (other.node_id_.color_  == this->node_id_.color_);
  bool task   = (other.node_id_.task_   == this->node_id_.task_);
  bool size   = (other.node_id_.size_   == this->node_id_.size_);
  return (ptr_cd && color && task && size);
}

char* CDHandle::GetName(void)
{
  if(ptr_cd_ != NULL)
    return const_cast<char*>(ptr_cd_->name_.c_str());  
  else
    assert(0);
}

// FIXME
bool CDHandle::IsMaster(void)
{
  return IsMaster_;
}

// FIXME
// For now task_id_==0 is always MASTER which is not good!
void CDHandle::SetMaster(int task)
{
//  cout<<"In SetMaster, Newly born CDHandle's Task# is "<<task<<endl;
  if(task == 0)
    IsMaster_ = true;
  else
    IsMaster_ = false;
}

/// Synchronize the CD object in every task of that CD.
CDErrT CDHandle::Sync() 
{
  CDErrT err = INITIAL_ERR_VAL;
#if _CD_MPI
  int mpi_err = MPI_Barrier(node_id_.color_);
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
  if( IsMaster() ) {
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
  if( IsMaster() ) {

  }
  else {
    // It is at remote node so do something for that.
  }

  return kOK;
}

CDErrT CDHandle::CDAssertNotify (bool test_true, const SysErrT *error_to_report)
{
  if( IsMaster() ) {
    // STUB
  }
  else {
    // It is at remote node so do something for that.
  }

  return kOK;
}

std::vector< SysErrT > CDHandle::Detect (CDErrT *err_ret_val)
{

  std::vector< SysErrT > ret_prepare;


  return ret_prepare;

}

CDErrT CDHandle::RegisterRecovery (uint32_t error_name_mask, uint32_t error_loc_mask, RecoverObject *recover_object)
{
  if( IsMaster() ) {
    // STUB
  }
  else {
    // It is at remote node so do something for that.
  }
  return kOK;
}

CDErrT CDHandle::RegisterDetection (uint32_t system_name_mask, uint32_t system_loc_mask)
{
  if( IsMaster() ) {
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
              std::vector< SysErrT > errors)
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
  if( IsMaster() )
  {

    return (int)ptr_cd_->ctxt_prv_mode_;
  }
  else
  {
    //FIXME: need to get the flag from remote

  }

  return 0;
}

void CDHandle::CommitPreserveBuff()
{
  if( IsMaster() )
  {
    if( ptr_cd_->ctxt_prv_mode_ == CD::kExcludeStack) 
     {
        memcpy(ptr_cd_->jump_buffer_, jump_buffer_, sizeof(jmp_buf));
     }
     else
     {
        ptr_cd_->ctxt_ = this->ctxt_;
     }
 
  } 
  else
  {
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
  if( IsMaster() ) {

    return &ptr_cd_->ctxt_;
  }
  else {
    //FIXME: need to get the flag from remote

  }
}

jmp_buf* CDHandle::jump_buffer()
{
  if( IsMaster() ) {

    return &ptr_cd_->jump_buffer_;
  }
  else {
    //FIXME: need to get the flag from remote

  }
} 
*/



