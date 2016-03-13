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
#include "cd_internal.h"
#include "cd_path.h"

using namespace cd;
using namespace cd::internal;
using namespace cd::interface;
using namespace cd::logging;
using namespace std;

//#define INVALID_ROLLBACK_POINT 0xFFFFFFFF

clock_t cd::log_begin_clk;
clock_t cd::log_end_clk;
clock_t cd::log_elapsed_time;

int iterator_entry_count=0;
uint64_t cd::gen_object_id=0;

//EntryDirType::hasher cd::str_hash;
std::unordered_map<string,CDEntry*> cd::str_hash_map;
std::unordered_map<string,CDEntry*>::hasher cd::str_hash;
map<ENTRY_TAG_T, string> cd::tag2str;
list<CommInfo> CD::entry_req_;
map<ENTRY_TAG_T, CommInfo> CD::entry_request_req_;
map<ENTRY_TAG_T, CommInfo> CD::entry_recv_req_;
list<EventHandler *> CD::cd_event_;

map<uint32_t, uint32_t> Util::object_id;

map<string, uint32_t> CD::exec_count_;

//unordered_map<string,pair<int,int>> CD::num_exec_map_;

bool CD::need_reexec = false;
bool CD::need_escalation = false;
uint32_t CD::reexec_level = INVALID_ROLLBACK_POINT;

void cd::internal::Initialize(void)
{
#if _MPI_VER
  PMPI_Alloc_mem(sizeof(CDFlagT), MPI_INFO_NULL, &(CD::pendingFlag_));

  // Initialize pending flag
  *CD::pendingFlag_ = 0;
//  CD::pendingFlag_ = 0;
#endif

#if _MPI_VER
  // Tag-related
  void *v;
  int flag;
//  ENTRY_TAG_T max_tag_size = 2147483647;

//  PMPI_Comm_set_attr( MPI_COMM_WORLD, MPI_TAG_UB, &max_tag_size ); // It does not work for MPI_TAG_UB 
  PMPI_Comm_get_attr( MPI_COMM_WORLD, MPI_TAG_UB, &v, &flag ); 

  ENTRY_TAG_T predefined_max_tag_bits = *(int *)v + 1;

//  cout << "predefined_max_tag_bits : " << predefined_max_tag_bits << endl;

  int cnt = sizeof(ENTRY_TAG_T)*8;
  while(cnt >= 0) {
    if((predefined_max_tag_bits >> cnt) & 0x1) {
      max_tag_bit = cnt;
      break;
    }
    else {
      cnt--;
    }
  }
//  max_tag_bit = 30;
//  cout << "max bits : " << max_tag_bit << endl;
  max_tag_level_bit = 6;
  max_tag_rank_bit = max_tag_bit-max_tag_level_bit-1;
  max_tag_task_bit = max_tag_rank_bit/2;

#endif
}

void cd::internal::Finalize(void)
{

#if _MPI_VER
  PMPI_Free_mem(CD::pendingFlag_);
#endif

}



#if _MPI_VER
CDFlagT *CD::pendingFlag_ = NULL; 
//CDFlagT CD::pendingFlag_=0; 
#endif

/// Actual CD Object only exists in a single node and in a single process.
/// Potentially copy of CD Object can exist but it should not be used directly. 
/// We also need to think about when maintaining copy of CD objects, 
/// how are they going to be synchrnoized  (the values, and the entries and all that)

/// Handle would be an accessor to this object. (Kind of an interface for these)
/// If CD Object resides in current process then, it is act as pointer. 
/// If CD Object does not reside in current process, then it will do the appropriate things. 

/// Right now I am making single threaded version so don't consider CDHandle too much 
/// Complication will be delayed until we start developing multithreaded or multi node version.

/// TODO: Desgin decision on CD Tree
/// CD Tree: If CD tree is managed by N nodes, 
/// node (cd_id % N) is where we need to ask for insert, delete, and peek operations. 
/// Distributed over N nodes, and the guide line is cd_id. 
/// If every node that exists handles cd tree, then this means cd tree is always local. 
/// Root CD will, in this case, be located at Node 0 for example. 
/// TODO: how do we implement preempt stop function? 

/// ISSUE Kyushick
/// level+1 when we create children CDs.
/// Originally we do that inside CD object creator, 
/// but it can be inappropriate in some case.
/// ex. cd object can be created by something else I guess..
/// So I think it would be more desirable to increase level
/// inside Create() 

CD::CD(void)
  : file_handle_(),
    incomplete_log_(DEFAULT_INCOMPL_LOG_SIZE)
{
  reexecuted_ = false;
  recreated_ = false;
  is_window_reused_ = false;
  cd_type_ = kStrict;
  prv_medium_ = kDRAM;
  name_ = INITIAL_CDOBJ_NAME; 
  label_ = string(INITIAL_CDOBJ_LABEL); 
  sys_detect_bit_vector_ = 0;
  // Assuming there is only one CD Object across the entire system we initilize cd_id info here.
  cd_id_ = CDID();

  // Kyushick: Object ID should be unique for the copies of the same object?
  // For now, I decided it as an unique one
  cd_id_.object_id_ = Util::GenCDObjID();

  recoverObj_ = new RecoverObject;
//  need_reexec = false;
//  reexec_level = 0;
  num_reexecution_ = 0;
  Init();  
  
#if comm_log
  // SZ
  // create instance for comm_log_ptr_ for relaxed CDs
  // if no parent assigned, then means this is root, so log mode will be kGenerateLog at creation point
  assert(comm_log_ptr_ == NULL);

  LOG_DEBUG("Set child_seq_id_ to 0\n");
  child_seq_id_ = 0;
  SetCDLoggingMode(kStrictCD);

#endif
}

  // FIXME: only acquire root handle when needed. 
  // Most of the time, this might not be required.
  // Kyushick : I cannot fully understand this comment....
CD::CD(CDHandle* cd_parent, 
       const char* name, 
       const CDID& cd_id, 
       CDType cd_type, 
       PrvMediumT prv_medium, 
       uint64_t sys_bit_vector)
 :  cd_id_(cd_id),
    file_handle_(prv_medium, 
                 ((cd_parent!=NULL)? cd_parent->ptr_cd_->file_handle_.GetBasePath() : FilePath::prv_basePath_), 
                 cd_id.GetStringID() + string("_XXXXXX") ),
    incomplete_log_(DEFAULT_INCOMPL_LOG_SIZE)
{
  //GONG
  begin_ = false;

  reexecuted_ = false;
  if(cd_parent != NULL) {
    if(cd_parent->ptr_cd_->reexecuted_ == true || cd_parent->ptr_cd_->recreated_ == true) {
      recreated_ = true;
    }
    else {
      recreated_ = false;
    }
  }
  else {
    recreated_ = false;
  }

  is_window_reused_ = false;

  cd_type_ = cd_type; 
  prv_medium_ = prv_medium; 

  if(name != NULL)
    name_ = name; 
    //label_ = INITIAL_CDOBJ_LABEL; 
  label_ = string(INITIAL_CDOBJ_LABEL); 

  sys_detect_bit_vector_ = sys_bit_vector; 
  // FIXME 
  // cd_id_ = 0; 
  // We need to call which returns unique id for this cd. 
  // the id is recommeneded to have pre-allocated sections per node. 
  // This way, we don't need to have race condition to get unique id. 
  // Instead local counter is used to get the unique id.

  // Kyushick: Object ID should be unique for the copies of the same object?
  // For now, I decided it as an unique one
  CD_DEBUG("exec_count[%s] = %u\n", cd_id_.GetPhaseID().c_str(), exec_count_[cd_id_.GetPhaseID()]);
  if(exec_count_[cd_id_.GetPhaseID()] == 0) {
    cd_id_.object_id_ = Util::GenCDObjID(cd_id_.level());
  }
  exec_count_[cd_id_.GetPhaseID()]++;
  CD_DEBUG("exec_count[%s] = %u\n", cd_id_.GetPhaseID().c_str(), exec_count_[cd_id_.GetPhaseID()]);


//  cddbg << "cd object is created " << cd_id_.object_id_++ <<endl;

  // FIXME maybe call self generating function here?           
  // cd_self_  = self;  //FIXME maybe call self generating function here?           

  // FIXME 
  // cd_id_.level() = parent_cd_id.level() + 1;
  // we need to get parent id ... 
  // but if they are not local, this might be hard to acquire.... 
  // perhaps we should assume that cd_id is always store in the handle ...
  // Kyushick : if we pass cd_parent handle to the children CD object when we create it,
  // this can be resolved. 

  recoverObj_ = new RecoverObject;

//  need_reexec = false;
//  reexec_level = cd_id.level();
//  reexec_level = 0;
  num_reexecution_ = 0;

  Init();  

#if comm_log
  // SZ
  // create instance for comm_log_ptr_
  // comm_log is per thread per CD
  // if cd_parent is not NULL, then inherit comm log mode from parent,
  // otherwise means current cd is a root cd, so comm log mode is to generate log
  if (cd_parent != NULL)
  {
    // Only create CommLog object for relaxed CDs
    assert(comm_log_ptr_ == NULL);
    if (MASK_CDTYPE(cd_type_) == kRelaxed)
    {
      //SZ: if parent is a relaxed CD, then copy comm_log_mode from parent
      if (MASK_CDTYPE(cd_parent->ptr_cd()->cd_type_) == kRelaxed)
      {
        CommLogMode parent_log_mode = cd_parent->ptr_cd()->comm_log_ptr_->GetCommLogMode();
        LOG_DEBUG("With cd_parent (%p) relaxed CD, creating CommLog with parent's mode %d\n", cd_parent, parent_log_mode);
        comm_log_ptr_ = new CommLog(this, parent_log_mode);
        if (parent_log_mode == kGenerateLog){
          SetCDLoggingMode(kRelaxedCDGen);
        }
        else{
          SetCDLoggingMode(kRelaxedCDRead);
        }
      }
      //SZ: if parent is a strict CD, then child CD is always in kGenerateLog mode at creation point
      else
      {
        LOG_DEBUG("With cd_parent (%p) strict CD, creating CommLog with kGenerateLog mode\n", cd_parent);
        comm_log_ptr_ = new CommLog(this, kGenerateLog);
        SetCDLoggingMode(kRelaxedCDGen);
      }
    }
    else 
    {
      SetCDLoggingMode(kStrictCD);
    }

    // set child's child_seq_id_  to 0 
    LOG_DEBUG("Set child's child_seq_id_ to 0\n");
    child_seq_id_ = 0;

    uint32_t tmp_seq_id = cd_parent->ptr_cd()->child_seq_id_;
    LOG_DEBUG("With cd_parent = %p, set child's seq_id_ to parent's child_seq_id_(%d)\n", cd_parent, tmp_seq_id);
    cd_id_.SetSequentialID(tmp_seq_id);

#if CD_LIBC_LOG_ENABLED
    //GONG
    libc_log_ptr_ = new CommLog(this, kGenerateLog);
#endif

  }
  else 
  {
    // Only create CommLog object for relaxed CDs
    assert(comm_log_ptr_ == NULL);
    if (MASK_CDTYPE(cd_type_) == kRelaxed)
    {
      LOG_DEBUG("With cd_parent = NULL, creating CommLog with mode kGenerateLog\n");
      comm_log_ptr_ = new CommLog(this, kGenerateLog);
      SetCDLoggingMode(kRelaxedCDGen);
    }
    else {
      SetCDLoggingMode(kStrictCD);
    }

#if CD_LIBC_LOG_ENABLED
    //GONG:
    libc_log_ptr_ = new CommLog(this, kGenerateLog);
#endif

    LOG_DEBUG("Set child's child_seq_id_ to 0\n");
    child_seq_id_ = 0;
  }

#endif

}

void CD::Initialize(CDHandle* cd_parent, 
                    const char* name, 
                    CDID cd_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector)
{

  reexecuted_ = false;
  if(cd_parent != NULL) {
    if(cd_parent->ptr_cd_->reexecuted_ == true || cd_parent->ptr_cd_->recreated_ == true) {
      recreated_ = true;
    }
    else {
      recreated_ = false;
    }
  }
  else {
    recreated_ = false;
  }

  is_window_reused_ = false;

  cd_type_  = cd_type;
  name_     = name;
  label_ = string(INITIAL_CDOBJ_LABEL); 
  cd_id_    = cd_id;
  if(exec_count_[cd_id_.GetPhaseID()] == 0) {
    cd_id_.object_id_ = Util::GenCDObjID(cd_id.level());
    exec_count_[cd_id.GetPhaseID()]++;
  }
  CD_DEBUG("exec_count[%s] = %u\n", cd_id_.GetPhaseID().c_str(), exec_count_[cd_id_.GetPhaseID()]);
  sys_detect_bit_vector_ = sys_bit_vector;
  Init();  
}

void CD::Init()
{
  //ctxt_prv_mode_ = kExcludeStack; 
  ctxt_prv_mode_ = kIncludeStack; 
  cd_exec_mode_  = kSuspension;
  option_save_context_ = 0;

#if comm_log
  comm_log_ptr_ = NULL;
#endif
#if CD_LIBC_LOG_ENABLED
  libc_log_ptr_ = NULL;
  cur_pos_mem_alloc_log = 0;
  replay_pos_mem_alloc_log = 0;
#endif
// Kyushick : I think we already initialize cd_id_ object inside cd object creator (outside of Init method)
// So we do not have to get it here. 
// I think this should be inside CDID object creator because there is no information to pass from CD object to CDID object
//  cd_id_.domain_id_ = Util::GetCurrentDomainID();
//  cd_id_.object_id_ = Util::GenerateCDObjectID();
//  cd_id_.sequential_id_ = 0;

}

/// Kyushick:
/// What if a process is just killed by some reason, and this method could not be invoked?
/// Would it be safe to delete meta data and do proper operation when CD finish?
/// Or how about defining virtual CDErrT DeleteCD() for ~CD() ??
/// And if a process gets some signal for abort, it just call this DeleteCD() and
/// inside this function, we explicitly call ~CD() 
CD::~CD()
{
  // Erase all the CDEntries
//  for(std::list<CDEntry>::iterator it = entry_directory_.begin();
//      it != entry_directory_.end(); ++it) {
//    it->Delete();
//  }

  // Request to add me as a child to my parent
//  cd_parent_->RemoveChild(this);
  //FIXME : This will be done at the CDHandle::Destroy()

#if comm_log
  //Delete comm_log_ptr_
  if (comm_log_ptr_ != NULL)
  {
    LOG_DEBUG("Delete comm_log_ptr_\n");
    delete comm_log_ptr_;
  }
  else 
  {
    LOG_DEBUG("NULL comm_log_ptr_ pointer, this CD should be strict CD...\n");
  }
#endif
}

CDHandle *CD::Create(CDHandle* parent, 
                     const char* name, 
                     const CDID& child_cd_id, 
                     CDType cd_type, 
                     uint64_t sys_bit_vector, 
                     CD::CDInternalErrT *cd_internal_err)
{
  /// Create CD object with new CDID
  CDHandle* new_cd_handle = NULL;

  CD_DEBUG("CD::Create %s\n", name);

  *cd_internal_err = InternalCreate(parent, name, child_cd_id, cd_type, sys_bit_vector, &new_cd_handle);
  assert(new_cd_handle != NULL);

  CD_DEBUG("CD::Create done\n");

  this->AddChild(new_cd_handle);

  return new_cd_handle;

}

CDHandle *CD::CreateRootCD(const char* name, 
                           const CDID& root_cd_id, 
                           CDType cd_type, 
                           const string &basefilepath,
                           uint64_t sys_bit_vector, 
                           CD::CDInternalErrT *cd_internal_err)
{
  /// Create CD object with new CDID
  CDHandle *new_cd_handle = NULL;
  PrvMediumT new_prv_medium = static_cast<PrvMediumT>(MASK_MEDIUM(cd_type));

  *cd_internal_err = InternalCreate(NULL, name, root_cd_id, cd_type, sys_bit_vector, &new_cd_handle);

  // Only root define hash function.
  str_hash = str_hash_map.hash_function();

  // It sets filepath for preservation with base filename.
  // If CD_PRV_BASEPATH is set in env, 
  // the file path will be basefilepath/CDID_XXXXXX.
  if(new_prv_medium != kDRAM)
    new_cd_handle->ptr_cd_->file_handle_.SetFilePath(new_prv_medium, basefilepath, root_cd_id.GetStringID() + string("_XXXXXX"));

  assert(new_cd_handle != NULL);

  return new_cd_handle;
}


CDErrT CD::Destroy(bool collective)
{
  CD_DEBUG("CD::Destroy\n");
  CDErrT err=CDErrT::kOK;
  InternalDestroy(collective);


  return err;
}

 
CD::CDInternalErrT 
CD::InternalCreate(CDHandle* parent, 
                   const char* name, 
                   const CDID& new_cd_id, 
                   CDType cd_type, 
                   uint64_t sys_bit_vector, 
                   CDHandle** new_cd_handle)
{
  CD_DEBUG("Internal Create... level #%u, Node ID : %s\n", new_cd_id.level(), new_cd_id.node_id().GetString().c_str());

  PrvMediumT new_prv_medium = static_cast<PrvMediumT>(
                                  (MASK_MEDIUM(cd_type) == 0)? parent->ptr_cd()->prv_medium_ : 
                                                               MASK_MEDIUM(cd_type)
                              );

  if( !new_cd_id.IsHead() ) {

    CD_DEBUG("Mask medium : %d\n", MASK_MEDIUM(cd_type));

    CD *new_cd     = new CD(parent, name, new_cd_id, static_cast<CDType>(MASK_CDTYPE(cd_type)), new_prv_medium, sys_bit_vector);

    // Create memory region where RDMA is enabled
#if _MPI_VER
    int task_count = new_cd_id.task_count();

    if(task_count > 1) {
      
      CD_DEBUG("In CD::Create Internal Memory. Alloc Start. # tasks : %u\n", task_count);

      PMPI_Alloc_mem(sizeof(CDFlagT), 
                    MPI_INFO_NULL, &(new_cd->event_flag_));
      
      CD_DEBUG("In CD::Create Internal Memory. Alloc Done. # tasks : %u\n", task_count);

      // Initialization of event flags
      *(new_cd->event_flag_) = 0;

//      PMPI_Win_create(NULL, 0, 1,
//                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->mailbox_));
      CD_DEBUG("CD MPI Win create for %u windows start.\n", task_count);

      PMPI_Win_create(new_cd->event_flag_, sizeof(CDFlagT), sizeof(CDFlagT),
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->mailbox_));

      CD_DEBUG("CD MPI Win create for %u windows done.\n", task_count);

      // FIXME : should it be MPI_COMM_WORLD?
      PMPI_Win_create(new_cd->pendingFlag_, sizeof(CDFlagT), sizeof(CDFlagT), 
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->pendingWindow_));

      CD_DEBUG("HeadCD mpi win create for %u pending window done, new Node ID : %s\n", task_count, new_cd_id.node_id_.GetString().c_str());
    } 
    else {
      new_cd->is_window_reused_ = true;
    }
#endif


    if( new_cd->GetPlaceToPreserve() == kPFS ) 
      new_cd->pfs_handler_ = new PFSHandle( new_cd, new_cd->file_handle_.GetFilePath() ); 

    *new_cd_handle = new CDHandle(new_cd, new_cd_id.node_id());
  }
  else {
    // Create a CD object for head.
    CD_DEBUG("Mask medium : %d\n", MASK_MEDIUM(cd_type));

    HeadCD *new_cd = new HeadCD(parent, name, new_cd_id, static_cast<CDType>(MASK_CDTYPE(cd_type)), new_prv_medium, sys_bit_vector);

#if _MPI_VER
    // Create memory region where RDMA is enabled
    CD_DEBUG("HeadCD create internal memory.\n");

    uint32_t task_count = new_cd_id.task_count();

    if(task_count > 1) {

      CD_DEBUG("In CD::Create Internal Memory. Alloc Start. # tasks : %u\n", task_count);

      PMPI_Alloc_mem(task_count*sizeof(CDFlagT), 
                    MPI_INFO_NULL, &(new_cd->event_flag_));

      CD_DEBUG("In CD::Create Internal Memory. Alloc Done. # tasks : %u\n", task_count);

      // Initialization of event flags
      for(uint32_t i=0; i<task_count; i++) {
        new_cd->event_flag_[i] = 0;
      }
    
      PMPI_Win_create(new_cd->event_flag_, task_count*sizeof(CDFlagT), sizeof(CDFlagT),
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->mailbox_));

      CD_DEBUG("HeadCD mpi win create for %u mailbox done\n", task_count);

      // FIXME : should it be MPI_COMM_WORLD?
      PMPI_Win_create(new_cd->pendingFlag_, sizeof(CDFlagT), sizeof(CDFlagT), 
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->pendingWindow_));

      CD_DEBUG("HeadCD mpi win create for %u pending window done, new Node ID : %s\n", task_count, new_cd_id.node_id_.GetString().c_str());

    }
    else {
      new_cd->is_window_reused_ = true;
    }
