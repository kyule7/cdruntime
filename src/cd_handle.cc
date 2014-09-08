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
#include <assert.h>
#include <utility>
#include <math.h>
#include <mpi.h>

using namespace cd;
using namespace std;

std::map<uint64_t, int> cd::nodeMap;  // Unique CD Node ID - Communicator
std::vector<CDHandle*>  cd::CDPath;
std::vector<MasterCD*>  cd::MasterCDPath;
bool cd::is_visualized = false;
int cd::myTaskID = 0;
int cd::status = 0;

#if _PROFILER

#include "rdtsc.h"
#include "sight.h"
using namespace sight;

#if _ENABLE_MODULE
std::list<module*>     CDHandle::mStack;
modularApp*            CDHandle::ma;
#endif

#if _ENABLE_HIERGRAPH
std::list<hierGraph*>  CDHandle::hgStack;
hierGraphApp*          CDHandle::hga;
#endif

#if _ENABLE_SCOPE
std::list<scope*>      CDHandle::sStack;
graph*                 CDHandle::scopeGraph;
#endif

#if _ENABLE_ATTR
attrIf*                CDHandle::attrScope;
std::list<attrAnd*>    CDHandle::attrStack;
std::list<attr*>       CDHandle::attrValStack;
#endif

#if _ENABLE_CDNODE
std::list<CDNode*>     CDHandle::cdStack;
#endif

#if _ENABLE_COMP
std::vector<comparison*> CDHandle::compTagStack;
//std::vector<int> CDHandle::compKeyStack;
//std::list<comparison*> CDHandle::compStack;
#endif

#endif



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

CDHandle* cd::GetCurrentCD(void) 
{ return cd::CDPath.back(); }

CDHandle* cd::GetRootCD(void)    
{ return cd::CDPath.front(); }

CDHandle* cd::GetParentCD(const CDID& cd_id)
{ return CDPath.at(cd_id.level_ - 1); }


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
  CDPath.push_back(root_cd);

#if _PROFILER
  root_cd->InitViz();
#endif

  return root_cd;

}

void cd::CD_Finalize(void)
{
  assert(CDPath.size()==1); // There should be only on CD which is root CD
  assert(CDPath.back()!=NULL);
  GetRootCD()->Destroy();

#if _PROFILER
  GetRootCD()->FinalizeViz();
#endif

}

// CDHandle Member Methods ------------------------------------------------------------

CDHandle::CDHandle()
  : ptr_cd_(0), node_id_()
{

#if _PROFILER 
  InitProfile();
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
  InitProfile();
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
  InitProfile();
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
      assert(ptr_cd != nullptr);
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

  CDPath.push_back(new_cd);
//  cout<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;;
  // Request to add me as a child to my parent
  // It could be a local execution or a remote one. 
  ptr_cd_->AddChild(this);

  
  return new_cd;
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

//  Sync();
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);


  NodeID new_node(INITIAL_COLOR, 0, 0, 0);
  
  // Split the node
  // (x,y,z)
  // taskID = x + y * nx + z * (nx*ny)
  // x = taskID % nx
  // y = ( taskID % ny ) - x 
  // z = r / (nx*ny)
  int num_x = round(pow(node_id_.size_, 1/3.));
  int num_y = num_x;
  int num_z = num_x;
  new_node.size_ = (int)(node_id_.size_ / num_children);
//  cout<<"new node size = "<< new_node.size_<<endl;

//  int new_num_x=0;
//  if(num_node.size_ > 1) 
//    new_num_x = round(pow(new_node.size_, 1/3.));
//  else if(num_node.size_ == 1)
//    new_num_x = 1;
//  else
//    assert(0);
//    
//  int new_num_y = new_num_x;
//  int new_num_z = new_num_x;

  int num_children_x = 0;
  if(num_children > 1) 
    num_children_x = round(pow(num_children, 1/3.));
  else if(num_children == 1)
    num_children_x = 1;
  else
    assert(0);

  int num_children_y = num_children_x;
  int num_children_z = num_children_x;

  int new_num_x = num_x / num_children_x;
  int new_num_y = num_y / num_children_y;
  int new_num_z = num_z / num_children_z;

