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

#include "cd.h"
#include "cd_handle.h"
#include "cd_id.h"
//#include "data_handle.h"
#include "cd_entry.h"
#include "cd_path.h"
#include "event_handler.h"
#include <stdexcept>
#include <typeinfo>
#include <csetjmp>

using namespace cd;
using namespace std;

//#ifdef comm_log
//#include "cd_path.h"
//#endif

int iterator_entry_count=0;
std::hash<std::string> cd::str_hash;
std::map<ENTRY_TAG_T, std::string> cd::tag2str;

#if _MPI_VER
#if _KL
CDFlagT *CD::pendingFlag_; 
#endif
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

CD::CD()
//  : cd_type_(kStrict), name_("INITIAL_NAME"), sys_detect_bit_vector_(0)
{
  cd_type_ = kStrict; 
  name_ = "INITIAL_NAME"; 
  sys_detect_bit_vector_ = 0;
  // Assuming there is only one CD Object across the entire system we initilize cd_id info here.
  cd_id_ = CDID();

  // Kyushick: Object ID should be unique for the copies of the same object?
  // For now, I decided it as an unique one
  cd_id_.object_id_++;

  recoverObj_ = new RecoverObject;
  
  need_reexec = false;
  num_reexecution_ = 0;
  Init();  

#ifdef comm_log
  // SZ
  // create instance for comm_log_ptr_ for relaxed CDs
  // if no parent assigned, then means this is root, so log mode will be kGenerateLog at creation point
  assert(comm_log_ptr_ == NULL);

  PRINT_DEBUG("Set child_seq_id_ to 0\n");
  child_seq_id_ = 0;

  //// init incomplete_log_
  //incomplete_log_size_ = incomplete_log_size_unit_;
  //incomplete_log_.resize(incomplete_log_size_);
#endif
}

  // FIXME: only acquire root handle when needed. 
  // Most of the time, this might not be required.
  // Kyushick : I cannot fully understand this comment....
CD::CD(CDHandle* cd_parent, 
       const char* name, 
       const CDID& cd_id, 
       CDType cd_type, 
       uint64_t sys_bit_vector)
//  : cd_type_(cd_type), name_(name), sys_detect_bit_vector_(0), cd_id_(cd_id)
{
  //GONG
  begin_ = false;

  cd_type_ = cd_type; 
  if(name != NULL)
    name_ = name; 
  sys_detect_bit_vector_ = 0; 
  cd_id_ = cd_id;
  // FIXME 
  // cd_id_ = 0; 
  // We need to call which returns unique id for this cd. 
  // the id is recommeneded to have pre-allocated sections per node. 
  // This way, we don't need to have race condition to get unique id. 
  // Instead local counter is used to get the unique id.

  // Kyushick: Object ID should be unique for the copies of the same object?
  // For now, I decided it as an unique one
  cd_id_.object_id_++;

//  dbg << "cd object is created " << cd_id_.object_id_++ <<endl;

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

  need_reexec = false;
  num_reexecution_ = 0;

  Init();  

#ifdef comm_log
  // SZ
  // create instance for comm_log_ptr_
  // comm_log is per thread per CD
  // if cd_parent is not NULL, then inherit comm log mode from parent,
  // otherwise means current cd is a root cd, so comm log mode is to generate log
  if (cd_parent != NULL)
  {
    // Only create CommLog object for relaxed CDs
    assert(comm_log_ptr_ == NULL);
    if (cd_type_ == kRelaxed)
    {
      //SZ: if parent is a relaxed CD, then copy comm_log_mode from parent
      if (cd_parent->ptr_cd()->cd_type_ == kRelaxed)
      {
        CommLogMode parent_log_mode = cd_parent->ptr_cd()->comm_log_ptr_->GetCommLogMode();
        PRINT_DEBUG("With cd_parent (%p) relaxed CD, creating CommLog with parent's mode %d\n", cd_parent, parent_log_mode);
        comm_log_ptr_ = new CommLog(this, parent_log_mode);
      }
      //SZ: if parent is a strict CD, then child CD is always in kGenerateLog mode at creation point
      else
      {
        PRINT_DEBUG("With cd_parent (%p) strict CD, creating CommLog with kGenerateLog mode\n", cd_parent);
        comm_log_ptr_ = new CommLog(this, kGenerateLog);
      }
    }

    // set child's child_seq_id_  to 0 
    PRINT_DEBUG("Set child's child_seq_id_ to 0\n");
    child_seq_id_ = 0;

    uint32_t tmp_seq_id = cd_parent->ptr_cd()->child_seq_id_;
    PRINT_DEBUG("With cd_parent = %p, set child's seq_id_ to parent's child_seq_id_(%d)\n", cd_parent, tmp_seq_id);
    cd_id_.SetSequentialID(tmp_seq_id);
    //GONG
    libc_log_ptr_ = new CommLog(this, kGenerateLog);
  }
  else 
  {
    // Only create CommLog object for relaxed CDs
    assert(comm_log_ptr_ == NULL);
    if (cd_type_ == kRelaxed)
    {
      PRINT_DEBUG("With cd_parent = NULL, creating CommLog with mode kGenerateLog\n");
      comm_log_ptr_ = new CommLog(this, kGenerateLog);
    }
    //GONG:
    libc_log_ptr_ = new CommLog(this, kGenerateLog);

    PRINT_DEBUG("Set child's child_seq_id_ to 0\n");
    child_seq_id_ = 0;
  }

  //// init incomplete_log_
  //incomplete_log_size_ = incomplete_log_size_unit_;
  //incomplete_log_.resize(incomplete_log_size_);
#endif

}

void CD::Initialize(CDHandle* cd_parent, 
                    const char* name, 
                    CDID cd_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector)
{
  cd_type_  = cd_type;
  name_     = name;
  cd_id_    = cd_id;
  cd_id_.object_id_++;
  sys_detect_bit_vector_ = sys_bit_vector;
  Init();  
}

void CD::Init()
{
  ctxt_prv_mode_ = kExcludeStack; 
  cd_exec_mode_  = kSuspension;
  option_save_context_ = 0;
//#if _WORK 
//  path = Path("ssd", "hhd");
//  path.SetSSDPath("./SSDpath/");
//  path.SetHDDPath("./HDDpath/");
//  InitOpenHDD();
//  InitOpenSSD();
//#endif

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

#ifdef comm_log
  //Delete comm_log_ptr_
  if (comm_log_ptr_ != NULL)
  {
    PRINT_DEBUG("Delete comm_log_ptr_\n");
    delete comm_log_ptr_;
  }
  else 
  {
    PRINT_DEBUG("NULL comm_log_ptr_ pointer, this CD should be strict CD...\n");
  }
#endif
}

CDHandle* CD::Create(CDHandle* parent, 
                     const char* name, 
                     const CDID& child_cd_id, 
                     CDType cd_type, 
                     uint64_t sys_bit_vector, 
                     CD::CDInternalErrT *cd_internal_err)
{
  /// Create CD object with new CDID
  CDHandle* new_cd_handle = NULL;

  *cd_internal_err = InternalCreate(parent, name, child_cd_id, cd_type, sys_bit_vector, &new_cd_handle);
  assert(new_cd_handle != NULL);

  this->AddChild(new_cd_handle);

  return new_cd_handle;

}

CDHandle* CD::CreateRootCD(CDHandle* parent, 
                           const char* name, 
                           const CDID& root_cd_id, 
                           CDType cd_type, 
                           uint64_t sys_bit_vector, 
                           CD::CDInternalErrT *cd_internal_err)
{
  /// Create CD object with new CDID
  CDHandle* new_cd_handle = NULL;

  *cd_internal_err = InternalCreate(parent, name, root_cd_id, cd_type, sys_bit_vector, &new_cd_handle);
  assert(new_cd_handle != NULL);

  return new_cd_handle;
}


CDErrT CD::Destroy(void)
{
  dbg << "~~~~~~~~~~~~~~~~~~~~~"<<endl;
  CDErrT err=CDErrT::kOK;
  InternalDestroy();


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
  dbg << "dbg: Internal Create... Level : " << new_cd_id.level()<< ", node : "<< new_cd_id.node_id() << endl; dbgBreak();
//  dbg << dbg.str() << endl; dbgBreak();
  if( !new_cd_id.IsHead() ) {
    CD *new_cd     = new CD(parent, name, new_cd_id, cd_type, sys_bit_vector);

    // Create memory region where RDMA is enabled
#if _MPI_VER
#if _KL
    int task_count = new_cd_id.task_count();

    if(task_count > 1) {
      dbg << "in CD::Create Internal Memory. task count is "<< task_count <<endl;
      MPI_Alloc_mem(sizeof(CDFlagT), 
                    MPI_INFO_NULL, &(new_cd->event_flag_));

      // Initialization of event flags
      *(new_cd->event_flag_) = 0;

//      MPI_Win_create(NULL, 0, 1,
//                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->mailbox_));

      MPI_Win_create(new_cd->event_flag_, sizeof(CDFlagT), sizeof(CDFlagT),
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->mailbox_));

      // FIXME : should it be MPI_COMM_WORLD?
      MPI_Win_create(new_cd->pendingFlag_, sizeof(CDFlagT), sizeof(CDFlagT), 
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->pendingWindow_));
    }
#endif
#endif

    if( new_cd->GetPlaceToPreserve() == kPFS ) {
      new_cd->Par_IO_Man = new CD_Parallel_IO_Manager( new_cd, "./PFS_Root/" ); 
    }
    *new_cd_handle = new CDHandle(new_cd, new_cd_id.node_id());
  }
  else {
    // Create a CD object for head.
    HeadCD *new_cd = new HeadCD(parent, name, new_cd_id, cd_type, sys_bit_vector);




#if _MPI_VER
#if _KL
    // Create memory region where RDMA is enabled
    dbg << "HeadCD create internal memory " << endl;
    int task_count = new_cd_id.task_count();

    if(task_count > 1) {
      MPI_Alloc_mem(task_count*sizeof(CDFlagT), 
                    MPI_INFO_NULL, &(new_cd->event_flag_));
      // Initialization of event flags
      for(int i=0; i<task_count; i++) {
        new_cd->event_flag_[i] = 0;
      }
    
//    new_cd->mailbox_ = new CDMailBoxT[task_count];
  
      dbg << "HeadCD mpi win create for "<< task_count << " mailboxes"<<endl;
//    for(int i=0; i<task_count; ++i) {
      MPI_Win_create(new_cd->event_flag_, task_count*sizeof(CDFlagT), sizeof(CDFlagT),
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->mailbox_));
//    }


      // FIXME : should it be MPI_COMM_WORLD?
      MPI_Win_create(new_cd->pendingFlag_, sizeof(CDFlagT), sizeof(CDFlagT), 
                     MPI_INFO_NULL, new_cd_id.color(), &(new_cd->pendingWindow_));


    }
//    AttachChildCD(new_cd);
#endif
#endif
    if( new_cd->GetPlaceToPreserve() == kPFS ) {
      new_cd->Par_IO_Man = new CD_Parallel_IO_Manager( new_cd, "./PFS_Root/" ); 
    }
    *new_cd_handle = new CDHandle(new_cd, new_cd_id.node_id());
  }
  
  dbg << "after KL : " << new_cd_id.node_id() << " -- " << (*new_cd_handle)->node_id() << endl;
  return CD::CDInternalErrT::kOK;
}










// FIXME 11092014
void AttachChildCD(HeadCD *new_cd)
{
  
}

inline CD::CDInternalErrT CD::InternalDestroy(void)
{

  cout << "in CD::Destroy Internal Memory"<<endl;

#if _MPI_VER
#if _KL
  int task_count = cd_id_.task_count();
  dbg << "mpi win free for "<< task_count << " mailboxes"<<endl;

  if(task_size() > 1) {  
    MPI_Win_free(&pendingWindow_);
    MPI_Win_free(&mailbox_);
  }
  else
    dbg << "KL : size is 1" << endl;


#endif
#endif

//GONG
//#if _WORK
//  //When we destroy a CD, we need to delete its log (preservation file)
//  for(std::list<CDEntry>::iterator it = entry_directory_.begin(); 
//      it != entry_directory_.end() ; ++it) {
//
//    bool use_file = false;
//    if(cd_id_.level()<=1)  use_file = true;
//    if( use_file == true)  {
////      if(cd_id_.level()==1) { // HDD
////        it->CloseFile(&HDDlog);  
////      }
////      else { // SSD
////        it->CloseFile(&SSDlog);  
////      }
//    }
//
//  }  // for loop ends
//#endif

#ifdef comm_log
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
    delete Par_IO_Man;
  }

  delete this;

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
  //GONG
  app_side = false;
  begin_ = true;
  PRINT_DEBUG("inside CD::Begin\n");

#ifdef comm_log
  //SZ: if in reexecution, need to unpack logs to childs
  if (GetParentHandle()!=NULL)
  {
    // Only need to if for both parent and child are relaxed CDs, 
    // if child is relaxed but parent is strict, then create the CommLog object with kGenerateLog mode
    // if child is strict, then do not need to do anything for comm_log_ptr_...
    if(GetParentHandle()->ptr_cd_->cd_type_ == kRelaxed && cd_type_ == kRelaxed)
    {
      if (GetParentHandle()->ptr_cd_->comm_log_ptr_->GetCommLogMode() == kReplayLog)
      {
        PRINT_DEBUG("With comm log mode = kReplayLog, unpack logs to children\n");
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
          //SZ: FIXME: need to figure out a way to unpack logs if child is not in the same address space with parent
          PRINT_DEBUG("Should not be here to unpack logs!!\n");
        }
      }
    }
    //GONG: as long as parent CD is in replay(check with ), child CD needs to unpack logs
    if(GetParentHandle()->ptr_cd_->libc_log_ptr_->GetCommLogMode() == kReplayLog){
      PRINT_DEBUG("unpack libc logs to children - replay mode\n");
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
          PRINT_DEBUG("Should not be here to unpack logs!! - libc\n");
        }
    }

  }