//    AttachChildCD(new_cd);
#endif

    if( new_cd->GetPlaceToPreserve() == kPFS ) 
      new_cd->pfs_handler_ = new PFSHandle( new_cd, new_cd->file_handle_.GetFilePath() ); 

    *new_cd_handle = new CDHandle(new_cd, new_cd_id.node_id());
  }
  
  CD_DEBUG("[CD::InternalCreate] Done. New Node ID: %s -- %s\n", 
           new_cd_id.node_id().GetString().c_str(), 
           (*new_cd_handle)->node_id().GetString().c_str());

  return CD::CDInternalErrT::kOK;
}


void AttachChildCD(HeadCD *new_cd)
{
  // STUB
  // This routine is not needed for MPI-version CD runtime  
}

inline 
CD::CDInternalErrT CD::InternalDestroy(bool collective)
{

#if _MPI_VER
  CD_DEBUG("[%s] clean up CD meta data (%d windows) at %s (%s) level #%u\n", __func__, task_size(), name_.c_str(), label_.c_str(), level());

#if CD_DEBUG_DEST == 1
    fflush(cdout);
#endif
  bool orig_need_reexec = need_reexec;
  if(collective)
    SyncCDs(this);

//  if(need_reexec != orig_need_reexec) {
//    CD_DEBUG("\n\n[%s]Reexec (Before calling ptr_cd_->GetCDToRecover()->Recover(false);\n\n", __func__);
//    CD::GetCDToRecover(GetCurrentCD(), false)->ptr_cd()->Recover();
//  } else {
//    CD_DEBUG("\n\nReexec is false\n");
//  }
  
  if(task_size() > 1 && (is_window_reused_==false)) {  
    PMPI_Win_free(&pendingWindow_);
    PMPI_Win_free(&mailbox_);
    PMPI_Free_mem(event_flag_);
    CD_DEBUG("[%s Window] CD's Windows are destroyed.\n", __func__);
  }
  else
    CD_DEBUG("[%s Window] Task size == 1 or Window is reused.\n", __func__);

#if CD_DEBUG_DEST == 1
    fflush(cdout);
#endif

#endif


#if comm_log
  if (GetParentHandle()!=NULL)
  {
    GetParentHandle()->ptr_cd_->child_seq_id_ = cd_id_.sequential_id();
  }
#endif

  if(GetCDID().level() != 0) {   // non-root CD

  } 
  else {    // Root CD

  }
  if( GetPlaceToPreserve() == kPFS ) {
    delete pfs_handler_;
  }

  delete this;

  if(need_reexec != orig_need_reexec) {
    CD_DEBUG("\n\n[%s]Reexec (Before calling ptr_cd_->GetCDToRecover()->Recover(false);\n\n", __func__);
    CD::GetCDToRecover(GetParentCD(level()), true)->ptr_cd()->Recover();
  } else {
    CD_DEBUG("\n\nReexec is false\n");
  }
  return CDInternalErrT::kOK;

}



/*  CD::Begin()
 *  (1) Call all the user-defined error checking functions. 
 *      Jinsuk: Why should we call error checking function at the beginning?
 *      Kyushick: It doesn't have to. I wrote it long time ago, so explanation here might be quite old.
 *    Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *
 */ 

// Here we don't need to follow the exact CD API this is more like internal thing. 
// CDHandle will follow the standard interface. 
CDErrT CD::Begin(bool collective, const char* label)
{
  begin_ = true;

  CD_DEBUG("Inside CD::Begin\n");
//  auto rit = num_exec_map_.find(name_);
//  if(rit == num_exec_map_.end()){ 
//    num_exec_map_[name_].first = 1;
//    num_exec_map_[name_].second = 0;
////    cout << "first! " << name_ << endl;
//  } else {
//    num_exec_map_[name_].first += 1;
////    cout << "not first! " << name_ << " " << num_exec_map_[name_].first << endl;
//  }

//  label_[string(label)] = 0;
  if(label != NULL)
    label_ = string(label);

#if comm_log
  //SZ: if in reexecution, need to unpack logs to childs
  if (GetParentHandle()!=NULL)
  {
#if _PGAS_VER
    // increment parent's SC value and record in current CD
    sync_counter_ = GetParentHandle()->ptr_cd()->GetSyncCounter();
    sync_counter_++;
    LOG_DEBUG("Increment SC at CD::Begin, with SC (%d) after.\n", sync_counter_);
#endif
    // Only need to if for both parent and child are relaxed CDs, 
    // if child is relaxed but parent is strict, then create the CommLog object with kGenerateLog mode
    // if child is strict, then do not need to do anything for comm_log_ptr_...
    if(MASK_CDTYPE(GetParentHandle()->ptr_cd_->cd_type_) == kRelaxed && MASK_CDTYPE(cd_type_) == kRelaxed)
    {
      if (GetParentHandle()->ptr_cd_->comm_log_ptr_->GetCommLogMode() == kReplayLog)
      {
        LOG_DEBUG("With comm log mode = kReplayLog, unpack logs to children\n");
        if (IsParentLocal())
        {
          comm_log_ptr_->SetCommLogMode(kReplayLog);
          CommLogErrT ret;
          ret = GetParentHandle()->ptr_cd_->comm_log_ptr_->UnpackLogsToChildCD(this);
          if (ret == kCommLogChildLogNotFound)
          {
            // need to reallocate table and data array...
            comm_log_ptr_->Realloc();
          }
        }
        else
        {
          comm_log_ptr_->SetCommLogMode(kReplayLog);
          //SZ: FIXME: need to figure out a way to unpack logs if child is not in the same address space with parent
          LOG_DEBUG("Should not be here to unpack logs!!\n");
        }
      }
    }

#if CD_LIBC_LOG_ENABLED
    //GONG: as long as parent CD is in replay(check with ), child CD needs to unpack logs
    if(GetParentHandle()->ptr_cd_->libc_log_ptr_->GetCommLogMode() == kReplayLog){
      LOG_DEBUG("unpack libc logs to children - replay mode\n");
      //the same issue as above: address space 
      if (IsParentLocal())
      {
          libc_log_ptr_->SetCommLogMode(kReplayLog);
          CommLogErrT ret;
          ret = GetParentHandle()->ptr_cd_->libc_log_ptr_->UnpackLogsToChildCD_libc(this);
          if (ret == kCommLogChildLogNotFound)
          {
            // need to reallocate table and data array...
            libc_log_ptr_->Realloc();
          }
      }
      else
      {
        LOG_DEBUG("Should not be here to unpack logs!! - libc\n");
      }
      cur_pos_mem_alloc_log = PullMemLogs();
      replay_pos_mem_alloc_log = 0;
    }

#endif
  }
#if _PGAS_VER
  else {
    // as CD_Begin will start a new epoch, so sync_counter_ will be 1 instead of 0
    sync_counter_ = 1;
    LOG_DEBUG("Increment SC at root's CD::Begin, with SC (%d) after.\n", sync_counter_);
  }
#endif

#endif

//  if(collective && task_size() > 1) {
//    MPI_Win_fence(0, mailbox_);
//    CD_DEBUG("[%s] Barrier!!!! Important!!\n", __func__);
//    //PMPI_Barrier(color());
//  } else {
//    CD_DEBUG("No Barrier!!!!! %d %u\n", collective, task_size());
//  }

  if(cd_exec_mode_ == kReexecution || collective)
    SyncCDs(this);

  if(CD::need_reexec) {
    CD_DEBUG("\n\n[%s]Reexec (Before calling ptr_cd_->GetCDToRecover()->Recover(false);\n\n", __func__);
    CD::GetCDToRecover(GetCurrentCD(), false)->ptr_cd()->Recover();
//    CD *cd_to_recover = ptr_cd_->GetCDToRecover();
//    cd_to_recover->recoverObj_->Recover(cd_to_recover);
  } else {
    CD_DEBUG("\n\nReexec is false\n");
  }

  if( cd_exec_mode_ != kReexecution ) { // normal execution
    num_reexecution_ = 0;
    cd_exec_mode_ = kExecution;
  }
  else {
    //printf("don't need reexec next time. Now it is in reexec mode\n");
//    need_reexec = false;
//    reexec_level = INVALID_ROLLBACK_POINT;
  }
//  else {
//    cout << "Begin again! " << endl; //getchar();
//    num_reexecution_++ ;
//  }

//  if(MASK_CDTYPE(cd_type_) == kRelaxed) {printf("cdtype is relaxed\n");assert(0);}
//  if(GetCDLoggingMode() == kRelaxedCDGen) {printf("CDLoggingmode is relaxed cd gen\n");assert(0);}
//  if(GetCDLoggingMode() == kRelaxedCDRead) {printf("CDLoggingmode is relaxed cd read\n");assert(0);}

  return CDErrT::kOK;
}

// static
void CD::SyncCDs(CD *cd_lv_to_sync)
{

  if(cd_lv_to_sync->task_size() > 1) {
    CD_DEBUG("[%s] fence 1 in at %s level %u\n", __func__, cd_lv_to_sync->name_.c_str(), cd_lv_to_sync->level());
    //printf("[%s] fence 1 in at %s level %u\n", __func__, cd_lv_to_sync->name_.c_str(), cd_lv_to_sync->level());
    MPI_Win_fence(0, cd_lv_to_sync->mailbox_);
    cd_lv_to_sync->CheckMailBox();
    CD_DEBUG("[%s] fence 2 in at %s level %u\n", __func__, cd_lv_to_sync->name_.c_str(), cd_lv_to_sync->level());
    //printf("[%s] fence 2 in at %s level %u\n", __func__, cd_lv_to_sync->name_.c_str(), cd_lv_to_sync->level());
    MPI_Win_fence(0, cd_lv_to_sync->mailbox_);
    cd_lv_to_sync->CheckMailBox();
    CD_DEBUG("[%s] fence out \n\n", __func__);
    //printf("[%s] fence out \n\n", __func__);
  } else {
    CD_DEBUG("[%s] No fence\n", __func__);
    //printf("[%s] No fence\n", __func__);
  }

}

// static
CDHandle *CD::GetCDToRecover(CDHandle *target, bool collective)
{
#if _MPI_VER
  // Before longjmp, it should decrement the event counts handled so far.
  target->ptr_cd_->DecPendingCounter();
#endif
  uint32_t level = target->level();
  CD_DEBUG("[%s] level : %u (current) == %u (reexec_level)\n", __func__, level, reexec_level);
//  printf("[%s] level : %u (current) == %u (reexec_level)\n", __func__, level, reexec_level);
//  printf("#### [%s] level : %u (current) == %u (reexec_level) ####\n", __func__, level, reexec_level);
  if(level == reexec_level) {
    // for tasks that detect error at completion point or collective create point.
    // It already called SyncCDs() at that point,
    // so should not call SyncCDs again in this routine.
    // If the tasks needs to escalate several levels,
    // then it needs to call SyncCDs to be coordinated with
    // the other tasks in the other CDs of current level,
    // but in the same CDs at the level to escalate to.
//    if(need_destroy) {  
//      target->ptr_cd_->CompleteLogs();
//      target->->ptr_cd_->DeleteEntryDirectory();
//    }
    if(collective)
      SyncCDs(target->ptr_cd());

    // It is possible for other task set to reexec_level lower than original.
    // It handles that case.
    if(level != reexec_level) { 
#if CD_PROFILER_ENABLED
//      if(myTaskID == 0) printf("[%s] CD level #%u (%s)\n", __func__, level, target->ptr_cd_->label_.c_str()); 
      target->profiler_->FinishProfile();
#endif
      target->ptr_cd_->CompleteLogs();
      target->ptr_cd_->DeleteEntryDirectory();
      target->Destroy();
      return GetCDToRecover(CDPath::GetCDLevel(--level), true);
    }
    else {
      if(MASK_CDTYPE(target->ptr_cd_->cd_type_)==kRelaxed) {
        target->ptr_cd_->ProbeIncompleteLogs();
      }
      if(MASK_CDTYPE(target->ptr_cd_->cd_type_)==kStrict) {
        target->ptr_cd_->InvalidateIncompleteLogs();
      }
      else {
        ERROR_MESSAGE("[%s] Wrong control path. CD type is %d\n", __func__, target->ptr_cd_->cd_type_);
      }
      //printf("...1\n");
      need_reexec = false;
      need_escalation = false;
      return target;
    }
  }
  else if(level > reexec_level && reexec_level != INVALID_ROLLBACK_POINT) {
    //printf("...2\n");
//    if( CDPath::GetCDLevel(reexec_level)->task_size() != target->task_size() ){
    if(collective)
      SyncCDs(target->ptr_cd());
//    } else {
//      assert(CDPath::GetCDLevel(reexec_level)->color() == target->color());
//    }
#if CD_PROFILER_ENABLED
//    if(myTaskID == 0) printf("[%s] CD level #%u (%s)\n", __func__, level, target->ptr_cd_->label_.c_str()); 
    target->profiler_->FinishProfile();
#endif
    target->ptr_cd_->CompleteLogs();
    target->ptr_cd_->DeleteEntryDirectory();
    target->Destroy(false);
    
    return GetCDToRecover(CDPath::GetCDLevel(--level), true);
  } else {
    ERROR_MESSAGE("Invalid eslcation point %u (current %u)\n", reexec_level, level);
    return NULL;
  } 
}

//CD *CD::GetCDToRecover(bool escalation)
//{
//    CDHandle *curr_cdh = CDPath::GetCurrentCD();
//
//#if _MPI_VER
//    // Before longjmp, it should decrement the event counts handled so far.
//    DecPendingCounter();
//#endif
//
//    CD_DEBUG("\n\n\n\n Reexec #%u -> #%u (%s %s)\n\n\n\n", 
//                       level(), reexec_level, cd_id_.GetString().c_str(), name_.c_str());
//    printf("\n\n\n\n Reexec #%u -> #%u (%s %s)\n\n\n\n", 
//                       level(), reexec_level, cd_id_.GetString().c_str(), name_.c_str());
//    need_reexec = false;
//    need_escalation = false;
////    reexec_level = 0xFFFFFFFF;
//    //this->Recover(); 
////    CDHandle *cdh = CDPath::GetCDLevel(reexec_level);
//    uint32_t level = curr_cdh->level();
////    printf("%d > %d\n", reexec_level, level);
//
//    if(reexec_level > level) {
//      printf("\n\nReexec from level #%u from level #%u\n\n", reexec_level, level);
//      CD_DEBUG("\n\nReexec from level #%u from level #%u\n\n", reexec_level, level);
//      assert(0); // This should not happen. 
//    }
//
//  CD *cd_lv_to_sync = CDPath::GetParentCD(cd_id_.level())->ptr_cd_;
//
//  if(escalation) {
//
//  } else {
//    CD_DEBUG("[%s] Not call fence %s\n", __func__, cd_lv_to_sync->name_.c_str());
//  }
//
//
//
//#if CD_DEBUG_DEST == 1
//    fflush(cdout);
//#endif
//
//    while(reexec_level < level && reexec_level != INVALID_ROLLBACK_POINT) { // Escalation. 
////      curr_cdh->Complete(false);
//#if CD_PROFILER_ENABLED
//      curr_cdh->profiler_->FinishProfile();
//#endif
//
//      curr_cdh->ptr_cd_->CompleteLogs();
//      curr_cdh->ptr_cd_->DeleteEntryDirectory();
//      curr_cdh->Destroy();
//      curr_cdh = CDPath::GetParentCD(level--);
//    }
//    
////    reexec_level = INVALID_ROLLBACK_POINT;
//#if CD_DEBUG_DEST == 1
//    fflush(cdout);
//#endif
//
//    return curr_cdh->ptr_cd_;
//}
//    GetCDLevel()->Recover();
/*  CD::Complete()
 *  (1) Call all the user-defined error checking functions.
 *      Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *
 */