//  cout<<"num_children = "<< num_children 
//  <<", num_x = "<< num_x 
//  <<", new_children_x = "<< num_children_x 
//  <<", new_num_x = "<< new_num_x <<"\n\n"<<endl; //getchar();
  Sync();
//  cout<<"split check"<<endl;
  assert(num_x*num_y*num_z == node_id_.size_);
  assert(num_children_x * num_children_y * num_children_z == (int)num_children);
  assert(new_num_x * new_num_y * new_num_z == new_node.size_);
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


/*
  int X = taskID % num_x;
  int Y = (taskID % num_y) - X;
  int Z = round((double)taskID / (num_x * num_y));
*/
//  cout<<"num_children_x*num_children_y = "<<num_children_x * num_children_y <<endl;
//  cout<<"new_num_x*new_num_y = "<<new_num_x * new_num_y <<endl;
  cout << "(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<")"<< endl; Sync();
//  getchar();
  unsigned int new_color = (int)(X / new_num_x + (Y / new_num_y)*num_children_x + (Z / new_num_z)*(num_children_x * num_children_y));
  new_node.task_  = (int)(X % new_num_x + (Y % new_num_y)*new_num_x      + (Z % new_num_z)*(new_num_x * new_num_y));
  new_node.color_ = new_color;
//  cout << "(color,task,size) = (" << new_node.color_ << ","<< new_node.task_ << "," << new_node.size_ <<") <-- "
//       <<"(X,Y,Z) = (" << X << ","<<Y << "," <<Z <<") -- (color,task,size) = (" << node_id_.color_ << ","<< node_id_.task_ << "," << node_id_.size_ <<")"
//       << "ZZ : " << round((double)Z / new_num_z)
//       << endl;
//  getchar();

/*
  for(int z=0; z < new_num_z; ++z) {     // Z axis
    
    for(int y=0; y < new_num_y; ++y) {   // Y axis
      
      for(int x=0; x < new_num_x; ++x) { // X axis


//    if(i*GetTaskSize()/num_children =< GetTaskID()
//      && GetTaskID() < (i+1)*GetTaskSize()/num_children)
//    {
        X = X % new_num_x + (Y % new_num_y)*new_num_x + (Z % new_num_z)*nxny;
        Y = (taskID % ny) - X;
        Z = taskID / (nx*ny);

        new_node.color_ = scale_i + j*scale_j + k;
        new_node.task_  = taskID %  + j*scale_j + i*scale_i;
//    }


      }
    }
  }
*/  
  // Create CDHandle for multiple tasks (MPI rank or threads)
  new_color++;
  Sync(); cout<<"[Before] old: "<<node_id_<<", new: "<<new_node<<"\n"<< "new_color : "<<new_color<<"\n"<<endl; //getchar();

  MPI_Comm_split(node_id_.color_, new_color, new_node.task_, &(new_node.color_));
  MPI_Comm_size(new_node.color_, &(new_node.size_));
  MPI_Comm_rank(new_node.color_, &(new_node.task_));
  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  Sync();
//  for(int i=0; i<node_id_.task_*100000; i++) { int a = 5 * 5; } 
//  cout<<"[After] old: "<<node_id_<<", new: "<<new_node<<"\n"<<endl; //getchar();
//  cout<<"New CD created : " << new_node <<endl; 
//  getchar();

  CDHandle* new_cd = new CDHandle(this, name, new_node, type, sys_bit_vec);

  CDPath.push_back(new_cd);