#endif

  dbg<<"inside CD::Begin"<<endl;
  if( cd_exec_mode_ != kReexecution ) { // normal execution
    num_reexecution_ = 0;
    cd_exec_mode_ = kExecution;
  }
  else {
    cout << "Begin again! " << endl; getchar();
    num_reexecution_++ ;
  }

  //GONG
  app_side = true;

  return CDErrT::kOK;
}

/*  CD::Complete()
 *  (1) Call all the user-defined error checking functions.
 *      Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *
 */
CDErrT CD::Complete(bool collective, bool update_preservations)
{
  //GONG
  app_side = false;
  begin_ = false;
  //printf("app_side = false in CD::Complete\n");
#ifdef comm_log
  // SZ: pack logs and push to parent
  if (GetParentHandle()!=NULL) {
    if (cd_type_ == kRelaxed) {
      if (IsParentLocal()) {
        if (IsNewLogGenerated() && GetParentHandle()->ptr_cd_->cd_type_ == kRelaxed) {
          PRINT_DEBUG("Pushing logs to parent...\n");
          comm_log_ptr_->PackAndPushLogs(GetParentHandle()->ptr_cd_);

#if _DEBUG
          PRINT_DEBUG("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
          PRINT_DEBUG("\nchild comm_log print:\n");
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

#if _DEBUG
          PRINT_DEBUG("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
          PRINT_DEBUG("parent comm_log print:\n");
          GetParentHandle()->ptr_cd_->comm_log_ptr_->Print();
#endif
        }

        comm_log_ptr_->Reset();
      }
      else  {
        //SZ: FIXME: need to figure out a way to push logs to parent that resides in other address space
        PRINT_DEBUG("Should not come to here...\n");
      }
    }

    // push incomplete_log_
    if (IsParentLocal() && incomplete_log_.size()!=0) {
      //vector<struct IncompleteLogEntry> *pincomplog 
      //                                    = &(GetParentHandle()->ptr_cd_->incomplete_log_);
      CD* ptmp = GetParentHandle()->ptr_cd_;

      // push incomplete logs to parent
      ptmp->incomplete_log_.insert(ptmp->incomplete_log_.end(),
                                   incomplete_log_.begin(),
                                   incomplete_log_.end());

      // clear incomplete_log_ of current CD 
      incomplete_log_.clear();
    }
    else if (!IsParentLocal()) {
      //SZ: FIXME: need to figure out a way to push logs to parent that resides in other address space
      PRINT_DEBUG("Should not come to here...\n");
    }
  }
  //GONG
  if(GetParentHandle()!=NULL) {
    if(IsParentLocal()) {
      if(IsNewLogGenerated_libc())  {
        PRINT_DEBUG("Pushing logs to parent...\n");
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
      PRINT_DEBUG("parent and child are in different memory space - libc\n");
    }
  }
//    std::cout<<"size: "<<mem_alloc_log_.size()<<std::endl;

    //GONG: DO actual free completed mem_alloc_log_
    std::vector<struct IncompleteLogEntry>::iterator it;
    for (it=mem_alloc_log_.begin(); it!=mem_alloc_log_.end(); it++)
    {
//      printf("check log %p %i %i\n", it->p_, it->complete_, it->pushed_);
      if(it->complete_)
      {       
        if(it->pushed_)
        {
          printf(" free - completed + pushed - %p\n", it->p_);
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
//    std::cout<<"size  :"<<mem_alloc_log_.size()<<" app_side: "<<app_side<<std::endl;
    //GONG: push mem_alloc_log_
  if(GetParentHandle()!=NULL)
  {
    if (IsParentLocal() && mem_alloc_log_.size()!=0)
    {
      CD* ptmp = GetParentHandle()->ptr_cd_;
      // push memory allocation logs to parent
      std::vector<struct IncompleteLogEntry>::iterator ii;
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
      PRINT_DEBUG("Should not come to here...\n");
    }
  }


#endif


  // Increase sequential ID by one
  cd_id_.sequential_id_++;
  
  /// It deletes entry directory in the CD (for every Complete() call). 
  /// We might modify this in the profiler to support the overlapped data among sequential CDs.
  DeleteEntryDirectory();

  // TODO ASSERT( cd_exec_mode_  != kSuspension );
  // FIXME don't we have to wait for others to be completed?  
  cd_exec_mode_ = kSuspension; 

  //GONG
  app_side = true;

  return CDErrT::kOK;
} // CD::Complete ends


//GONG
bool CD::PushedMemLogSearch(void* p)
{
  bool ret = false;
  if(GetCDID().level()!=0)
  {
    CD* parent_CD = GetParentHandle()->ptr_cd();
    if(parent_CD!=NULL)
    {
      if(parent_CD->mem_alloc_log_.size()!=0)
      {
        std::vector<struct IncompleteLogEntry>::iterator it;  
        for(it=parent_CD->mem_alloc_log_.begin(); it!=parent_CD->mem_alloc_log_.end();it++)
        {
          if(it->p_ == p)
          {
            ret = true;
            //If isnt completed, change it completed.
            it->complete_ = true;
            break;
          }
        }
      }
      else
      {
        ret = PushedMemLogSearch(p);
      }
    }
    else
    {
      printf("CANNOT find parent CD\n");
      exit(1);
    }
  }

  return ret;
}

void* CD::MemAllocSearch(void* p_update)
{
//  printf("MemAllocSearch\n");
  void* ret = NULL;
  if(GetCDID().level()!=0)
  {
    CD* parent_CD = GetParentHandle()->ptr_cd();
    if(parent_CD!=NULL)
    {
      //GONG:       
      if(parent_CD->mem_alloc_log_.size()!=0)
      {
        if(!p_update)
        {
          ret = parent_CD->mem_alloc_log_[parent_CD->cur_pos_mem_alloc_log].p_;
        }
        else
        {
          ret = parent_CD->mem_alloc_log_[parent_CD->cur_pos_mem_alloc_log].p_;
          parent_CD->mem_alloc_log_[parent_CD->cur_pos_mem_alloc_log].p_ = p_update;
        }
        parent_CD->cur_pos_mem_alloc_log++;
      }
      else
      {
        ret = MemAllocSearch(p_update);
      }
    }
    else
    {
      printf("CANNOT find parent CD\n");
      exit(1);
    }
  }
  else
  {
    printf("rootCD is trying to search further\n");
    exit(1);
  }
  
  if(ret == NULL)
  {
    printf("somethig wrong!\n");
    exit(1);
  
  }

  return ret;
}





void *CD::SerializeRemoteEntryDir(uint32_t& len_in_bytes) 
{
  Packer entry_dir_packer;
  uint32_t entry_count = 0;

  for(auto it = remote_entry_directory_map_.begin(); 
           it!= remote_entry_directory_map_.end(); ++it) {
    uint32_t entry_len=0;
    void *packed_entry_p=0;
    if( !it->second->name().empty() ){ 
      packed_entry_p = it->second->Serialize(entry_len);
      entry_dir_packer.Add(entry_count++, entry_len, packed_entry_p);
    }
  }
  
  return entry_dir_packer.GetTotalData(len_in_bytes);
}


void CD::DeserializeRemoteEntryDir(std::map<uint64_t, CDEntry*> &remote_entry_dir, void *object, uint32_t task_count, uint32_t unit_size) 
{
  void *unpacked_entry_p=0;
  uint32_t dwGetID=0;
  uint32_t return_size=0;
  char *begin = (char *)object;

  dbg <<"KYU - "<< object << " CD level : " << CDPath::GetCurrentCD()->ptr_cd()->GetCDID().level() <<endl;

  for(uint64_t i=0; i<task_count; i++) {
    Unpacker entry_dir_unpacker;
    while(1) {
      unpacked_entry_p = entry_dir_unpacker.GetNext(begin + i * unit_size, dwGetID, return_size);
      if(unpacked_entry_p == NULL) {
//      dbg<<"DESER new ["<< cnt++ << "] i: "<< i <<"\ndwGetID : "<< dwGetID << endl;
//      dbg<<"return size : "<< return_size << endl;
  
        break;
      }
      CDEntry *cd_entry = new CDEntry();
      cd_entry->Deserialize(unpacked_entry_p);
      if(CDPath::GetCurrentCD()->IsHead()) dbg << "entry check before insert to entry dir: " << tag2str[cd_entry->entry_tag_] << " , " << object << endl; //getchar();
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
    
    MPI_Send(receiver, head, msg_for_receiver); // it will go to receiver
    MPI_Send(sender, head, msg_for_sender0);    // it will go to sender
  }
}


CD::CDInternalErrT CD::RequestDataMove(int sender, int receiver, const char *found_ref_name)
{

  MPI_Recv(me, head, msg); // this task will know if I am sender or receiver with this msg from head.

  if( am_I_sender() ){
    MPI_Send(receiver_from_msg, me, msg);
  } else {
    MPI_Recv(me, sender_from_msg, msg);
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

//  MPI_Allreduce(); // Get the number of entry search requests for HeadCD

  if( !IsHead() ) {

    uint32_t entry_count=0;

    void *packed_entry_dir_p = SerializeEntryDir(entry_count);
    
    MPI_Gather(sendBuf, num_elements, INTEGER, recvBuf, recv_count, INTEGER, GetHead(), node_id_.color_);

  } 
  else { // HeadCD
    
    MPI_Gather(sendBuf, num_elements, INTEGER, recvBuf, recv_count, INTEGER, GetHead(), node_id_.color_);

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




/*  CD::preserve(char* data_p, int data_l, enum preserveType prvTy, enum mediumLevel medLvl)
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
 *
 */


/*
CDErrT CD::Preserve(void* data, 
                    uint64_t len_in_bytes, 
                    uint32_t preserve_mask, 
                    const char* my_name, 
                    const char* ref_name, 
                    uint64_t ref_offset, 
                    const RegenObject* regen_object, 
                    PreserveUseT data_usage)
{

  // FIXME MALLOC should use different ones than the ones for normal malloc
  // For example, it should bypass malloc wrap functions.
  // FIXME for now let's just use regular malloc call 
  if(cd_exec_mode_ == kExecution ) {
//    return ((CDErrT)kOK==InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage)) ? (CDErrT)kOK : (CDErrT)kError;
//    return (kOK==InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage)) ? kOK : kError;
    return InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
  }
  else if(cd_exec_mode_ == kReexecution) {
    // it is in re-execution mode, so instead of preserving data, restore the data.
    // Two options, one is to do the job here, 
    // another is that just skip and do nothing here but do the restoration job in different place, 
    // and go though all the CDEntry and call Restore() method. 
    // The later option seems to be more efficient but it is not clear that 
    // whether this brings some consistency issue as restoration is done at the very beginning 
    // while preservation was done one by one 
    // and sometimes there could be some computation in between the preservations.. (but wait is it true?)
  
    // Jinsuk: Because we want to make sure the order is the same as preservation, we go with  Wait...... It does not make sense... 
    // Jinsuk: For now let's do nothing and just restore the entire directory at once.
    // Jinsuk: Caveat: if user is going to read or write any memory space that will be eventually preserved, 
    // FIRST user need to preserve that region and use them. 
    // Otherwise current way of restoration won't work. 
    // Right now restore happens one by one. 
    // Everytime restore is called one entry is restored. 
    if( iterator_entry_ == entry_directory_.end() ) {
      //ERROR_MESSAGE("Error: Now in re-execution mode but preserve function is called more number of time than original"); 
      // NOT TRUE if we have reached this point that means now we should actually start preserving instead of restoring.. 
      // we reached the last preserve function call. 

      //Since we have reached the last point already now convert current execution mode into kExecution
      //      PRINT_DEBUG("Now reached end of entry directory, now switching to normal execution mode\n");
      cd_exec_mode_  = kExecution;    
  //  return InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
      return (kOK==InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage)) ? kOK : kError;
    }
    else {
      //     PRINT_DEBUG("Reexecution mode...\n");
      CDEntry cd_entry = *iterator_entry_;
      iterator_entry_++;
#if _WORK
      bool use_file = (bool)ref_name;
      bool open = true;
//      if(cd_id_.level()<=1)
//        use_file = true;
      if( use_file == true) {

//        if(cd_id_.level()==1) {  // HDD
//          bool _isOpenHDD = isOpenHDD();
//          if(!_isOpenHDD)  
//            OpenHDD(); // set flag 'open_HDD'       
//          return cd_entry.Restore(_isOpenHDD, &HDDlog);
//        }
//        else { // SSD
//          bool _isOpenSSD = isOpenSSD();
//          if(!_isOpenSSD)
//            OpenSSD(); // set flag 'open_SSD'       
//          return cd_entry.Restore(_isOpenSSD, &SSDlog);
//        }
        switch(GetPlaceToPreserve()) {
          case kMemory:
            assert(0);
            break;
          case kHDD:
//            bool _isOpenHDD = isOpenHDD();
            if( !isOpenHDD() )  
              OpenHDD(); // set flag 'open_HDD'       
            return cd_entry.Restore(isOpenHDD(), &HDDlog);
        
          case kSSD:
//            bool _isOpenSSD = isOpenSSD();
            if( !isOpenSSD() )
              OpenSSD(); // set flag 'open_SSD'       
            return cd_entry.Restore(isOpenSSD(), &SSDlog);
        
          case kPFS:
            assert(0);
            break;
          default:
            break;
        }

      }
      else {
        return cd_entry.Restore(open, &HDDlog);
      }
#else
      return cd_entry.Restore() kOK : kError;
#endif
    }

  } // Reexecution ends
  return kError; // we should not encounter this point

}
*/

// FIXME
PrvMediumT CD::GetPlaceToPreserve()
{
#if _MEMORY
  return kMemory;
#elif _PFS
  return kPFS;
#elif _HDD
  return kHDD;
#elif _SSD
  return kSSD;
#else
  return kMemory;
#endif
}

bool CD::TestComm(bool test_until_done)
{
  bool is_all_valid = true;

  for(auto it=entry_request_req_.begin(); it!=entry_request_req_.end(); ) {
    MPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      entry_request_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }

  for(auto it=entry_recv_req_.begin(); it!=entry_recv_req_.end(); ) {
    MPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      entry_recv_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }

  for(auto it=entry_send_req_.begin(); it!=entry_send_req_.end(); ) {
    MPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      entry_send_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }
  return is_all_valid;
}

bool HeadCD::TestComm(bool test_until_done)
{
  bool is_all_valid = true;

  for(auto it=entry_request_req_.begin(); it!=entry_request_req_.end(); ) {
    MPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      entry_request_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }


  for(auto it=entry_search_req_.begin(); it!=entry_search_req_.end(); ) {
    MPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      entry_search_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }

  // Here is the same as CD::TestComm

  for(auto it=entry_recv_req_.begin(); it!=entry_recv_req_.end(); ) {
    MPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      entry_recv_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }

  for(auto it=entry_send_req_.begin(); it!=entry_send_req_.end(); ) {
    MPI_Test(&(it->second.req_), &(it->second.valid_), &(it->second.stat_));

    if(it->second.valid_) {
      entry_send_req_.erase(it++);
    }
    else {
      is_all_valid &= it->second.valid_;
      ++it;
    }
  }


  return is_all_valid;
} 


CDErrT CD::Preserve(void *data, 
                    uint64_t len_in_bytes, 
                    uint32_t preserve_mask, 
                    const char *my_name, 
                    const char *ref_name, 
                    uint64_t ref_offset, 
                    const RegenObject *regen_object, 
                    PreserveUseT data_usage)
{
  //GONG
  app_side = false;
  //printf("app_side = false in CD::Preserve\n");

  // FIXME MALLOC should use different ones than the ones for normal malloc
  // For example, it should bypass malloc wrap functions.
  // FIXME for now let's just use regular malloc call 

  if(cd_exec_mode_  == kExecution ) {      // Normal execution mode -> Preservation
//    dbg<<"my_name "<< my_name<<endl;
    switch( InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage) ) {
      case CDInternalErrT::kOK            : 
              //GONG
              //printf("app_side = true in CD::Preserve\n");
              app_side = true;
              return CDErrT::kOK;
      case CDInternalErrT::kExecModeError :
              //GONG
              //printf("app_side = true in CD::Preserve\n");
              app_side = true;
              return CDErrT::kError;
      case CDInternalErrT::kEntryError    : 
              //GONG
              //printf("app_side = true in CD::Preserve\n");
              app_side = true;
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
    if( iterator_entry_ != entry_directory_.end() ) { // normal case

//      printf("Reexecution mode start...\n");
      dbg<< "Now reexec!!!" << iterator_entry_count++ <<endl<<endl;  

      CDEntry *cd_entry = &*iterator_entry_;
      ++iterator_entry_;

      CDErrT cd_err;
      switch( cd_entry->Restore() ) {
        case CDEntry::CDEntryErrT::kOK : 
          //GONG
          app_side = true;

          cd_err = CDErrT::kOK; 
          break;
        case CDEntry::CDEntryErrT::kOutOfMemory : 
          //GONG
          app_side = true;

          cd_err = CDErrT::kError;
          break;
        case CDEntry::CDEntryErrT::kFileOpenError : 
          //GONG
          app_side = true;

          cd_err = CDErrT::kError;
          break;
        case CDEntry::CDEntryErrT::kEntrySearchRemote : {
          //GONG
          app_side = true;
#if _FIX
          this->TestComm();
#endif
          cd_err = CDErrT::kError;
          break;
        }
        default : assert(0);
      }

      if(iterator_entry_ == entry_directory_.end()) {
#if _FIX
        while( this->TestComm() ) {
          // FIXME : Becareful! CheckMailBox() has some collectives.
          CheckMailBox();
        }
#endif
        cd_exec_mode_ = kExecution;
        // This point means the beginning of body stage. Request EntrySearch at this routine
      }

      return cd_err;
 
    }
    else {  // abnormal case
      //return CDErrT::kOK;

      dbg<< "The end of reexec!!!"<<endl<<endl;  
      // NOT TRUE if we have reached this point that means now we should actually start preserving instead of restoring.. 
      // we reached the last preserve function call. 
      // Since we have reached the last point already now convert current execution mode into kExecution
      
//      ERROR_MESSAGE("Error: Now in re-execution mode but preserve function is called more number of time than original"); 
      printf("Now reached end of entry directory, now switching to normal execution mode\n");

      cd_exec_mode_  = kExecution;    
      switch( InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage) ) {
        case CDInternalErrT::kOK            : 
          //GONG
          //printf("app_side = true in CD::Preserve - abnormal\n");
          app_side = false;
          return CDErrT::kOK;
        case CDInternalErrT::kExecModeError : 
          //GONG
          //printf("app_side = true in CD::Preserve - abnormal\n");
          app_side = false;
          return CDErrT::kError;
        case CDInternalErrT::kEntryError    : 
          //GONG
          //printf("app_side = true in CD::Preserve - abnormal\n");
          app_side = false;
          return CDErrT::kError;
        default : assert(0);
      }

    }

    PRINT_DEBUG("Reexecution mode finished...\n");
  }   // Re-execution mode ends
  else {  // Suspension mode
    // Is it okay ?
    // Is it possible to call Preserve() at Suspension mode?
    assert(0);
  }

  //GONG
  //printf("app_side = true in CD::Preserve - kError\n");
  app_side = true;
  
  return kError; // we should not encounter this point
}

// Non-blocking Preserve
CDErrT CD::Preserve(CDEvent &cd_event,     
                    void *data_ptr, 
                    uint64_t len, 
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
                     uint64_t len_in_bytes, 
                     uint32_t preserve_mask, 
                     std::string my_name, 
                     const char *ref_name, 
                     uint64_t ref_offset, 
                     const RegenObject* regen_object, 
                     PreserveUseT data_usage)
{

  if(cd_exec_mode_  == kExecution ) { // Normal case
    dbg<< "\nNormal execution mode (internal preservation call)\n"<<endl;

    // Now create entry and add to list structure.
    //FIXME Jinsuk: we don't have the way to determine the storage   
    // Let's move all these allocation deallocation stuff to CDEntry. 
    // Object itself will know better than class CD. 

    CDEntry* cd_entry = 0;
//    if(my_name==0) my_name = "INITIAL_ENTRY";
//    dbg << "preserve remote check\npreserve_mask : "<< preserve_mask <<  endl; //dbgBreak();
    // Get cd_entry
    if( CHECK_PRV_TYPE(preserve_mask,kCopy) ) {                // via-copy, so it saves data right now!

      dbg << "\nPreservation via Copy to %d(memory or file)\n" << GetPlaceToPreserve() << endl;
      dbg << "Prv Mask : " << preserve_mask << ", CHECK PRV TYPE : " << CHECK_PRV_TYPE(preserve_mask, kShared) << endl;
      switch(GetPlaceToPreserve()) {
        case kMemory: {
          dbg<<"[kMemory] ------------------------------------------\n" << endl;
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kMemory, 0, len_in_bytes, cd_id_.node_id_), 
                                 my_name);
          cd_entry->set_my_cd(this);

          CDEntry::CDEntryErrT err = cd_entry->SaveMem();

          entry_directory_.push_back(*cd_entry);

          if( !my_name.empty() ) {

            if( !CHECK_PRV_TYPE(preserve_mask, kShared) ) {
              entry_directory_map_[str_hash(my_name)] = cd_entry;

              cout <<"register local entry_dir. my_name : "<<my_name << " - " << str_hash(my_name) 
                  <<", value : "<<*(reinterpret_cast<int*>(cd_entry->dst_data_.address_data())) 
                  <<", address: " <<cd_entry->dst_data_.address_data()<< endl;
            } 
            else{
              remote_entry_directory_map_[str_hash(my_name)] = cd_entry;

              cout <<"register remote entry_dir. my_name : "<<my_name
                  <<", value : "<<*(reinterpret_cast<int*>(cd_entry->dst_data_.address_data())) 
                  <<", address: " <<cd_entry->dst_data_.address_data()<< endl;


            }

          }
          else {
            cerr << "No entry name is provided. Currently it is not supported." << endl;
            assert(0);
          }
          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }
        case kHDD: {
          dbg << "kHDD ----------------------------------\n" << endl;
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kOSFile, 0, len_in_bytes, cd_id_.node_id_), 
                                 my_name);
          cd_entry->set_my_cd(this); 

          if( !log_handle_.isOpenHDD() ) log_handle_.OpenHDD(); // set flag 'open_HDD'       
          CDEntry::CDEntryErrT err = cd_entry->SaveFile(log_handle_.path.GetHDDPath(), log_handle_.isOpenHDD(), &(log_handle_.HDDlog));

          entry_directory_.push_back(*cd_entry); 

          if( !my_name.empty() ) {
            if( !CHECK_PRV_TYPE(preserve_mask, kShared) ) {
              entry_directory_map_[str_hash(my_name)] = cd_entry;

              dbg <<"register local entry_dir. my_name : "<<my_name
                        <<", value : "<<*(reinterpret_cast<int*>(cd_entry->dst_data_.address_data())) 
                        <<", address: " <<cd_entry->dst_data_.address_data()<< endl;
            }
            else {
              remote_entry_directory_map_[str_hash(my_name)] = cd_entry;

              dbg <<"register remote entry_dir. my_name : "<<my_name
                        <<", value : "<<*(reinterpret_cast<int*>(cd_entry->dst_data_.address_data())) 
                        <<", address: " <<cd_entry->dst_data_.address_data()<< endl;
            }
          }
          else {
            cerr << "No entry name is provided. Currently it is not supported." << endl;
            assert(0);
          }

          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }
        case kSSD: {
          dbg <<"kSSD ----------------------------------\n" << endl;
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kOSFile, 0, len_in_bytes, cd_id_.node_id_), 
                                 my_name);
          cd_entry->set_my_cd(this); 

          if( !log_handle_.isOpenSSD() ) log_handle_.OpenSSD(); // set flag 'open_SSD'       
          CDEntry::CDEntryErrT err = cd_entry->SaveFile(log_handle_.path.GetSSDPath(), log_handle_.isOpenSSD(), &(log_handle_.SSDlog));

          entry_directory_.push_back(*cd_entry);  

          if( !my_name.empty() ) {
            if( !CHECK_PRV_TYPE(preserve_mask, kShared) ) {
              entry_directory_map_[str_hash(my_name)] = cd_entry;

              assert(entry_directory_map_[str_hash(my_name)]);
              assert(entry_directory_map_[str_hash(my_name)]->src_data_.address_data());
              dbg <<"internal local entry_dir. my_name : "<<entry_directory_map_[str_hash(my_name)]->name()
                        <<", value : "<<*(reinterpret_cast<int*>(entry_directory_map_[str_hash(my_name)]->src_data_.address_data())) 
                        <<", address: " <<entry_directory_map_[str_hash(my_name)]->src_data_.address_data()
                        <<", address: " <<cd_entry->src_data_.address_data()<< endl;
            }
            else {
              remote_entry_directory_map_[str_hash(my_name)] = cd_entry;

              assert(remote_entry_directory_map_[str_hash(my_name)]);
              assert(remote_entry_directory_map_[str_hash(my_name)]->src_data_.address_data());
              dbg <<"register remote entry_dir. my_name : "<<my_name
                        <<", value : "<<*(reinterpret_cast<int*>(cd_entry->dst_data_.address_data())) 
                        <<", address: " <<cd_entry->dst_data_.address_data()<< endl;

            }
          }
          else {
            cerr << "No entry name is provided. Currently it is not supported." << endl;
            assert(0);
          }

          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }

        case kPFS: {
          dbg << "kPFS --------------------------------------------------------\n" << endl;
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kPFS, 0, len_in_bytes, cd_id_.node_id_), 
                                 my_name);
          cd_entry->set_my_cd(this); 

		      //Do we need to check for anything special for accessing to the global filesystem? 
		      //Potentially=> CDEntry::CDEntryErrT err = cd_entry->SavePFS(log_handle_.path.GetPFSPath(), log_handle_.isPFSAccessible(), &(log_handle_.PFSlog));
          if( !log_handle_.isOpenPFS() ) log_handle_.OpenPFS(); // set flag 'open_SSD'       
		      CDEntry::CDEntryErrT err = cd_entry->SavePFS( &(log_handle_.PFSlog) );//I don't know what should I do with the log parameter. I just add it for compatibility.
          

          entry_directory_.push_back(*cd_entry);  
          if( !my_name.empty() ) {
            if( !CHECK_PRV_TYPE(preserve_mask, kShared) ) {
              entry_directory_map_[ str_hash(my_name) ] = cd_entry;
              assert( entry_directory_map_[ str_hash( my_name ) ] );
              assert( entry_directory_map_[ str_hash( my_name ) ]->src_data_.address_data() );
            }
          }
          else {
            remote_entry_directory_map_[str_hash(my_name)] = cd_entry;
            assert(remote_entry_directory_map_[str_hash(my_name)]);
            assert(remote_entry_directory_map_[str_hash(my_name)]->src_data_.address_data());
          }
          dbg << "-------------------------------------------------------------\n" << endl;
          return (err == CDEntry::CDEntryErrT::kOK) ? CDInternalErrT::kOK : CDInternalErrT::kEntryError; 
        }
/*
        case kPFS: {
          dbg << "kPFS ----------------------------------\n"<< endl;
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                                 DataHandle(DataHandle::kOSFile, 0, len_in_bytes, cd_id_.node_id_), 
                                 my_name);
          cd_entry->set_my_cd(this); 
          entry_directory_.push_back(*cd_entry);  
//          if( ref_name != 0 ) entry_directory_map_.emplace(ref_name, *cd_entry);
          if( !my_name.empty() ) entry_directory_map_[str_hash(my_name)] = cd_entry;
          dbg << "\nPreservation to PFS which is not supported yet. ERROR\n" << endl;
          assert(0);
          break;
        }
*/
        default:
          break;
      }

    } // end of preserve via copy
    else if( CHECK_PRV_TYPE(preserve_mask, kRef) ) { // via-reference

      dbg << "\nPreservation via %d (reference)\n" << GetPlaceToPreserve() << endl;

      // set handle type and ref_name/ref_offset
      cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes, cd_id_.node_id_), 
                             DataHandle(DataHandle::kReference, 0, len_in_bytes, ref_name, ref_offset), 
                             my_name);
      cd_entry->set_my_cd(this); // this required for tracking parent later.. this is needed only when via ref
      entry_directory_.push_back(*cd_entry);  
//      entry_directory_map_.emplace(ref_name, *cd_entry);
//      entry_directory_map_[ref_name] = *cd_entry;
      if( !my_name.empty() ) {
        entry_directory_map_[str_hash(my_name)] = cd_entry;
        if( CHECK_PRV_TYPE(preserve_mask, kShared) ) {
          cout << "[CD::InternalPreserve. Error : kRef | kShared\n"
               << "Tried to preserve via reference but tried to allow itself as reference to other node. "
               << "If it allow itself for reference locally, it is fine!" << endl;
        }
      }

      return CDInternalErrT::kOK;
      
    }
    else if( CHECK_PRV_TYPE(preserve_mask, kRegen) ) { // via-regeneration

      //TODO
      cerr << "Preservation via Regeneration is not supported, yet. :-(" << endl;
      assert(0);
      return CDInternalErrT::kOK;
      
    }

    dbg << "\nUnsupported preservation type\n" << endl;
    assert(0);
    return CDInternalErrT::kEntryError;

  }
  else { // Abnormal case
    dbg << "\nReexecution mode (internal preservation call) which is wrong, it outputs kExecModeError\n" << endl;
    assert(0);
  }

  // In the normal case, it should not reach this point. It is an error.
  dbg << "\nkExecModeError\n" << endl;
  assert(0);
  return kExecModeError; 
}

