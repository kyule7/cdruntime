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

#include <sys/stat.h>
#include "cd_config.h"
#include "cd_features.h"
#include "cd_handle.h"
#include "cd_def_internal.h"
#include "cd_path.h"


#include "cd_features.h"
//#include "cd_global.h"
#include "node_id.h"
#include "sys_err_t.h"
#include "cd_internal.h"
//#include "profiler_interface.h"

#if CD_PROFILER_ENABLED
#include "cd_profiler.h"
#endif

//#include "error_injector.h"


using namespace cd;
using namespace cd::interface;
using namespace cd::internal;
using namespace std;



/// KL
/// cddbg is a global variable to be used for debugging purpose.
/// General usage is that it stores any strings, numbers as a string internally,
/// and when cddbg.flush() is called, it write the internally stored strings to file,
/// then clean up the internal storage.
#if CD_DEBUG_ENABLED
cd::DebugBuf cd::cddbg;
#endif

#if CD_DEBUG_DEST == CD_DEBUG_TO_FILE  // Print to fileout -----
FILE *cdout=NULL;
FILE *cdoutApp=NULL;
#endif

#if CD_ERROR_INJECTION_ENABLED
#define RANDOM_SEED 17
MemoryErrorInjector *CDHandle::memory_error_injector_ = NULL;
SystemErrorInjector *CDHandle::system_error_injector_ = NULL;
#define CHECK_SYS_ERR_VEC(X,Y) \
  (((X) & (Y)) == (X))
#endif

clock_t cd::tot_begin_clk;
clock_t cd::tot_end_clk;
clock_t cd::begin_clk;
clock_t cd::end_clk;
clock_t cd::elapsed_time;

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
#if CD_MPI_ENABLED
MPI_Group cd::whole_group;
int  cd::max_tag_bit = 0;
int  cd::max_tag_level_bit = 0;
int  cd::max_tag_task_bit  = 0;
int  cd::max_tag_rank_bit  = 0;
#endif
int cd::myTaskID = 0;
int cd::totalTaskSize = 1;