CDErrT CD::Complete(bool collective, bool update_preservations)
{
  CD_DEBUG("\nCD::Complete : %s %s \t Reexec: %d\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str(), need_reexec);
//  if(need_reexec)
//    printf("#### level : %u (current) == %u (reexec_level) ####\n", __func__, level(), reexec_level);

//  printf("\nCD::Complete (%s): %s %s \t Reexec: %d from %u to %u\n", 
//      name_.c_str(), GetCDName().GetString().c_str(), GetNodeID().GetString().c_str(), need_reexec, level(), reexec_level);
  begin_ = false;
//  bool my_need_reexec = need_reexec;

  // This is important synchronization point 
  // to guarantee the correctness of CD-enabled program.
  uint32_t orig_reexec_level = reexec_level;
  if(collective) {
    SyncCDs(this);
  }

  if(need_reexec) { 
    // If another task set reexec_level lower than this task (error occurred at this task),
    // initiator is false. 
    // Let's say it was set to 3. But another task set to 1. Then it is false;
    // Or originally it is INVALID_ROLLBACK_POINT, then it is false.
    // (ex. 0xFFFFFFF > any other level. 
    // Therefore, if orig_reexec_flag is INVALID_ROLLBACK_POINT,
    // this conditino is always false.
    // This is true only if this task got failed and set orig_reexec_flag to some level to reexecute from,
    // and no other task raised some lower escalation point than this level.
    // TODO: What if it is reported while calling GetCDToRecover??
    // This is tricky. 
    // Let's say, some other task is stuck at level 1. 
    // And task A set to level 3 from 4 (escalation)
    // It will somehow coordinate tasks in the same CD in level 3, not the other tasks in different CDs at level 3.
    // The tasks corresponding to the CD in level 3 will reach some point,
    // and it detected that it needs to go to level 1 by some tasks in the different CD in the same level 3.
    // 0309
    // Rethink the condition to sync
    // <Example>
    // Task0 is at level 4 and reexec_level is 3
    // The other thread is at level 3. And They are waiting for Task0.
    // This case Task0 must call sync.
    // 
    //
    //bool initiator = orig_reexec_level <= reexec_level && level() != reexec_level;
    // FIXME
//    printf("GetCDLevel : %u (cur %u), path size: %lu\n", reexec_level, level(), CDPath::uniquePath_->size());
    bool initiator = true;//orig_reexec_level <= reexec_level && (CDPath::GetCDLevel(reexec_level)->task_size() != task_size());
//    printf("initiator? %d = %u <= %u\n", initiator, orig_reexec_level, reexec_level);
//    CD_DEBUG("initiator? %d = %u <= %u\n", initiator, orig_reexec_level, reexec_level);
//    GetCDToRecover(reexec_level < cd_id_.cd_name_.level() && reexec_level != INVALID_ROLLBACK_POINT)->Recover(false);
//    reexec_level == level() -> false
//    bool collective = MPI_Group_compare(group());
//    printf("initiator? %d = %u <= %u\n", initiator, orig_reexec_level, reexec_level);
    CD_DEBUG("initiator? %d = %u <= %u\n", initiator, orig_reexec_level, reexec_level);
    GetCDToRecover(GetCurrentCD(), initiator)->ptr_cd()->Recover();
  }

  CompleteLogs();

  // Increase sequential ID by one
  cd_id_.sequential_id_++;
  
  /// It deletes entry directory in the CD (for every Complete() call). 
  /// We might modify this in the profiler to support the overlapped data among sequential CDs.
  DeleteEntryDirectory();


  // TODO ASSERT( cd_exec_mode_  != kSuspension );
  // FIXME don't we have to wait for others to be completed?  
  cd_exec_mode_ = kSuspension; 


  return CDErrT::kOK;
}




CD::CDInternalErrT CD::CompleteLogs(void) {
#if comm_log
  // SZ: pack logs and push to parent
  if (GetParentHandle()!=NULL) {
    // This is for pushing complete log to parent
    if (MASK_CDTYPE(cd_type_) == kRelaxed) {
      if (IsParentLocal()) {
        if (IsNewLogGenerated() && MASK_CDTYPE(GetParentHandle()->ptr_cd_->cd_type_) == kRelaxed) {
          LOG_DEBUG("Pushing logs to parent...\n");
          comm_log_ptr_->PackAndPushLogs(GetParentHandle()->ptr_cd_);

#if CD_DEBUG_ENABLED
          LOG_DEBUG("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
          LOG_DEBUG("\nchild comm_log print:\n");
          comm_log_ptr_->Print();
#endif

          //SZ: when child pushes logs to parent, parent has new log generated...
          GetParentHandle()->ptr_cd_->comm_log_ptr_->SetNewLogGenerated(true);
          
          ////SZ: if parent is in kReplayLog mode, but child has flipped back to kGenerateLog,
          ////    then parent needs to flip back to kGenerateLog 
          ////FIXME: need to coordinate with other child CDs, and what if some completed CDs reach end of logs, 
          ////      but others do not...
          //if (GetParentHandle()->ptr_cd_->comm_log_ptr_->GetCommLogMode()==kReplayLog && comm_log_ptr_->GetCommLogMode()==kGenerateLog)
          if (comm_log_ptr_->GetCommLogMode()==kGenerateLog) {
            GetParentHandle()->ptr_cd_->comm_log_ptr_->SetCommLogMode(kGenerateLog);
          }

#if CD_DEBUG_ENABLED
          LOG_DEBUG("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
          LOG_DEBUG("parent comm_log print:\n");
          GetParentHandle()->ptr_cd_->comm_log_ptr_->Print();
#endif
        }

        comm_log_ptr_->Reset();
      }
      else  {
        //SZ: FIXME: need to figure out a way to push logs to parent that resides in other address space
        LOG_DEBUG("Should not come to here...\n");
      }

      // push incomplete_log_ to parent
      if (IsParentLocal() && incomplete_log_.size()!=0) {
        //vector<struct IncompleteLogEntry> *pincomplog 
        //                                    = &(GetParentHandle()->ptr_cd_->incomplete_log_);
        CD *ptmp = GetParentHandle()->ptr_cd_;
  
        // push incomplete logs to parent
        ptmp->incomplete_log_.insert(ptmp->incomplete_log_.end(),
                                     incomplete_log_.begin(),
                                     incomplete_log_.end());
  
        // clear incomplete_log_ of current CD 
        incomplete_log_.clear();
      }
      else if (!IsParentLocal()) {
        //SZ: FIXME: need to figure out a way to push logs to parent that resides in other address space
        LOG_DEBUG("Should not come to here...\n");
      }

      ProbeIncompleteLogs();
    }
    else { // kStrict CDs
//      ProbeIncompleteLogs();
      //InvalidateIncompleteLogs();
      //printf("%s %s %lu\n", GetCDID().GetString().c_str(), label_.c_str(), incomplete_log_.size());
    }

#if _LOG_PROFILING
    GetParentHandle()->ptr_cd()->CombineNumLogEntryAndLogVolume(num_log_entry_, tot_log_volume_);
    num_log_entry_ = 0;
    tot_log_volume_ = 0;
#endif

#if _PGAS_VER
    GetParentHandle()->ptr_cd()->SetSyncCounter(++sync_counter_); 
    LOG_DEBUG("Set parent's SC at CD::Complete with value (%d).\n", sync_counter_);
#endif
  }
#endif

#if CD_LIBC_LOG_ENABLED
  //GONG
  if(GetParentHandle()!=NULL) {
    if(IsParentLocal()) {
      if(IsNewLogGenerated_libc())  {
        LOG_DEBUG("Pushing logs to parent...\n");
        libc_log_ptr_->PackAndPushLogs_libc(GetParentHandle()->ptr_cd_);
        //libc_log_ptr_->Print();
        GetParentHandle()->ptr_cd_->libc_log_ptr_->SetNewLogGenerated(true);
        if(libc_log_ptr_->GetCommLogMode()==kGenerateLog) {
          GetParentHandle()->ptr_cd_->libc_log_ptr_->SetCommLogMode(kGenerateLog);
        }
        //GetParentHandle()->ptr_cd_->libc_log_ptr_->Print();
      }

      libc_log_ptr_->Reset();
    }
    else {
      LOG_DEBUG("parent and child are in different memory space - libc\n");
    }
  }
//    std::cout<<"size: "<<mem_alloc_log_.size()<<std::endl;

    //GONG: DO actual free completed mem_alloc_log_
    std::vector<IncompleteLogEntry>::iterator it;
    for (it=mem_alloc_log_.begin(); it!=mem_alloc_log_.end(); it++)
    {
//      printf("check log %p %i %i\n", it->p_, it->complete_, it->pushed_);
      if(it->complete_)
      {       
        if(it->pushed_)
        {
//          printf(" free - completed + pushed - %p\n", it->p_);
          if(it->isrecv_)
            free(it->p_);
          else
            fclose((FILE*)it->p_);
          mem_alloc_log_.erase(it);
          it--;
        }
          if(it==mem_alloc_log_.end()){
            break;
        }
      }
    }
/*    for (it=mem_alloc_log_.begin(); it!=mem_alloc_log_.end(); it++)
    {
        printf(" log - %p\n", it->p_);
    }
*/
    //GONG: push mem_alloc_log_
  if(GetParentHandle()!=NULL)
  {
    if (IsParentLocal() && mem_alloc_log_.size()!=0)
    {
      CD *ptmp = GetParentHandle()->ptr_cd_;
      // push memory allocation logs to parent
      std::vector<IncompleteLogEntry>::iterator ii;
      for(it=mem_alloc_log_.begin(); it!=mem_alloc_log_.end(); it++)
      {
        bool found = false;      
        for(ii=ptmp->mem_alloc_log_.begin(); ii!=ptmp->mem_alloc_log_.end(); ii++)
        {
          if(ii->p_ == it->p_)
                  found = true;
        }
//        std::cout<<"push check: "<<it->p_<<" found: "<<found<<std::endl;      

        if(!found)
        {
          it->pushed_ = true;      
          ptmp->mem_alloc_log_.insert(ptmp->mem_alloc_log_.end(), *it);
//          if(it->complete_)
//            ptmp->mem_alloc_log_.end()->pushed_ = true;
        }
      }
      //remove log after pushing to parent 
      mem_alloc_log_.clear();
      //check parent's
/*      std::cout<<"size(parent) :"<<ptmp->mem_alloc_log_.size()<<" app_side: "<<app_side<<std::endl;
      for(ii=ptmp->mem_alloc_log_.begin(); ii!=ptmp->mem_alloc_log_.end(); ii++)
        printf("parent's %p %i %i\n", ii->p_, ii->complete_, ii->pushed_);
*/        
    }
    else if(!IsParentLocal())
    {
      LOG_DEBUG("Should not come to here...\n");
    }
  }


#endif


  return CDInternalErrT::kOK;
} // CD::Complete ends





#if CD_LIBC_LOG_ENABLED
//GONG
bool CD::PushedMemLogSearch(void* p, CD *curr_cd)
{
  bool ret = false;
  CDHandle *cdh_temp = CDPath::GetParentCD(curr_cd->level());
  if(cdh_temp != NULL)
  {
//    cdh_temp = CDPath::GetParentCD(curr_cd->level());
    CD *parent_CD = cdh_temp->ptr_cd();
    if(parent_CD!=NULL)
    {
      if(parent_CD->mem_alloc_log_.size()!=0)
      {
        std::vector<IncompleteLogEntry>::iterator it;  
        for(it=parent_CD->mem_alloc_log_.begin(); it!=parent_CD->mem_alloc_log_.end();it++)    {
          if(it->p_ == p)
          {
            ret = true;
            //If isnt completed, change it completed.
//            it->complete_ = true;
            break;
          }
        }
      }
      else
      {
        ret = PushedMemLogSearch(p, parent_CD);
      }
    }
    else
    {
      CD_DEBUG("CANNOT find parent CD\n");
      assert(0);
//      exit(1);
    }
  }

  return ret;
}



unsigned int CD::PullMemLogs()
{
  unsigned int num_logs = 0;
  CDHandle *cdh_temp = CDPath::GetParentCD(level());
  std::vector<IncompleteLogEntry>::iterator it;  
  if(cdh_temp != NULL)
  {
    CD *parent_CD = cdh_temp->ptr_cd();
    if(parent_CD!=NULL)
    {
      for(it=parent_CD->mem_alloc_log_.begin(); it!=parent_CD->mem_alloc_log_.end();it++)
      { 
        //bring all logs lower than or equal to current CD
        CD_DEBUG("try to pull logs %p %lu %u %u\n", it->p_, it->flag_, it->level_, level());
        if(it->level_ >= level())
        {
          mem_alloc_log_.insert(mem_alloc_log_.end(), *it);      
          num_logs++;
        }
      }
    }
    else
    {
      CD_DEBUG("CANNOT find parent CD\n");
      assert(0);
    }
  }
  for(it=mem_alloc_log_.begin(); it!=mem_alloc_log_.end(); it++)
    CD_DEBUG("pulled logs %p %i %i %lu\n", it->p_, it->complete_, it->pushed_, it->flag_);
    CD_DEBUG("cur_pos_mem_alloc_log: %u\n", num_logs);

  return num_logs;

}





void* CD::MemAllocSearch(CD *curr_cd, unsigned int level, unsigned long index, void* p_update)
{
//  printf("MemAllocSearch\n");
  void* ret = NULL;
  CDHandle *cdh_temp = CDPath::GetParentCD(curr_cd->level());
//  if(GetCDID().level()!=0)
  if(cdh_temp != NULL)
  {
    CD *parent_CD = cdh_temp->ptr_cd();
    if(parent_CD!=NULL)
    {
      //GONG:       
      if(parent_CD->mem_alloc_log_.size()!=0)
      {
        if(!p_update)
        {
          CD_DEBUG("parent_CD->cur_pos_mem_alloc_log: %lu\n", parent_CD->cur_pos_mem_alloc_log);      
          std::vector<IncompleteLogEntry>::iterator it;
          for (it=parent_CD->mem_alloc_log_.begin(); it!=parent_CD->mem_alloc_log_.end(); it++){
            //should be unique!
            if(it->level_ == level && it->flag_ == index){
              CD_DEBUG("level: %u, index: %lu\n", level, index);   
              ret = it->p_;
//              break;
            }
          }
          //ret = parent_CD->mem_alloc_log_[parent_CD->cur_pos_mem_alloc_log].p_;

         }
         else
         {
           ret = parent_CD->mem_alloc_log_[parent_CD->cur_pos_mem_alloc_log].p_;
           parent_CD->mem_alloc_log_[parent_CD->cur_pos_mem_alloc_log].p_ = p_update;
         }
//       parent_CD->cur_pos_mem_alloc_log++;
       }
       else
       {
         CD_DEBUG("mem_alloc_log_.size()==0, search parent's log\n");      
         ret = MemAllocSearch(parent_CD, level, index, p_update);
      }
    }
    else
    {
      ERROR_MESSAGE("CANNOT find parent CD\n");
//      exit(1);
    }
  }
  else
  {
    ERROR_MESSAGE("rootCD is trying to search further\n");
    assert(0);
//    exit(1);
  }
  
  if(ret == NULL)
  {
    ERROR_MESSAGE("somethig wrong!\n");
//    exit(1);
  
  }

  return ret;
}

#endif



void *CD::SerializeRemoteEntryDir(uint64_t &len_in_bytes) 
{
  Packer entry_dir_packer;
  uint32_t entry_count = 0;

  for(auto it = remote_entry_directory_map_.begin(); 
           it!= remote_entry_directory_map_.end(); ++it) {
    uint64_t entry_len=0;
    void *packed_entry_p=0;
    if( !it->second->name().empty() ){ 
      packed_entry_p = it->second->Serialize(entry_len);
      entry_dir_packer.Add(entry_count++, entry_len, packed_entry_p);
    }
  }
  
  return entry_dir_packer.GetTotalData(len_in_bytes);
}


void CD::DeserializeRemoteEntryDir(EntryDirType &remote_entry_dir, void *object, uint32_t task_count, uint32_t unit_size) 
{
  void *unpacked_entry_p=0;
  uint32_t dwGetID=0;
  uint32_t return_size=0;
  char *begin = (char *)object;

  CD_DEBUG("\n[CD::DeseralizeRemoteEntryDir] addr: %p at level #%u", object, CDPath::GetCurrentCD()->ptr_cd()->GetCDID().level());

  for(uint64_t i=0; i<task_count; i++) {
    Unpacker entry_dir_unpacker;
    while(1) {
      unpacked_entry_p = entry_dir_unpacker.GetNext(begin + i * unit_size, dwGetID, return_size);
      if(unpacked_entry_p == NULL) {

//      cddbg<<"DESER new ["<< cnt++ << "] i: "<< i <<"\ndwGetID : "<< dwGetID << endl;
//      cddbg<<"return size : "<< return_size << endl;
  
        break;
      }
      CDEntry *cd_entry = new CDEntry();
      cd_entry->Deserialize(unpacked_entry_p);

      if(CDPath::GetCurrentCD()->IsHead()) {
        CD_DEBUG("Entry check before insert to entry dir: %s, addr: %p\n", tag2str[cd_entry->entry_tag_].c_str(), object);
      }

      remote_entry_dir.insert(std::pair<ENTRY_TAG_T, CDEntry*>(cd_entry->entry_tag_, cd_entry));
//    remote_entry_dir[cd_entry->entry_tag_] = cd_entry;
    }
  }
}



/*

CD::CDInternalErrT HeadCD::RequestDataMove(int sender, int receiver, const char *found_ref_name)
{
  while( found_entry_list is empty ) {
    CDEntry entry = cur_entry_in_found_entry_list;
    int msg_for_sender0 = receiver;
    char* msg_for_sender1 = found_ref_name;
    int msg_for_receiver = sender;
    
    PMPI_Send(receiver, head, msg_for_receiver); // it will go to receiver
    PMPI_Send(sender, head, msg_for_sender0);    // it will go to sender
  }
}


CD::CDInternalErrT CD::RequestDataMove(int sender, int receiver, const char *found_ref_name)
{

  PMPI_Recv(me, head, msg); // this task will know if I am sender or receiver with this msg from head.

  if( am_I_sender() ){
    PMPI_Send(receiver_from_msg, me, msg);
  } else {
    PMPI_Recv(me, sender_from_msg, msg);
  }

}

CD::CDInternalErrT CD::EntrySearch()
{
  if( !IsHead() ) {
    // Request HeadCD to find the entry in the entry directory
    // It is enough to send just ref_name to Head

  } 
  else { // HeadCD

    // Receive requests from the other tasks and find entry with ref_name
    // If it find ref_name it calls, RequestDataMovement(Sender, Receiver)
    // Receiver will be the requester for EntrySearch()

  }

  RequestDataMove(found_entry_list);
  ForwardEntryToParent(unfound_entry_list);
}


CD::CDInternalErrT CD::GatherEntryDirMapToHead()
{
  char sendBuf[SEND_BUF_SIZE];
  char recvBuf[num_sibling][SEND_BUF_SIZE];

//  PMPI_Allreduce(); // Get the number of entry search requests for HeadCD

  if( !IsHead() ) {

    uint32_t entry_count=0;

    void *packed_entry_dir_p = SerializeEntryDir(entry_count);
    
    PMPI_Gather(sendBuf, num_elements, INTEGER, recvBuf, recv_count, INTEGER, GetHead(), node_id_.color_);

  } 
  else { // HeadCD
    
    PMPI_Gather(sendBuf, num_elements, INTEGER, recvBuf, recv_count, INTEGER, GetHead(), node_id_.color_);

    void *entry_object = getEntryObjFromBuf();

    std::vector<CDEntry> entry_dir = DeserializeEntryDir(entry_object);

  }

  return CD::CDInternalErrT::kOK;
}
*/


//
//void *CD::SerializeEntryDir(uint32_t& entry_count) 
//{
//  Packer entry_dir_packer;
//  uint32_t len_in_bytes=0;
//
//  for(auto it=entry_directory_.begin(); it!=entry_directory_.end(); ++it) {
//    uint32_t entry_len=0;
//    void *packed_entry_p=0;
//    if( !it->name().empty() ){ 
//      packed_entry_p = it->Serialize(entry_len);
//      entry_dir_packer.Add(entry_count++, entry_len, packed_entry_p);
//      len_in_bytes += entry_len;
//    }
//  }
//  
//  return entry_dir_packer.GetTotalData(len_in_bytes);
//}
//
//
//std::vector<CDEntry> CD::DeserializeEntryDir(void *object) 
//{
//  std::vector<CDEntry> entry_dir;
//  Unpacker entry_dir_unpacker;
//  void *unpacked_entry_p=0;
//  uint32_t dwGetID=0;
//  uint32_t return_size=0;
//  while(1) {
//    unpacked_entry_p = entry_dir_unpacker.GetNext((char *)object, dwGetID, return_size);
//    if(unpacked_entry_p == NULL) break;
//    cd_entry.Deserialize(unpacked_entry_p);
//    entry_dir.push_back(cd_entry);
//  }
//
//  return entry_dir;  
//}



//void *HeadCD::SerializeEntryDir(uint32_t& entry_count) 
//{
//  Packer entry_dir_packer;
//  uint32_t len_in_bytes=0;
//
//  for(auto it=entry_directory_.begin(); it!=entry_directory_.end(); ++it) {
//    uint32_t entry_len=0;
//    void *packed_entry_p=0;
//    if( !it->name().empty() ){ 
//      packed_entry_p = it->Serialize(entry_len);
//      entry_dir_packer.Add(entry_count++, entry_len, packed_entry_p);
//      len_in_bytes += entry_len;
//    }
//  }
//
//  for(auto it=remote_entry_directory_.begin(); it!=entry_directory_.end(); ++it) {
//    uint32_t entry_len=0;
//    void *packed_entry_p=0;
//    if( !it->name().empty() ){ 
//      packed_entry_p = it->Serialize(entry_len);
//      entry_dir_packer.Add(entry_count++, entry_len, packed_entry_p);
//      len_in_bytes += entry_len;
//    }
//  }
//  
//  return entry_dir_packer.GetTotalData(len_in_bytes);
//}
//
//
//void HeadCD::DeserializeEntryDir(void *object) 
//{
//  std::vector<CDEntry> entry_dir;
//  Unpacker entry_dir_unpacker;
//  void *unpacked_entry_p=0;
//  uint32_t dwGetID=0;
//  uint32_t return_size=0;
//
//  while(1) {
//    unpacked_entry_p = entry_dir_unpacker.GetNext((char *)object, dwGetID, return_size);
//    if(unpacked_entry_p == NULL) break;
//    cd_entry.Deserialize(unpacked_entry_p);
//    remote_entry_directory_.push_back(cd_entry);
//  }
//
//  return entry_dir;  
//}





// FIXME
PrvMediumT CD::GetPlaceToPreserve()
{

  return prv_medium_;
}

#if 0
bool CD::TestReqComm(bool is_all_valid)
{
  cddbg << "\nCD::TestReqComm at " << GetCDName() << " " << GetNodeID() << "\n" << endl
      << "entry request req Q size : " << entry_request_req_.size() <<endl; cddbg.flush();
  is_all_valid = true;
  for(auto it=entry_request_req_.begin(); it!=entry_request_req_.end(); ) {
    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      cddbg << it->second.valid_ << " ";
      entry_request_req_.erase(it++);
    }
    else {
      cddbg << it->second.valid_ << " ";
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }
  cddbg << endl;
  cddbg << "TestReqComm end"<<endl; cddbg.flush();
  return is_all_valid;
}

bool CD::TestRecvComm(bool is_all_valid)
{
  cddbg << "\nCD::TestReqComm at " << GetCDName() << " " << GetNodeID() << "\n" << endl
      << "entry request req Q size : " << entry_recv_req_.size() <<endl; cddbg.flush();
  is_all_valid = true;
  for(auto it=entry_recv_req_.begin(); it!=entry_recv_req_.end(); ) {
    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      cddbg << it->second.valid_ << " ";
      entry_recv_req_.erase(it++);
    }
    else {
      cddbg << it->second.valid_ << " ";
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }
  cddbg << endl; 
  cddbg << "TestRecvComm end"<<endl; cddbg.flush();
  return is_all_valid;
}


bool CD::TestComm(bool test_until_done)
{
  cddbg << "\nCD::TestComm at " << GetCDName() << " " << GetNodeID() << "\n" << endl;
  cddbg << "entry req Q size : " << entry_req_.size() << endl 
       << "\nentry recv Q size : " << entry_recv_req_.size() << endl
      << "entry request req Q size : " << entry_request_req_.size() <<endl; cddbg.flush();
//       << "\nentry search Q size : " << entry_search_req_.size()
//       << "\nentry recv Q size : " << entry_recv_req_.size()
//       << "\nentry send Q size : " << entry_send_req_.size() << endl;
  bool is_all_valid = true;
  is_all_valid = TestReqComm(is_all_valid);
  cddbg << "==================" << endl; cddbg.flush(); 

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
  cddbg << "+++" << endl; cddbg.flush(); 
    PMPI_Test(&(it->req_), &(it->valid_), &(it->stat_));

    if(it->valid_) {
      cddbg << it->valid_ << " ";
      is_all_valid &= it->valid_;
      entry_req_.erase(it++);
    }
    else {
      cddbg << it->valid_ << " ";
      is_all_valid &= it->valid_;
      ++it;
    }
  }
  cddbg << endl;
  cddbg << "Is all valid : " << is_all_valid << endl;
  cddbg.flush();
  return is_all_valid;
}

#endif


//bool HeadCD::TestComm(bool test_until_done)
//{
//  cddbg << "\nHeadCD::TestComm at " << GetCDName() << " " << GetNodeID() << "\n" << endl;
//  cout << "\nHeadCD::TestComm at " << GetCDName() << " " << GetNodeID() << "\n" << endl;
//  cout << "entry request Q size : " << entry_request_req_.size()
//       << "\nentry search Q size : " << entry_search_req_.size()
//       << "\nentry recv Q size : " << entry_recv_req_.size()
//       << "\nentry send Q size : " << entry_send_req_.size() << endl;
//  bool is_all_valid = true;
//
//  for(auto it=entry_request_req_.begin(); it!=entry_request_req_.end(); ) {
//    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));
//
//    if(it->second.valid_) {
//      entry_request_req_.erase(it++);
//    }
//    else {
//      cout << it->second.valid_ << " ";
//      is_all_valid &= it->second.valid_;
//      ++it;
//    }
//  }
//
//
//  for(auto it=entry_search_req_.begin(); it!=entry_search_req_.end(); ) {
//    PMPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));
//
//    if(it->second.valid_) {
//      entry_search_req_.erase(it++);
//    }
//    else {
//      cout << it->second.valid_ << " ";
//      is_all_valid &= it->second.valid_;
//      ++it;
//    }
//  }
//
//  // Here is the same as CD::TestComm
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
//
//  for(auto it=entry_req_.begin(); it!=entry_req_.end(); ) {
//    PMPI_Test(&(it->req_), &(it->valid_), &(it->stat_));
//
//    if(it->valid_) {
//      entry_req_.erase(it++);
//    }
//    else {
//      cout << it->valid_ << " ";
//      is_all_valid &= it->valid_;
//      ++it;
//    }
//  }
//
//  cout << endl;
//  return is_all_valid;
//} 

/*  This is old comments, but left here just in case.
 *
 *  CD::Preserve(char* data_p, int data_l, enum preserveType prvTy, enum mediumLevel medLvl)
 *  Register data information to preserve if needed.
 *  (For now, since we restore per CD, this registration per cd_entry would be thought unnecessary.
 *  We can already know the data to preserve which is likely to be corrupted in future, 
 *  and its address information as well which is in the current memory space.
 *  Main Purpose: 1. Initialize cd_entry information
 *       2. cd_entry::preserveEntry function call. 
 *          -> performs appropriate operation for preservation per cd_entry such as actual COPY.
 *  We assume that the AS *dst_data could be known from Run-time system 
 *  and it hands to us the AS itself from current rank or another rank.
 *  However, regarding COPY type, it stores the back-up data in current memory space beforehand. 
 *  So it allocates(ManGetNewAllocation) memory space for back-up data
 *  and it copies the data from current or another rank to the address 
 *  for the back-up data in my current memory space. 
 *  And we assume that the data from current or another rank for preservation 
 *  can be also known from Run-time system.
 *  ManGetNewAllocation is for memory allocation for CD and cd_entry.
 *  CD_MALLOC is for memory allocation for Preservation with COPY.
 *
 *  Jinsuk: For re-execution we will use this function call to restore the data. 
 *  So basically it needs to know whether it is in re-execution mode or not.
 *  it is in re-execution mode, so instead of preserving data, restore the data.
 *  Two options, one is to do the job here, 
 *  another is that just skip and do nothing here but do the restoration job in different place, 
 *  and go though all the CDEntry and call Restore() method. 
 *  The later option seems to be more efficient but it is not clear that 
 *  whether this brings some consistency issue as restoration is done at the very beginning 
 *  while preservation was done one by one 
 *  and sometimes there could be some computation in between the preservations.. (but wait is it true?)
 *  
 *  Jinsuk: Because we want to make sure the order is the same as preservation, we go with  Wait...... It does not make sense... 
 *  Jinsuk: For now let's do nothing and just restore the entire directory at once.
 *  Jinsuk: Caveat: if user is going to read or write any memory space that will be eventually preserved, 
 *  FIRST user need to preserve that region and use them. 
 *  Otherwise current way of restoration won't work. 
 *  Right now restore happens one by one. 
 *  Everytime restore is called one entry is restored. 
 *
 */

CDErrT CD::Preserve(void *data, 
                    uint64_t &len_in_bytes, 
                    uint32_t preserve_mask, 
                    const char *my_name, 
                    const char *ref_name, 
                    uint64_t ref_offset, 
                    const RegenObject *regen_object, 
                    PreserveUseT data_usage)
{

  CD_DEBUG("\n\n[CD::Preserve] data addr: %p, len: %lu, entry name : %s, ref name: %s, [cd_exec_mode : %d]\n", 
           data, len_in_bytes, my_name, ref_name, cd_exec_mode_); 

  CD_DEBUG("prv mask (%d) : %d(kCopy) %d(kRef) %d(kRegen) , kCoop : %d]\n\n",
           preserve_mask,
           CHECK_PRV_TYPE(preserve_mask, kCopy),
           CHECK_PRV_TYPE(preserve_mask, kRef),
           CHECK_PRV_TYPE(preserve_mask, kRegen),
            CHECK_PRV_TYPE(preserve_mask, kCoop));

  if(cd_exec_mode_  == kExecution ) {      // Normal execution mode -> Preservation
//    cddbg<<"my_name "<< my_name<<endl;
    switch( InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage) ) {
      case CDInternalErrT::kOK            : 
              return CDErrT::kOK;
      case CDInternalErrT::kExecModeError :
              return CDErrT::kError;
      case CDInternalErrT::kEntryError    : 
              return CDErrT::kError;
      default : assert(0);
    }

  }
  else if(cd_exec_mode_ == kReexecution) { // Re-execution mode -> Restoration
    // it is in re-execution mode, so instead of preserving data, restore the data 
    // Two options, one is to do the job here, 
    // another is that just skip and do nothing here but do the restoration job in different place 
    // and go though all the CDEntry and call Restore() method. The later option seems to be more efficient 
    // but it is not clear that whether this brings some consistency issue as restoration is done at the very beginning 
    // while preservation was done one by one and sometimes there could be some computation in between the preservations.. 
    // (but wait is it true?)
  
    // Jinsuk: Because we want to make sure the order is the same as preservation, we go with  Wait...... It does not make sense... 
    // Jinsuk: For now let's do nothing and just restore the entire directory at once.
    // Jinsuk: Caveat: if user is going to read or write any memory space that will be eventually preserved, 
    // FIRST user need to preserve that region and use them. Otherwise current way of restoration won't work. 
    // Right now restore happens one by one. 
    // Everytime restore is called one entry is restored.
    CD_DEBUG("\n\nReexecution!!! entry directory size : %zu\n\n", entry_directory_.size());

    if( iterator_entry_ != entry_directory_.end() ) { // normal case

//      printf("Reexecution mode start...\n");
      CD_DEBUG("\n\nNow reexec!!! %d\n\n", iterator_entry_count++);

      CDEntry *cd_entry = &*iterator_entry_;
      ++iterator_entry_;

      CDErrT cd_err;

      switch( cd_entry->Restore() ) {
        case CDEntry::CDEntryErrT::kOK : 
          cd_err = CDErrT::kOK; 
          break;
        case CDEntry::CDEntryErrT::kOutOfMemory : 
          cd_err = CDErrT::kError;
          break;
        case CDEntry::CDEntryErrT::kFileOpenError : 
          cd_err = CDErrT::kError;
          break;
        case CDEntry::CDEntryErrT::kEntrySearchRemote : {
//#if _MPI_VER
//          while(!TestReqComm()) {
//            CheckMailBox();
//          }
//          while( !(TestComm()) ) {
//
//          }
//#endif
          cd_err = CDErrT::kError;
          break;
        }
        default : assert(0);
      }

      if(iterator_entry_ != entry_directory_.end()) {

//        if(task_size() > 1) {
//        PMPI_Win_fence(0, mailbox_);
//        }

#if _MPI_VER
        CheckMailBox();
        if(IsHead()) { 
        
//          TestComm();
//          CheckMailBox();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 1] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestReqComm();
//          TestComm();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 2] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//          TestReqComm();
//          CheckMailBox();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 3] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
          TestComm();
          TestReqComm();

          if(task_size() > 1) {
//            PMPI_Win_fence(0, mailbox_);
            CheckMailBox();
          }
          TestRecvComm();

        }
        else {
          TestComm();
          TestReqComm();
          if(task_size() > 1) {
//            PMPI_Win_fence(0, mailbox_);
            CheckMailBox(); 
          }
          TestRecvComm();
//          TestComm();
//          TestReqComm();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 1] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//          TestReqComm();
//          CheckMailBox();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 2] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestReqComm();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 3] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//          TestRecvComm();
//          CheckMailBox();
//          
//        
        }
#endif

      }
      else { // The end of entry directory

#if _MPI_VER
        CD_DEBUG("Test Asynch messages until start at %s / %s\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
  
        while( !(TestComm()) ); 
 
//        if(IsHead()) { 
//          CheckMailBox();
//          if(task_size() > 1) {
//            PMPI_Win_fence(0, mailbox_);
//          }
//        }
//        else {
//  
//          if(task_size() > 1) {
//            PMPI_Win_fence(0, mailbox_);
//          }
//          CheckMailBox();
//        }
        CheckMailBox();

//        if(IsHead()) { 
//        
//          TestComm();
//          CheckMailBox();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 1] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 2] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//          CheckMailBox();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 3] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//        }
//        else {
//      
//          TestComm();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 1] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//          CheckMailBox();
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 1] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          if(task_size() > 1) {
//            CD_DEBUG("\n\n[Barrier 1] CD - %s / %s \n\n", GetCDName().GetString().c_str(), GetNodeID().GetString().c_str());
//            Sync(color());
//          }
//          TestComm();
//          CheckMailBox();
//        }
  
        while(!TestRecvComm());
  
        CD_DEBUG("Test Asynch messages until done \n");

#endif
        CD_DEBUG("Return to kExec\n");
        cd_exec_mode_ = kExecution;

        // It is also possible case that current task sets reexec from upper level,
        // but actually it was reexecuting some lower level CDs. 
        // while executing lower-level reexecution,
        // reexec_level was set to upper level, 
        // which means escalation request from another task. 
        // Therefore, this flag should be carefully reset to exec mode.
        // level == reexec_level enough condition for resetting to exec mode,
        // because nobody overwrited these flags set by current flag.
        // (In other word, nobody requested escalation requests)
        // This current task is performing reexecution corresponding to these flag set by itself.
        if(level() == reexec_level) {
          need_reexec = false;
          reexec_level = INVALID_ROLLBACK_POINT;
//          printf("Reexec %u\n", reexec_level);
        }
        // This point means the beginning of body stage. Request EntrySearch at this routine
      }

      return cd_err;
 
    }
    else {  // abnormal case
      //return CDErrT::kOK;

      CD_DEBUG("The end of reexec!!!\n");
      // NOT TRUE if we have reached this point that means now we should actually start preserving instead of restoring.. 
      // we reached the last preserve function call. 
      // Since we have reached the last point already now convert current execution mode into kExecution
      
//      ERROR_MESSAGE("Error: Now in re-execution mode but preserve function is called more number of time than original"); 
      CD_DEBUG("Now reached end of entry directory, now switching to normal execution mode\n");

      cd_exec_mode_  = kExecution;    
      switch( InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage) ) {
        case CDInternalErrT::kOK            : 
          return CDErrT::kOK;
        case CDInternalErrT::kExecModeError : 
          return CDErrT::kError;
        case CDInternalErrT::kEntryError    : 
          return CDErrT::kError;
        default : assert(0);
      }

    }

    CD_DEBUG("Reexecution mode finished...\n");
  }   // Re-execution mode ends
  else {  // Suspension mode
    // Is it okay ?
    // Is it possible to call Preserve() at Suspension mode?
    assert(0);
  }

  
  return kError; // we should not encounter this point
}



// Non-blocking Preserve
CDErrT CD::Preserve(CDEvent &cd_event,     
                    void *data_ptr, 
                    uint64_t &len, 
                    uint32_t preserve_mask, 
                    const char *my_name, 
                    const char *ref_name, 
                    uint64_t ref_offset, 
                    const RegenObject *regen_object, 
                    PreserveUseT data_usage)
{
  // stub
  return kError; 
}

CD::CDInternalErrT 
CD::InternalPreserve(void *data, 
                     uint64_t &len_in_bytes, 
                     uint32_t preserve_mask, 
                     std::string my_name, 
                     const char *ref_name, 
                     uint64_t ref_offset, 
                     const RegenObject* regen_object, 
                     PreserveUseT data_usage)
{
  CD_DEBUG("\n[CD::InternalPreserve] cd_exec_mode : %d\n", cd_exec_mode_);
  if(cd_exec_mode_  == kExecution ) { // Normal case
    CD_DEBUG("Normal execution mode (internal preservation call)\n");

    // Now create entry and add to list structure.
    //FIXME Jinsuk: we don't have the way to determine the storage   
    // Let's move all these allocation deallocation stuff to CDEntry. 
    // Object itself will know better than class CD. 

    CDEntry* cd_entry = 0;

    void *dst_data = NULL;
    if( CHECK_PRV_TYPE(preserve_mask, kSerdes) ) {
      dst_data = (static_cast<Serializable *>(data))->Serialize(len_in_bytes);
     // printf("[%s] serialize len2 : %lu\n", __func__,len_in_bytes);
      assert(len_in_bytes);
    }
   // printf("[%s] serialize len2 : %lu\n", __func__,len_in_bytes);

    // Get cd_entry
    if( CHECK_PRV_TYPE(preserve_mask,kCopy) ) { // via-copy, so it saves data right now!

      CD_DEBUG("\nPreservation via Copy to %d(memory or file)\n", GetPlaceToPreserve());
      CD_DEBUG("Prv Mask : %d, Is it coop? %d, medium : %d, cd type : %d\n", 
               preserve_mask, CHECK_PRV_TYPE(preserve_mask, kCoop), GetPlaceToPreserve(), cd_type_);

      switch(GetPlaceToPreserve()) {
        case kDRAM: {
          CD_DEBUG("[MEDIUM TYPE : kDRAM] ------------------------------------------\n");
#if _PGAS_VER
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kMemory, dst_data, len_in_bytes, cd_id_.node_id_), 
                                 my_name, this, GetSyncCounter());
#else
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kMemory, dst_data, len_in_bytes, cd_id_.node_id_), 
                                 my_name, this, (uint32_t)prv_medium_ | (uint32_t)preserve_mask);
#endif

//          if(!CHECK_PRV_TYPE(preserve_mask, kSerdes)) {
//          CDEntry::CDEntryErrT err = cd_entry->SaveMem();
          CDEntry::CDEntryErrT err = cd_entry->Save();

          entry_directory_.push_back(*cd_entry);

          CD_DEBUG("Push back one entry. entry directory size : %zu\n", entry_directory_.size());

          if( !my_name.empty() ) {

            if( !CHECK_PRV_TYPE(preserve_mask, kCoop) ) {
              entry_directory_map_[cd_hash(my_name)] = cd_entry;
              assert(entry_directory_map_[cd_hash(my_name)]);
              assert(entry_directory_map_[cd_hash(my_name)]->src_data_.address_data());

              CD_DEBUG("Register local entry dir. my_name : %s - %u, value : %d, address: %p\n", 
                      entry_directory_map_[cd_hash(my_name)]->name().c_str(), 
                      cd_hash(my_name), 
                      *(reinterpret_cast<int*>(entry_directory_map_[cd_hash(my_name)]->src_data_.address_data())),
                      cd_entry->dst_data_.address_data());
            } 
            else{
              remote_entry_directory_map_[cd_hash(my_name)] = cd_entry;
              assert(remote_entry_directory_map_[cd_hash(my_name)]);
              assert(remote_entry_directory_map_[cd_hash(my_name)]->src_data_.address_data());
              CD_DEBUG("Register remote entry dir. my_name : %s - %u, value : %d, address: %p\n", 
                      remote_entry_directory_map_[cd_hash(my_name)]->name().c_str(), 
                      cd_hash(my_name), 
                      *(reinterpret_cast<int*>(remote_entry_directory_map_[cd_hash(my_name)]->src_data_.address_data())),
                      cd_entry->dst_data_.address_data());

            }

          }
          else {
            ERROR_MESSAGE("No entry name is provided. Currently it is not supported.\n");
          }
          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }
        case kHDD: 
        case kSSD:
        {
          char *filepath = file_handle_.GetFilePath();
          CD_DEBUG("[MEDIUM TYPE : File %d] ------------------------------------------\n", GetPlaceToPreserve());
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kOSFile, dst_data, len_in_bytes, cd_id_.node_id_, 
//                                            file_handle_.GetFilePath(),
                                            filepath,
                                            file_handle_.fp_, 
                                            file_handle_.UpdateFilePos(len_in_bytes)), 
                                 my_name, this, (uint32_t)prv_medium_ | (uint32_t)preserve_mask);