/* RELEASE
#ifdef IMPL

// Only for reference, this section is not activated
int CD::Preserve(char* data_p, int data_l, enum preserveType prvTy, enum mediumLevel medLvl)
{

  abstraction_trans_func(); // FIXME

  AS* src_data = new AS(cd_self->rankID, SOURCE, data_p, data_l); //It is better to define AS src/dst_data here because we can know rankID.
  switch(prvTy){
    case COPY:
      // AS *DATA_dest = (AS *) manGetNewAllocation(DATA, mediumType);  
      // DATA is passed to figure out the length we need
      if(medLvl==DRAM) {
        char* DATA = DATA_MALLOC(data_l);
        AS* dst_data = new AS(cd_self->rankID, DESTINATION, DATA, data_l);
        if(!DATA){  // FIXME 
          fprintf(stderr, "ERROR DATA_MALLOC FAILED due to insufficient memory");
          return FAILURE;
        }
      } else {
        char* fp = GenerateNewFile(WorkingDir);
        AS* dst_data = new AS(cd_self->rankID, DESTINATION, medLvl, fp);
      }
      cd_entry *entry  = new cd_entry(RankIDfromRuntime, src_data, dst_data, enum preserveType prvTy, enum mediumLevel medLvl, NULL);
      entry->preserveEntry(); // we need to add entry to some sort of list. 
      break;

      ///////////////////////////////////////////////////////////////////////////////////////////
      //song
    case REFER:
      //find parent's (grandparent's or even earlier ancestor's) preserved entry
      //need some name matching
      //TODO: if parent also uses preserve via refer, is the entry in this child cd pointing to its parent's entry, or grandparent's entry??
      //Seonglyong: I think if parent's preserved entry is "via REFER", to let the corresponding entry point (refer to) the original entry 
      //whether it is grandparent's or parent of grandparent's entry will be reasonable by copying the parent's entry. More efficient than searching through a few hops.
      //TODO: need to determine to when to find the entry? in preservation or restoration?
      //    1) in preservation, find the refer entry in parent (or earlier ancestors), set the entry of this cd to be a pointer to that ancestor 
      //    (or copy all the entry information to this entry --> this is messier, and should not be called preserve via refer...)
      //    2) or we could do all those stuff in the restoration, when preserve, we only mark this entry to be preserve via reference, 
      //    and when some errors happen, we could follow the pointer to parents to find the data needed. 
      //Seonglyong: "In restoration" would be right. In terms of efficiency, we need to search the "preserved" entry via the tag in the restoration. 
      cd_handle* temp_parent = cd_parent;
      std::list<cd_entry*>::iterator it_entry = (temp_parent->cdInstance)->entryDirectory.begin();
      for (;it_entry!=(temp_parent->cdInstance)->entryDirectory.end();it_entry++){
        if (it_entry->entryNameMatching(EntryName)) { //TODO: need a parameter of EntryName
          //    not name, but tags...
          //
        }       
      }

      //TODO: should the entry of this cd be just a pointer to parent? 
      //or it will copy all the entry information from parent?
      //SL: Pointer to parent or one of ancestors. What is the entry information? maybe not the preserved data (which is not REFER type anymore) 
      //Whenever more than one hop is expected, we need to copy the REFER entry (which is kind of pointer to ancestor's entry or application data) 
      cd_entry *entry  = new cd_entry(RankIDfromRuntime, src_data, dst_data, prvTy, medLvl, NULL);
      entry->preserveEntry();
      break;
      ///////////////////////////////////////////////////////////////////////////////////////////

    case REGEN:
      customRegenFunc = getRegenFunc(); //Send request to Run-time to get an appropriate regenFunc
      cd_entry *entry  = new cd_entry(RankIDfromRuntime, src_data, dst_data, prvTy, medLvl, customRegenFunc);
      entry->preserveEntry();
      break;
    default:
      fprintf(stderr, "ERROR CD::Preserve: Unknown restoration type: %d\n", prvTy);
      return FAILURE;
  }

  return CDErrT::kOK;

}

#endif
*/