//cd::SystemConfig cd::CDHandle::system_config_;
namespace cd {
// Global functions -------------------------------------------------------

//void LoadSystemConfig(SystemConfig &system_config)
//{
//  system_config[ERROR_TYPE_0]  = ERROR_RATE_TYPE_0;
//  system_config[ERROR_TYPE_1]  = ERROR_RATE_TYPE_1;
//  system_config[ERROR_TYPE_2]  = ERROR_RATE_TYPE_2;
//  system_config[ERROR_TYPE_3]  = ERROR_RATE_TYPE_3;
//  system_config[ERROR_TYPE_4]  = ERROR_RATE_TYPE_4;
//  system_config[ERROR_TYPE_5]  = ERROR_RATE_TYPE_5;
//  system_config[ERROR_TYPE_6]  = ERROR_RATE_TYPE_6;
//  system_config[ERROR_TYPE_7]  = ERROR_RATE_TYPE_7;
//}

/// KL
CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium)
{
  //GONG
  CDPrologue();
  cd::tot_begin_clk = clock();
  myTaskID      = myTask;
  totalTaskSize = numTask;

  cd::system_config.LoadSystemConfig();

  string dbg_basepath(CD_DEFAULT_DEBUG_OUT);
#if CD_DEBUG_DEST == CD_DEBUG_TO_FILE
  char *filepath = getenv( "CD_DEBUG_OUT" );
  char dbg_log_filename[] = "cddbg_log_rank_";

  if(filepath != NULL) {
    // Overwrite filepath with CD_DEBUG_OUT value
    dbg_basepath = filepath;
  }

  if(myTaskID == 0) {
    struct stat sb;
    if (stat(dbg_basepath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
//      printf("Path exists!\n");
    }
    else {
      char debug_dir[256];
      sprintf(debug_dir, "mkdir -p %s", dbg_basepath.c_str());
//      printf("debug dir path size : %d\n", (int)sizeof(debug_dir));
      if( system(debug_dir) == -1 )
        ERROR_MESSAGE("ERROR: Failed to create directory for debug info. %s\n", debug_dir);
    }
  }
  
#if CD_MPI_ENABLED 
  // Synchronization is needed. Otherwise, some task may execute CD_DEBUG before head creates directory 
  PMPI_Barrier(MPI_COMM_WORLD);
#endif

  char dbg_filepath[256]={};
  snprintf(dbg_filepath, 256, "%s%s%d", dbg_basepath.c_str(), dbg_log_filename, myTaskID);
//  printf("dbg filepath : %s\n", dbg_filepath);
//  dbg_basepath = dbg_basepath + log_filename + to_string(static_cast<unsigned long long>(myTaskID));

  cdout = fopen(dbg_filepath, "w");
 
#endif

#if CD_DEBUG_ENABLED
  char app_dbg_log_filename[] = "cddbg_app_output_";
  char app_dbg_filepath[256]={};
  snprintf(app_dbg_filepath, 256, "%s%s%d", dbg_basepath.c_str(), app_dbg_log_filename, myTaskID);
//  printf("app dbg filepath : %s\n", app_dbg_filepath);
  cddbg.open(app_dbg_filepath);
#endif

  cd::internal::Initialize();

  // Base filepath setup for preservation
  char *filepath_env = getenv("CD_PRV_BASEPATH");
  if(filepath_env != NULL) {
    FilePath::prv_basePath_ = filepath_env;
  }

//  if(myTaskID == 0) {
//    struct stat sb;
//    if (stat(prv_basePath_.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
//      printf("Prv Path exists!\n");
//    }
//    else {
//      char prv_file_dir[256];
//      sprintf(prv_file_dir, "mkdir -p %s", prv_basePath_.c_str());
//      printf("preservation file path size : %d\n", (int)sizeof(prv_file_dir));
//      if( system(prv_file_dir) == -1 )
//        ERROR_MESSAGE("ERROR: Failed to create directory for debug info. %s\n", prv_file_dir);
//    }
//  }

  NodeID new_node_id = NodeID(ROOT_COLOR, myTask, ROOT_HEAD_ID, numTask);
#if CD_MPI_ENABLED 
  PMPI_Comm_group(MPI_COMM_WORLD, &whole_group); 
  new_node_id = CDHandle::GenNewNodeID(0, new_node_id);
#endif
  CD::CDInternalErrT internal_err;
  CDHandle *root_cd_handle = CD::CreateRootCD("Root", CDID(CDNameT(0), new_node_id), 
                                              static_cast<CDType>(kStrict | prv_medium), 
                                              FilePath::prv_basePath_, 
                                              ROOT_SYS_DETECT_VEC, &internal_err);

  CDPath::GetCDPath()->push_back(root_cd_handle);

#if CD_PROFILER_ENABLED
  // Profiler-related
  root_cd_handle->profiler_->InitViz();
#endif

#if CD_ERROR_INJECTION_ENABLED
  double random_seed = 0.0;
  if(myTaskID == 0) {
    random_seed = clock();
    CD_DEBUG("Random seed: %lf\n", random_seed);
  }
//  random_seed = 137378;
  PMPI_Bcast(&random_seed, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  srand48(random_seed * (double)(RANDOM_SEED + myTaskID));
  // To be safe
  CDHandle::memory_error_injector_ = NULL;
  CDHandle::system_error_injector_ = new SystemErrorInjector(system_config);
#endif

#if CD_DEBUG_ENABLED
  cddbg.flush();
#endif

  //GONG
  CDEpilogue();

  return root_cd_handle;
}

inline void WriteDbgStream(void)
{
#if CD_DEBUG_DEST == CD_DEBUG_TO_FILE
  fclose(cdout);
#endif

#if CD_DEBUG_ENABLED
  cddbg.flush();
  cddbg.close();
#endif
}

void CD_Finalize(void)
{
  //GONG
  CDPrologue();

  CD_DEBUG("========================= Finalize [%d] ===============================\n", myTaskID);

  assert(CDPath::GetCDPath()->size()==1); // There should be only on CD which is root CD
  assert(CDPath::GetCDPath()->back()!=NULL);


#if CD_PROFILER_ENABLED
  // Profiler-related  
  CDPath::GetRootCD()->profiler_->FinalizeViz();
#endif
  CDPath::GetRootCD()->InternalDestroy(false);

#if CD_ERROR_INJECTION_ENABLED
  if(CDHandle::memory_error_injector_ != NULL);
    delete CDHandle::memory_error_injector_;

  if(CDHandle::system_error_injector_ != NULL);
    delete CDHandle::system_error_injector_;
#endif

  cd::internal::Finalize();
  cd::tot_end_clk = clock();

#if CD_MPI_ENABLED
  double cd_elapsed = ((double)cd::elapsed_time) / CLOCKS_PER_SEC;
  double msg_elapsed = ((double)cd::msg_elapsed_time) / CLOCKS_PER_SEC;
  double log_elapsed = ((double)cd::log_elapsed_time) / CLOCKS_PER_SEC;
  double tot_elapsed= ((double)(cd::tot_end_clk - cd::tot_begin_clk)) / CLOCKS_PER_SEC;
  double sendbuf[8] = {tot_elapsed, 
                       tot_elapsed * tot_elapsed,
                       cd_elapsed,
                       cd_elapsed  * cd_elapsed,
                       msg_elapsed,
                       msg_elapsed  * msg_elapsed,
                       log_elapsed,
                       log_elapsed  * log_elapsed
                      };
//  printf("\n\n=====================================\n");
//  printf("%lf\t%lf\t%lf\t%lf\n", sendbuf[0],sendbuf[1],sendbuf[2],sendbuf[3]);
//  printf("=====================================\n\n");
  double recvbuf[8] = {0.0,0.0,0.0,0.0,0.0,0.0};
  MPI_Reduce(sendbuf, recvbuf, 8, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if(cd::myTaskID == 0) {
//  printf("\n\n=====================================\n");
//  printf("%lf\t%lf\t%lf\t%lf\n", recvbuf[0],recvbuf[1],recvbuf[2],recvbuf[3]);
//  printf("=====================================\n\n");
    printf("\n\n============================================\n");
    printf("Total elapsed time : %lf (%lf) (var: %lf)\n", recvbuf[0]/cd::totalTaskSize, 
                                                            sendbuf[0], 
                                                            (recvbuf[1] - recvbuf[0]*recvbuf[0]/cd::totalTaskSize)/cd::totalTaskSize);
    printf("CD overhead time : %lf (%lf) (var: %lf)\n", recvbuf[2]/cd::totalTaskSize, 
                                                          sendbuf[2], 
                                                          (recvbuf[3] - recvbuf[2]*recvbuf[2]/cd::totalTaskSize)/cd::totalTaskSize);
    printf("Msg overhead time : %lf (%lf) (var: %lf)\n", recvbuf[4]/cd::totalTaskSize, 
                                                          sendbuf[4], 
                                                          (recvbuf[5] - recvbuf[4]*recvbuf[4]/cd::totalTaskSize)/cd::totalTaskSize);
    printf("Log overhead time : %lf (%lf) (var: %lf)\n", recvbuf[6]/cd::totalTaskSize, 
                                                          sendbuf[6], 
                                                          (recvbuf[7] - recvbuf[6]*recvbuf[6]/cd::totalTaskSize)/cd::totalTaskSize);
    printf("Ratio : %lf (Total) %lf (CD runtime) %lf (logging)\n", 
                            ((recvbuf[2]+recvbuf[4]) / recvbuf[0]) * 100,
                             (recvbuf[2] / recvbuf[0]) * 100, 
                             (recvbuf[4] / recvbuf[0]) * 100);

#if CD_DEBUG_ENABLED && CD_PROFILER_ENABLED 
    printf("Profile Result =================================\n");
    printf("%s\n", Profiler::GetTotalInfo().GetString().c_str());
    printf("================================================\n\n");
#endif
  }
#endif

#if CD_DEBUG_ENABLED
  WriteDbgStream();
#endif
  CDEpilogue();
}


CDHandle *GetCurrentCD(void) 
{ return CDPath::GetCurrentCD(); }

CDHandle *GetLeafCD(void)
{ return CDPath::GetLeafCD(); }
  
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
  : ptr_cd_(0), node_id_(-1), ctxt_(CDPath::GetRootCD()->ctxt_)
{
  // FIXME
  assert(0);
  SplitCD = &SplitCD_3D;

#if CD_PROFILER_ENABLED
  profiler_ = Profiler::CreateProfiler(0, this);
#endif

#if CD_ERROR_INJECTION_ENABLED
  cd_error_injector_ = NULL;
#endif


//  if(node_id().size() > 1)
//    PMPI_Win_create(pendingFlag_, 1, sizeof(CDFlagT), PMPI_INFO_NULL, PMPI_COMM_WORLD, &pendingWindow_);
//  else
//    cddbg << "KL : size is 1" << endl;
  
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
CDHandle::CDHandle(CD *ptr_cd, const NodeID& node_id) 
  : ptr_cd_(ptr_cd), node_id_(node_id), ctxt_(ptr_cd->ctxt_)
{
  SplitCD = &SplitCD_3D;

#if CD_PROFILER_ENABLED
  profiler_ = Profiler::CreateProfiler(0, this);
#endif

#if CD_ERROR_INJECTION_ENABLED
  cd_error_injector_ = NULL;
#endif

//  if(node_id_.size() > 1)
//    PMPI_Win_create(pendingFlag_, 1, sizeof(CDFlagT), PMPI_INFO_NULL, PMPI_COMM_WORLD, &pendingWindow_);
//  else
//    cddbg << "KL : size is 1" << endl;
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

#if CD_ERROR_INJECTION_ENABLED
  if(cd_error_injector_ != NULL);
    delete cd_error_injector_;
#endif

#if CD_PROFILER_ENABLED
  if(profiler_ != NULL);
    delete profiler_;
#endif

}

void CDHandle::Init(CD *ptr_cd, const NodeID& node_id)
{ 
  ptr_cd_   = ptr_cd;
  node_id_  = node_id;
}

// Non-collective
CDHandle *CDHandle::Create(const char *name, 
                           int cd_type, 
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
  
  NodeID new_node_id = GenNewNodeID(0, node_id_);


  // Generate CDID
  CDNameT new_cd_name(ptr_cd_->GetCDName(), 1, 0);
  CD_DEBUG("New CD Name : %s\n", new_cd_name.GetString().c_str());

  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CD::CDInternalErrT internal_err;
  CDHandle *new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), static_cast<CDType>(cd_type), sys_bit_vec, &internal_err);

  CDPath::GetCDPath()->push_back(new_cd_handle);
  
  //GONG
  CDEpilogue();

  return new_cd_handle;
}



CDErrT CDHandle::RegisterSplitMethod(SplitFuncT split_func)
{ 
  SplitCD = split_func; 
  return kOK;
}


NodeID CDHandle::GenNewNodeID(const int &new_head, const NodeID &node_id)
{
  // just set the same as parent.
  NodeID new_node_id(node_id);
#if CD_MPI_ENABLED
  PMPI_Comm_dup(node_id.color_, &(new_node_id.color_));
  PMPI_Comm_group(new_node_id.color_, &(new_node_id.task_group_));
#endif
  new_node_id.set_head(new_head);
  //new_node.init_node_id(node_id_.color(), 0, INVALID_HEAD_ID,1);
  return new_node_id;
}




// Collective
CDHandle *CDHandle::Create(uint32_t  num_children,
                           const char *name, 
                           int cd_type, 
                           uint32_t error_name_mask, 
                           uint32_t error_loc_mask, 
                           CDErrT *error)
{
  CDPrologue();
#if CD_MPI_ENABLED

  CD_DEBUG("CDHandle::Create Node ID : %s\n", node_id_.GetString().c_str());
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
  CD_DEBUG("check mode : %d at level %d\n", ptr_cd()->cd_exec_mode_,  ptr_cd()->level());

  //CheckMailBox();


//  Sync();
  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);
  int new_size = (int)((double)node_id_.size() / num_children);
  int new_color=0, new_task = 0;
  int err=0;

//  ColorT new_comm = 0;
//  NodeID new_node_id(new_comm, INVALID_TASK_ID, INVALID_HEAD_ID, new_size);
  NodeID new_node_id(node_id_);

//  CD_DEBUG("[Before] old: %s, new: %s\n", node_id_.GetString().c_str(), new_node_id.GetString().c_str());
 
  if(num_children > 1) {
    err = SplitCD(node_id_.task_in_color(), node_id_.size(), num_children, new_color, new_task);
    CD_DEBUG("[%s], GenNewNodeID\n", __func__);
    new_node_id = GenNewNodeID(node_id_.color(), new_color, new_task, 0);
    assert(new_size == new_node_id.size());
  }
  else if(num_children == 1) {
    new_node_id = GenNewNodeID(0, node_id_);
  }
  else {
    ERROR_MESSAGE("Number of children to create is wrong.\n");
  }

  // Generate CDID
  CD_DEBUG("new_color : %d in %s\n", new_color, node_id_.GetString().c_str());

  CDNameT new_cd_name(ptr_cd_->GetCDName(), num_children, new_color);
  CD_DEBUG("New CD Name : %s\n", new_cd_name.GetString().c_str());

  CD_DEBUG("Remote Entry Dir size: %lu", ptr_cd_->remote_entry_directory_map_.size());

  // Sync buffer with file to prevent important metadata 
  // or preservation file at buffer in parent level from being lost.
  ptr_cd_->SyncFile();

  // Make a superset of CD preservation entries
  CollectHeadInfoAndEntry(new_node_id); 

  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CD::CDInternalErrT internal_err;
  CDHandle *new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), static_cast<CDType>(cd_type), sys_bit_vec, &internal_err);


  CDPath::GetCDPath()->push_back(new_cd_handle);

  if(err<0) {
    ERROR_MESSAGE("CDHandle::Create failed.\n"); assert(0);
  }