//          CDEntry::CDEntryErrT err = cd_entry->SaveFile();
          CDEntry::CDEntryErrT err = cd_entry->Save();

          entry_directory_.push_back(*cd_entry); 

          CD_DEBUG("Push back one entry. entry directory size : %zu\n", entry_directory_.size());

          if( !my_name.empty() ) {
            if( !CHECK_PRV_TYPE(preserve_mask, kCoop) ) {
              entry_directory_map_[cd_hash(my_name)] = cd_entry;
              assert(entry_directory_map_[cd_hash(my_name)]);
              assert(entry_directory_map_[cd_hash(my_name)]->src_data_.address_data());

              CD_DEBUG("Register local entry dir. my_name : %s - %u, value : %d, address: %p\n", 
                      entry_directory_map_[cd_hash(my_name)]->name().c_str(), 
                      cd_hash(my_name), 
                      *(reinterpret_cast<int*>(entry_directory_map_[cd_hash(my_name)]->src_data_.address_data())),
                      cd_entry->dst_data_.address_data());

            }
            else {
              remote_entry_directory_map_[cd_hash(my_name)] = cd_entry;
              assert(remote_entry_directory_map_[cd_hash(my_name)]);
              assert(remote_entry_directory_map_[cd_hash(my_name)]->src_data_.address_data());
              CD_DEBUG("Register remote entry dir. my_name : %s - %u, value : %d, address: %p\n", 
                      remote_entry_directory_map_[cd_hash(my_name)]->name().c_str(), 
                      cd_hash(my_name), 
                      *(reinterpret_cast<int*>(remote_entry_directory_map_[cd_hash(my_name)]->src_data_.address_data())),
                      cd_entry->dst_data_.address_data());
            }
          }
          else {
            ERROR_MESSAGE("No entry name is provided. Currently it is not supported.\n");
          }

          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }

        case kPFS: {
          CD_DEBUG("[MEDIUM TYPE : kPFS] ------------------------------------------\n");

          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kPFS, dst_data, len_in_bytes, cd_id_.node_id_), 
                                 my_name, this, (uint32_t)prv_medium_ | (uint32_t)preserve_mask);

          //Do we need to check for anything special for accessing to the global filesystem? 
          //Potentially=> CDEntry::CDEntryErrT err = cd_entry->SavePFS(file_handle_.GetFilePath(), 
          //file_handle_.isPFSAccessible(), &(file_handle_.PFSlog));
          //I don't know what should I do with the log parameter. I just add it for compatibility.
//          CDEntry::CDEntryErrT err = cd_entry->SavePFS(); 
          CDEntry::CDEntryErrT err = cd_entry->Save(); 

          entry_directory_.push_back(*cd_entry); 
          CD_DEBUG("Push back one entry. entry directory size : %zu\n", entry_directory_.size());
 
          if( !my_name.empty() ) {
            if( !CHECK_PRV_TYPE(preserve_mask, kCoop) ) {
              entry_directory_map_[ cd_hash(my_name) ] = cd_entry;
              assert( entry_directory_map_[ cd_hash( my_name ) ] );
              assert( entry_directory_map_[ cd_hash( my_name ) ]->src_data_.address_data() );

              CD_DEBUG("Register remote entry dir. my_name : %s - %u, value : %d, address: %p\n", 
                      entry_directory_map_[cd_hash(my_name)]->name().c_str(), 
                      cd_hash(my_name), 
                      *(reinterpret_cast<int*>(entry_directory_map_[cd_hash(my_name)]->src_data_.address_data())),
                      cd_entry->dst_data_.address_data());
            }
            else {
              remote_entry_directory_map_[cd_hash(my_name)] = cd_entry;
              assert(remote_entry_directory_map_[cd_hash(my_name)]);
              assert(remote_entry_directory_map_[cd_hash(my_name)]->src_data_.address_data());

              CD_DEBUG("Register remote entry dir. my_name : %s - %u, value : %d, address: %p\n", 
                      remote_entry_directory_map_[cd_hash(my_name)]->name().c_str(), 
                      cd_hash(my_name), 
                      *(reinterpret_cast<int*>(remote_entry_directory_map_[cd_hash(my_name)]->src_data_.address_data())),
                      cd_entry->dst_data_.address_data());
  
            }
          }
          else {
            ERROR_MESSAGE("No entry name is provided. Currently it is not supported.\n");
          }
          CD_DEBUG("-------------------------------------------------------------\n");
          return (err == CDEntry::CDEntryErrT::kOK) ? CDInternalErrT::kOK : CDInternalErrT::kEntryError; 
        }

        default:
          ERROR_MESSAGE("Unsupported medium type. medium type : %d\n", GetPlaceToPreserve());
      }

    } // end of preserve via copy
    else if( CHECK_PRV_TYPE(preserve_mask, kRef) ) { // via-reference

      CD_DEBUG("Preservation via %d (reference)\n", GetPlaceToPreserve());

      // set handle type and ref_name/ref_offset
      cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                             DataHandle(DataHandle::kReference, 0, len_in_bytes, ref_name, ref_offset, cd_id_.node_id_), 
                             my_name, this, (uint32_t)prv_medium_ | (uint32_t)preserve_mask);