/*  CD::Restore()
 *  (1) Copy preserved data to application data. It calls restoreEntry() at each node of entryDirectory list.
 *
 *  (2) Do something for process state
 *
 *  (3) Logged data for recovery would be just replayed when reexecuted. We need to do something for this.
 */

//Jinsuk: Side question: how do we know if the recovery was successful or not? 
//It seems like this is very important topic, we could think the recovery was successful, 
//but we still can miss, or restore with wrong data and the re-execute, 
//for such cases, do we have a good detection mechanisms?
// Jinsuk: 02092014 are we going to leave this function? 
//It seems like this may not be required. Or for optimization, we can do this. 
//Bulk restoration might be more efficient than fine grained restoration for each block.
CDErrT CD::Restore()
{


  // this code section is for restoring all the cd entries at once. 
  // Now this is defunct. 

  /*  for( std::list<CDEntry*>::iterator it = entry_directory_.begin(), it_end = entry_directory_.end(); it != it_end ; ++it)
      {
      (*it)->Restore();
      } */

  // what we need to do here is just reset the iterator to the beginning of the list.
//  dbg<<"CD::Restore()"<<endl;
  iterator_entry_ = entry_directory_.begin();
//  dbg<<"entry dir size : "<<entry_directory_.size()<<", entry dir map size : "<<entry_directory_map_.size()<<endl;
//  dbg<<"dst addr : "<<iterator_entry_->dst_data_.address_data()<<", size: "<<iterator_entry_->dst_data_.len()<< endl<<endl;

//  assert(iterator_entry_->dst_data_.address_data() != 0);

  //TODO currently we only have one iterator. This should be maintined to figure out the order. 
  // In case we need to find reference name quickly we will maintain seperate structure such as binary search tree and each item will have CDEntry *.

  //Ready for long jump? Where should we do that. here ? for now?

  //GONG
  begin_ = false;

  return CDErrT::kOK;
}



/*  CD::detect()
 *  (1) Call all the user-defined error checking functions.
 *    Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *  (2) 
 *
 *  (3)
 *
 */
CDErrT CD::Detect()
{

  return CDErrT::kOK;
}

void CD::Recover(void)
{ 
  recoverObj_->Recover(this); 
} 


CDErrT CD::Assert(bool test)
{

  if(test == false) {
    // SZ
    // FIXME: need to create a function to change:
    //        cd_exec_mode_ and stop all children 
    //        also change CommLog::comm_log_mode_

    //Restore the data  (for now just do only this no other options for recovery)
    recoverObj_->Recover(this); 
  }

  return CDErrT::kOK;
}

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
  // SZ: FIXME: this is not safe because programmers may have their own RecoverObj
  //            we need to change the cd_exec_mode_ and comm_log_mode_ outside this function
  cd_exec_mode_ = kReexecution; 

#ifdef comm_log
  // SZ
  //// change the comm_log_mode_ into CommLog class
  //comm_log_mode_ = kReplayLog;  
  if (cd_type_ == kRelaxed)
    comm_log_ptr_->ReInit();
  
  //SZ: reset to child_seq_id_ = 0 
  PRINT_DEBUG("Reset child_seq_id_ to 0 at the point of re-execution\n");
  child_seq_id_ = 0;

  //GONG
  if(libc_log_ptr_!=NULL){
    libc_log_ptr_->ReInit();
    PRINT_DEBUG("reset log_table_reexec_pos_\n");
    //  libc_log_ptr_->Print();
  }
#endif

  //TODO We need to make sure that all children has stopped before re-executing this CD.
  Stop();

  cout << "node id : "<< task_in_color() << endl;
  //TODO We need to consider collective re-start. 
  if(ctxt_prv_mode_ == kExcludeStack) {
    printf("longjmp\n");
    
    dbg << "\n\nlongjmp \t" << jmp_buffer_ << " at level " << level()<< endl; //dbgBreak();
    longjmp(jmp_buffer_, jmp_val_);
  }
  else if (ctxt_prv_mode_ == kIncludeStack) {
    printf("setcontext\n");
    setcontext(&ctxt_); 
  }

  return CDErrT::kOK;
}

CDErrT HeadCD::Reexecute(void)
{


  cd_exec_mode_ = kReexecution; 

  //TODO We need to make sure that all children has stopped before re-executing this CD.
  Stop();

  cout << "node id : "<< task_in_color() << endl;
  //TODO We need to consider collective re-start. 
  if(ctxt_prv_mode_ == kExcludeStack) {
    printf("longjmp\n");
    
    dbg << "\n\nlongjmp \t" << jmp_buffer_ << " at level " << level()<< endl; //dbgBreak();
    longjmp(jmp_buffer_, jmp_val_);
  }
  else if (ctxt_prv_mode_ == kIncludeStack) {
    PRINT_DEBUG("setcontext\n");
    setcontext(&ctxt_); 
  }

  return CDErrT::kOK;

//  return (dynamic_cast<CD *>(this)->Reexecute());
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

  // Maybe blocking Recv here? or MPI_Wait()?
  
  return CDErrT::kOK;
}

CDErrT CD::Resume(void)
{
  return CDErrT::kOK;
}

CDErrT CD::AddChild(CDHandle* cd_child) 
{ 
//  dbg<<"sth wrong"<<endl; 
//  dbgBreak(); 
  // Do nothing?
  if(cd_child->IsHead()) {
    //Send it to Head
  }

  return CDErrT::kOK;  
}

CDErrT CD::RemoveChild(CDHandle* cd_child) 
{
//  dbg<<"sth wrong"<<endl; 
//  dbgBreak(); 
  // Do nothing?
  return CDErrT::kOK;
}

//FIXME 11112014
void RegisterMeToParentHeadCD(int taskID)
{
  
//  MPI_Put(&taskID, 1, MPI_INTEGER, cd_id().node_id().head(), target_disp, target_count, MPI_INTEGER, &win);

}

HeadCD::HeadCD()
{
  error_occurred = false;
}

HeadCD::HeadCD( CDHandle* cd_parent, 
                    const char* name, 
                    const CDID& cd_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector)
  : CD(cd_parent, name, cd_id, cd_type, sys_bit_vector)
{
  RegisterMeToParentHeadCD(cd_id.task_in_color());
  error_occurred = false;
//  cd_parent_ = cd_parent;
}


//CD::CDInternalErrT CD::CreateInternalMemory(CD *cd_ptr, const CDID& new_cd_id)
//{
//  int task_count = new_cd_id.task_count();
//  dbg << "in CD::Create Internal Memory"<<endl;
//  if(task_count > 1) {
//    for(int i=0; i<task_count; ++i) {
//    
//      MPI_Win_create(NULL, 0, 1,
//                     MPI_INFO_NULL, new_cd_id.color(), &((cd_ptr->mailbox_)[i]));
//  
//    }
//  }
//  else {
//    dbg << "CD::mpi win create for "<< task_count << " mailboxes"<<endl;
//    MPI_Win_create(NULL, 0, 1,
//                   MPI_INFO_NULL, new_cd_id.color(), cd_ptr->mailbox_);
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
//  dbg << "in CD::Destroy Internal Memory"<<endl;
//  int task_count = cd_id_.task_count();
//  if(task_count > 1) {
//    dbg << "mpi win free for "<< task_count << " mailboxes"<<endl;
//    for(int i=0; i<task_count; ++i) {
//      dbg << i << endl;
//      MPI_Win_free(&(cd_ptr->mailbox_[i]));
//    }
//  }
//  else {
//    dbg << "mpi win free for one mailbox"<<endl;
//    MPI_Win_free(cd_ptr->mailbox_);
//  }
//
//  return CD::CDInternalErrT::kOK;
//}