#else

  CDHandle *new_cd_handle = Create(name, cd_type, error_name_mask, error_loc_mask, error );

#endif

  CDEpilogue();

  return new_cd_handle;
}


// Collective
CDHandle *CDHandle::Create(uint32_t color, 
                           uint32_t task_in_color, 
                           uint32_t num_children, 
                           const char *name, 
                           int cd_type, 
                           uint32_t error_name_mask, 
                           uint32_t error_loc_mask, 
                           CDErrT *error )
{
  CDPrologue();
#if CD_MPI_ENABLED

  uint64_t sys_bit_vec = SetSystemBitVector(error_name_mask, error_loc_mask);
  int err=0;

//  ColorT new_comm;
//  NodeID new_node_id(new_comm, INVALID_TASK_ID, INVALID_HEAD_ID, num_children);
  NodeID new_node_id = GenNewNodeID(node_id_.color(), color, task_in_color, 0);

  // Generate CDID
  CDNameT new_cd_name(ptr_cd_->GetCDName(), num_children, color);

  // Sync buffer with file to prevent important metadata 
  // or preservation file at buffer in parent level from being lost.
  ptr_cd_->SyncFile();

  // Make a superset
  CollectHeadInfoAndEntry(new_node_id); 

  // Then children CD get new MPI rank ID. (task ID) I think level&taskID should be also pair.
  CD::CDInternalErrT internal_err;
  CDHandle *new_cd_handle = ptr_cd_->Create(this, name, CDID(new_cd_name, new_node_id), static_cast<CDType>(cd_type), sys_bit_vec, &internal_err);

  CDPath::GetCDPath()->push_back(new_cd_handle);

  if(err<0) {
    ERROR_MESSAGE("CDHandle::Create failed.\n"); assert(0);
  }


#else
  CDHandle *new_cd_handle = Create(name, cd_type, error_name_mask, error_loc_mask, error );
#endif

  CDEpilogue();
  return new_cd_handle;
}