//  cout<<"push back cd"<<ptr_cd()->GetCDID().level_<<endl;;
  // Request to add me as a child to my parent
  // It could be a local execution or a remote one. 
  ptr_cd_->AddChild(this);

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

  MPI_Comm_split(node_id_.color_, new_node_tmp.color_, new_node_tmp.task_, &(new_node.color_));
  MPI_Comm_size(new_node.color_, &(new_node.size_));
  MPI_Comm_rank(new_node.color_, &(new_node.task_));
  CDHandle* new_cd = new CDHandle(this, name, new_node, type, sys_bit_vec);
  CDPath.push_back(new_cd);
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

  if(this != GetRootCD()) { 

    // Mark that this is destroyed
    // this->parent_->is_child_destroyed = true;

  } 
  else {

  }


  if(CDPath.size() > 1) {
    GetParentCD(ptr_cd_->GetCDID())->RemoveChild(this);
  }

#if _PROFILER
    FinishProfile();
#endif 
 
  assert(CDPath.size()>0);
  assert(CDPath.back() != NULL);
//  assert(MasterCDPath.size()>0);
//  assert(MasterCDPath.back() != NULL);

  if(IsMaster()) {
    MasterCDPath.pop_back();
  }
  

  err = ptr_cd_->Destroy();
  CDPath.pop_back();

   

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
  int onOff = 0;
  if(ptr_cd()->GetCDID().sequential_id_ < 2) onOff = 1;

  if(label_.first != label) { 
    // diff
    //cout<<"label is diff"<<endl;
    //getchar();
    SetCollectProfile(true);

    // Dynamic Method Selection
    if(label_.first != "INITIAL_LABEL")
      FinishProfile();

    // Set label. INITIAL_LABEL should be unique for one CD
    // CD name cannot be "INITIAL_LABEL"
    if(label_.first != "INITIAL_LABEL") {
      label_.first = label;
      
    }
    else { // label_.first == "INITIAL_LABEL"
      label_.first = ptr_cd_->name_;
    }
//    cout<<"start profile------"<<endl;
//    getchar();
    label_.second = onOff;
    StartProfile();
  }
  else {
    // the same exec
    //cout<<"label is the same"<<endl;
    //getchar();
    SetCollectProfile(false);
  }
#endif

  assert(ptr_cd_ != 0);


  cout<<22222<<"  "<<ptr_cd_<<"   "<<ptr_cd_->GetCDID().object_id_<<endl;
  getchar();

  CDErrT err = ptr_cd_->Begin(collective, label);
  return err;
}