//CD::CDInternalErrT HeadCD::CreateInternalMemory(HeadCD *cd_ptr, const CDID& new_cd_id)
//{
//  dbg << "HeadCD create internal memory " << endl;
//  int task_count = new_cd_id.task_count();
//#if _MPI_VER
////  if(new_cd_id.color() == MPI_COMM_WORLD) {
////    dbg << "\n\nthis is root! " << task_count << "\n\n"<<endl;
////    MPI_Alloc_mem(sizeof(CDFlagT)*task_count, 
////                  MPI_INFO_NULL, &event_flag_);
////    mailbox_ = new CDMailBoxT[task_count];
////    for(int i=0; i<task_count; ++i) {
////      MPI_Win_create(&((event_flag_)[i]), 1, sizeof(CDFlagT),
////                     MPI_INFO_NULL, MPI_COMM_WORLD, &((mailbox_)[i]));
////    }
////
////
////    exit(-1);
////    return CD::CDInternalErrT::kOK;
////  }
//
//  MPI_Alloc_mem(sizeof(CDFlagT)*task_count, 
//                MPI_INFO_NULL, &(cd_ptr->event_flag_));
//
//  if(task_count > 1) {
//
//    dbg << "HeadCD mpi win create for "<< task_count << " mailboxes"<<endl;
//    mailbox_ = new CDMailBoxT[task_count];
//    for(int i=0; i<task_count; ++i) {
//      MPI_Win_create(&((cd_ptr->event_flag_)[i]), 1, sizeof(CDFlagT),
//                     MPI_INFO_NULL, new_cd_id.color(), &((cd_ptr->mailbox_)[i]));
//    }
//
////      MPI_Win_create(event_flag_, task_count, sizeof(CDFlagT),
////                     MPI_INFO_NULL, new_cd_id.color(), mailbox_);
//
//  }
//  else {
//    dbg << "HeadCD mpi win create for "<< task_count << " mailboxes"<<endl;
//    MPI_Win_create(cd_ptr->event_flag_, task_count, sizeof(CDFlagT),
//                   MPI_INFO_NULL, new_cd_id.color(), cd_ptr->mailbox_);
//    
//  }
//
////  MPI_Win_allocate(task_count*sizeof(CDFlagT), sizeof(CDFlagT), MPI_INFO_NULL, new_cd_id.color(), &event_flag_, &mailbox_);
//  //dbgBreak();
//#endif
//  return CD::CDInternalErrT::kOK;
//}
//
//CD::CDInternalErrT HeadCD::DestroyInternalMemory(HeadCD *cd_ptr)
//{
//#if _MPI_VER
//  dbg << "in HeadCD::Destroy"<<endl;
//  int task_count = cd_id_.task_count();
//  if(task_count > 1) {
//    dbg << "HeadCD mpi win free for "<< task_count << " mailboxes"<<endl;
//    for(int i=0; i<task_count; ++i) {
//      dbg << i << endl;
//      MPI_Win_free(&(cd_ptr->mailbox_[i]));
//    }
//  }
//  else {
//    dbg << "HeadCD mpi win free for one mailbox"<<endl;
//    MPI_Win_free(cd_ptr->mailbox_);
//  }
//  MPI_Free_mem(cd_ptr->event_flag_);
//
//
//
////  MPI_Win_free(&mailbox_);
//#endif
//  return CD::CDInternalErrT::kOK;
//}






HeadCD::~HeadCD()
{
//  DestroyInternalMemory();
//    MPI_Free_mem(event_flag_);
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
//    MPI_Win_create(&((ptr_headcd->family_event_flag_)[i]), 1, sizeof(CDFlagT),
//                   MPI_INFO_NULL, new_cd_id.color(), &((ptr_headcd->family_mailbox_)[i]));
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


CDErrT HeadCD::Destroy(void)
{
  CDErrT err=CDErrT::kOK;

  cout << "HeadCD::Destroy\n" << endl;
#if _MPI_VER
#if _KL
  if(GetCDID().node_id().size() > 1) {
    MPI_Free_mem(event_flag_);
  }
#endif
#endif

  InternalDestroy();

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
    dbg << "It is not desirable to let the same task be head twice!" << endl;
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
//  MPI_Send(, send_count, MPI_INTEGER, recv_buf, recv_count, MPI_INTEGER, cd_id().node_id().head(), cd_id().color());
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
//  MPI_Status status;
//  for(int i=1; i<cd_id().sibling_count(); ++i) {
//    MPI_Recv(&(recv_buf[i]), 1, MPI_INTEGER, MPI_ANY_SOURCE, 0, cd_id().color(), &status);
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
  try {
    cout << "InternalGetEntry : " << entry_name;
    cout << " - " << tag2str[entry_name] << endl;
    
    auto it = entry_directory_map_.find(entry_name);
    auto jt = remote_entry_directory_map_.find(entry_name);
    if(it == entry_directory_map_.end() && jt == remote_entry_directory_map_.end()) {
      dbg<<"[InternalGetEntry] There is no entry for reference of "<< tag2str[entry_name]
               <<" at CD level " << GetCDID().level() << endl;
      //dbgBreak();
      return NULL;
    }
    else if(it != entry_directory_map_.end()) {
      // Found entry at local directory
      CDEntry* cd_entry = entry_directory_map_.find(entry_name)->second;
      
      dbg<<"[InternalGetEntry] ref_name: " <<entry_directory_map_[entry_name]->dst_data_.address_data()
               << ", address: " <<entry_directory_map_[entry_name]->dst_data_.address_data()<< endl;

//      if(cd_entry->isViaReference())
//        return NULL;
//      else
        return cd_entry;
    }
    else if(jt != remote_entry_directory_map_.end()) {
      // Found entry at local directory, but it is a buffer to send remotely.
      CDEntry* cd_entry = remote_entry_directory_map_.find(entry_name)->second;
      
      dbg<<"[InternalGetEntry] ref_name: " <<remote_entry_directory_map_[entry_name]->dst_data_.address_data()
               << ", address: " <<remote_entry_directory_map_[entry_name]->dst_data_.address_data()<< endl;
//      if(cd_entry->isViaReference())
//        return NULL;
//      else
        return cd_entry;
    }
    else {
      dbg << "ERROR: there is the same name of entry in entry_directory_map and remote_entry_directory_map.\n"<< endl;
      assert(0);
    }
  }
  catch (const std::out_of_range &oor) {
    std::cerr << "Out of Range error: " << oor.what() << '\n';
    return 0;
  }
}


CDFlagT *CD::event_flag(void)
{
  return event_flag_;
}
CDFlagT *HeadCD::event_flag(void)
{
  return event_flag_;
}


void CD::DeleteEntryDirectory(void)
{
//  dbg<<"Delete Entry In"<<endl; dbgBreak();
  for(std::list<CDEntry>::iterator it = entry_directory_.begin();
      it != entry_directory_.end(); ) {


/* Serializer test
    uint32_t entry_len=0;
    void *ser_entry = it->Serialize(entry_len);

    dbg << "ser entry : "<< ser_entry << endl;
    CDEntry new_entry;
    dbg << "\n\n--------------------------------\n"<<endl;
    new_entry.Deserialize(ser_entry);
    dbg << "before!!!! " << (it->src_data_).address_data()<<endl<<endl;
    dbg << "\n\n\nafter!!!! " << new_entry.src_data_.address_data()<<endl;

    dbg << "before!!!! " << it->name() <<endl<<endl;
    dbg << "\n\n\nafter!!!! " << new_entry.name()<<endl;
    dbg << (*it == new_entry) << endl;
*/


//    uint32_t data_handle_len=0;
//    dbg << "=========== Check Ser ==============" << endl;
//    dbg <<"[Before packed] :\t"<< it->dst_data_.node_id_ << endl << endl;
//    void *ser_data_handle = (it->dst_data_).Serialize(data_handle_len);
//    DataHandle new_data_handle;
//    new_data_handle.Deserialize(ser_data_handle);
//
//    dbg <<"\n\n\noriginal : "<<(it->dst_data_).file_name() << endl;
//    dbg <<"[After unpacked] :\t"<<new_data_handle.node_id_ << endl << endl;
//    dbgBreak();

    it->Delete();
    entry_directory_map_.erase(it->name_tag());
    remote_entry_directory_map_.erase(it->name_tag());
    entry_directory_.erase(it++);
  }

//  for(std::map<std::string, CDEntry*>::iterator it = entry_directory_map_.begin();
//      it != entry_directory_map_.end(); ++it) {
//    //entry_directory_map_.erase(it);
//  }
//  dbg<<"Delete Entry Out"<<endl; dbgBreak();
}










// ------------------------- Preemption ---------------------------------------------------

CDErrT CD::CheckMailBox(void)
{

  CD::CDInternalErrT ret=kOK;
  int event_count = *pendingFlag_;

  // Reset handled event counter
  handled_event_count = 0;
  CDHandle *curr_cdh = CDPath::GetCurrentCD();
  dbg << "\n\n=================== Check Mail Box Start [Level " << level() <<"] , # of pending events : " << event_count << " ========================" << endl;

  if(event_count > 0) {

//    if( task_size() > 1) ret = InternalCheckMailBox();
    while( curr_cdh != NULL ) { // Terminate when it reaches upto root

      if( task_size() > 1) {
        ret = InternalCheckMailBox();
      }
      else {
//        dbg << "[ReadMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
      }
      // If current CD is Root CD and GetParentCD is called, it returns NULL
//      dbg << "ReadMailBox " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
//          << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;
//      dbg << "\nCheckMailBox " << myTaskID << ", # of events : " << event_count << ", Is it Head? "<< IsHead() <<endl;
//      dbg << "An event is detected" << endl;
//      dbg << "Task : " << myTaskID << ", Level : " << level() <<" CD Name : " << GetCDName() << ", Node ID: " << GetNodeID() <<endl;
    	curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
    } 

  }
  else {
    dbg << "No event is detected" << endl;
  }
  
  dbg << "\n-------------------------------------------------------------------" << endl;
  int temp = handled_event_count;
  this->InvokeErrorHandler();
  dbg << "\nCheck MailBox is done. handled_event_count : " << temp << " -> " << handled_event_count;
  (*pendingFlag_) -= handled_event_count;
  dbg << ", pending counter : " << *pendingFlag_ << endl;
  dbg << "-------------------------------------------------------------------\n" << endl;
  dbg << "=================== Check Mail Box Done  [Level " << level() <<"] =====================================================\n\n" << endl;

  return static_cast<CDErrT>(ret);
}

int requested_event_count = 0;
int cd::handled_event_count = 0;

CD::CDInternalErrT CD::InternalCheckMailBox(void)
{
  CD::CDInternalErrT ret=kOK;
  requested_event_count = *pendingFlag_;
  handled_event_count = 0;


  this->ReadMailBox();

//  CDHandle *curr_cdh = CDPath::GetCurrentCD();

//  ReadMailBoxFromRemote();

//	while( curr_cdh != NULL ) {
//    // If this task is head at the current CD level, read mailbox
//    if( curr_cdh->IsHead() ) {
//      ret = curr_cdh->ptr_cd()->ReadMailBox();
//    }
  
//    curr_cdh->ptr_cd()->ReadMailBox();

//    dbg << "[Searching for CD Level where current task is head] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
    // Termination condition
    // It finished every requests, and does not need to look at mailbox any more
//    if(requested_event_count == handled_event_count) {
//      dbg<<"I finished handling events!!"<<endl;
////      break;
//    }
//    else {
//      // If current CD is Root CD and GetParentCD is called, it returns NULL
//		  curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
//    }


//    curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());

//	} 

/*
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
*/
  return ret;
}

CD::CDInternalErrT CD::ReadMailBox(void) 
{
  CDInternalErrT ret = kOK;
  assert(event_flag_);
  dbg << "pending counter : "<< *pendingFlag_ << endl;
  dbg << "\nTask : " << GetCDName() << " / " << GetNodeID() << "), Error #" << *event_flag_ << "\t";

  // Initialize the resolved flag
  CDFlagT event = *event_flag_;
  // FIXME : possible concurrent bug. it kind of localized event_flag_ and check local variable.
  // but if somebody set event_flag, it is not updated in this event check routine.
  CDEventHandleT resolved = HandleEvent(event);

  // Reset the event flag after handling the event
  switch(resolved) {
    case CDEventHandleT::kEventNone : 
      // no event
      break;
    case CDEventHandleT::kEventResolved :
      // There was an event, but resolved

      //(*pendingFlag_)--;
//      handled_event_count--;
      break;
    case CDEventHandleT::kEventPending :
      // There was an event, but could not resolved.
      // Escalation required
      break;
    default :
      break;
  }
  dbg << "\tResolved? : " << resolved << endl;

  return ret;
}

CD::CDInternalErrT HeadCD::ReadMailBox(void) 
{
  CDEventHandleT resolved = CDEventHandleT::kEventNone;
  CDInternalErrT ret = kOK;
  CDFlagT *event = event_flag_;
  assert(event_flag_);
  cout << "\n\n\n\n\n\n\nCHECK IT OUT\n\n\n\n\n\n\n" << endl; //getchar();
  for(int i=0; i<task_size(); i++) {

    
    dbg << "\nTask["<< i <<"] (" << GetCDName() << " / " << GetNodeID() << "), Error #" << event[i] << endl;;

    // Initialize the resolved flag
    resolved = kEventNone;
    resolved = HandleEvent(&(event[i]), i);

    // Reset the event flag after handling the event
    switch(resolved) {
      case CDEventHandleT::kEventNone : 
        // no event
        break;
      case CDEventHandleT::kEventResolved :
        // There was an event, but resolved

        //(*pendingFlag_)--;
//        handled_event_count--;
        break;
      case CDEventHandleT::kEventPending :
        // There was an event, but could not resolved.
        // Escalation required
        break;
      default :
        break;
    }
//    dbg << "\tResolved? : " << resolved << endl;
  }
//  MPI_Win_fence(0, mailbox_);
  return ret;
}




#if 1
/*

CD::CDInternalErrT CD::ReadMailBoxFromRemote(void)
{
  CD::CDInternalErrT ret = kOK;
  CDHandle *curr_cdh = CDPath::GetCurrentCD();

	while( task_size()==1 ) {
    if(curr_cdh == CDPath::GetRootCD()) {
      dbg << "[ReadMailBoxFromRemote] there is a single task in the root CD" << endl;
      assert(0);
    }
    dbg << "[ReadMailBoxFromRemote|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
    // If current CD is Root CD and GetParentCD is called, it returns NULL
		curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
	} 
  dbg << "ReadMailBoxFromRemote " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
      << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;
  CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_; 

  CDFlagT myEvent = 0;


  if(curr_cdh->IsHead() == false) {
    // This is not head, so needs to bring its mailbox from head
    // Check leaf's status first

//    MPI_Win_start(task_group, 0, mailbox);

    MPI_Win_fence(0, mailbox);
    MPI_Get(&myEvent, 1, MPI_INT, curr_cdh->node_id_.head(), curr_cdh->node_id_.task_in_color(), 1, MPI_INT, mailbox);
    dbg << "Complete event # " << myEvent << endl;
    MPI_Win_fence(0, mailbox);
    
//    MPI_Win_complete(mailbox);


  }
  else {
    // This task is head
//    MPI_Win_post(task_group, 0, mailbox);
    MPI_Win_fence(0, mailbox);
    dbg << "Epoch for getting mailbox for each task " << curr_cdh->IsHead() << ", node id: " << curr_cdh->node_id_ << endl;
    myEvent = (curr_cdh->ptr_cd()->event_flag())[curr_cdh->node_id_.task_in_color()];
    MPI_Win_fence(0, mailbox);
//    MPI_Win_wait(mailbox);
  }

  dbg << "Task "<< myTaskID <<" (" << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << "), Error #" << myEvent << "\t";
  CDEventHandleT ret_handled = HandleEvent(&myEvent);

  dbg << "\tResolved? : " << ret_handled << endl;

  return ret;
}



*/


#else
CD::CDInternalErrT CD::ReadMailBoxFromRemote(void)
{
  CD::CDInternalErrT ret = kOK;
  CDHandle *curr_cdh = CDPath::GetCurrentCD();

	while( curr_cdh->node_id_.size()==1 ) {
    if(curr_cdh == CDPath::GetRootCD()) {
      dbg << "[ReadMailBoxFromRemote] there is a single task in the root CD" << endl;
      assert(0);
    }
    dbg << "[ReadMailBoxFromRemote|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
    // If current CD is Root CD and GetParentCD is called, it returns NULL
		curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
	} 
  dbg << "ReadMailBoxFromRemote " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
      << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;
  CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_; 

//  FIXME 
//  MPI_Group task_group;
//  MPI_Comm_group(curr_cdh->node_id_.color(), &task_group);

  CDFlagT myEvent = 0;
  if(curr_cdh->IsHead() == false) {
    // This is not head, so needs to bring its mailbox from head
    // Check leaf's status first

//    MPI_Win_start(task_group, 0, mailbox);

    MPI_Win_fence(0, mailbox);
    MPI_Get(&myEvent, 1, MPI_INT, curr_cdh->node_id_.head(), curr_cdh->node_id_.task_in_color(), 1, MPI_INT, mailbox);
    dbg << "Complete event # " << myEvent << endl;
    MPI_Win_fence(0, mailbox);
    
//    MPI_Win_complete(mailbox);

//    dbg << "Task "<< myTaskID <<" (" << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ 
//        << ") got an error #" << myEvent << " (";
//    CDEventHandleT ret_handled = HandleEvent(&myEvent);
//
//    switch( ret_handled ) {
//      case CDEventHandleT::kEventNone : 
//        // no event
//        break;
//      case CDEventHandleT::kEventResolved :
//        // There was an event, but resolved
//
//        //(*pendingFlag_)--;
//        handled_event_count--;
//        break;
//      case CDEventHandleT::kEventPending :
//        // There was an event, but could not resolved.
//        // Escalation required
//        break;
//      default :
//        break;
//    }
//    dbg << "),\t\tResolved? : " << ret_handled << endl;

  }
  else {
    // This task is head
//    MPI_Win_post(task_group, 0, mailbox);
    MPI_Win_fence(0, mailbox);
    dbg << "Epoch for getting mailbox for each task " << curr_cdh->IsHead() << ", node id: " << curr_cdh->node_id_ << endl;
    myEvent = (curr_cdh->ptr_cd()->event_flag())[curr_cdh->node_id_.task_in_color()];
    MPI_Win_fence(0, mailbox);
//    MPI_Win_wait(mailbox);
  }

  dbg << "Task "<< myTaskID <<" (" << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << "), Error #" << myEvent << "\t";
  CDEventHandleT ret_handled = HandleEvent(&myEvent);

  switch( ret_handled ) {
    case CDEventHandleT::kEventNone : 
      // no event
      break;
    case CDEventHandleT::kEventResolved :
      // There was an event, but resolved

      //(*pendingFlag_)--;
//      handled_event_count--;
      break;
    case CDEventHandleT::kEventPending :
      // There was an event, but could not resolved.
      // Escalation required
      break;
    default :
      break;
  }
  dbg << "\tResolved? : " << ret_handled << endl;

  return ret;
}



#endif

#if 0
CDEventHandleT CD::HandleEvent(CDFlagT *p_event, int idx)
{
  CDEventHandleT ret = CDEventHandleT::kEventResolved;
//  int handle_ret = 0;
  CDFlagT event = *p_event;
  if( CHECK_NO_EVENT(event) ) {
    dbg << "CD Event kNoEvent\t\t\t";
  }
  else {
    if( CHECK_EVENT(event, kErrorOccurred) ) {

      HandleErrorOccurred();

      handled_event_count++;
    }

    if( CHECK_EVENT(event, kEntrySearch) ) {
      if(IsHead()) {
        cout << "[event: entry search] This is event for head CD.";
//        assert(0);
      }
#if _FIX
      ENTRY_TAG_T recvBuf[2] = {0,0};
      bool entry_searched = false; 
      if( IsHead() ) {
        entry_searched = HandleEntrySearch(idx, recvBuf);
      }
      else {
        
//        ptr_head_cd()->HandleEntrySearch(this, idx, recvBuf);
      }
      if(entry_searched == false) {
        CDHandle *parent = CDPath::GetParentCD();
        CDEventT entry_search = kEntrySearch;
        parent->SetMailBox(entry_search);
        MPI_Isend(recvBuf, 
                  2, 
                  MPI_UNSIGNED_LONG_LONG,
                  parent->node_id().head(), 
                  parent->ptr_cd()->GetCDID().GenMsgTag(MSG_TAG_ENTRY_TAG), 
                  parent->node_id().color(),
                  &(parent->ptr_cd()->entry_request_req_[recvBuf[0]].req_));
      }
#endif
      // Search Entry
      handled_event_count++;
    }

    if( CHECK_EVENT(event, kEntrySend) ) {

#if _FIX
      if(!IsHead()) {
        cout << "[event: entry send] This is event for non-head CD.";
//        assert(0);
      }
      HandleEntrySend();
#endif
      dbg << "CD Event kEntrySend\t\t\t";
      handled_event_count++;
      
    }

    if( CHECK_EVENT(event, kAllPause) ) {
      cout << "What?? " << GetNodeID() << endl;
      HandleAllPause();

      handled_event_count++;

    }

    if( CHECK_EVENT(event, kAllResume) ) {

      HandleAllResume();

      handled_event_count++;

    }

    if( CHECK_EVENT(event, kAllReexecute) ) {

      HandleAllReexecute();

      handled_event_count++;

    }
    
//    (*pendingFlag_) -= handled_event_count;
  } 
  return ret;
}

#else

CDEventHandleT CD::HandleEvent(CDFlagT &event)
{
  CDEventHandleT ret = CDEventHandleT::kEventResolved;
//  int handle_ret = 0;
  if( CHECK_NO_EVENT(event) ) {
    dbg << "CD Event kNoEvent\t\t\t";
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

#if _FIX
      if(!IsHead()) {
        cout << "[event: entry send] This is event for non-head CD.";
//        assert(0);
      }
      HandleEntrySend();
      cd_event_.push_back(new HandleEntrySend(this));
#endif
      dbg << "CD Event kEntrySend\t\t\t";

//      handled_event_count++;
      
    }

    if( CHECK_EVENT(event, kAllPause) ) {
      cout << "What?? kAllPause" << GetNodeID() << endl;
//      HandleAllPause();
      cd_event_.push_back(new HandleAllPause(this));

//      handled_event_count++;

    }


    if( CHECK_EVENT(event, kAllReexecute) ) {

      cd_event_.push_back(new HandleAllReexecute(this));

//      handled_event_count++;

    }

//    (*pendingFlag_) -= handled_event_count;
  } 
  return ret;
}