CDHandle *CDHandle::CreateAndBegin(uint32_t num_children, 
                                   const char *name, 
                                   int cd_type, 
                                   uint32_t error_name_mask, 
                                   uint32_t error_loc_mask, 
                                   CDErrT *error )
{
  CDPrologue();
  CDHandle *new_cdh = Create(num_children, name, static_cast<CDType>(cd_type), error_name_mask, error_loc_mask, error);
  new_cdh->Begin(false, name);
  CDEpilogue();
  return new_cdh;
}

CDErrT CDHandle::Destroy(bool collective) {
  CDPrologue();
  CD_DEBUG("[%s] %s %s at level %u (reexecInfo %d (%u))\n", __func__, ptr_cd_->name_.c_str(), ptr_cd_->name_.c_str(), 
                                                                      level(), need_reexec(), *CD::rollback_point_);
  CDErrT err = InternalDestroy(collective);
  CDEpilogue();
  return err;
}

CDErrT CDHandle::InternalDestroy(bool collective)
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

#if CD_PROFILER_ENABLED
  CD_DEBUG("Calling finish profiler\n");
  
  profiler_->Delete();

  //if(ptr_cd()->cd_exec_mode_ == 0) { 
//    profiler_->FinishProfile();
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


CDErrT CDHandle::Begin(bool collective, const char *label, const uint64_t &sys_error_vec)
{
  CDPrologue();

  assert(ptr_cd_ != 0);

  // sys_error_vec is zero, then do not update it in Begin.
  if(sys_error_vec != 0) 
    ptr_cd_->sys_detect_bit_vector_ = sys_error_vec;

  CDErrT err = ptr_cd_->Begin(collective, label);

#if CD_PROFILER_ENABLED
  // Profile-related
  profiler_->StartProfile();
#endif

  CDEpilogue();
  return err;
}