//      cd_entry->set_my_cd(this); // this required for tracking parent later.. this is needed only when via ref

      entry_directory_.push_back(*cd_entry);  
//      entry_directory_map_.emplace(ref_name, *cd_entry);
//      entry_directory_map_[ref_name] = *cd_entry;
      if( !my_name.empty() ) {
        entry_directory_map_[cd_hash(my_name)] = cd_entry;

        if( CHECK_PRV_TYPE(preserve_mask, kCoop) ) {
          CD_DEBUG("[CD::InternalPreserve] Error : kRef | kCoop\nTried to preserve via reference but tried to allow itself as reference to other node. If it allow itself for reference locally, it is fine!");
        }
      }

      return CDInternalErrT::kOK;
      
    }
    else if( CHECK_PRV_TYPE(preserve_mask, kRegen) ) { // via-regeneration

      //TODO
      ERROR_MESSAGE("Preservation via Regeneration is not supported, yet. :-(");

      return CDInternalErrT::kOK;
    }
    else {  // Preservation Type is none of kCopy, kRef, kRegen.

      ERROR_MESSAGE("\nUnsupported preservation type : %d\n", preserve_mask);

      return CDInternalErrT::kEntryError;
    }
  }
  else { // Abnormal case

    ERROR_MESSAGE("\nkReexecution mode in CD::InternalPreserve. It must be called in kExecution mode.\n");

    return kExecModeError; 
  }

  ERROR_MESSAGE("Something wrong\n");
  return kExecModeError; 

}




/*  CD::Restore()
 *  (1) Copy preserved data to application data. It calls restoreEntry() at each node of entryDirectory list.
 *
 *  (2) Do something for process state
 *
 *  (3) Logged data for recovery would be just replayed when reexecuted. We need to do something for this.
 */
CDErrT CD::Restore()
{
  //Jinsuk: Side question: how do we know if the recovery was successful or not? 
  //It seems like this is very important topic, we could think the recovery was successful, 
  //but we still can miss, or restore with wrong data and the re-execute, 
  //for such cases, do we have a good detection mechanisms?
  // Jinsuk: 02092014 are we going to leave this function? 
  //It seems like this may not be required. Or for optimization, we can do this. 
  //Bulk restoration might be more efficient than fine grained restoration for each block.
  // this code section is for restoring all the cd entries at once. 
  // Now this is defunct. 

  iterator_entry_ = entry_directory_.begin();

  //TODO currently we only have one iterator. This should be maintined to figure out the order. 
  // In case we need to find reference name quickly we will maintain seperate structure such as binary search tree and each item will have CDEntry *.


  //GONG
  begin_ = false;

  return CDErrT::kOK;
}



/*  CD::detect()
 *  (1) Call all the user-defined error checking functions.
 *    Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *  (2) 
 *
 */
CD::CDInternalErrT CD::Detect(uint32_t &rollback_point)
{
  CD::CDInternalErrT internal_err = kOK;
  rollback_point = -1;
  // STUB

  return internal_err;
}

void CD::Recover(bool collective)
//void CD::Recover()
{

//  if(collective) {
//    if(task_size() > 1) {
//      CD_DEBUG("[%s] fence 1 in at %s level %u\n", __func__, name_.c_str(), level());
//    //  PMPI_Barrier(color());
//      MPI_Win_fence(0, mailbox_);
//      CheckMailBox();
//      CD_DEBUG("[%s] fence 2 in at %s level %u\n", __func__, name_.c_str(), level());
//      MPI_Win_fence(0, mailbox_);
//      CheckMailBox();
//      CD_DEBUG("[%s] fence out \n\n", __func__);
//    } else {
//      CD_DEBUG("[%s] No fence\n", __func__);
//    }
//  } else {
//    CD_DEBUG("[%s] Not call fence %s\n", __func__, name_.c_str());
//  }
  recoverObj_->Recover(this); 
} 


CD::CDInternalErrT CD::Assert(bool test)
{

  CDInternalErrT internal_err = kOK;


#if _MPI_VER
  if(test == false) {
    need_reexec = true;
//    bool need_escalation = false;
    if(totalTaskSize != 1) {
      
      // Before Assert(false), some other tasks might raise error flag in this task,
      // and that can be less than this point, which means escalation request.
      // reexec_level was set to a number less than current task's level,
      // Then do not set it to current task's level, because it needs to be escalated.
      if(reexec_level >= level()) {
        reexec_level = level();
      }
      else {
        need_escalation = true;
      }

      if(task_size() > 1) {
        if(need_escalation) {
          GetParentCD(reexec_level)->SetMailBox(kErrorOccurred);
        }
        else {
          SetMailBox(kErrorOccurred);
        }
        internal_err = kErrorReported;
      }
      else { // There is a single task in this CD.
        
        if(need_escalation) { // Only escalation case need to set mailbox
          SetMailBox(kErrorOccurred);
          internal_err = kErrorReported;
        }
        else {
          // Do not report
        }
      }
      // SetMailBox for MPI-version is done
    }
    else { // This is just for the case that it is compiled with MPI_VER but rank size is 1.
      if(need_escalation) 
//        reexec_level = level();
      return internal_err;  // Actually this will not be reached. 
    }
  }

//  Sync(color());
//  
//  if(IsHead()) {
//    CheckMailBox();
//    if(task_size() > 1) {
//      Sync(color());
//    }
//  }
//  else {
//    if(task_size() > 1) {
//      Sync(color());
//    }
//    CheckMailBox();
//  }
  CheckMailBox();
#else
  if(test == false) {
    need_reexec = true;
  }

#endif

  return internal_err;
}

//void CD::SetRecoverFlag(void) {
//  // Before Assert(false), some other tasks might raise error flag in this task,
//  // and that can be less than this point, which means escalation request.
//  // reexec_level was set to a number less than current task's level,
//  // Then do not set it to current task's level, because it needs to be escalated.
//  if(reexec_level > level()) {
//    reexec_level = level();
//  }
//  else {
//    need_escalation = true;
//  }
//}

CD::CDInternalErrT CD::RegisterDetection(uint32_t system_name_mask, 
                                     uint32_t system_loc_mask)
{
  // STUB
  return CDInternalErrT::kOK;
} 

CD::CDInternalErrT CD::RegisterRecovery(uint32_t error_name_mask, 
                                    uint32_t error_loc_mask, 
                                    RecoverObject *recover_object)
{
  recoverObj_ = recover_object;
  return CDInternalErrT::kOK;
}

CD::CDInternalErrT CD::RegisterRecovery(uint32_t error_name_mask, 
                                    uint32_t error_loc_mask, 
                                    CDErrT(*recovery_func)(std::vector< SysErrT > errors))
{
  // STUB
  return CDInternalErrT::kOK;
}


CDErrT CD::Reexecute(void)
{

  return InternalReexecute();
}

CDErrT HeadCD::Reexecute(void)
{

  return InternalReexecute();
}

CDErrT CD::InternalReexecute(void)
{
  // SZ: FIXME: this is not safe because programmers may have their own RecoverObj
  //            we need to change the cd_exec_mode_ and comm_log_mode_ outside this function.
  // KL: I think it should be here, because recovery action might not be reexecution, but something else.

  CD_DEBUG("[CD::InternalReexecute] reexecuted : %d, reexecution # : %d\n", reexecuted_, num_reexecution_);

  cd_exec_mode_ = kReexecution; 
  reexecuted_ = true;
  num_reexecution_++;

//  auto rit = num_exec_map_.find(name_);
//  if(rit == num_exec_map_.end()){
//    assert(0); 
//  } else {
//    num_exec_map_[name_].second += 1;
//  }

#if comm_log
  // SZ
  //// change the comm_log_mode_ into CommLog class
  //comm_log_mode_ = kReplayLog;  
  if (MASK_CDTYPE(cd_type_) == kRelaxed) {
    comm_log_ptr_->ReInit();
    SetCDLoggingMode(kRelaxedCDRead);
  }
  
  //SZ: reset to child_seq_id_ = 0 
  LOG_DEBUG("Reset child_seq_id_ to 0 at the point of re-execution\n");
  child_seq_id_ = 0;
#endif

#if CD_LIBC_LOG_ENABLED
  //GONG
  if(libc_log_ptr_!=NULL) {
    libc_log_ptr_->ReInit();
    LOG_DEBUG("reset log_table_reexec_pos_\n");
    //  libc_log_ptr_->Print();
  }
#endif

  //TODO We need to make sure that all children has stopped before re-executing this CD.
  Stop();


  //TODO We need to consider collective re-start. 
  if(ctxt_prv_mode_ == kExcludeStack) {

    CD_DEBUG("longjmp\n");
   
//    CD_DEBUG("\n\nlongjmp \t %d at level #%u\n", (tmp<<jmp_buffer_).rdbuf(), level());

    longjmp(jmp_buffer_, jmp_val_);
  }
  else if (ctxt_prv_mode_ == kIncludeStack) {
    CD_DEBUG("setcontext at level : %d (reexec_level: %d) (%s)\n", level(), reexec_level, name_.c_str());
    setcontext(&ctxt_); 
  }

  return CDErrT::kOK;
}






// Let's say re-execution is being performed and thus all the children should be stopped, 
// need to provide a way to stop currently running task and then re-execute from some point. 
CDErrT CD::Stop(CDHandle* cdh)
{
  //TODO Stop current CD.... here how? what needs to be done? 
  // may be wait for others to complete? wait for an event?
  //if current thread (which stop function is running) and the real cd's thread id is different, 
  //that means we need to stop real cd's thread..
  //otherwise what should we do here? nothing?

  // Maybe blocking Recv here? or PMPI_Wait()?
  
  return CDErrT::kOK;
}

CDErrT CD::Resume(void)
{
  return CDErrT::kOK;
}

CDErrT CD::AddChild(CDHandle* cd_child) 
{ 
  // Do nothing?
  if(cd_child->IsHead()) {
    //Send it to Head
  }

  return CDErrT::kOK;  
}

CDErrT CD::RemoveChild(CDHandle* cd_child) 
{
  // Do nothing?
  return CDErrT::kOK;
}

//FIXME 11112014
void RegisterMeToParentHeadCD(int taskID)
{
  
//  PMPI_Put(&taskID, 1, PMPI_INTEGER, cd_id().node_id().head(), target_disp, target_count, PMPI_INTEGER, &win);

}

HeadCD::HeadCD()
{
  error_occurred = false;
}

HeadCD::HeadCD( CDHandle* cd_parent, 
                    const char* name, 
                    const CDID& cd_id, 
                    CDType cd_type, 
                    PrvMediumT prv_medium, 
                    uint64_t sys_bit_vector)
  : CD(cd_parent, name, cd_id, cd_type, prv_medium, sys_bit_vector)
{
  RegisterMeToParentHeadCD(cd_id.task_in_color());
  error_occurred = false;
//  cd_parent_ = cd_parent;
}


//CD::CDInternalErrT CD::CreateInternalMemory(CD *cd_ptr, const CDID& new_cd_id)
//{
//  int task_count = new_cd_id.task_count();
//  cddbg << "in CD::Create Internal Memory"<<endl;
//  if(task_count > 1) {
//    for(int i=0; i<task_count; ++i) {
//    
//      PMPI_Win_create(NULL, 0, 1,
//                     PMPI_INFO_NULL, new_cd_id.color(), &((cd_ptr->mailbox_)[i]));
//  
//    }
//  }
//  else {
//    cddbg << "CD::mpi win create for "<< task_count << " mailboxes"<<endl;
//    PMPI_Win_create(NULL, 0, 1,
//                   PMPI_INFO_NULL, new_cd_id.color(), cd_ptr->mailbox_);
//    
//  }
//
//  return CD::CDInternalErrT::kOK;
//}
//
//
//CD::CDInternalErrT CD::DestroyInternalMemory(CD *cd_ptr)
//{
//
//  cddbg << "in CD::Destroy Internal Memory"<<endl;
//  int task_count = cd_id_.task_count();
//  if(task_count > 1) {
//    cddbg << "mpi win free for "<< task_count << " mailboxes"<<endl;
//    for(int i=0; i<task_count; ++i) {
//      cddbg << i << endl;
//      PMPI_Win_free(&(cd_ptr->mailbox_[i]));
//    }
//  }
//  else {
//    cddbg << "mpi win free for one mailbox"<<endl;
//    PMPI_Win_free(cd_ptr->mailbox_);
//  }
//
//  return CD::CDInternalErrT::kOK;
//}

//CD::CDInternalErrT HeadCD::CreateInternalMemory(HeadCD *cd_ptr, const CDID& new_cd_id)
//{
//  cddbg << "HeadCD create internal memory " << endl;
//  int task_count = new_cd_id.task_count();
//#if _MPI_VER
////  if(new_cd_id.color() == PMPI_COMM_WORLD) {
////    cddbg << "\n\nthis is root! " << task_count << "\n\n"<<endl;
////    PMPI_Alloc_mem(sizeof(CDFlagT)*task_count, 
////                  PMPI_INFO_NULL, &event_flag_);
////    mailbox_ = new CDMailBoxT[task_count];
////    for(int i=0; i<task_count; ++i) {
////      PMPI_Win_create(&((event_flag_)[i]), 1, sizeof(CDFlagT),
////                     PMPI_INFO_NULL, PMPI_COMM_WORLD, &((mailbox_)[i]));
////    }
////
////
////    exit(-1);
////    return CD::CDInternalErrT::kOK;
////  }
//
//  PMPI_Alloc_mem(sizeof(CDFlagT)*task_count, 
//                PMPI_INFO_NULL, &(cd_ptr->event_flag_));
//
//  if(task_count > 1) {
//
//    cddbg << "HeadCD mpi win create for "<< task_count << " mailboxes"<<endl;
//    mailbox_ = new CDMailBoxT[task_count];
//    for(int i=0; i<task_count; ++i) {
//      PMPI_Win_create(&((cd_ptr->event_flag_)[i]), 1, sizeof(CDFlagT),
//                     PMPI_INFO_NULL, new_cd_id.color(), &((cd_ptr->mailbox_)[i]));
//    }
//
////      PMPI_Win_create(event_flag_, task_count, sizeof(CDFlagT),
////                     PMPI_INFO_NULL, new_cd_id.color(), mailbox_);
//
//  }
//  else {
//    cddbg << "HeadCD mpi win create for "<< task_count << " mailboxes"<<endl;
//    PMPI_Win_create(cd_ptr->event_flag_, task_count, sizeof(CDFlagT),
//                   PMPI_INFO_NULL, new_cd_id.color(), cd_ptr->mailbox_);
//    
//  }
//
////  PMPI_Win_allocate(task_count*sizeof(CDFlagT), sizeof(CDFlagT), PMPI_INFO_NULL, new_cd_id.color(), &event_flag_, &mailbox_);
//  //cddbgBreak();
//#endif
//  return CD::CDInternalErrT::kOK;
//}
//
//CD::CDInternalErrT HeadCD::DestroyInternalMemory(HeadCD *cd_ptr)
//{
//#if _MPI_VER
//  cddbg << "in HeadCD::Destroy"<<endl;
//  int task_count = cd_id_.task_count();
//  if(task_count > 1) {
//    cddbg << "HeadCD mpi win free for "<< task_count << " mailboxes"<<endl;
//    for(int i=0; i<task_count; ++i) {
//      cddbg << i << endl;
//      PMPI_Win_free(&(cd_ptr->mailbox_[i]));
//    }
//  }
//  else {
//    cddbg << "HeadCD mpi win free for one mailbox"<<endl;
//    PMPI_Win_free(cd_ptr->mailbox_);
//  }
//  PMPI_Free_mem(cd_ptr->event_flag_);
//
//
//
////  PMPI_Win_free(&mailbox_);
//#endif
//  return CD::CDInternalErrT::kOK;
//}






HeadCD::~HeadCD()
{
//  DestroyInternalMemory();
//    PMPI_Free_mem(event_flag_);
}

CDHandle *HeadCD::Create(CDHandle* parent, 
                     const char* name, 
                     const CDID& child_cd_id, 
                     CDType cd_type, 
                     uint64_t sys_bit_vector, 
                     CD::CDInternalErrT *cd_internal_err)
{

  // CD runtime system are not able to be aware of how many children the current HeadCD task will have.
  // So it does not make sense to create mailboxes for children CD's HeadCD tasks at the HeadCD creation time.
  // It does make sense to create those mailboxes when it create children CDs.
  //
//  HeadCD *ptr_headcd = dynamic_cast<HeadCD*>(ptr_cd_);


//  int family_mailbox_count = child_cd_id.sibling_count() + 1;
//  for(int i=0; i<num_children; ++i) {
//    PMPI_Win_create(&((ptr_headcd->family_event_flag_)[i]), 1, sizeof(CDFlagT),
//                   PMPI_INFO_NULL, new_cd_id.color(), &((ptr_headcd->family_mailbox_)[i]));
//  }


  /// Create CD object with new CDID
  CDHandle* new_cd_handle = NULL;
  *cd_internal_err = InternalCreate(parent, name, child_cd_id, cd_type, sys_bit_vector, &new_cd_handle);
  assert(new_cd_handle != NULL);


  this->AddChild(new_cd_handle);

  return new_cd_handle;
  /// Send entry_directory_map_ to HeadCD
//  GatherEntryDirMapToHead();
}


CDErrT HeadCD::Destroy(bool collective)
{
  CD_DEBUG("HeadCD::Destroy\n");
  CDErrT err=CDErrT::kOK;

  InternalDestroy(collective);

  return err;
}

CDErrT HeadCD::Stop(CDHandle* cdh)
{

//  if(IsLocalObject()){
//    if( !cd_children_.empty() ) {
//      // Stop children CDs
//      int res;
//      std::list<CD>::iterator itstr = cd_children_.begin(); 
//      std::list<CD>::iterator itend = cd_children_.end();
//      while(1) {
//        if(itstr < itend) {
//          res += (*it).Stop(cdh);
//        }
//        if(res == cd_children_.size()) {
//          break;
//        }
//      }
//    
//    }
//    else {  // terminate
//      return 1;
//    }
//  }
//  else {
//    // return RemoteStop(cdh);
//  }

  return CDErrT::kOK;
}


CDErrT HeadCD::Resume(void)
{
  //FIXME: return value needs to be fixed 

//  for( std::list<CD>::iterator it = cd_children_.begin(), it_end = cd_children_.end(); 
//       it != it_end ; ++it) {
//    (*it).Resume();
//  }

  return CDErrT::kOK;
}


CDErrT HeadCD::AddChild(CDHandle *cd_child) 
{
  if(cd_child->IsHead()) {
    CD_DEBUG("It is not desirable to let the same task be head twice!\n");
//    cd_children_.push_back(cd_child->ptr_cd()->cd_id().task());

//    GatherChildHead();
  }

  return CDErrT::kOK;  
}


//FIXME 11112014
//void CD::GatherChildHead(const CDID& child_cd_id)
//{
//#if _MPI_VER
//
//  // Gather heads of children CDs
//  int send_buf;
//  int send_count;
//  int recv_buf[cd_id().sibling_count()];
//  int recv_count;
//
//  PMPI_Send(, send_count, PMPI_INTEGER, recv_buf, recv_count, PMPI_INTEGER, cd_id().node_id().head(), cd_id().color());
//
//#endif
//}
//
//void Head::GatherChildHead(void)
//{
//#if _MPI_VER
//
//  // Gather heads of children CDs
//  int recv_buf[cd_id().sibling_count()];
//  PMPI_Status status;
//  for(int i=1; i<cd_id().sibling_count(); ++i) {
//    PMPI_Recv(&(recv_buf[i]), 1, PMPI_INTEGER, PMPI_ANY_SOURCE, 0, cd_id().color(), &status);
//  }
//
//#endif
//}

CDErrT HeadCD::RemoveChild(CDHandle* cd_child) 
{
  //FIXME Not optimized operation. 
  // Search might be slow, perhaps change this to different data structure. 
  // But then insert operation will be slower.
  cd_children_.remove(cd_child);

  return CDErrT::kOK;
}