CDEventHandleT HeadCD::HandleEvent(CDFlagT *p_event, int idx)
{
  dbg << "CDEventHandleT HeadCD::HandleEvent(CDFlagT *p_event, int idx)" << endl;
  CDEventHandleT ret = CDEventHandleT::kEventResolved;
//  int handle_ret = 0;
  CDFlagT event = *p_event;
  if( CHECK_NO_EVENT(event) ) {
    dbg << "CD Event kNoEvent\t\t\t";
  }
  else {
    if( CHECK_EVENT(event, kErrorOccurred) ) {

//      HandleErrorOccurred(idx);
      if(error_occurred == true) {
        dbg << "HeadCD Event kErrorOccurred " << idx;
        event_flag_[idx] &= ~kErrorOccurred;
        handled_event_count++;
      }
      else {
        dbg << "?? HeadCD Event kErrorOccurred " << idx;
        headcd_event_.push_back(new HandleErrorOccurred(this, idx));
        handled_event_count++;
        error_occurred = true;
      }
//      handled_event_count++;
    }

    if( CHECK_EVENT(event, kEntrySearch) ) {

#if _FIX
//      ENTRY_TAG_T recvBuf[2] = {0,0};
//      bool entry_searched = HandleEntrySearch(idx, recvBuf);
      headcd_event_.push_back(new HandleEntrySearch(this, idx));
////FIXME_KL
//      if(entry_searched == false) {
//        CDHandle *parent = CDPath::GetParentCD();
//        CDEventT entry_search = kEntrySearch;
//        parent->SetMailBox(entry_search);
//        MPI_Isend(recvBuf, 
//                  2, 
//                  MPI_UNSIGNED_LONG_LONG,
//                  parent->node_id().head(), 
//                  parent->ptr_cd()->GetCDID().GenMsgTag(MSG_TAG_ENTRY_TAG), 
//                  parent->node_id().color(),
//                  &(parent->ptr_cd()->entry_request_req_[recvBuf[0]].req_));
//      }
//      else {
//        // Search Entry
////        handled_event_count++;
//      }
#endif
    }

    if( CHECK_EVENT(event, kAllResume) ) {

      headcd_event_.push_back(new HandleAllResume(this));

//      handled_event_count++;

    }
// -------------------------------------------------------------------------
// Events requested from non-head to head.
// If current task is head, it means this events is requested from head to head, 
// which does not make sense. (perhaps something like loopback.)
// Currently, I think head can just ignore for checking kEntrySend/kAllPause/kAllReexecute, 
// but I will leave if for now.
    if( CHECK_EVENT(event, kEntrySend) || CHECK_EVENT(event, kAllPause) ||  CHECK_EVENT(event, kAllReexecute) ) {
      assert(0);
    }

#if 0 
    if( CHECK_EVENT(event, kEntrySend) ) {

#if _FIX
      if(!IsHead()) {
        cout << "[event: entry send] This is event for non-head CD.";
//        assert(0);
      }
//      HandleEntrySend();
      cd_event_.push_back(new HandleEntrySend(this));
#endif
      dbg << "CD Event kEntrySend\t\t\t";
//      handled_event_count++;
      
    }

    if( CHECK_EVENT(event, kAllPause) ) {
      cout << "What?? " << GetNodeID() << endl;
      HandleAllPause();

//      handled_event_count++;

    }

    if( CHECK_EVENT(event, kAllReexecute) ) {

      HandleAllReexecute();

//      handled_event_count++;

    }
#endif

    if(idx == task_in_color()) {
      dbg << "Handled : " << handled_event_count << endl;
      dbg << "currnt count : " << *pendingFlag_ << endl;
      
//      (*pendingFlag_) -= handled_event_count;
    }
  } 
  return ret;
}

#endif