CDErrT CDHandle::Complete(bool collective, bool update_preservations)
{
  CDPrologue();
  CD_DEBUG("[%s] %s %s at level %u (reexecInfo %d (%u))\n", __func__, ptr_cd_->name_.c_str(), ptr_cd_->name_.c_str(), 
                                                                      level(), need_reexec(), *CD::rollback_point_);

  // Call internal Complete routine
  assert(ptr_cd_ != 0);

  // Profile will be acquired inside CD::Complete()
  CDErrT ret = ptr_cd_->Complete(collective);

#if CD_PROFILER_ENABLED
  // Profile-related
  profiler_->FinishProfile();
#endif

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

#if CD_PROFILER_ENABLED
  bool is_execution = (GetExecMode() == kExecution);
#endif
  /// Preserve meta-data
  /// Accumulated volume of data to be preserved for Sequential CDs. 
  /// It will be averaged out with the number of seq. CDs.
  assert(ptr_cd_ != 0);
  CDErrT err = ptr_cd_->Preserve(data_ptr, len, preserve_mask, 
                                 my_name, ref_name, ref_offset, 
                                 regen_object, data_usage);

#if CD_ERROR_INJECTION_ENABLED
  if(memory_error_injector_ != NULL) {
    memory_error_injector_->PushRange(data_ptr, len/sizeof(int), sizeof(int), my_name);
    memory_error_injector_->Inject();
  }
#endif

#if CD_PROFILER_ENABLED
  if(is_execution) {
    if(CHECK_PRV_TYPE(preserve_mask,kCopy)) {
      profiler_->RecordProfile(PRV_COPY_DATA, len);
    }
    else if(CHECK_PRV_TYPE(preserve_mask,kRef)) {
      profiler_->RecordProfile(PRV_REF_DATA, len);
    }
  }
#endif

  CDEpilogue();
  return err;
}

CDErrT CDHandle::Preserve(Serializable &serdes,                           
                          uint32_t preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject *regen_object, 
                          PreserveUseT data_usage)
{
  CDPrologue();
  
  /// Preserve meta-data
  /// Accumulated volume of data to be preserved for Sequential CDs. 
  /// It will be averaged out with the number of seq. CDs.
  assert(ptr_cd_ != 0); 
  uint64_t len = 0;
#if CD_PROFILER_ENABLED
  bool is_execution = (GetExecMode() == kExecution);
#endif
  CDErrT err = ptr_cd_->Preserve((void *)&serdes, len, kSerdes | preserve_mask, 
                                 my_name, ref_name, ref_offset, 
                                 regen_object, data_usage);

#if CD_PROFILER_ENABLED
  if(is_execution) {
//    printf("\nserialize len?? : %lu, check kSerdes : %d (%x)\n\n", len, CHECK_PRV_TYPE(preserve_mask, kSerdes), preserve_mask);
    if(len==0) getchar();
    if(CHECK_PRV_TYPE(preserve_mask,kCopy)) {
      profiler_->RecordProfile(PRV_COPY_DATA, len);
    }
    else if(CHECK_PRV_TYPE(preserve_mask,kRef)) {
      profiler_->RecordProfile(PRV_REF_DATA, len);
    }
//    assert(len);
  }
#endif
  
  CDEpilogue();
  return err;
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

  assert(ptr_cd_ != 0);
#if CD_PROFILER_ENABLED
  bool is_execution = (GetExecMode() == kExecution);
#endif

  // TODO CDEvent object need to be handled separately, 
  // this is essentially shared object among multiple nodes.
  CDErrT err = ptr_cd_->Preserve(data_ptr, len, preserve_mask, 
                              my_name, ref_name, ref_offset,  
                              regen_object, data_usage);

#if CD_ERROR_INJECTION_ENABLED
  if(memory_error_injector_ != NULL) {
    memory_error_injector_->PushRange(data_ptr, len/sizeof(int), sizeof(int), my_name);
    memory_error_injector_->Inject();
  }
#endif

#if CD_PROFILER_ENABLED
  if(is_execution) {
    if(CHECK_PRV_TYPE(preserve_mask,kCopy)) {
      profiler_->RecordProfile(PRV_COPY_DATA, len);
    }
    else if(CHECK_PRV_TYPE(preserve_mask,kRef)) {
      profiler_->RecordProfile(PRV_REF_DATA, len);
    }
  }
#endif
  CDEpilogue();
  return err;
}


char *CDHandle::GetName(void) const
{
  if(ptr_cd_ != NULL) {
    return const_cast<char*>(ptr_cd_->name_.c_str());  
  }
  else {
    return NULL;
  }
}

char *CDHandle::GetLabel(void) const
{
  if(ptr_cd_ != NULL) {
    return const_cast<char*>(ptr_cd_->label_.c_str());  
  }
  else {
    return NULL;
  }
}