CDHandle* HeadCD::cd_parent(void)
{
  return cd_parent_;
}

void HeadCD::set_cd_parent(CDHandle* cd_parent)
{
  cd_parent_ = cd_parent;
}




CDEntry *CD::InternalGetEntry(ENTRY_TAG_T entry_name) 
{
//  try 
//  {
  CD_DEBUG("\nCD::InternalGetEntry : %u - %s\n", entry_name, tag2str[entry_name].c_str());
  
  auto it = entry_directory_map_.find(entry_name);
  auto jt = remote_entry_directory_map_.find(entry_name);
  if(it == entry_directory_map_.end() && jt == remote_entry_directory_map_.end()) {
    CD_DEBUG("[InternalGetEntry Failed] There is no entry for reference of %s at level #%u\n", tag2str[entry_name].c_str(), level());
    return NULL;
  }
  else if(it != entry_directory_map_.end()) {
    // Found entry at local directory
    CDEntry* cd_entry = entry_directory_map_.find(entry_name)->second;
    
    CD_DEBUG("[InternalGetEntry Local] ref_name: %s, address: %p\n", 
              entry_directory_map_[entry_name]->dst_data_.ref_name().c_str(), 
              entry_directory_map_[entry_name]->dst_data_.address_data());

//      if(cd_entry->isViaReference())
//        return NULL;
//      else
      return cd_entry;
  }
  else if(jt != remote_entry_directory_map_.end()) {
    // Found entry at local directory, but it is a buffer to send remotely.
    CDEntry* cd_entry = remote_entry_directory_map_.find(entry_name)->second;

    CD_DEBUG("[InternalGetEntry Remote] ref_name: %s, address: %p\n", 
              remote_entry_directory_map_[entry_name]->dst_data_.ref_name().c_str(), 
              remote_entry_directory_map_[entry_name]->dst_data_.address_data());
    
//      if(cd_entry->isViaReference())
//        return NULL;
//      else
      return cd_entry;
  }
  else {
    cddbg << "ERROR: there is the same name of entry in entry_directory_map and remote_entry_directory_map.\n"<< endl;
    assert(0);
  }
//  }
//  catch (const std::out_of_range &oor) 
//  {
//    std::cerr << "Out of Range error: " << oor.what() << '\n';
//    return 0;
//  }
}


#if _MPI_VER
CDFlagT *CD::event_flag(void)
{
  return event_flag_;
}

CDFlagT *HeadCD::event_flag(void)
{
  return event_flag_;
}
#endif


void CD::DeleteEntryDirectory(void)
{
//  cddbg<<"Delete Entry In"<<endl; cddbgBreak();
  for(std::list<CDEntry>::iterator it = entry_directory_.begin();
      it != entry_directory_.end(); ) {


/* Serializer test
    uint32_t entry_len=0;
    void *ser_entry = it->Serialize(entry_len);

    cddbg << "ser entry : "<< ser_entry << endl;
    CDEntry new_entry;
    cddbg << "\n\n--------------------------------\n"<<endl;
    new_entry.Deserialize(ser_entry);
    cddbg << "before!!!! " << (it->src_data_).address_data()<<endl<<endl;
    cddbg << "\n\n\nafter!!!! " << new_entry.src_data_.address_data()<<endl;

    cddbg << "before!!!! " << it->name() <<endl<<endl;
    cddbg << "\n\n\nafter!!!! " << new_entry.name()<<endl;
    cddbg << (*it == new_entry) << endl;
*/


//    uint32_t data_handle_len=0;
//    cddbg << "=========== Check Ser ==============" << endl;
//    cddbg <<"[Before packed] :\t"<< it->dst_data_.node_id_ << endl << endl;
//    void *ser_data_handle = (it->dst_data_).Serialize(data_handle_len);
//    DataHandle new_data_handle;
//    new_data_handle.Deserialize(ser_data_handle);
//
//    cddbg <<"\n\n\noriginal : "<<(it->dst_data_).file_name() << endl;
//    cddbg <<"[After unpacked] :\t"<<new_data_handle.node_id_ << endl << endl;
//    cddbgBreak();

    it->Delete();
    entry_directory_map_.erase(it->name_tag());
    remote_entry_directory_map_.erase(it->name_tag());
    entry_directory_.erase(it++);
  }

  CD_DEBUG("Delete entry directory!\n");

//  for(std::map<std::string, CDEntry*>::iterator it = entry_directory_map_.begin();
//      it != entry_directory_map_.end(); ++it) {
//    //entry_directory_map_.erase(it);
//  }
//  cddbg<<"Delete Entry Out"<<endl; cddbgBreak();
}










// ------------------------- Preemption ---------------------------------------------------
//
//CDErrT CD::CheckMailBox(void)
//{
//
//  CD::CDInternalErrT ret=kOK;
//  int event_count = *pendingFlag_;
//
//  // Reset handled event counter
//  handled_event_count = 0;
//  CDHandle *curr_cdh = CDPath::GetCurrentCD();
//  cddbg.flush();
//  cddbg << "\n\n=================== Check Mail Box Start [Level " << level() <<"] , # of pending events : " << event_count << " ========================" << endl;
//
//  if(event_count == 0) {
//    cddbg << "No event is detected" << endl;
//
//    //FIXME
//    cddbg << "\n-----------------KYU--------------------------------------------------" << endl;
//    int temp = handled_event_count;
//    InvokeErrorHandler();
//    cddbg << "\nCheck MailBox is done. handled_event_count : " << temp << " -> " << handled_event_count << ", pending events : " << *pendingFlag_;
//  
//    DecPendingCounter();
//  
//    cddbg << " -> " << *pendingFlag_ << endl;
//    cddbg << "-------------------------------------------------------------------\n" << endl;
//
//
//  }
//  else if(event_count > 0) {
//    while( curr_cdh != NULL ) { // Terminate when it reaches upto root
//      cddbg << "\n---- Internal Check Mail [Level " << curr_cdh->ptr_cd_->level() <<"] , # of pending events : " << event_count << " ----" << endl;
//
//      if( curr_cdh->node_id_.size() > 1) {
//        ret = curr_cdh->ptr_cd_->InternalCheckMailBox();
//      }
//      else {
//        cddbg << "[ReadMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
//      }
//      cddbg << "\n--------------------------------------------------------\n" << endl;
//      // If current CD is Root CD and GetParentCD is called, it returns NULL
//      cddbg << "ReadMailBox " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
//          << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;
////      cddbg << "\nCheckMailBox " << myTaskID << ", # of events : " << event_count << ", Is it Head? "<< IsHead() <<endl;
////      cddbg << "An event is detected" << endl;
////      cddbg << "Task : " << myTaskID << ", Level : " << level() <<" CD Name : " << GetCDName() << ", Node ID: " << GetNodeID() <<endl;
//      curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
//      cddbg.flush();
//    } 
//
//    cddbg << "\n-------------------------------------------------------------------" << endl;
//    int temp = handled_event_count;
//    InvokeErrorHandler();
//    cddbg << "\nCheck MailBox is done. handled_event_count : " << temp << " -> " << handled_event_count << ", pending events : " << *pendingFlag_;
//  
//    DecPendingCounter();
//  
//    cddbg << " -> " << *pendingFlag_ << endl;
//    cddbg << "-------------------------------------------------------------------\n" << endl;
//  }
//  else {
//    cerr << "Pending event counter is less than zero. Something wrong!" << endl;
////    assert(0);
//  }
//  
//  cddbg << "=================== Check Mail Box Done  [Level " << level() <<"] =====================================================\n\n" << endl;
//  cddbg.flush();
//
//  return static_cast<CDErrT>(ret);
//}
//
//int requested_event_count = 0;
//int cd::handled_event_count = 0;
//
///*
//CD::CDInternalErrT CD::InternalCheckMailBox___(void)
//{
//  CD::CDInternalErrT ret=kOK;
//  requested_event_count = *pendingFlag_;
//
//
//  this->ReadMailBox();
//
//
//  int event_count = *pendingFlag_;  
//  if(requested_event_count != handled_event_count) {
//    // There are some events not handled at this point
//    ret = requested_event_count - handled_event_count;
//    assert(event_count != (requested_event_count-handled_event_count));
//  }
//  else {
//    // All events were handled
//    // If every event was handled, event_count must be 0
//    ret = 0;
//    assert(event_count!=0);
//  }
//
//  return ret;
//}
//*/
//
//
//CD::CDInternalErrT CD::InternalCheckMailBox(void) 
//{
//  CDInternalErrT ret = kOK;
//  assert(event_flag_);
//  cout << "\nInternalCheckMailBox\n" << endl; 
//  cddbg << "pending counter : "<< *pendingFlag_ << endl;
//  cddbg << "\nTask : " << GetCDName() << " / " << GetNodeID() << "), Error #" << *event_flag_ << "\t";
//
//  // Initialize the resolved flag
//  CDFlagT event = *event_flag_;
//
//  // FIXME : possible concurrent bug. it kind of localized event_flag_ and check local variable.
//  // but if somebody set event_flag, it is not updated in this event check routine.
//  CDEventHandleT resolved = ReadMailBox(event);
//
//  // Reset the event flag after handling the event
//  switch(resolved) {
//    case CDEventHandleT::kEventNone : 
//      // no event
//      break;
//    case CDEventHandleT::kEventResolved :
//      // There was an event, but resolved
//
//      break;
//    case CDEventHandleT::kEventPending :
//      // There was an event, but could not resolved.
//      // Escalation required
//      break;
//    default :
//      break;
//  }
//  cddbg << "\tResolved? : " << resolved << endl; //getchar();
//  cddbg << "cd event queue size : " << cd_event_.size() <<", handled event count : " << handled_event_count << endl;
//  return ret;
//}
//
//CD::CDInternalErrT HeadCD::InternalCheckMailBox(void) 
//{
//  CDEventHandleT resolved = CDEventHandleT::kEventNone;
//  CDInternalErrT ret = kOK;
//  CDFlagT *event = event_flag_;
//  assert(event_flag_);
//
//  cout << "\n\n\n\nInternalCheckMailBox CHECK IT OUT HEAD\n" << endl; //getchar();
//  cddbg << "pending counter : "<< *pendingFlag_ << endl;
//
//  for(int i=0; i<task_size(); i++) {
//    cddbg << "\nTask["<< i <<"] (" << GetCDName() << " / " << GetNodeID() << "), Error #" << event[i] << endl;;
//
//    // Initialize the resolved flag
//    resolved = kEventNone;
//    resolved = ReadMailBox(&(event[i]), i);
//
//    // Reset the event flag after handling the event
//    switch(resolved) {
//      case CDEventHandleT::kEventNone : 
//        // no event
//        break;
//      case CDEventHandleT::kEventResolved :
//        // There was an event, but resolved
//
//        break;
//      case CDEventHandleT::kEventPending :
//        // There was an event, but could not resolved.
//        // Escalation required
//        break;
//      default :
//        break;
//    }
//  }
//  cddbg << "\tResolved? : " << resolved << endl; //getchar();
//  cddbg << "cd event queue size : " << cd_event_.size() << ", headcd event queue size : " << cd_event_.size() <<", handled event count : " << handled_event_count << endl;
//  return ret;
//}
//
//CDEventHandleT CD::ReadMailBox(CDFlagT &event)
//{
//  cddbg << "\nCD::ReadMailBox("<< event2str(event) << ")\n"<< endl; cddbg.flush();
//  CDEventHandleT ret = CDEventHandleT::kEventResolved;
////  int handle_ret = 0;
//  if( CHECK_NO_EVENT(event) ) {
//    cddbg << "CD Event kNoEvent\t\t\t";
//  }
//  else {
//// Events requested from head to non-head.
//// Current task is non-head,
//// so, kEntrySearch | kErrorOccurred does not make sense here,
//// because non-head task does not have an ability to deal with that.
//// kAllResume is also weird here. Imagine that it is just "Resume" when you open the mailbox.
//// I am currently "resumed" (my current forward execution), so it does not make sense.
//
//    if( CHECK_EVENT(event, kEntrySearch) || CHECK_EVENT(event, kErrorOccurred) ) {
//      assert(0);
//    }
//
//    if( CHECK_EVENT(event, kEntrySend) ) {
//
////#if _FIX
////      HandleEntrySend();
//      cddbg << "ENTRYSEND" << endl;
//      cd_event_.push_back(new HandleEntrySend(this));
////  cddbg << "CD Event kEntrySend\t\t\t";
////  if(IsHead()) {
////    assert(0);//event_flag_[idx] &= ~kEntrySend;
////  }
////  else {
////    *(event_flag_) &= ~kEntrySend;
////  }
////  IncHandledEventCounter();
////#endif
//      cddbg << "CD Event kEntrySend\t\t\t";
//    }
//
//    if( CHECK_EVENT(event, kAllPause) ) {
//      cddbg << "What?? kAllPause" << GetNodeID() << endl;
//      cd_event_.push_back(new HandleAllPause(this));
//    }
//
//
//    if( CHECK_EVENT(event, kAllReexecute) ) {
//      cddbg << "ReadMailBox. push back HandleAllReexecute" << endl;
//      cd_event_.push_back(new HandleAllReexecute(this));
//    }
//
//  } 
//  return ret;
//}
//
//
//CDEventHandleT HeadCD::ReadMailBox(CDFlagT *p_event, int idx)
//{
//  cddbg << "CDEventHandleT HeadCD::HandleEvent(" << event2str(*p_event) << ", " << idx <<")" << endl; cddbg.flush();
//  CDEventHandleT ret = CDEventHandleT::kEventResolved;
////  int handle_ret = 0;
//  if(idx == head()) {
//    if(head() != task_in_color()) assert(0);
//    cout << "event in head : " << *p_event << endl; //getchar();
//  }
//
//  CDFlagT event = *p_event;
//  if( CHECK_NO_EVENT(event) ) {
//    cddbg << "CD Event kNoEvent\t\t\t";
//  }
//  else {
//    if( CHECK_EVENT(event, kErrorOccurred) ) {
//      cddbg << "Error Occurred at task ID : " << idx << " level " << level() << endl;
//      if(error_occurred == true) {
//        if(task_size() == 1) assert(0);
//        cddbg << "Event kErrorOccurred " << idx;
//        event_flag_[idx] &= ~kErrorOccurred;
//        IncHandledEventCounter();
//      }
//      else {
//        if(task_size() == 1) assert(0);
//        cddbg << "!!!!Event kErrorOccurred " << idx;
//        cd_event_.push_back(new HandleErrorOccurred(this, idx));
//        error_occurred = true;
//      }
//    }
//
//    if( CHECK_EVENT(event, kEntrySearch) ) {
//
////#if _FIX
//      cddbg << "ENTRYSEARCH" << endl;
////      ENTRY_TAG_T recvBuf[2] = {0,0};
////      bool entry_searched = HandleEntrySearch(idx, recvBuf);
//      cd_event_.push_back(new HandleEntrySearch(this, idx));
//
//
//
////    event_flag_[idx] &= ~kEntrySearch;
////    IncHandledEventCounter();
//////FIXME_KL
////      if(entry_searched == false) {
////        CDHandle *parent = CDPath::GetParentCD();
////        CDEventT entry_search = kEntrySearch;
////        parent->SetMailBox(entry_search);
////        PMPI_Isend(recvBuf, 
////                  2, 
////                  MPI_UNSIGNED_LONG_LONG,
////                  parent->node_id().head(), 
////                  parent->ptr_cd()->GetCDID().GenMsgTag(MSG_TAG_ENTRY_TAG), 
////                  parent->node_id().color(),
////                  &(parent->ptr_cd()->entry_request_req_[recvBuf[0]].req_));
////      }
////      else {
////        // Search Entry
//////        IncHandledEventCounter();
////      }
////#endif
//    }
//
//    if( CHECK_EVENT(event, kAllResume) ) {
//      cddbg << "Head ReadMailBox. kAllResume" << endl;
//      cd_event_.push_back(new HandleAllResume(this));
//
//
//    }
//    if( CHECK_EVENT(event, kAllReexecute) ) {
//      cddbg << "Handle All Reexecute at " << GetCDName() << " | " << GetNodeID() << endl;
//      cd_event_.push_back(new HandleAllReexecute(this));
//
//    }
//// -------------------------------------------------------------------------
//// Events requested from non-head to head.
//// If current task is head, it means this events is requested from head to head, 
//// which does not make sense. (perhaps something like loopback.)
//// Currently, I think head can just ignore for checking kEntrySend/kAllPause/kAllReexecute, 
//// but I will leave if for now.
//    if( CHECK_EVENT(event, kEntrySend) || CHECK_EVENT(event, kAllPause) ) {
//      assert(0);
//    }
//
//#if 0 
//    if( CHECK_EVENT(event, kEntrySend) ) {
//
//#if _FIX
//      if(!IsHead()) {
//        cout << "[event: entry send] This is event for non-head CD.";
////        assert(0);
//      }
////      HandleEntrySend();
//      cd_event_.push_back(new HandleEntrySend(this));
//#endif
//      cddbg << "CD Event kEntrySend\t\t\t";
//      
//    }
//
//    if( CHECK_EVENT(event, kAllPause) ) {
//      cout << "What?? " << GetNodeID() << endl;
//      HandleAllPause();
//
//
//    }
//
//    if( CHECK_EVENT(event, kAllReexecute) ) {
//
//      HandleAllReexecute();
//
//
//    }
//#endif
//
//  } // ReadMailBox ends 
//  return ret;
//}
//
//
//
//
//
//
//
//
//
//#if 1
//CDErrT CD::SetMailBox(const CDEventT &event)
//{
//  cddbg << "\n\n=================== Set Mail Box Start ==========================\n" << endl;
//  cddbg << "\nSetMailBox " << myTaskID << ", event : " << event2str(event) << ", Is it Head? "<< IsHead() <<endl;
//
//  cddbg.flush();
//  CD::CDInternalErrT ret=kOK;
////  int val = 1;
//
//  CDHandle *curr_cdh = CDPath::GetCurrentCD();
////  int head_id = head();
//  if(event == CDEventT::kNoEvent) {
//
//    cddbg << "\n=================== Set Mail Box Done ==========================\n" << endl;
//
//    return static_cast<CDErrT>(ret);  
//  }
////  if(task_size() == 1) {
//    // This is the leaf that has just single task in a CD
//    cddbg << "KL : [SetMailBox Leaf and have 1 task] size is " << task_size() << ". Check mailbox at the upper level." << endl;
//
//    while( curr_cdh->node_id_.size()==1 ) {
//      if(curr_cdh == CDPath::GetRootCD()) {
//        cddbg << "[SetMailBox] there is a single task in the root CD" << endl;
//        assert(0);
//      }
//      cddbg << "[SetMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
//      // If current CD is Root CD and GetParentCD is called, it returns NULL
//      curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
//    } 
//    cddbg << "[SetMailBox] " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
//        << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;
//
////    head_id = curr_cdh->node_id().head();
////  }
//
//  cddbg.flush();
//  if(curr_cdh->IsHead()) assert(0);
//  cddbg << "Before RemoteSetMailBox" << endl; cddbg.flush();
////  curr_cdh->ptr_cd()
//  ret = RemoteSetMailBox(curr_cdh->ptr_cd(), event);
//  
///*
//  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, curr_cdh->ptr_cd()->pendingWindow_);
//  if(event != CDEventT::kNoEvent) {
//    cddbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
//    // Increment pending request count at the target task (requestee)
//    PMPI_Accumulate(&val, 1, MPI_INT, 
//                    head_id, 0, 1, MPI_INT, 
//                    MPI_SUM, curr_cdh->ptr_cd()->pendingWindow_);
//    cddbg << "MPI Accumulate done for tast #"<< head_id <<" (head)"<< endl; //getchar();
//  }
//  PMPI_Win_unlock(head_id, curr_cdh->ptr_cd()->pendingWindow_);
//
//  CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_;
//
//
//
//  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, mailbox);
//
//  // Inform the type of event to be requested
//  cddbg << "Set event start" << endl; //getchar();
//  if(event != CDEventT::kNoEvent) {
//    PMPI_Accumulate(&event, 1, MPI_INT, 
//                    head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, 
//                    MPI_BOR, mailbox);
//  }
//  cddbg << "MPI Accumulate done for task #"<< head_id <<"(head)"<< endl; //getchar();
//
//  PMPI_Win_unlock(head_id, mailbox);
//*/
//
//  cddbg << "\n=================== Set Mail Box Done ==========================\n" << endl;
//
//  cddbg.flush();
//  return static_cast<CDErrT>(ret);
//}
//
//CD::CDInternalErrT CD::RemoteSetMailBox(CD *curr_cd, const CDEventT &event)
//{
//  cddbg <<"\nCD::RemoteSetMailBox, event" << event2str(event) << " at " << curr_cd->GetCDName() << " " << curr_cd->GetNodeID() << endl;
//  cddbg.flush();
//  CDInternalErrT ret=kOK;
//  int head_id = curr_cd->head();
//  int val = 1;
//  if(event != CDEventT::kNoEvent) {
//    PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, curr_cd->pendingWindow_);
//    cddbg << "Set CD Event "<< event2str(event) <<". Level : " << curr_cd->level() <<" CD Name : " << curr_cd->GetCDName()<< endl;
//    // Increment pending request count at the target task (requestee)
//    PMPI_Accumulate(&val, 1, MPI_INT, 
//                    head_id, 0, 1, MPI_INT, 
//                    MPI_SUM, curr_cd->pendingWindow_);
//    cddbg << "MPI Accumulate done for task #"<< head_id <<" (head)"<< endl; //getchar();
//  
//    cddbg.flush();
//    PMPI_Win_unlock(head_id, curr_cd->pendingWindow_);
//  
//    CDMailBoxT &mailbox = curr_cd->mailbox_;
//
//    PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, mailbox);
//  
//    // Inform the type of event to be requested
//    cddbg << "Set event start" << endl; //getchar();
//    PMPI_Accumulate(&event, 1, MPI_INT, 
//                    head_id, curr_cd->task_in_color(), 1, MPI_INT, 
//                    MPI_BOR, mailbox);
//    PMPI_Win_unlock(head_id, mailbox);
//  }
//  cddbg << "MPI Accumulate done for task #"<< head_id <<"(head)"<< endl; //getchar();
//  cddbg.flush();
//  cddbg << "CD::RemoteSetMailBox Done." << endl;
//  cddbg.flush();
//  return ret;
//}
//
//
//CDErrT HeadCD::SetMailBox(const CDEventT &event, int task_id)
//{
//  cddbg << "\n\n=================== Set Mail Box ("<<event2str(event)<<", "<<task_id<<") Start! myTaskID #"<< myTaskID << "==========================\n" << endl;
//  cddbg.flush();
// 
//  if(event == CDEventT::kErrorOccurred || event == CDEventT::kEntrySearch) {
//    cerr << "[HeadCD::SetMailBox(event, task_id)] Error, the event argument is something wrong. event: " << event << endl;
//    assert(0);
//  }
//  if(task_size() == 1) {
//    cerr << "[HeadCD::SetMailBox(event, task_id)] Error, the # of task should be greater than 1 at this routine. # of tasks in current CD: " << task_size() << endl;
//    assert(0);
//  }
//
//  CDErrT ret=CDErrT::kOK;
//  int val = 1;
//  if(event == CDEventT::kNoEvent) {
//    // There is no vent to set.
//    cddbg << "No event to set" << endl;
//  }
//  else {
//    if(task_id != task_in_color()) {
//      
//
// 
//      // Increment pending request count at the target task (requestee)
//      if(event != CDEventT::kNoEvent) {
//        PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_id, 0, pendingWindow_);
//        cddbg << "Set CD Event " << event2str(event) <<". Level : " << level() <<" CD Name : " << GetCDName()<< endl;
//        cddbg << "Accumulate event at " << task_id << endl;
//  
//        PMPI_Accumulate(&val, 1, MPI_INT, 
//                       task_id, 0, 1, MPI_INT, 
//                       MPI_SUM, pendingWindow_);
//        cddbg << "PMPI_Accumulate done for task #" << task_id << endl; //getchar();
//        cddbg.flush();
//        PMPI_Win_unlock(task_id, pendingWindow_);
//    
//    
//        cddbg << "Finished to increment the pending counter at task #" << task_id << endl;
//        cddbg.flush();
//    
//        if(task_id == task_in_color()) { 
//          cddbg << "after accumulate --> pending counter : " << *pendingFlag_ << endl;
//        }
//        
//        // Inform the type of event to be requested
//        PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_id, 0, mailbox_);
//        cddbg << "Set event start" << endl; //getchar();
//        PMPI_Accumulate(&event, 1, MPI_INT, 
//                       task_id, 0, 1, MPI_INT, 
//                       MPI_BOR, mailbox_);
//        PMPI_Win_unlock(task_id, mailbox_);
//      }
//      cddbg << "PMPI_Accumulate done for task #" << task_id << endl; //getchar();
//      cddbg.flush();
//  
//      if(task_id == task_in_color()) { 
//        cddbg << "after accumulate --> event : " << event2str(event_flag_[task_id]) << endl;
//      }
//    
//    
//    }
//    else {
//      // If the task to set event is the head itself,
//      // it does not increase pending counter and set event flag through RDMA,
//      // but it locally increase the counter and directly register event handler.
//      // The reason for this is that the head task's CheckMailBox is scheduled before non-head task's,
//      // and after head's CheckMailBox associated with SetMailBox for some events such as kErrorOccurred,
//      // (kErrorOccurred at head tast associates kAllReexecute for all task in the CD currently)
//      // head CD does not check mail box to invoke kAllRexecute for itself. 
//      // Therefore, it locally register the event handler right away, 
//      // and all the tasks including head task can reexecute after the CheckMailBox.
//      cddbg << "\nEven though it calls SetMailBox( "<<event2str(event)<<", "<<task_id<<"=="<<task_in_color()<<" ), locally set/handle events\n"<<endl;
//      CDInternalErrT cd_ret = LocalSetMailBox(this, event);
//      ret = static_cast<CDErrT>(cd_ret);
//    }
//  }
//  cddbg << "\n=================== Set Mail Box Done ==========================\n" << endl;
//
//  cddbg.flush();
//  return ret;
//}
//
//
//CDErrT HeadCD::SetMailBox(const CDEventT &event)
//{
//  // If the task to set event is the head itself,
//  // it does not increase pending counter and set event flag through RDMA,
//  // but it locally increase the counter and directly register event handler.
//  // The reason for this is that the head task's CheckMailBox is scheduled before non-head task's,
//  // and after head's CheckMailBox associated with SetMailBox for some events such as kErrorOccurred,
//  // (kErrorOccurred at head tast associates kAllReexecute for all task in the CD currently)
//  // head CD does not check mail box to invoke kAllRexecute for itself. 
//  // Therefore, it locally register the event handler right away, 
//  // and all the tasks including head task can reexecute after the CheckMailBox.
//  cddbg << "\n---------- HeadCD::SetMailBox( "<< event2str(event) <<" ) at level #"<<level()<<"-------------\n" << endl;
//  cddbg.flush();
//  CDInternalErrT ret = kOK;
//  CDHandle *curr_cdh = CDPath::GetCurrentCD();
//  while( curr_cdh->task_size() == 1 ) {
//    if(curr_cdh == CDPath::GetRootCD()) {
//      cddbg << "[SetMailBox] there is a single task in the root CD" << endl;
//      assert(0);
//    }
//    // If current CD is Root CD and GetParentCD is called, it returns NULL
//    curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
//  } 
//  cddbg.flush();
////  if(curr_cdh->IsHead()) assert(0);
//
//
//  if(curr_cdh->IsHead()) {
//    ret = LocalSetMailBox(static_cast<HeadCD *>(curr_cdh->ptr_cd()), event);
//  }
//  else {
//    ret = RemoteSetMailBox(curr_cdh->ptr_cd(), event);
//  }
//
//  cddbg.flush();
///*
//  CDErrT ret=CDErrT::kOK;
//  if(event != kNoEvent) {
//    if(curr_cdh->task_in_color() != head()) assert(0);
//  
//    if(event == kAllResume || event == kAllPause || event == kEntrySearch || event == kEntrySend) { 
//      assert(0);
//    }
////  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_in_color(), 0, pendingWindow_);
//    (*pendingFlag_)++;
////  PMPI_Win_unlock(task_in_color(), pendingWindow_);
//    if( CHECK_EVENT(event, kErrorOccurred) ) {
//        cddbg << "kErrorOccurred in HeadCD::SetMailBox" << endl; //getchar();
//        curr_cdh->ptr_cd()->cd_event_.push_back(new HandleErrorOccurred(static_cast<HeadCD *>(curr_cdh->ptr_cd())));
////        curr_cdh->ptr_cd()->cd_event_.push_back(new HandleAllReexecute(this));
//        // it is not needed to register here. I will be registered by HandleErrorOccurred functor.
//    }
//    if( CHECK_EVENT(event, kAllReexecute) ) {
//        cddbg << "kAllRexecute in HeadCD::SetMailBox" << endl; //getchar();
//        assert(curr_cdh->ptr_cd() == this);
//        curr_cdh->ptr_cd()->cd_event_.push_back(new HandleAllReexecute(curr_cdh->ptr_cd()));
//    }
//  
//  }
//*/
//  cddbg << "\n------------------------------------------------------------\n" << endl;
//  return static_cast<CDErrT>(ret);
//}
//
//CD::CDInternalErrT HeadCD::LocalSetMailBox(HeadCD *curr_cd, const CDEventT &event)
//{
//  cddbg << "HeadCD::LocalSetMailBox Event : " << event2str(event) << endl;
//  cddbg.flush();
//  CDInternalErrT ret=kOK;
//  if(event != kNoEvent) {
//    if(curr_cd->task_in_color() != head()) assert(0);
//  
//    if(event == kAllResume || event == kAllPause  || event == kEntrySearch ) {//|| event == kEntrySend) {
//      cddbg << "HeadCD::LocalSetMailBox -> Event: " << event2str(event) << endl; cddbg.flush();
//      cerr << "HeadCD::LocalSetMailBox -> Event: " << event2str(event) << endl; 
//      assert(0);
//    }
////  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_in_color(), 0, pendingWindow_);
//    (*pendingFlag_)++;
////  PMPI_Win_unlock(task_in_color(), pendingWindow_);
//    if( CHECK_EVENT(event, kErrorOccurred) ) {
//        cddbg << "kErrorOccurred in HeadCD::SetMailBox" << endl; //getchar();
//        curr_cd->cd_event_.push_back(new HandleErrorOccurred(curr_cd));
//        // it is not needed to register here. I will be registered by HandleErrorOccurred functor.
//    }
//    if( CHECK_EVENT(event, kAllReexecute) ) {
//        cddbg << "kAllRexecute in HeadCD::SetMailBox" << endl; //getchar();
//        assert(curr_cd == this);
//        curr_cd->cd_event_.push_back(new HandleAllReexecute(curr_cd));
//    }
//    if( CHECK_EVENT(event, kEntrySend) ) {
//        assert(0);
//        cddbg << "kEntrySend in HeadCD::SetMailBox" << endl; //getchar();
//        assert(curr_cd == this);
//        curr_cd->cd_event_.push_back(new HandleEntrySend(curr_cd));
//    }
//  
//  }
//  return ret;
//}
//
//#else
///*
//CDErrT CD::SetMailBox(CDEventT &event)
//{
//  cddbg << "\n\n=================== Set Mail Box Start ==========================\n" << endl;
//  cddbg << "\nSetMailBox " << myTaskID << ", event : " << event << ", Is it Head? "<< IsHead() <<endl;
//  CD::CDInternalErrT ret=kOK;
//  int val = 1;
//
//  if(task_size() > 1) {
//    cddbg << "KL : [SetMailBox] size is " << task_size() << ". Check mailbox at the upper level." << endl;
//  
//    PMPI_Win_fence(0, pendingWindow_);
//    if(event != CDEventT::kNoEvent) {
////      PMPI_Win_fence(0, pendingWindow_);
//
//      cddbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
//      // Increment pending request count at the target task (requestee)
//      PMPI_Accumulate(&val, 1, MPI_INT, 
//                     head(), 0, 1, MPI_INT, 
//                     MPI_SUM, pendingWindow_);
//      cddbg << "MPI Accumulate done" << endl; //getchar();
//
////      PMPI_Win_fence(0, pendingWindow_);
//    }
//    PMPI_Win_fence(0, pendingWindow_);
//
//    CDMailBoxT &mailbox = mailbox_;  
//
//    PMPI_Win_fence(0, mailbox);
//    if(event != CDEventT::kNoEvent) {
//      // Inform the type of event to be requested
//      PMPI_Accumulate(&event, 1, MPI_INT, 
//                     head(), task_in_color(), 1, MPI_INT, 
//                     MPI_BOR, mailbox);
//
////    PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//      cddbg << "PMPI_Accumulate done" << endl; //getchar();
//    }
//  
////    switch(event) {
////      case CDEventT::kNoEvent :
////        cddbg << "Set CD Event kNoEvent" << endl;
////        break;
////      case CDEventT::kErrorOccurred :
////        // Inform the type of event to be requested
////        PMPI_Accumulate(&event, 1, MPI_INT, node_id_.head(), node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
//////        PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
////        cddbg << "PMPI_Put done" << endl; getchar();
////        break;
////      case CDEventT::kAllPause :
////        cddbg << "Set CD Event kAllPause" << endl;
////  
////        break;
////      case CDEventT::kAllResume :
////  
////        break;
////      case CDEventT::kEntrySearch :
////  
////        break;
////      case CDEventT::kEntrySend :
////  
////        break;
////      case CDEventT::kAllReexecute :
////  
////        break;
////      default:
////  
////        break;
////    }
//  
//    PMPI_Win_fence(0, mailbox);
//  
//  }
//  else {
//    // This is the leaf that has just single task in a CD
//    cddbg << "KL : [SetMailBox Leaf and have 1 task] size is " << task_size() << ". Check mailbox at the upper level." << endl;
//    CDHandle *curr_cdh = CDPath::GetCurrentCD();
//
//    while( curr_cdh->node_id_.size()==1 ) {
//      if(curr_cdh == CDPath::GetRootCD()) {
//        cddbg << "[SetMailBox] there is a single task in the root CD" << endl;
//        assert(0);
//      }
//      cddbg << "[SetMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
//      // If current CD is Root CD and GetParentCD is called, it returns NULL
//      curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
//    } 
//    cddbg << "[SetMailBox] " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
//        << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;
//
//
//    int parent_head_id = curr_cdh->node_id().head();
//
//    PMPI_Win_lock(PMPI_LOCK_EXCLUSIVE, parent_head_id, 0, curr_cdh->ptr_cd()->pendingWindow_);
//    if(event != CDEventT::kNoEvent) {
//      cddbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
//      // Increment pending request count at the target task (requestee)
//      PMPI_Accumulate(&val, 1, MPI_INT, parent_head_id, 
//                     0, 1, MPI_INT, 
//                     MPI_SUM, curr_cdh->ptr_cd()->pendingWindow_);
//      cddbg << "MPI Accumulate done" << endl; //getchar();
//    }
//    PMPI_Win_unlock(parent_head_id, curr_cdh->ptr_cd()->pendingWindow_);
//
//    CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_;
//
//    PMPI_Win_lock(PMPI_LOCK_EXCLUSIVE, parent_head_id, 0, mailbox);
//    // Inform the type of event to be requested
//    cddbg << "Set event start" << endl; //getchar();
//    if(event != CDEventT::kNoEvent) {
//      PMPI_Accumulate(&event, 1, MPI_INT, parent_head_id, 
//                     curr_cdh->node_id_.task_in_color(), 1, MPI_INT, 
//                     MPI_BOR, mailbox);
////    PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//    }
//    cddbg << "PMPI_Accumulate done" << endl; //getchar();
//
////    switch(event) {
////      case CDEventT::kNoEvent :
////        cddbg << "Set CD Event kNoEvent" << endl;
////        break;
////      case CDEventT::kErrorOccurred :
////        // Inform the type of event to be requested
////        cddbg << "Set event start" << endl; getchar();
////        PMPI_Accumulate(&event, 1, MPI_INT, parent_head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
//////        PMPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
////        cddbg << "PMPI_Accumulate done" << endl; getchar();
////        break;
////      case CDEventT::kAllPause :
////        cddbg << "Set CD Event kAllPause" << endl;
////  
////        break;
////      case CDEventT::kAllResume :
////  
////        break;
////      case CDEventT::kEntrySearch :
////  
////        break;
////      case CDEventT::kEntrySend :
////  
////        break;
////      case CDEventT::kAllReexecute :
////  
////        break;
////      default:
////  
////        break;
////    }
//    PMPI_Win_unlock(parent_head_id, mailbox);
//  }
//  cddbg << "\n================================================================\n" << endl;
//
//  return static_cast<CDErrT>(ret);
//}
//*/
//#endif