#if 1
CDErrT CD::SetMailBox(CDEventT &event)
{
  dbg << "\n\n=================== Set Mail Box Start ==========================\n" << endl;
  dbg << "\nSetMailBox " << myTaskID << ", event : " << event << ", Is it Head? "<< IsHead() <<endl;
  CD::CDInternalErrT ret=kOK;
  int val = 1;

  CDHandle *curr_cdh = CDPath::GetCurrentCD();
  int head_id = head();
  if(event == CDEventT::kNoEvent) {

    dbg << "\n=================== Set Mail Box Done ==========================\n" << endl;

    return static_cast<CDErrT>(ret);  
  }
  if(task_size() == 1) {
    // This is the leaf that has just single task in a CD
    dbg << "KL : [SetMailBox Leaf and have 1 task] size is " << task_size() << ". Check mailbox at the upper level." << endl;

  	while( curr_cdh->node_id_.size()==1 ) {
      if(curr_cdh == CDPath::GetRootCD()) {
        dbg << "[SetMailBox] there is a single task in the root CD" << endl;
        assert(0);
      }
      dbg << "[SetMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
      // If current CD is Root CD and GetParentCD is called, it returns NULL
  		curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
  	} 
    dbg << "[SetMailBox] " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
        << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;

    head_id = curr_cdh->node_id().head();
  }




  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, curr_cdh->ptr_cd()->pendingWindow_);
  if(event != CDEventT::kNoEvent) {
    dbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
    // Increment pending request count at the target task (requestee)
    MPI_Accumulate(&val, 1, MPI_INT, 
                   head_id, 0, 1, MPI_INT, 
                   MPI_SUM, curr_cdh->ptr_cd()->pendingWindow_);
    dbg << "MPI Accumulate done for "<< head_id <<"(head)"<< endl; //getchar();
  }
  MPI_Win_unlock(head_id, curr_cdh->ptr_cd()->pendingWindow_);

  CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_;



  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, head_id, 0, mailbox);

  // Inform the type of event to be requested
  dbg << "Set event start" << endl; //getchar();
  if(event != CDEventT::kNoEvent) {
    MPI_Accumulate(&event, 1, MPI_INT, 
                   head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, 
                   MPI_BOR, mailbox);
  }
  dbg << "MPI Accumulate done for "<< head_id <<"(head)"<< endl; //getchar();

  MPI_Win_unlock(head_id, mailbox);
  dbg << "\n=================== Set Mail Box Done ==========================\n" << endl;

  return static_cast<CDErrT>(ret);
}


CDErrT HeadCD::SetMailBox(CDEventT &event, int task_id)
{
  dbg << "\n\n=================== Set Mail Box (From Head) Start ==========================\n" << endl;
  dbg << "\nSetMailBox " << myTaskID << ", event : " << event << ", Is it Head? "<< IsHead() <<endl;
 
  if(event == CDEventT::kErrorOccurred || event == CDEventT::kEntrySearch) {
    cerr << "[HeadCD::SetMailBox(event, task_id)] Error, the event argument is something wrong. event: " << event << endl;
    assert(0);
  }
  if(task_size() == 1) {
    cerr << "[HeadCD::SetMailBox(event, task_id)] Error, the # of task should be greater than 1 at this routine. # of tasks in current CD: " << task_size() << endl;
    assert(0);
  }

  CD::CDInternalErrT ret=kOK;
  int val = 1;
  if(event == CDEventT::kNoEvent) {
    dbg << "\n=================== Set Mail Box Done ==========================\n" << endl;

    return static_cast<CDErrT>(ret); 
  }


  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_id, 0, pendingWindow_);
  if(event != CDEventT::kNoEvent) {
    dbg << "Set CD Event " << event <<". Level : " << level() <<" CD Name : " << GetCDName()<< endl;
    dbg << "Accumulate event at " << task_id << endl;
    // Increment pending request count at the target task (requestee)
//    MPI_Put(&val, 1, MPI_INT, 
//                   task_id, 0, 1, MPI_INT, 
//                   pendingWindow_);
    MPI_Accumulate(&val, 1, MPI_INT, 
                   task_id, 0, 1, MPI_INT, 
                   MPI_SUM, pendingWindow_);
  }
  dbg << "MPI_Accumulate done for " << task_id << endl; //getchar();
  MPI_Win_unlock(task_id, pendingWindow_);


  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, task_id, 0, mailbox_);

  // Inform the type of event to be requested
  dbg << "Set event start" << endl; //getchar();
  if(event != CDEventT::kNoEvent) {
    MPI_Accumulate(&event, 1, MPI_INT, 
                   task_id, 0, 1, MPI_INT, 
                   MPI_BOR, mailbox_);
  }
  dbg << "MPI_Accumulate done for " << task_id << endl; //getchar();

  MPI_Win_unlock(task_id, mailbox_);
  dbg << "\n=================== Set Mail Box Done ==========================\n" << endl;

  return static_cast<CDErrT>(ret);
}








#else

CDErrT CD::SetMailBox(CDEventT &event)
{
  dbg << "\n\n=================== Set Mail Box Start ==========================\n" << endl;
  dbg << "\nSetMailBox " << myTaskID << ", event : " << event << ", Is it Head? "<< IsHead() <<endl;
  CD::CDInternalErrT ret=kOK;
  int val = 1;

  if(task_size() > 1) {
    dbg << "KL : [SetMailBox] size is " << task_size() << ". Check mailbox at the upper level." << endl;
  
    MPI_Win_fence(0, pendingWindow_);
    if(event != CDEventT::kNoEvent) {
//      MPI_Win_fence(0, pendingWindow_);

      dbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
      // Increment pending request count at the target task (requestee)
      MPI_Accumulate(&val, 1, MPI_INT, 
                     head(), 0, 1, MPI_INT, 
                     MPI_SUM, pendingWindow_);
      dbg << "MPI Accumulate done" << endl; getchar();

//      MPI_Win_fence(0, pendingWindow_);
    }
    MPI_Win_fence(0, pendingWindow_);

    CDMailBoxT &mailbox = mailbox_;  

    MPI_Win_fence(0, mailbox);
    if(event != CDEventT::kNoEvent) {
      // Inform the type of event to be requested
      MPI_Accumulate(&event, 1, MPI_INT, 
                     head(), task_in_color(), 1, MPI_INT, 
                     MPI_BOR, mailbox);

//    MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
      dbg << "MPI_Put done" << endl; getchar();
    }
  
//    switch(event) {
//      case CDEventT::kNoEvent :
//        dbg << "Set CD Event kNoEvent" << endl;
//        break;
//      case CDEventT::kErrorOccurred :
//        // Inform the type of event to be requested
//        MPI_Accumulate(&event, 1, MPI_INT, node_id_.head(), node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
////        MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//        dbg << "MPI_Put done" << endl; getchar();
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
  
    MPI_Win_fence(0, mailbox);
  
  }
  else {
    // This is the leaf that has just single task in a CD
    dbg << "KL : [SetMailBox Leaf and have 1 task] size is " << task_size() << ". Check mailbox at the upper level." << endl;
    CDHandle *curr_cdh = CDPath::GetCurrentCD();

  	while( curr_cdh->node_id_.size()==1 ) {
      if(curr_cdh == CDPath::GetRootCD()) {
        dbg << "[SetMailBox] there is a single task in the root CD" << endl;
        assert(0);
      }
      dbg << "[SetMailBox|Searching for CD Level having non-single task] Current Level : " << curr_cdh->ptr_cd()->GetCDID().level() << endl;
      // If current CD is Root CD and GetParentCD is called, it returns NULL
  		curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
  	} 
    dbg << "[SetMailBox] " << curr_cdh->ptr_cd()->GetCDName() << " / " << curr_cdh->node_id_ << " at level : " << curr_cdh->ptr_cd()->GetCDID().level()
        << "\nOriginal current CD " << GetCDName() << " / " << GetNodeID() << " at level : " << level() << "\n" << endl;


    int parent_head_id = curr_cdh->node_id().head();

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, parent_head_id, 0, curr_cdh->ptr_cd()->pendingWindow_);
    if(event != CDEventT::kNoEvent) {
      dbg << "Set CD Event kErrorOccurred. Level : " << level() <<" CD Name : " << GetCDName()<< endl;
      // Increment pending request count at the target task (requestee)
      MPI_Accumulate(&val, 1, MPI_INT, parent_head_id, 
                     0, 1, MPI_INT, 
                     MPI_SUM, curr_cdh->ptr_cd()->pendingWindow_);
      dbg << "MPI Accumulate done" << endl; getchar();
    }
    MPI_Win_unlock(parent_head_id, curr_cdh->ptr_cd()->pendingWindow_);

    CDMailBoxT &mailbox = curr_cdh->ptr_cd()->mailbox_;

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, parent_head_id, 0, mailbox);
    // Inform the type of event to be requested
    dbg << "Set event start" << endl; getchar();
    if(event != CDEventT::kNoEvent) {
      MPI_Accumulate(&event, 1, MPI_INT, parent_head_id, 
                     curr_cdh->node_id_.task_in_color(), 1, MPI_INT, 
                     MPI_BOR, mailbox);
//    MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
    }
    dbg << "MPI_Accumulate done" << endl; getchar();

//    switch(event) {
//      case CDEventT::kNoEvent :
//        dbg << "Set CD Event kNoEvent" << endl;
//        break;
//      case CDEventT::kErrorOccurred :
//        // Inform the type of event to be requested
//        dbg << "Set event start" << endl; getchar();
//        MPI_Accumulate(&event, 1, MPI_INT, parent_head_id, curr_cdh->node_id_.task_in_color(), 1, MPI_INT, MPI_BOR, mailbox);
////        MPI_Put(&event, 1, MPI_INT, node_id_.head(), 0, 1, MPI_INT, mailbox);
//        dbg << "MPI_Accumulate done" << endl; getchar();
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
    MPI_Win_unlock(parent_head_id, mailbox);
  }
  dbg << "\n================================================================\n" << endl;

  return static_cast<CDErrT>(ret);
}
#endif

//char *CD::GenTag(const char* tag)
//{
//  Tag tag_gen;
//  CDNameT cd_name = ptr_cd()->GetCDName();
//  tag_gen << tag << node_id_.task_in_color_ <<'-'<<cd_name.level()<<'-'<<cd_name.rank_in_level();
////  cout << const_cast<char*>(tag_gen.str().c_str()) << endl; getchar();
//  return const_cast<char*>(tag_gen.str().c_str());
//}


#ifdef comm_log
//SZ
CommLogErrT CD::CommLogCheckAlloc(unsigned long length)
{
  return comm_log_ptr_->CheckChildLogAlloc(length);
}
//GONG
CommLogErrT CD::CommLogCheckAlloc_libc(unsigned long length)
{
  return libc_log_ptr_->CheckChildLogAlloc(length);
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
  return CDPath::GetParentCD();
}