CDID     &CDHandle::GetCDID(void)             { return ptr_cd_->GetCDID(); }
CDNameT  &CDHandle::GetCDName(void)           { return ptr_cd_->GetCDName(); }
NodeID   &CDHandle::node_id(void)             { return node_id_; }
CD       *CDHandle::ptr_cd(void)        const { return ptr_cd_; }
void      CDHandle::SetCD(CD *ptr_cd)         { ptr_cd_=ptr_cd; }
uint32_t  CDHandle::level(void)         const { return ptr_cd_->GetCDName().level(); }
uint32_t  CDHandle::rank_in_level(void) const { return ptr_cd_->GetCDName().rank_in_level(); }
uint32_t  CDHandle::sibling_num(void)   const { return ptr_cd_->GetCDName().size(); }
ColorT    CDHandle::color(void)         const { return node_id_.color(); }
int       CDHandle::task_in_color(void) const { return node_id_.task_in_color(); }
int       CDHandle::head(void)          const { return node_id_.head(); }
int       CDHandle::task_size(void)     const { return node_id_.size(); }
int       CDHandle::GetExecMode(void)   const { return ptr_cd_->cd_exec_mode_; }
int       CDHandle::GetSeqID(void)      const { return ptr_cd_->GetCDID().sequential_id(); }
CDHandle *CDHandle::GetParent(void)     const { return CDPath::GetParentCD(ptr_cd_->GetCDName().level()); }
bool      CDHandle::IsHead(void)        const { return node_id_.IsHead(); } // FIXME
// FIXME
// For now task_id_==0 is always Head which is not good!
// head is always task id 0 for now

bool CDHandle::operator==(const CDHandle &other) const 
{
  bool ptr_cd = (other.ptr_cd() == ptr_cd_);
  bool color  = (other.node_id_.color()  == node_id_.color());
  bool task   = (other.node_id_.task_in_color()   == node_id_.task_in_color());
  bool size   = (other.node_id_.size()   == node_id_.size());
  return (ptr_cd && color && task && size);
}

CDErrT CDHandle::Stop()
{ return ptr_cd_->Stop(); }

CDErrT CDHandle::AddChild(CDHandle *cd_child)
{
  CDErrT err=INITIAL_ERR_VAL;
  ptr_cd_->AddChild(cd_child);
  return err;
}

CDErrT CDHandle::RemoveChild(CDHandle *cd_child)  
{
  CDErrT err=INITIAL_ERR_VAL;
  ptr_cd_->RemoveChild(cd_child);
  return err;
}

CDErrT CDHandle::CDAssert (bool test, const SysErrT *error_to_report)
{
  CDPrologue();
  CD_DEBUG("Assert : %d at level %u\n", ptr_cd()->cd_exec_mode_, ptr_cd()->level());

  assert(ptr_cd_ != 0);
  CDErrT err = kOK;

#if CD_PROFILER_ENABLED
//    if(!test_true) {
//      profiler_->Delete();
//    }
#endif


#if CD_ERROR_INJECTION_ENABLED
  if(cd_error_injector_ != NULL) {
    if(cd_error_injector_->Inject()) {
      test = false;
      err = kAppError;
    }
  }
#endif

  CD::CDInternalErrT internal_err = ptr_cd_->Assert(test);
  if(internal_err == CD::CDInternalErrT::kErrorReported)
    err = kAppError;

  CDEpilogue();
  return err;
}

CDErrT CDHandle::CDAssertFail (bool test_true, const SysErrT *error_to_report)
{
  CDPrologue();
  if( IsHead() ) {

  }
  else {
    // It is at remote node so do something for that.
  }

  CDEpilogue();
  return kOK;
}

CDErrT CDHandle::CDAssertNotify(bool test_true, const SysErrT *error_to_report)
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