//char *CD::GenTag(const char* tag)
//{
//  Tag tag_gen;
//  CDNameT cd_name = ptr_cd()->GetCDName();
//  tag_gen << tag << node_id_.task_in_color_ <<'-'<<cd_name.level()<<'-'<<cd_name.rank_in_level();
//  return const_cast<char*>(tag_gen.str().c_str());
//}




#if CD_LIBC_LOG_ENABLED
//GONG
CommLogErrT CD::CommLogCheckAlloc_libc(unsigned long length)
{
  LogPrologue();
  CommLogErrT ret = libc_log_ptr_->CheckChildLogAlloc(length);
  LogEpilogue();
  return ret;
}

//GONG
CommLogMode CD::GetCommLogMode_libc()
{
  return libc_log_ptr_->GetCommLogMode();
}

//GONG
bool CD::IsNewLogGenerated_libc()
{
  return libc_log_ptr_->IsNewLogGenerated_();
}

#endif




#if comm_log
//SZ
CommLogErrT CD::CommLogCheckAlloc(unsigned long length)
{
  LogPrologue();
  CommLogErrT ret = comm_log_ptr_->CheckChildLogAlloc(length);
  LogEpilogue();
  return ret;
}


//SZ 
bool CD::IsParentLocal()
{
  //FIXME: for now assuming cd_parent_ is always local
  //       need to implement inside CDID object to test if parent is local, such as using process_id_
  return 1;
}

//SZ
CDHandle* CD::GetParentHandle()
{
  return CDPath::GetParentCD(level());
}

#if 0
//SZ
CommLogErrT CD::ProbeAndReadData(unsigned long flag)
{
  // look for the entry in incomplete_log_
  int found = 0;
  std::vector<struct IncompleteLogEntry>::iterator it;
  CD *tmp_cd = this;
  for (it=incomplete_log_.begin(); it!=incomplete_log_.end(); it++)
  {
    if (it->flag_ == flag) 
    {
      found = 1;
      LOG_DEBUG("Found the entry in incomplete_log_\n");
      break;
    }
  }

  if (found == 0)
  {
    // recursively go up to search parent's incomplete_log_
    while (GetParentHandle()!=NULL)
    {
      tmp_cd = GetParentHandle()->ptr_cd_; 
      for (it = tmp_cd->incomplete_log_.begin(); 
           it != tmp_cd->incomplete_log_.end(); 
           it++)
      {
        //FIXME: potential bug, what if two PMPI_Wait within one CD using the same request??
        //       e.g. begin, irecv, wait, irecv, wait, complete
        if (it->flag_ == flag){
          found = 1;
          LOG_DEBUG("Found the entry in incomplete_log_\n");
          break;
        }
      }
      if (found){
        break;
      }
    }

    if (!found)
    {
      ERROR_MESSAGE("Do not find corresponding Isend/Irecv incomplete log!!\n")
    }
  }
  return kCommLogOK;
}
#endif


// KL
// When task has matching MPI calls such as the pair of MPI_Irecv and MPI_Wait,
// it is possible to call MPI_Irecv twice and MPI_Wait once.
// Possible situation is like below.
// Task 1 called MPI_Irecv and got escalation event before reacing MPI_Wait. 
// It escalates to beginning of CD and invokes MPI_Irecv again. 
// Then it calls MPI_Wait. 
// The problem is that strict CD does not log MPI calls 
// but regenerate messages in reexecution path.
// Possible solution is invalidating the original MPI_Irecv with MPI_Cancel 
// inside CD runtime and calls new MPI_Irecv in reexecution path.
CommLogErrT CD::InvalidateIncompleteLogs(void)
{
  LogPrologue();
  //printf("### [%s] %s at level #%u\n", __func__, label_.c_str(), level());
  if(incomplete_log_.size()!=0) {
    CD_DEBUG("### [%s] %s Incomplete log size: %lu at level #%u\n", __func__, label_.c_str(), incomplete_log_.size(), level());
//    printf("### [%s] %s Incomplete log size: %lu at level #%u\n", __func__, label_.c_str(), incomplete_log_.size(), level());
  }
#if _MPI_VER
  void *flag = NULL; //
//  for(auto it=incomplete_log_.begin(); it!=incomplete_log_.end(); ++it) {
////    PMPI_Cancel(reinterpret_cast<MPI_Request>(it->flag_));
//    flag = it->flag_;
//    printf("%lx\n", flag);
//    PMPI_Cancel((MPI_Request *)(it->flag_));
//    auto jt = it;
//    incomplete_log_.erase(jt);
//  }
  while(incomplete_log_.size() != 0) {
    auto incompl_log = incomplete_log_.back();
    incomplete_log_.pop_back();
    flag = incompl_log.flag_;
//    printf("Trying to cancel ptr:%lx\n", flag);
    CD_DEBUG("Trying to cancel ptr:%lx\n", flag); CD_DEBUG_FLUSH;
    PMPI_Cancel((MPI_Request *)(incompl_log.flag_));
  }
#endif
  LogEpilogue();
  return kCommLogOK;
}