#if 0
//SZ
CommLogErrT CD::ProbeAndReadData(unsigned long flag)
{
  // look for the entry in incomplete_log_
  int found = 0;
  std::vector<struct IncompleteLogEntry>::iterator it;
  CD* tmp_cd = this;
  for (it=incomplete_log_.begin(); it!=incomplete_log_.end(); it++)
  {
    if (it->flag_ == flag) 
    {
      found = 1;
      PRINT_DEBUG("Found the entry in incomplete_log_\n");
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
        //FIXME: potential bug, what if two MPI_Wait within one CD using the same request??
        //       e.g. begin, irecv, wait, irecv, wait, complete
        if (it->flag_ == flag){
          found = 1;
          PRINT_DEBUG("Found the entry in incomplete_log_\n");
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

//SZ
CommLogErrT CD::ProbeAndLogData(unsigned long flag)
{
  // look for the entry in incomplete_log_
  int found = 0;
  std::vector<struct IncompleteLogEntry>::iterator it;
  CD* tmp_cd = this;
  for (it=incomplete_log_.begin(); it!=incomplete_log_.end(); it++)
  {
    if (it->flag_ == flag) 
    {
      found = 1;
      PRINT_DEBUG("Found the entry in incomplete_log_ in current CD\n");
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
        if (it->flag_ == flag){
          found = 1;
          PRINT_DEBUG("Found the entry in incomplete_log_ in one parent CD\n");
          break;
        }
      }
      if (found){
        break;
      }
    }

    if (!found)
    {
      //ERROR_MESSAGE("Do not find corresponding Isend/Irecv incomplete log!!\n")
      PRINT_DEBUG("Possible bug: Isend/Irecv incomplete log NOT found (maybe deleted by MPI_Test)!!\n")
    }
  }
  
  // ProbeAndLogData for comm_log_ptr_ if relaxed CD
  // If NOT found, then work has been completed by previous Test Ops
  if (comm_log_ptr_ != NULL && found)
  {
    // ProbeAndLogData:
    // 1) change Isend/Irecv entry to complete state if there is any
    // 2) log data if Irecv
#if _DEBUG
    PRINT_DEBUG("Print Incomplete Log before calling comm_log_ptr_->ProbeAndLogData\n");
    PrintIncompleteLog();
#endif
    bool found = comm_log_ptr_->ProbeAndLogData((void*)(it->addr_), it->length_, flag, it->isrecv_);
    if (!found)
    {
      CD* tmp_cd = this;
      while (tmp_cd->GetParentHandle() != NULL)
      {
        if (tmp_cd->GetParentHandle()->ptr_cd()->cd_type_==kRelaxed)
        {
          found = GetParentHandle()->ptr_cd()->comm_log_ptr_
            ->ProbeAndLogDataPacked((void*)(it->addr_), it->length_, flag, it->isrecv_);
          if (found)
            break;
        }
        else {
          break;
        }
      }
      if (!found)
      {
        PRINT_DEBUG("Possible bug: not found the incomplete logs...\n");
      }
    }
    // need to log that wait op completes 
    comm_log_ptr_->LogData((MPI_Request*)flag, 0);
  }
  else if (comm_log_ptr_ != NULL)
  {
    // need to log that wait op completes 
    comm_log_ptr_->LogData((MPI_Request*)flag, 0);
  }

  // delete the incomplete log entry
  tmp_cd->incomplete_log_.erase(it);
  return kCommLogOK;
}

//SZ
CommLogErrT CD::LogData(const void *data_ptr, unsigned long length, 
                      bool completed, unsigned long flag, bool isrecv, bool isrepeated)
{
  if (comm_log_ptr_ == NULL)
  {
    ERROR_MESSAGE("Null pointer of comm_log_ptr_ when trying to log data!\n");
    return kCommLogError;
  }
  return comm_log_ptr_->LogData(data_ptr, length, completed, flag, isrecv, isrepeated);
}

//SZ
CommLogErrT CD::ProbeData(const void *data_ptr, unsigned long length)
{
  if (comm_log_ptr_ == NULL)
  {
    ERROR_MESSAGE("Null pointer of comm_log_ptr_ when trying to read data!\n");
    return kCommLogError;
  }
  return comm_log_ptr_->ProbeData(data_ptr, length);
}

//SZ
CommLogErrT CD::ReadData(void *data_ptr, unsigned long length)
{
  if (comm_log_ptr_ == NULL)
  {
    ERROR_MESSAGE("Null pointer of comm_log_ptr_ when trying to read data!\n");
    return kCommLogError;
  }
  return comm_log_ptr_->ReadData(data_ptr, length);
}

//SZ
CommLogMode CD::GetCommLogMode()
{
  return comm_log_ptr_->GetCommLogMode();
}
//GONG
CommLogMode CD::GetCommLogMode_libc()
{
  return libc_log_ptr_->GetCommLogMode();
}

//SZ
bool CD::IsNewLogGenerated()
{
  return comm_log_ptr_->IsNewLogGenerated_();
}

//GONG
bool CD::IsNewLogGenerated_libc()
{
  return libc_log_ptr_->IsNewLogGenerated_();
}


//SZ
//  struct IncompleteLogEntry{
//    unsigned long addr_;
//    unsigned long length_;
//    unsigned long flag_;
//    bool complete_;
//    bool isrecv_;
//  };
void CD::PrintIncompleteLog()
{
  if (incomplete_log_.size()==0) return;
  PRINT_DEBUG("incomplete_log_.size()=%ld\n", incomplete_log_.size());
  for (std::vector<struct IncompleteLogEntry>::iterator ii=incomplete_log_.begin();
        ii != incomplete_log_.end(); ii++)
  {
    PRINT_DEBUG("\nPrint Incomplete Log information:\n");
    PRINT_DEBUG("    addr_=%lx\n", ii->addr_);
    PRINT_DEBUG("    length_=%ld\n", ii->length_);
    PRINT_DEBUG("    flag_=%ld\n", ii->flag_);
    PRINT_DEBUG("    complete_=%d\n", ii->complete_);
    PRINT_DEBUG("    isrecv_=%d\n", ii->isrecv_);
  }
}
#endif







/*


bool HeadCD::HandleEntrySearch::operator()(void)
{
  int entry_requester_id = task_id_;
  ENTRY_TAG_T *recvBuf = recvBuf_;

  dbg << "HeadCD::HandleEntrySearch\t";

  MPI_Status status;
  MPI_Recv(recvBuf, 
           2, 
           MPI_UNSIGNED_LONG_LONG, 
           entry_requester_id, 
           MSG_TAG_ENTRY_TAG, 
           color(), // Entry requester is in the same CD task group. 
           &status);

  ENTRY_TAG_T &tag_to_search  = recvBuf[0];
  ENTRY_TAG_T &source_task_id = recvBuf[1];
  int found_level = 0;

  CDEntry *target_entry = SearchEntry(tag_to_search, found_level);
  
  if(target_entry != NULL) {
    // It needs some effort to avoid the naming alias problem of entry tags.
    entry_search_req_[tag_to_search] = CommInfo();
  
    // Found the entry!! 
    // Then, send it to the task that has the entry of actual copy
    ENTRY_TAG_T tag_to_search = target_entry->name_tag();
    int target_task_id = target_entry->dst_data_.node_id().task_in_color();

    // SetMailBox for entry send to target
    CDEventT entry_send = kEntrySend;
    SetMailBox(entry_send, target_task_id);

    ENTRY_TAG_T sendBuf[2] = {tag_to_search, source_task_id};
    MPI_Isend(sendBuf, 
              2, 
              MPI_UNSIGNED_LONG_LONG, 
              target_task_id, 
              cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG),
              color(), // Entry sender is also in the same CD task group.
              &(entry_search_req_[tag_to_search]).req_);
  
    event_flag_[entry_requester_id] &= ~kEntrySearch;
    handled_event_count++;
    return true;
  }
  else {
    // failed to search the entry at this level of CD.
    cerr<<"Failed to search the entry at current CD level. Request to upper-level head to search the entry. Not implemented yet\n"<<endl;
    assert(0);

    event_flag_[entry_requester_id] &= ~kEntrySearch;
    handled_event_count++;
    return false;
  }
}

void CD::HandleEntrySend::operator()(void)
{
  dbg << "CD::HandleEntrySend\n" << endl;

  MPI_Status status;
  ENTRY_TAG_T recvBuf[2] = {INIT_TAG_VALUE, INIT_ENTRY_SRC};
  MPI_Recv(recvBuf, 
           2, 
           MPI_UNSIGNED_LONG_LONG, 
           head(), 
           cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG),
           color(), // Entry searcher is in the same CD task gruop (head) 
           &status);

  ENTRY_TAG_T &tag_to_search = recvBuf[0];
  int entry_source_task_id  = static_cast<int>(recvBuf[1]);
  dbg << "Received tag : " << tag_to_search << " = " << tag2str[tag_to_search] << endl; 

  // Find entry
  if(tag_to_search == INIT_TAG_VALUE) {
    cerr << "Entry sender failed to receive the entry tag to search for the right entry with. CD Level " << level() << endl;
    assert(0);
  }

//  CDEntry *entry = InternalGetEntry(tag_to_search);
  int found_level=level();
  CDEntry *entry = SearchEntry(tag_to_search, found_level);

  if(entry == NULL) {
    cerr << "The received tag ["<< tag_to_search << "] does not work in "<< cd_id_.node_id() <<". CD Level " << level() << endl;
    assert(0);
  }
  else
    dbg << "HandleEntrySend: entry was found at " << found_level << endl;

  entry_send_req_[tag_to_search] = CommInfo();
  // Should be non-blocking send to avoid deadlock situation. 
  MPI_Isend(entry->dst_data_.address_data(), 
            entry->dst_data_.len(), 
            MPI_BYTE, 
            entry_source_task_id, 
            cd_id_.GenMsgTag(tag_to_search), 
            MPI_COMM_WORLD, // could be any task in the whole rank group 
            &(entry_send_req_[tag_to_search].req_));  

  dbg << "CD Event kEntrySend\t\t\t";
  *event_flag_ &= ~kEntrySend;
  handled_event_count++;
}



void HeadCD::HandleErrorOccurred::operator()(void)
{
  if(task_size() == 1) return;
  int task_id = task_id_;
#if 1
//  // Pause all the tasks in current CD, first.
//  for(int i=0; i<task_size(); i++) {
//    if(i == head()) continue;
//    CDEventT all_pause = kAllPause;
//    SetMailBox(all_pause, i);
//  }
//
//  // Make decision for the error handling
//  bool error_handled = true;
//  if(error_handled) {
//    // Resume
////    MPI_Barrier(color());
//  }
//  else {
//    for(int i=0; i<task_size(); i++) {
//      if(i == head()) continue;
//      CDEventT all_reexecute = kAllReexecute;
//      SetMailBox(all_reexecute, i);
//    }
//    // Resume
//    MPI_Barrier(color());
//    HandleAllReexecute();
//  }
#endif

  dbg << "HeadCD Event kErrorOccurred";
  event_flag_[task_id] &= ~kErrorOccurred;
  handled_event_count++;
  
}


void HeadCD::HandleAllResume::operator()(void)
{
  if(task_size() == 1) return;

  dbg << "CD Event kAllResume\t\t\t";
  event_flag_[head()] &= ~kAllResume;
  handled_event_count++;

//  MPI_Barrier(color());

}

void CD::HandleAllPause::operator()(void)
{
  if(task_size() == 1) return;
  
  dbg << "CD Event kAllPause\t\t\t";
  *event_flag_ &= ~kAllResume;
  handled_event_count++;
//  MPI_Barrier(color());

  if( CHECK_EVENT(*event_flag_, kAllReexecute) ) {
    cout << "\n==================Reexecuted by somebody================\n" << endl;;
    dbg << "\n================== Reexecuted at level " << level() << "================\n" << endl;;
    HandleAllReexecute();
  }
  else {
    dbg << "\n================== Resumed at level " << level() << "================\n" << endl;;

  }
}

void CD::HandleAllReexecute::operator()(void)
{
  if(task_size() == 1) return;

  dbg << "CD Event kAllReexecute\t\t";
  *event_flag_ &= ~kAllReexecute;
  handled_event_count++;

  recoverObj_->Recover(this);
}

//void HeadCD::HandleAllPause(void)
//{
//  if(task_size() == 1) return;
//
//  assert(0);
//////  for(int i=0; i<task_size(); i++) {
//////    if(i == head() continue;
//////    event_flag_[i] |= kAllPause;
////    }
////  dbg << "HeadCD Event kAllPause\t\t\t";
////
////  cout <<"============ Paused  ================" << endl;
////  HandleAllResume();
////  cout <<"============ Resumed ================" << endl;
//}



*/




CD::CDInternalErrT CD::InvokeErrorHandler(void)
{
  dbg << "CD::CDInternalErrT CD::InvokeErrorHandler(void), event queue size : " << cd_event_.size() << endl;
  CDInternalErrT cd_err = kOK;

  while(cd_event_.size() > 0) {
    CDEventHandler * cd_event_handler = cd_event_.back();
    cd_event_handler->HandleEvent();
    delete cd_event_handler;
    cd_event_.pop_back();
  }

  if(need_reexec) { 
    need_reexec = false;
    (*pendingFlag_) -= handled_event_count;
    this->Recover(); 
  }

  return cd_err;
}

CD::CDInternalErrT HeadCD::InvokeErrorHandler(void)
{
  dbg << "HeadCD::CDInternalErrT CD::InvokeErrorHandler(void), event queue size : " << cd_event_.size() << ", (head) event queue size : " << headcd_event_.size()<< endl;
  CDInternalErrT cd_err = kOK;

  while(cd_event_.size() > 0) {
    CDEventHandler *cd_event_handler = cd_event_.back();
    cd_event_handler->HandleEvent();
    delete cd_event_handler;
    cd_event_.pop_back();
  }

  while(headcd_event_.size() > 0) {
    HeadCDEventHandler *headcd_event_handler = headcd_event_.back();
    headcd_event_handler->HandleEvent();
    delete headcd_event_handler;
    headcd_event_.pop_back();
  }

  if(need_reexec) {
    need_reexec = false;
    (*pendingFlag_) -= handled_event_count;
    this->Recover();
  }

  return cd_err;
}


//-------------------------------------------------------

CDEntry *CD::SearchEntry(ENTRY_TAG_T entry_tag_to_search, int &found_level)
{
    cout << "Search Entry : " << entry_tag_to_search;
    cout << " - " << tag2str[entry_tag_to_search] << endl;
		CDHandle *parent_cd = CDPath::GetParentCD();
    CDEntry *entry_tmp = parent_cd->ptr_cd()->InternalGetEntry(entry_tag_to_search);

    dbg<<"parent name: "<<parent_cd->GetName()<<endl;
    if(entry_tmp != NULL) { 
      dbg << "parent dst addr : " << entry_tmp->dst_data_.address_data()
                << ", parent entry name : " << entry_tmp->dst_data_.ref_name()<<endl;
    } else {
      dbg<<"there is no reference in parent level"<<endl;
    }
//		if( ptr_cd_ == 0 ) { ERROR_MESSAGE("Pointer to CD object is not set."); assert(0); }

//		CDEntry* entry = parent_cd->ptr_cd()->InternalGetEntry(entry_tag_to_search);
//		dbg <<"ref name: "    << entry->dst_data_.ref_name() 
//              << ", at level: " << entry->ptr_cd()->GetCDID().level()<<endl;
//		if( entry != 0 ) {

      //Here, we search Parent's entry and Parent's Parent's entry and so on.
      //if ref_name does not exit, we believe it's original. 
      //Otherwise, there is original copy somewhere else, maybe grand parent has it. 

      CDEntry* entry = NULL;
			while( parent_cd != NULL ) {
		    entry = parent_cd->ptr_cd()->InternalGetEntry(entry_tag_to_search);
				cout << "InternalGetEntry done at Level: " << parent_cd->ptr_cd()->GetCDID().level()<<endl;

        if(entry != NULL) {
          found_level = parent_cd->ptr_cd()->level();
          cout<<"\n\nI got my reference here!! " << found_level << "\n"<<endl;
				  cout <<"Current entry name : "<< entry->name() << " with ref name : "  << entry->dst_data_.ref_name() 
                  << ", at level: " << found_level<<endl;
          break;
        }
        else {
				  parent_cd = CDPath::GetParentCD(parent_cd->ptr_cd()->level());
//          dbg<< "Gotta go to upper level! -> " << parent_cd->GetName() << " at level "<< parent_cd->ptr_cd()->GetCDID().level() << endl;
        }
			} 
      if(parent_cd == NULL) entry = NULL;
      cout<<"here?? check entry " << entry << endl; //(int)(entry ===NULL)<endl;
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






//void CD::AddEntryToSend(const ENTRY_TAG_T &entry_tag_to_search) 
//{
////  entry_recv_req_[entry_tag_to_search];
//  MPI_Alloc_mem(MAX_ENTRY_BUFFER_SIZE, MPI_INFO_NULL, &(entry_recv_req_[entry_tag_to_search]));
//}
//
//void CD::RequestEntrySearch(void)
//{
//  dbg << "REQ" << endl;
//  for(auto it=entry_recv_req_.begin(); it!=entry_recv_req_.end(); ++it) {
//    dbg << tag2str[*it] << endl;
//  }
////  MPI_Status status;
////  void *entry_list_to_search;
////  MPI_Probe(source, tag, cdh->node_id().color(), &status)
//
//  MPI_Irecv(entry_recv_req_[entry_tag_to_search], PACKED_EN, MPI_UNSIGNED_LONG_LONG, src, entry_tag_to_search, node_id().color(), &status);  
//  
//}
//












/*  CD::setDetectFunc(void *() custom_detect_func)
 *  (1) 
 *      
 *  (2) 
 *
 *  (3)
 *
 */

//int CD::AddDetectFunc(void *() custom_detect_func)
//{
//
//  return CDErrT::kOK;
//}