std::vector<SysErrT> CDHandle::Detect(CDErrT *err_ret_val)
{
  CDPrologue();
  CD_DEBUG("[%s] check mode : %d at %s %s level %u (reexecInfo %d (%u))\n", __func__, ptr_cd()->cd_exec_mode_, 
                                                          ptr_cd_->name_.c_str(), ptr_cd_->name_.c_str(), 
                                                          level(), need_reexec(), *CD::rollback_point_);

  std::vector<SysErrT> ret_prepare;
  CDErrT err = kOK;
  uint32_t rollback_point = INVALID_ROLLBACK_POINT;

  int err_desc = (int)ptr_cd_->Detect(rollback_point);
#if CD_ERROR_INJECTION_ENABLED
  err_desc = CheckErrorOccurred(rollback_point);
#endif

#if CD_MPI_ENABLED

  CD_DEBUG("[%s] rollback %u -> %u (%d == %d)\n", __func__, rollback_point, level(), err_desc, CD::CDInternalErrT::kErrorReported);
  if(err_desc == CD::CDInternalErrT::kErrorReported) {
    err = kError;
    // FIXME
//    if((unsigned)rollback_point == level()) 
//      CD::need_reexec = true;
//    else
//      CDPath::GetCDLevel(rollback_point)->SetMailBox(kErrorOccurred);

//    CD::need_reexec = true;
//    *CD::rollback_point_ = rollback_point;
    
    printf("### Error Injected. Rollback Level #%u (%s %s) ###\n", 
        rollback_point, ptr_cd_->cd_id_.GetStringID().c_str(), ptr_cd_->label_.c_str()); 

    CDHandle *rb_cdh = CDPath::GetCDLevel(rollback_point);
    assert(rb_cdh != NULL);
    if(rb_cdh->task_size() > 1) {
      rb_cdh->SetMailBox(kErrorOccurred);
    } 

    // If there is a single task in a CD, everybody is the head in that level.
    if(IsHead()) { 
      ptr_cd_->SetRollbackPoint(rollback_point, false);
    }
//      if(rollback_point < *CD::rollback_point_) {
//        CD::need_reexec = true;
//        *CD::rollback_point_ = rollback_point;
////        printf("*rollback_point_ %d\n", *CD::rollback_point_);
////        rb_cdh->SetMailBox(kErrorOccurred);
//      }
//      if(rollback_point != level())
//        CDPath::GetCDLevel(rollback_point)->SetMailBox(kErrorOccurred);

    err = kAppError;
    
//    Sync(CDPath::GetCoarseCD(this)->color());
//    CD_DEBUG("\n\n[Barrier] CDHandle::Detect 1 - %s / %s\n\n", ptr_cd_->GetCDName().GetString().c_str(), node_id_.GetString().c_str());

  }
  else {
#if CD_ERROR_INJECTION_ENABLED
/*
    CD_DEBUG("EIE Before\n");
    CD_DEBUG("Is it NULL? %p, recreated? %d, reexecuted? %d\n", cd_error_injector_, ptr_cd_->recreated(), ptr_cd_->reexecuted());
    if(cd_error_injector_ != NULL && ptr_cd_ != NULL) {
      CD_DEBUG("EIE It is after : reexec # : %d, exec mode : %d at level #%u\n", ptr_cd_->num_reexecution_, GetExecMode(), level());
      CD_DEBUG("recreated? %d, recreated? %d\n", ptr_cd_->recreated(), ptr_cd_->reexecuted());
      if(cd_error_injector_->Inject() && ptr_cd_->recreated() == false && ptr_cd_->reexecuted() == false) {
        CD_DEBUG("EIE Reached SetMailBox. recreated? %d, reexecuted? %d\n", ptr_cd_->recreated(), ptr_cd_->reexecuted());
        SetMailBox(kErrorOccurred);
        err = kAppError;

        
        PMPI_Win_fence(0, CDPath::GetCoarseCD(this)->ptr_cd()->mailbox_);
        CD_DEBUG("\n\n[Barrier] CDHandle::Detect 1 - %s / %s\n\n", ptr_cd_->GetCDName().GetString().c_str(), node_id_.GetString().c_str());

      }
      else {
        
        PMPI_Win_fence(0, CDPath::GetCoarseCD(this)->ptr_cd()->mailbox_);
        CD_DEBUG("\n\n[Barrier] CDHandle::Detect 1 - %s / %s\n\n", ptr_cd_->GetCDName().GetString().c_str(), node_id_.GetString().c_str());
        CheckMailBox();

      }
    }
    else {
      
//      PMPI_Win_fence(0, CDPath::GetCoarseCD(this)->ptr_cd()->mailbox_);
//      CD_DEBUG("\n\n[Barrier] CDHandle::Detect 1 - %s / %s\n\n", ptr_cd_->GetCDName().GetString().c_str(), node_id_.GetString().c_str());
//      CheckMailBox();
    }
*/    

#endif

//    PMPI_Win_fence(0, CDPath::GetCoarseCD(this)->ptr_cd()->mailbox_);
    CD_DEBUG("\n\n[Barrier] CDHandle::Detect 1 - %s / %s\n\n", ptr_cd_->GetCDName().GetString().c_str(), node_id_.GetString().c_str());
  }

  CheckMailBox();


#endif

  if(err_ret_val != NULL)
    *err_ret_val = err;
#if CD_DEBUG_DEST == 1
  fflush(cdout);
#endif
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

  CDPrologue();
  CDEpilogue();
  return 0;
}

float CDHandle::RequireErrorProbability (SysErrT error_type, uint32_t error_num, float probability, bool fail_over)
{

  CDPrologue();
  CDEpilogue();

  return 0;
}




#if CD_ERROR_INJECTION_ENABLED
//void CDHandle::RegisterErrorInjector(const ErrorType &error_type, ErrorInjector *error_injector)
//{
//  app_side = false;
//
//  CD_DEBUG("RegisterErrorInjector: %d at level #%u\n", GetExecMode(), level());
//  if(cd_error_injector_ == NULL) {
//    if( recreated() == false && reexecuted() == false ) {
//      CD_DEBUG("Registered!!\n");
//      error_injector_dir_[error_type] = error_injector;
//    }
//    else {
//      CD_DEBUG("Failed to be Registered!!\n");
//      delete cd_error_injector;
//    }
//  }
//  else {
//    CD_DEBUG("No injector registered!!\n");
//  }
//
//  error_injector_dir_[error_type] = error_injector;
//
//  app_side = true;
//}

void CDHandle::RegisterMemoryErrorInjector(MemoryErrorInjector *memory_error_injector)
{ CDHandle::memory_error_injector_ = memory_error_injector; }