CDErrT CDHandle::Complete (bool collective, bool update_preservations)
{
  SetStatus(0);

  // Call internal Complete routine
  assert(ptr_cd_ != 0);

#if _PROFILER

  if( CheckCollectProfile() ) {
    // After acquiring the profile data, 
    // aggregate the data to master task
    // Only master task will call sight APIs.
    // Aggregated data from every task belonging to one CD node
    // will be averaged out, and avg value with std will be shown.
    //    
    // <PROFILE_NAME>   | <SEND?>
    // LOOP_COUNT       |   X
    // EXEC_CYCLE       |   O
    // PRV_COPY_DATA    |   O
    // PRV_REF_DATA     |   O
    // OVERLAPPED_DATA  |   O
    // SYSTEM_BIT_VECTOR|   X

    //Dynamic Method Selection
//    ptr_cd_->FinishProfile();

/* FIXME
    uint64_t sendBuf[MAX_PROFILE_DATA-2]={0,};
    uint64_t recvBuf[MAX_PROFILE_DATA-2]={0,};
 
//    uint64_t profile[MAX_PROFILE_DATA];
//    profile = ptr_cd_->GetLocalAvg();
    ptr_cd_->GetLocalAvg();


    cout << "Collect Profile -------------------\n" << endl;
    //getchar();
    

    if( IsMaster() == false) {
      for(int i=0; i<MAX_PROFILE_DATA-2; ++i) {
        recvBuf[i] = (ptr_cd_->profile_data_)[i+1];
      }
    }

    MPI_Reduce(&sendBuf, &recvBuf, MAX_PROFILE_DATA-2, MPI_INTEGER, MPI_SUM, master_, node_id_.color_);

    if(IsMaster()) {  // Master gets Avg.
      for(int i=1; i<MAX_PROFILE_DATA-1; ++i) {
         profile_data[i] = (uint64_t)((double)recvBuf[i-1] / node_id_.size_);
      }
    }
*/
/*  
    MPI_Bcast();  // sends avg val to every task in the current CD
      // Get dist. from each task
    MPI_Reduce(); // aggregate dist. to Master
  
    if(IsMaster()) {  // Master gets Standard dist.
      
    }
*/
    
    SetCollectProfile(false); // initialize this flag for the next Begin
  }
//  else {
//    // Aggregate profile locally
//    GetLocalProfile();
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

//bool CDHandle::CheckCollectProfile(void)
//{
//  return ptr_cd_->CheckCollectProfile();
//}
//
//void CDHandle::SetCollectProfile(bool flag)
//{
//  ptr_cd_->SetCollectProfile(flag);
//}

CDErrT CDHandle::Preserve ( void *data_ptr, 
                            uint64_t len, 
                            uint32_t preserve_mask, 
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
  this->GetPrvData(data_ptr, len, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
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
                            uint32_t preserve_mask, 
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
    return ptr_cd_->name_;  
  else
    assert(0);
}

//CDHandle* CDHandle::GetParent()
//{
//  if(ptr_cd_ != NULL) {
//    //cout<<"level : " <<ptr_cd_->GetCDID().level_<<endl;
//    //getchar();
//    
//    return CDPath.at(ptr_cd_->GetCDID().level_ - 1);
//  }
//  else {
//    cout<< "ERROR in GetParent" <<endl;
//    assert(0);
//  }
//}


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



#if _PROFILER

void CDHandle::InitProfile(std::string label)
{
  sibling_id_ = 0;
  level_      = 0;
  label_.first      = label;
  this_point_ = 0;
  that_point_ = 0;
  collect_profile_   = false;
#if _ENABLE_HIERGRAPH || _ENABLE_MODULE
  usr_profile_enable = false;
#endif
  is_child_destroyed = false;
  profile_data_.clear();
}

void CDHandle::InitViz()
{

//  cout<< "\n---------------- Initialize Visualization --------------\n" <<endl;
  //getchar();
  
//  if(!CDPath.empty()){
//    cout<<"\nMasterCD  "<< GetCDID().level_ <<" : ("<< GetRootCD()->GetNodeID() <<", "<< GetRootCD()->GetTaskID() <<")"<<endl;
//    getchar();
//  }
//  else {
//    cout<<"\nMasterCD  "<< GetCDID().level_ <<" : ("<< 0 <<", "<< 0 <<")"<<endl;
//    getchar();
//  }

  //SightInit(txt()<<"CDs", txt()<<"dbg_CDs_"<<GetCDID().node_id_.color_<<"_"<< GetCDID().node_id_.task_ );
  SightInit(txt()<<"CDs", txt()<<"dbg_CDs_"<<myTaskID );

  /// Title
  dbg << "<h1>Containment Domains Profiling and Visualization</h1>" << endl;

  /// Explanation
#if _ENABLE_ATTR
  attrValStack.push_back(new attr("INIT", 1));
  attrScope = new attrIf(new attrEQ("INIT", 1));
#endif
  dbg << "Some explanation on CD runtime "<<endl<<endl;

  
  /// Create modularApp and graph. Those objects should be unique in the program.
//  root_cd->ptr_cd()->InitViz();

#if _ENABLE_MODULE
  /// modularApp exists only one, and is created at init stage
  ma = new modularApp("CD Modular App");
#endif

#if _ENABLE_HIERGRAPH
  /// modularApp exists only one, and is created at init stage
  hga = new hierGraphApp("CD Hierarchical Graph App");
#endif

#if _ENABLE_SCOPE
#if _ENABLE_GRAPH
  /// graph exists only one. It is created at init stage
  scopeGraph = new graph();
#endif
#endif

}

void CDHandle::FinalizeViz(void)
{
//  cout<< "\n---------------- Finalize Visualization --------------\n" <<endl;

#if _ENABLE_SCOPE
#if _ENABLE_GRAPH
    assert(scopeGraph);
    delete scopeGraph;
#endif

  assert(attrScope);
  delete attrScope;

  assert(attrValStack.size()>0);
  assert(attrValStack.back() != NULL);
  delete attrValStack.back();
  attrValStack.pop_back();
#endif

#if _ENABLE_HIERGRAPH
    assert(hga);
    delete hga;
#endif

#if _ENABLE_MODULE
    assert(ma);
    delete ma;
#endif



}


void CDHandle::StartProfile(void)
{

//  cout<< "\n\t-------- Start Profile --------\n" <<endl;
// Profile starts -- COMP | MODULE | SCOPE | CDNODE -- 
  /// Timer on
  this->this_point_ = rdtsc();

#if _ENABLE_ATTR  
  CreateAttr();
#endif

#if _ENABLE_COMP   
  CreateComparison();
#endif

#if _ENABLE_MODULE 
  CreateModule();
#endif

#if _ENABLE_HIERGRAPH 
  CreateHierGraph();
#endif

#if _ENABLE_SCOPE  
  CreateScope();
#endif

#if _ENABLE_CDNODE 
  CreateCDNode();
#endif


}



void CDHandle::FinishProfile(void)
{

  //cout<< "\n\t-------- Finish Profile --------\n" <<endl;
  //getchar();

// Profile starts -- COMP | MODULE | SCOPE | CDNODE -- 

  // outputs the preservation / detection info
  /// Timer off
  uint64_t tmp_point = rdtsc();
  that_point_ = tmp_point;

  /// Loop Count (# of seq. CDs) + 1
//  (this->profile_data_)[label_.first][LOOP_COUNT] += 1;
  (profile_data_)[label_.first][LOOP_COUNT] = ptr_cd_->GetCDID().sequential_id_;

  /// Calcualate the execution time
  (profile_data_)[label_.first][EXEC_CYCLE] += (that_point_) - (this_point_);

#if _ENABLE_CDNODE 
  DestroyCDNode();
#endif

#if _ENABLE_SCOPE  
  DestroyScope();
#endif

#if _ENABLE_HIERGRAPH 
  DestroyHierGraph();
#endif

#if _ENABLE_MODULE 
//  cout<<"Destroy Module"<<endl; getchar();
  DestroyModule();
#endif

#if _ENABLE_COMP   
  DestroyComparison();
#endif

#if _ENABLE_ATTR  
  DestroyAttr();
#endif
}


bool CDHandle::CheckCollectProfile(void)
{
  return collect_profile_;
}

void CDHandle::SetCollectProfile(bool flag)
{
  collect_profile_ = flag;
}



void CDHandle::GetPrvData(void *data, 
                          uint64_t len_in_bytes,
                          uint32_t preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject * regen_object, 
                          PreserveUseT data_usage)
{
  if(preserve_mask == kCopy){

    profile_data_[label_.first][PRV_COPY_DATA] += len_in_bytes;
//    if( (this->parent_ != nullptr) && check_overlap(this->parent_, ref_name) ){
      /// Sequential overlapped accumulated data. It will be calculated to OVERLAPPED_DATA/PRV_COPY_DATA
      /// overlapped data: overlapped data that is preserved only via copy method.
//      profile_data_[label_.first][OVERLAPPED_DATA] += len_in_bytes;
//      cout<<"something is weird  "<<"level is "<<this->this_cd_->level_ << "length : "<<len_in_bytes<<endl;

  } else if(preserve_mask == kReference) {

    profile_data_[label_.first][PRV_REF_DATA] += len_in_bytes;

  } else {
                                                                                                                                  
    cout<<"prvTy is not supported currently : "<< preserve_mask<<endl;          
//    exit(-1);

  }
}


void CDHandle::GetLocalAvg(void)
{
  cout<<"Master CD Get Local Avg"<<endl;
//  for(int i=1; i < MAX_PROFILE_DATA-1; ++i) {
//    profile_data_[label_.first][i] /= profile_data_[label_.first][LOOP_COUNT];
//  }
}

#if _ENABLE_HIERGRAPH || _ENABLE_MODULE
void CDHandle::SetUsrProfileInput(std::pair<std::string, long> name_list)
{
  AddUsrProfile(name_list.first, name_list.second, 0);
}

void CDHandle::SetUsrProfileInput(std::initializer_list<std::pair<std::string, long>> name_list)
{
  for(auto il = name_list.begin() ;
      il != name_list.end(); ++il) {
    AddUsrProfile(il->first, il->second, 0);
  }
}

void CDHandle::SetUsrProfileOutput(std::pair<std::string, long> name_list)
{
  AddUsrProfile(name_list.first, name_list.second, 1);
}

void CDHandle::SetUsrProfileOutput(std::initializer_list<std::pair<std::string, long>> name_list)
{
  for(auto il = name_list.begin() ;
      il != name_list.end(); ++il) {
  
    AddUsrProfile(il->first, il->second, 1);
  }
}

void CDHandle::AddUsrProfile(std::string key, long val, int mode)
{
  if(mode == 0) {
    usr_profile_input.add(key, val);
  }
  else if(mode == 1){
    usr_profile_output.add(key, val);
  }
  usr_profile_enable = true;
}
#endif
// -------------- CD Node -----------------------------------------------------------------------------

#if _ENABLE_CDNODE
inline void CDHandle::CreateCDNode()
{
//  CDNode* cdn = new CDNode(txt()<<label_.first, txt()<<GetCDID()); 
//  this->cdStack.push_back(cdn);
//  dbg << "{{{ CDNode Test -- "<<this->this_cd_->cd_id_<<", #cdStack="<<cdStack.size()<<endl;
}

inline void CDHandle::DestroyCDNode()
{
//  if(cdStack.back() != nullptr){
//    cout<<"add new info"<<endl;
///*
//    cdStack.back()->setStageNode( "preserve", "data_copy", profile_data_[label_.first]PRV_COPY_DATA]    );
//    cdStack.back()->setStageNode( "preserve", "data_overlapped", profile_data_[label_.first][OVERLAPPED_DATA]  );
//    cdStack.back()->setStageNode( "preserve", "data_ref", profile_data_[label_.first][PRV_REF_DATA]     );
//*/
//    
//    //scope s("Preserved Stats");
//    PreserveStageNode psn(txt()<<"Preserve Stage");
//    dbg << "data_copy="<<profile_data_[label_.first][PRV_COPY_DATA]<<endl;
//    dbg << "data_overlapped="<<profile_data_[label_.first][OVERLAPPED_DATA]<<endl;
//    dbg << "data_ref="<<profile_data_[label_.first][PRV_REF_DATA]<<endl;
//
//  }

//  dbg << " }}} CDNode Test -- "<<this->this_cd_->cd_id_<<", #cdStack="<<cdStack.size()<<endl;
//  assert(cdStack.size()>0);
//  assert(cdStack.back() != NULL);
//  delete cdStack.back();
//  cdStack.pop_back();
}
#endif

// -------------- Scope -------------------------------------------------------------------------------

#if _ENABLE_SCOPE
inline void CDHandle::CreateScope()
{

//#if _ENABLE_ATTR
//  attrValStack.push_back(new attr(label_.first, label_.second));
//  attrStack.push_back(new attrAnd(new attrEQ(label_.first, 1)));
////   attrStack.push_back(new attrAnd(new attrEQ("txtOn", 1)));
//#endif
  
  /// create a new scope at each Begin() call
  scope* s = new scope(txt()<<label_.first<<", cd_id="<<node_id_.color_);

#if _ENABLE_GRAPH
  /// Connect edge between previous node to newly created node
  if(this->sStack.size()>0)
    this->scopeGraph->addDirEdge(this->sStack.back()->getAnchor(), s->getAnchor());
#endif
  /// push back this node into sStack to manage scopes
  this->sStack.push_back(s);
//  dbg << "<<< Scope  Test -- "<<this->this_cd_->cd_id_<<", #sStack="<<sStack.size()<<endl;
}


inline void CDHandle::DestroyScope()
{
//  dbg << " >>> Scope  Test -- "<<this->this_cd_->cd_id_<<", #sStack="<<sStack.size()<<endl;
  assert(sStack.size()>0);
  assert(sStack.back() != NULL);
  delete sStack.back();
  sStack.pop_back();

//  assert(attrStack.size()>0);
//  assert(attrStack.back() != NULL);
//  delete attrStack.back();
//  attrStack.pop_back();
//
//  assert(attrValStack.size()>0);
//  assert(attrValStack.back() != NULL);
//  delete attrValStack.back();
//  attrValStack.pop_back();
}
#endif

// -------------- Module ------------------------------------------------------------------------------
#if _ENABLE_MODULE
inline void CDHandle::CreateModule()
{
//  cout<<"CreateModule call"<<endl;
  if(usr_profile_enable==false) {
//    cout<<11111<<endl<<endl; getchar();
    module* m = new module( instance(txt()<<ptr_cd_->name_, 1, 1), 
                            inputs(port(context("cd_id", (int)node_id_.color_, 
                                                "sequential_id", (int)(ptr_cd_->GetCDID().sequential_id_),
                                                "label", label_.first,
                                                "processID", (int)node_id_.task_))),
                            namedMeasures("time", new timeMeasure()) );
    mStack.push_back(m);
  }
  else {
  
//    cout<<22222<<endl<<endl; getchar();
    module* m = new module( instance(txt()<<label_.first<<"_"<<node_id_.color_, 2, 2), 
                            inputs(port(context("cd_id", txt()<<node_id_.task_, 
                                                "sequential_id", (int)(ptr_cd_->GetCDID().sequential_id_))),
                                   port(usr_profile_input)),
                            namedMeasures("time", new timeMeasure()) );
    this->mStack.push_back(m);
  }

//  dbg << "[[[ Module Test -- "<<this->this_cd_->cd_id_<<", #mStack="<<mStack.size()<<endl;
}


inline void CDHandle::DestroyModule()
{
//  cout<<"DestroyModule call"<<endl;
//  dbg << " ]]] Module Test -- "<<this->this_cd_->cd_id_<<", #mStack="<<mStack.size()<<endl;
  assert(mStack.size()>0);
  assert(mStack.back() != NULL);
  mStack.back()->setOutCtxt(0, context("data_copy=", (long)(profile_data_[label_.first][PRV_COPY_DATA]),
                                       "data_overlapped=", (long)(profile_data_[label_.first][OVERLAPPED_DATA]),
                                       "data_ref=" , (long)(profile_data_[label_.first][PRV_REF_DATA])));
  if(usr_profile_enable) {
    mStack.back()->setOutCtxt(1, usr_profile_output);
  }
/*
  mStack.back()->setOutCtxt(1, context("sequential id =" , (long)profile_data_[label_.first][PRV_REF_DATA],
                                       "execution cycle=", (long)profile_data_[label_.first][PRV_COPY_DATA],
                                       "estimated error rate=", (long)profile_data_[label_.first][OVERLAPPED_DATA]));
*/
  delete mStack.back();
  mStack.pop_back();
}
#endif

// -------------- Comparison --------------------------------------------------------------------------
#if _ENABLE_COMP
inline void CDHandle::CreateComparison()
{
//  compKeyStack.push_back(node_id_.color_);

   
  compTagStack.push_back(new comparison(txt()<<node_id_.color_));
//  comparison* comp = new comparison(node_id_.color_);
//  compStack.push_back(comp);

  //BeginComp();
//  for(vector<int>::const_iterator k=compKeyStack.begin(); 
//      k!=compKeyStack.end(); k++) {
//    compTagStack.push_back(new comparison(*k));
//  }
}

inline void CDHandle::DestroyComparison()
{
  //CompleteComp();
//  for(vector<comparison*>::reverse_iterator c=compTagStack.rbegin(); 
//      c!=compTagStack.rend(); c++) {
//    delete *c;
//  }

  assert(compTagStack.size()>0);
  assert(compTagStack.back() != NULL);
  delete compTagStack.back();
  compTagStack.pop_back();


}
#endif


int hierGraphID=0;
int verID=10;
// -------------- HierGraph ---------------------------------------------------------------------------
#if _ENABLE_HIERGRAPH
inline void CDHandle::CreateHierGraph()
{
//  cout<<"CreateHierGraph call"<<endl;
  if(usr_profile_enable==false) {
    cout<< "Default hierGraph visualization. input#: 1, output#: 1\n"<<endl; //getchar();
    hierGraph* hg = new hierGraph( HG_instance(txt()<<label_.first, 1, 1, hierGraphID, (int)node_id_.color_, verID), 
                                   HG_inputs(HG_port(HG_context("cd_id", (int)node_id_.color_, 
                                                                "sequential_id", (int)(ptr_cd_->GetCDID().sequential_id_),
                                                                "label", label_.first,
                                                                "processID", (int)node_id_.task_))),
                               namedMeasures("time", new timeMeasure()) );
    hgStack.push_back(hg);
  }
  else {
  
    cout<< "User-specified hierGraph visualization. input#: 2, output#: 2\n"<<endl; //getchar();
    hierGraph* hg = new hierGraph( HG_instance(txt()<<label_.first, 2, 2, hierGraphID, (int)node_id_.color_, verID), 
                                   HG_inputs(HG_port(HG_context("cd_id", txt()<<node_id_.task_, 
                                                                "sequential_id", (int)(ptr_cd_->GetCDID().sequential_id_))),
                                             HG_port(usr_profile_input)),
                                   namedMeasures("time", new timeMeasure()) );
    hgStack.push_back(hg);
  }

   hierGraphID+=1;
}


inline void CDHandle::DestroyHierGraph()
{
//  cout<<"DestroyHierGraph call"<<endl;
  assert(hgStack.size()>0);
  assert(hgStack.back() != NULL);
  hgStack.back()->setOutCtxt(0, HG_context("data_copy=", (long)(profile_data_[label_.first][PRV_COPY_DATA]),
                                        "data_overlapped=", (long)(profile_data_[label_.first][OVERLAPPED_DATA]),
                                        "data_ref=" , (long)(profile_data_[label_.first][PRV_REF_DATA])));
  if(usr_profile_enable) {
    hgStack.back()->setOutCtxt(1, usr_profile_output);
  }
/*
  hgStack.back()->setOutCtxt(1, HG_context("sequential id =" , (long)profile_data_[label_.first][PRV_REF_DATA],
                                       "execution cycle=", (long)profile_data_[label_.first][PRV_COPY_DATA],
                                       "estimated error rate=", (long)profile_data_[label_.first][OVERLAPPED_DATA]));
*/
  delete hgStack.back();
  hgStack.pop_back();
}
#endif


#if _ENABLE_ATTR  

inline void CDHandle::CreateAttr()
{
  
  attrValStack.push_back(new attr(label_.first, label_.second));
  attrStack.push_back(new attrAnd(new attrEQ(label_.first, 1)));
//   attrStack.push_back(new attrAnd(new attrEQ("txtOn", 1)));

}


inline void CDHandle::DestroyAttr()
{
  assert(attrStack.size()>0);
  assert(attrStack.back() != NULL);
  delete attrStack.back();
  attrStack.pop_back();

  assert(attrValStack.size()>0);
  assert(attrValStack.back() != NULL);
  delete attrValStack.back();
  attrValStack.pop_back();
}

#endif

#endif // _PROFILER ends 