// KL
// This function call is for resolving the above problem in relaxed CD case.
// CD could just escalate in the case that there are some incomplete logs in failure.
// However, it can be resolved by letting the corresponding message
// be delivered, but not completed in the process of recovery action.
// Before longjmp to the beginning of CD, CD runtime can keep probing asynchronously 
// all the incomplete logs happened in the current CD until all of them are probed.
// Then, it reexecute from the beginning. This mechanism assumed that every incomplete
// logs will be eventually resolved, which should be correct.
// Otherwise, the normal app semantic will be also problematic because it will keep waiting
// at MPI_Wait in the case of MPI_Irecv in application side.
CommLogErrT CD::ProbeIncompleteLogs(void)
{
  LogPrologue();
  //printf("### [%s] %s at level #%u\n", __func__, label_.c_str(), level());
  if(incomplete_log_.size()!=0) {
    CD_DEBUG("### [%s] %s Incomplete log size: %lu at level #%u\n", __func__, label_.c_str(), incomplete_log_.size(), level());
//    printf("### [%s] %s Incomplete log size: %lu at level #%u\n", __func__, label_.c_str(), incomplete_log_.size(), level());
  }
#if _MPI_VER
  const size_t num_log = incomplete_log_.size();
  MPI_Status incompl_log_stat[num_log];

  uint32_t idx=0;
  for(auto it=incomplete_log_.begin(); it!=incomplete_log_.end(); ++it) {
    // It will be blocking until each incomplete msg is resolved.
    PMPI_Probe(it->taskID_, it->tag_, it->comm_,  &(incompl_log_stat[idx]));
    idx++;
  }
#endif
  LogEpilogue();
  return kCommLogOK;
}

bool CD::DeleteIncompleteLog(void *flag)
{
  bool deleted = false;
//  for(auto it=incomplete_log_.begin(); it!=incomplete_log_.end(); ++it) {
//    auto jt = it;
//    if(it->flag_ == flag) {
//      incomplete_log_.erase(jt);
//      deleted = true;
//    }
//  }
  auto it = incomplete_log_.find(flag);
  if(it != incomplete_log_.end()) {
    incomplete_log_.erase(it);
    deleted = true;
    CD_DEBUG("FIND flag %p\n", flag);
  }
  
  return deleted;
}


//SZ
CommLogErrT CD::ProbeAndLogData(void *flag)
{
  LogPrologue();
  // look for the entry in incomplete_log_
  int found = 0;
  std::vector<IncompleteLogEntry>::iterator it;
  CD *tmp_cd = this;
  LOG_DEBUG("size of incomplete_log_=%ld\n",incomplete_log_.size());
  for (it=incomplete_log_.begin(); it!=incomplete_log_.end(); ++it) {
    LOG_DEBUG("it->flag_=%p, and flag=%p\n", it->flag_, flag);
    if (it->flag_ == flag) {
      found = 1;
      LOG_DEBUG("Found the entry in incomplete_log_ in current CD\n");
      break;
    }
  }

  if (found == 0) {
    // recursively go up to search parent's incomplete_log_
    while (tmp_cd->GetParentHandle() != NULL) {
      tmp_cd = tmp_cd->GetParentHandle()->ptr_cd_; 
      LOG_DEBUG("tmp_cd's level=%lu\n",(unsigned long)tmp_cd->cd_id_.level());
      LOG_DEBUG("tmp_cd->incomplete_log_.size()=%ld\n",
                    tmp_cd->incomplete_log_.size());
      for (it = tmp_cd->incomplete_log_.begin(); 
           it != tmp_cd->incomplete_log_.end(); 
           ++it) 
      {
        if (it->flag_ == flag) {
          found = 1;
          LOG_DEBUG("Found the entry in incomplete_log_ in one parent CD\n");
          break;
        }
      }
      if (found) {
        break;
      }
    }

    if (!found)
    {
      //ERROR_MESSAGE("Do not find corresponding Isend/Irecv incomplete log!!\n")
      LOG_DEBUG("Possible bug: Isend/Irecv incomplete log NOT found (maybe deleted by PMPI_Test)!!\n")
    }
  }
  
  // ProbeAndLogData for comm_log_ptr_ if relaxed CD
  // If NOT found, then work has been completed by previous Test Ops
  if (comm_log_ptr_ != NULL && found) {
    // ProbeAndLogData:
    // 1) change Isend/Irecv entry to complete state if there is any
    // 2) log data if Irecv
#if CD_DEBUG_ENABLED
    LOG_DEBUG("Print Incomplete Log before calling comm_log_ptr_->ProbeAndLogData\n");
    PrintIncompleteLog();
#endif

    // For inter-CD message, we record payload, otherwise, just record event itself.
    // FIXME
    if(it->intra_cd_msg_ == false) { 
      bool found = comm_log_ptr_->ProbeAndLogData(it->addr_, it->length_, flag, it->isrecv_);
      if (!found)  {
        CD *tmp_cd = this;
        while (tmp_cd->GetParentHandle() != NULL)  {
          //tmp_cd = tmp_cd->GetParentHandle()->ptr_cd_; 
          if (MASK_CDTYPE(tmp_cd->GetParentHandle()->ptr_cd()->cd_type_)==kRelaxed) {
            found = tmp_cd->GetParentHandle()->ptr_cd()->comm_log_ptr_
              ->ProbeAndLogDataPacked(it->addr_, it->length_, flag, it->isrecv_);
            if (found)
              break;
          }
          else {
            break;
          }
          tmp_cd = tmp_cd->GetParentHandle()->ptr_cd_; 
        }
        if (!found)
        {
          LOG_DEBUG("Possible bug: not found the incomplete logs...\n");
        }
      }
  
    }
    else {
      LOG_DEBUG("Skipped copying msg payload because it is intra-CD message!\n");
    }
    // need to log that wait op completes 
#if _MPI_VER
    comm_log_ptr_->LogData((MPI_Request*)flag, 0, it->taskID_);
//    comm_log_ptr_->LogData(&flag, 0, it->taskID_);
#elif _PGAS_VER
    comm_log_ptr_->LogData((void*)flag, 0, it->taskID_);
#endif
  }
  else if (comm_log_ptr_ != NULL) {
    // need to log that wait op completes 
#if _MPI_VER
   // std::cout << it->taskID_ << std::endl;
//    if( &*it != 0 ) // FIXME
    ERROR_MESSAGE("[%s] Wrong control flow!\n", __func__);
      comm_log_ptr_->LogData((MPI_Request*)flag, 0, it->taskID_);
//    comm_log_ptr_->LogData(&flag, 0, it->taskID_);
#elif _PGAS_VER
    comm_log_ptr_->LogData((void*)flag, 0, it->taskID_);
#endif
  }

  // delete the incomplete log entry
  if(found)
    tmp_cd->incomplete_log_.erase(it);

  LogEpilogue();
  return kCommLogOK;
}

//SZ
CommLogErrT CD::LogData(const void *data_ptr, unsigned long length, uint32_t task_id, 
                      bool completed, void *flag, bool isrecv, bool isrepeated, 
                      bool intra_cd_msg, int tag, ColorT comm)
{
  LogPrologue();
  CommLogErrT ret;
  if (comm_log_ptr_ == NULL) {
    ERROR_MESSAGE("Null pointer of comm_log_ptr_ when trying to log data!\n");
    ret = kCommLogError;
  }
#if _LOG_PROFILING
  num_log_entry_++;
  tot_log_volume_+=length;
#endif
  ret = comm_log_ptr_->LogData(data_ptr, length, task_id, completed, flag, isrecv, isrepeated, intra_cd_msg, tag, comm);
  LogEpilogue();
  return ret;
}

//SZ
CommLogErrT CD::ProbeData(const void *data_ptr, unsigned long length)
{
  LogPrologue();
  CommLogErrT ret;
  if (comm_log_ptr_ == NULL) {
    ERROR_MESSAGE("Null pointer of comm_log_ptr_ when trying to read data!\n");
    ret = kCommLogError;
  }
  ret = comm_log_ptr_->ProbeData(data_ptr, length);
  if (ret == kCommLogCommLogModeFlip){
    SetCDLoggingMode(kRelaxedCDGen);
    LOG_DEBUG("CDLoggingMode changed to %d\n", GetCDLoggingMode());
  }

  LogEpilogue();
  return ret;
}

//SZ
CommLogErrT CD::ReadData(void *data_ptr, unsigned long length)
{
  LogPrologue();
  CommLogErrT ret;
  if (comm_log_ptr_ == NULL)
  {
    ERROR_MESSAGE("Null pointer of comm_log_ptr_ when trying to read data!\n");
    return kCommLogError;
  }
  ret = comm_log_ptr_->ReadData(data_ptr, length);
  if (ret == kCommLogCommLogModeFlip){
    SetCDLoggingMode(kRelaxedCDGen);
    LOG_DEBUG("CDLoggingMode changed to %d\n", GetCDLoggingMode());
  }
  else if(ret == kCommLogMissing) {
    // Report escalation
  }
  LogEpilogue();

  return ret;
}

//SZ
CommLogMode CD::GetCommLogMode()
{
  if(comm_log_ptr_ == NULL) return kInvalidLogMode;
  else return comm_log_ptr_->GetCommLogMode();
}

//SZ
bool CD::IsNewLogGenerated()
{
  return comm_log_ptr_->IsNewLogGenerated_();
}


void CD::PrintIncompleteLog()
{
  if (incomplete_log_.size()==0) return;
  LOG_DEBUG("incomplete_log_.size()=%ld\n", incomplete_log_.size());
  for (std::vector<IncompleteLogEntry>::iterator ii=incomplete_log_.begin();
        ii != incomplete_log_.end(); ii++)
  {
    LOG_DEBUG("\nPrint Incomplete Log information:\n");
    LOG_DEBUG("    addr_=%lx\n", ii->addr_);
    LOG_DEBUG("    length_=%ld\n", ii->length_);
    LOG_DEBUG("    flag_=%ld\n", ii->flag_);
    LOG_DEBUG("    complete_=%d\n", ii->complete_);
    LOG_DEBUG("    isrecv_=%d\n", ii->isrecv_);
  }
}
#endif
//commLog ends 

//CDType CD::GetCDType()
//{ return static_cast<CDType>(MASK_CDTYPE(cd_type_)); }

CD::CDInternalErrT CD::InvokeAllErrorHandler(void) {
  CDInternalErrT err = kOK;
  CDHandle *curr_cdh = CDPath::GetCurrentCD();
  while(curr_cdh == NULL) {
    err = curr_cdh->ptr_cd()->InvokeErrorHandler();
    curr_cdh = CDPath::GetParentCD(curr_cdh->level());
  }
  return err;
}


CD::CDInternalErrT CD::InvokeErrorHandler(void)
{
//  CD_DEBUG("\n[CD::InvokeErrorHandler] event queue size : %zu\n", cd_event_.size());
  CDInternalErrT cd_err = kOK;

  while(!cd_event_.empty()) {
    CD_DEBUG("\n\n==============================\ncd event size : %zu\n", cd_event_.size());

    EventHandler *cd_event_handler = cd_event_.front();
    cd_event_handler->HandleEvent();
//    delete cd_event_handler;
    CD_DEBUG("before pop #%zu\n", cd_event_.size());
    cd_event_.pop_front();
    CD_DEBUG("after  pop #%zu\n", cd_event_.size());
    if(cd_event_.empty()) break;
  }
  
#if _MPI_VER
//  CD_DEBUG("Handled : %d, current pending flag (# pending events) : %d\n", handled_event_count, *pendingFlag_);
#endif


  return cd_err;
}

//CD::CDInternalErrT HeadCD::InvokeErrorHandler(void)
//{
//  cddbg << "HeadCD::CDInternalErrT CD::InvokeErrorHandler(void), event queue size : " << cd_event_.size() << ", (head) event queue size : " << cd_event_.size()<< endl;
//  CDInternalErrT cd_err = kOK;
//
//  while(!cd_event_.empty()) {
//    EventHandler *headcd_event_handler = cd_event_.back();
//    headcd_event_handler->HandleEvent();
//    delete headcd_event_handler;
//    cd_event_.pop_back();
//  }
//  while(!cd_event_.empty()) {
//    EventHandler *cd_event_handler = cd_event_.back();
//    cd_event_handler->HandleEvent();
//    delete cd_event_handler;
//    cd_event_.pop_back();
//  }
//
//
//  cddbg << "Handled event count : " << handled_event_count << endl;
//  cddbg << "currnt pending count : " << *pendingFlag_ << endl;
//
//
//
//  return cd_err;
//}


//-------------------------------------------------------



CDEntry *CD::SearchEntry(ENTRY_TAG_T tag_to_search, uint32_t &found_level)
{
    CD_DEBUG("Search Entry : %u (%s) at level #%u \n", tag_to_search, tag2str[tag_to_search].c_str(), level());

    CDHandle *parent_cd = CDPath::GetParentCD();
    CDEntry *entry_tmp = parent_cd->ptr_cd()->InternalGetEntry(tag_to_search);

    CD_DEBUG("Parent name : %s\n", parent_cd->GetName());

    if(entry_tmp != NULL) { 
      CD_DEBUG("Parent dst addr : %p \tParent entry name : %s\n", 
                entry_tmp->dst_data_.address_data(), entry_tmp->dst_data_.ref_name().c_str());
    } else {
      CD_DEBUG("There is no reference in parent level\n");
    }

//    if( ptr_cd_ == 0 ) { ERROR_MESSAGE("Pointer to CD object is not set."); assert(0); }

//    CDEntry* entry = parent_cd->ptr_cd()->InternalGetEntry(tag_to_search);
//    cddbg <<"ref name: "    << entry->dst_data_.ref_name() 
//              << ", at level: " << entry->ptr_cd()->GetCDID().level()<<endl;
//    if( entry != 0 ) {

      //Here, we search Parent's entry and Parent's Parent's entry and so on.
      //if ref_name does not exit, we believe it's original. 
      //Otherwise, there is original copy somewhere else, maybe grand parent has it. 

      CDEntry* entry = NULL;
      while( parent_cd != NULL ) {
        CD_DEBUG("InternalGetEntry at level #%u\n", parent_cd->ptr_cd()->GetCDID().level());

        entry = parent_cd->ptr_cd()->InternalGetEntry(tag_to_search);

        if(entry != NULL) {
          found_level = parent_cd->ptr_cd()->level();

          CD_DEBUG("\n\nI got my reference here!! found level : %d, Node ID : %s\n", 
                   found_level, GetNodeID().GetString().c_str());
          CD_DEBUG("Current entry name (%s) with ref name (%s) at level #%d\n", 
                   entry->name().c_str(), entry->dst_data_.ref_name().c_str(), found_level);
          break;
        }
        else {
          parent_cd = CDPath::GetParentCD(parent_cd->ptr_cd()->level());
          if(parent_cd != NULL) 
            CD_DEBUG("Gotta go to upper level! -> %s at level#%u\n", parent_cd->GetName(), parent_cd->ptr_cd()->GetCDID().level());
        }
      } 
      if(parent_cd == NULL) entry = NULL;
      CD_DEBUG("\n[CD::SearchEntry] Done. Check entry %p at Node ID %s, CDName %s\n", 
               entry, GetNodeID().GetString().c_str(), GetCDName().GetString().c_str());


//      // preservation via reference for remote copy.
//      if(parent_cd == NULL) {
//        if(entry == NULL) {
//          AddEntryToSend(tag_to_search);
//          RequestEntrySearch(tag_to_search);
//        }
//        else {
//          cerr << "Something is wrong. It failed to search entry at local task, but an entry is populated somehow.\n" << endl;
//          assert(0);
//        }
//      }

  return entry;
}

/*

CDEntry *CD::SearchEntry(ENTRY_TAG_T entry_tag_to_search, int &found_level)
{
    cddbg << "Search Entry : " << entry_tag_to_search << "(" << tag2str[entry_tag_to_search] <<")"<< endl;
    CDHandle *parent_cd = CDPath::GetParentCD();
    CDEntry *entry_tmp = parent_cd->ptr_cd()->InternalGetEntry(entry_tag_to_search);

    cddbg<<"parent name: "<<parent_cd->GetName()<<endl;
    if(entry_tmp != NULL) { 
      cddbg << "parent dst addr : " << entry_tmp->dst_data_.address_data()
                << ", parent entry name : " << entry_tmp->dst_data_.ref_name()<<endl;
    } else {
      cddbg<<"there is no reference in parent level"<<endl;
    }
//    if( ptr_cd_ == 0 ) { ERROR_MESSAGE("Pointer to CD object is not set."); assert(0); }

//    CDEntry* entry = parent_cd->ptr_cd()->InternalGetEntry(entry_tag_to_search);
//    cddbg <<"ref name: "    << entry->dst_data_.ref_name() 
//              << ", at level: " << entry->ptr_cd()->GetCDID().level()<<endl;
//    if( entry != 0 ) {

      //Here, we search Parent's entry and Parent's Parent's entry and so on.
      //if ref_name does not exit, we believe it's original. 
      //Otherwise, there is original copy somewhere else, maybe grand parent has it. 

      CDEntry* entry = NULL;
      while( parent_cd != NULL ) {
        cddbg << "InternalGetEntry at Level: " << parent_cd->ptr_cd()->GetCDID().level()<<endl;
        entry = parent_cd->ptr_cd()->InternalGetEntry(entry_tag_to_search);

        if(entry != NULL) {
          found_level = parent_cd->ptr_cd()->level();
          cout<<"\n\nI got my reference here!! found level: " << found_level << endl;
          cout <<"Current entry name : "<< entry->name() << " with ref name : "  << entry->dst_data_.ref_name() 
                  << ", at level: " << found_level<<endl;
          break;
        }
        else {
          parent_cd = CDPath::GetParentCD(parent_cd->ptr_cd()->level());
          if(parent_cd != NULL) cddbg<< "Gotta go to upper level! -> " << parent_cd->GetName() << " at level "<< parent_cd->ptr_cd()->GetCDID().level() << endl;
        }
        cddbg.flush();
      } 
      if(parent_cd == NULL) entry = NULL;
      cddbg<<"--------- CD::SearchEntry Done. Check entry " << entry <<", at " << GetNodeID() << " -----------" << endl; //(int)(entry ===NULL)<endl;
      cddbg.flush();
//       
//
//      // preservation via reference for remote copy.
//      if(parent_cd == NULL) {
//        if(entry == NULL) {
//          AddEntryToSend(entry_tag_to_search);
//          RequestEntrySearch(entry_tag_to_search);
//        }
//        else {
//          cerr << "Something is wrong. It failed to search entry at local task, but an entry is populated somehow.\n" << endl;
//          assert(0);
//        }
//      }
  return entry;
}
*/

//void CD::DecPendingCounter(void)
//{
////  PMPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_in_color(), 0, pendingWindow_);
//  (*pendingFlag_) -= handled_event_count;
//  // Initialize handled_event_count;
//  handled_event_count = 0;
////  PMPI_Win_unlock(task_in_color(), pendingWindow_);
//}



//void CD::AddEntryToSend(const ENTRY_TAG_T &entry_tag_to_search) 
//{
////  entry_recv_req_[entry_tag_to_search];
//  PMPI_Alloc_mem(MAX_ENTRY_BUFFER_SIZE, PMPI_INFO_NULL, &(entry_recv_req_[entry_tag_to_search]));
//}
//
//void CD::RequestEntrySearch(void)
//{
//  cddbg << "REQ" << endl;
//  for(auto it=entry_recv_req_.begin(); it!=entry_recv_req_.end(); ++it) {
//    cddbg << tag2str[*it] << endl;
//  }
////  PMPI_Status status;
////  void *entry_list_to_search;
////  PMPI_Probe(source, tag, cdh->node_id().color(), &status)
//
//  PMPI_Irecv(entry_recv_req_[entry_tag_to_search], PACKED_EN, MPI_UNSIGNED_LONG_LONG, src, entry_tag_to_search, node_id().color(), &status);  
//  
//}
//

//int CD::AddDetectFunc(void *() custom_detect_func)
//{
//
//  return CDErrT::kOK;
//}

void CD::Escalate(uint64_t error_name_mask, 
                  uint64_t error_location_mask,
                  std::vector<SysErrT> errors)
{
  // STUB
}

bool CD::CanRecover(uint64_t error_name_mask, 
                    uint64_t error_location_mask)
{
  // STUB
  return true;
}

CD::CDInternalErrT CD::Sync(ColorT color)
{

#if _MPI_VER
  PMPI_Barrier(color);
#endif

  return CDInternalErrT::kOK;
}

CD::CDInternalErrT CD::SyncFile(void)
{

  // STUB
  //
  return CDInternalErrT::kOK;
}