void CDHandle::RegisterErrorInjector(CDErrorInjector *cd_error_injector)
{
  app_side = false;
  CD_DEBUG("RegisterErrorInjector: %d at level #%u\n", GetExecMode(), level());
  if(cd_error_injector_ == NULL && recreated() == false && reexecuted() == false) {
    CD_DEBUG("Registered!!\n");
    cd_error_injector_ = cd_error_injector;
    cd_error_injector_->task_in_color_ = task_in_color();
    cd_error_injector_->rank_in_level_ = rank_in_level();
  }
  else {
    CD_DEBUG("Failed to be Registered!!\n");
    delete cd_error_injector;
  }
  app_side = true;
}


int CDHandle::CheckErrorOccurred(uint32_t &rollback_point)
{
  uint64_t sys_err_vec = system_error_injector_->Inject();
  bool found = false;
  CD_DEBUG("[%s] sys_err_vec : %lx\n", __func__, sys_err_vec);
  if(sys_err_vec == NO_ERROR_INJECTED) {
    return (int)CD::CDInternalErrT::kOK;
  } else {
    CDHandle *cdh = this;
    // Find CD level that claimed "the raised sys_err_vec is covered."
    // If sys_err_vec > 
    while(cdh != NULL) {

      CD_DEBUG("CHECK %lx %lx = %d\n", sys_err_vec, cdh->ptr_cd_->sys_detect_bit_vector_, CHECK_SYS_ERR_VEC(sys_err_vec, cdh->ptr_cd_->sys_detect_bit_vector_));
//      printf("CHECK %lx %lx = %d\n", sys_err_vec, cdh->ptr_cd_->sys_detect_bit_vector_, CHECK_SYS_ERR_VEC(sys_err_vec, cdh->ptr_cd_->sys_detect_bit_vector_));
      if(CHECK_SYS_ERR_VEC(sys_err_vec, cdh->ptr_cd_->sys_detect_bit_vector_)) {
        rollback_point = cdh->level();
        found = true;
//        CD_DEBUG("\nFOUND LEVEL FOR REEXEC \n");
        CD_DEBUG("\nFOUND LEVEL FOR REEXEC at #%u\n", rollback_point);
        break;
      }
      cdh = CDPath::GetParentCD(cdh->level());
    }
    if(found == false) assert(0);
    return (int)CD::CDInternalErrT::kErrorReported;
  }
}

#endif

void CDHandle::Recover (uint32_t error_name_mask, 
                        uint32_t error_loc_mask, 
                        std::vector<SysErrT> errors)
{
  // STUB
}


int CDHandle::ctxt_prv_mode()
{
  return (int)ptr_cd_->ctxt_prv_mode_;
}

void CDHandle::CommitPreserveBuff()
{
  CDPrologue();
//  if(ptr_cd_->cd_exec_mode_ ==CD::kExecution){
  if( ptr_cd_->ctxt_prv_mode_ == kExcludeStack) {
//  cddbg << "Commit jmp buffer!" << endl; cddbgBreak();
//  cddbg << "cdh: " << jmp_buffer_ << ", cd: " << ptr_cd_->jmp_buffer_ << ", size: "<< sizeof(jmp_buf) << endl; cddbgBreak();
    memcpy(ptr_cd_->jmp_buffer_, jmp_buffer_, sizeof(jmp_buf));
    ptr_cd_->jmp_val_ = jmp_val_;
//  cddbg << "cdh: " << jmp_buffer_ << ", cd: " << ptr_cd_->jmp_buffer_ << endl; cddbgBreak();
  }
  else {
//    ptr_cd_->ctxt_ = this->ctxt_;
  }
//  }
  CDEpilogue();
}


uint64_t CDHandle::SetSystemBitVector(uint64_t error_name_mask, uint64_t error_loc_mask)
{
  uint64_t sys_bit_vec = 0;
  if(error_name_mask == 0) {
    // STUB
  }
  else {
    // STUB
  }

  if(error_loc_mask == 0) {
    // STUB
  }
  else {
    // STUB
  }

  // FIXME
  sys_bit_vec = error_name_mask | error_loc_mask;

  return sys_bit_vec;
}







CDErrT CDHandle::CheckMailBox(void)
{
#if CD_MPI_ENABLED
  CDErrT cd_err = ptr_cd()->CheckMailBox();
  return cd_err;
#else
  return kOK;
#endif
}

CDErrT CDHandle::SetMailBox(CDEventT event)
{
#if CD_MPI_ENABLED
  CD_DEBUG("%s at level #%u\n", __func__, level());
  return ptr_cd()->SetMailBox(event);
#else
  return kOK;
#endif
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

bool     CDHandle::recreated(void)     const { return ptr_cd_->recreated_; }
bool     CDHandle::reexecuted(void)    const { return ptr_cd_->reexecuted_; }
bool     CDHandle::need_reexec(void)   const { return *CD::rollback_point_ != INVALID_ROLLBACK_POINT; }
uint32_t CDHandle::rollback_point(void)  const { return *CD::rollback_point_; }
CDType   CDHandle::GetCDType(void)     const { return ptr_cd_->GetCDType(); }
int CDHandle::GetCommLogMode(void) const {return ptr_cd_->GetCommLogMode(); }
int CDHandle::GetCDLoggingMode(void) const {return ptr_cd_->cd_logging_mode_;}
#if CD_TEST_ENABLED
void CDHandle::PrintCommLog(void) const {
  ptr_cd_->comm_log_ptr_->Print();
}
#endif
