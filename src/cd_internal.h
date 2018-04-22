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

#ifndef _CD_H 
#define _CD_H

/**
 * @file cd.h
 * @author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * @date March 2015
 *
 * @brief Containment Domains API v0.2 (C++)
 */

#include "cd_features.h"
#include "cd_global.h"
#include "cd_def_internal.h"
#include "cd_def_common.h"
#include "cd_handle.h"
//#include "cd_entry.h"
#include "util.h"
#include "cd_id.h"
#include "cd_file_handle.h"
#include "recover_object.h"
#include "serializable.h"
#include "event_handler.h"
//#include "cd_pfs.h"
//#include "packer.h"
#include "persistence/cd_packer.hpp"

#include <list>

#include <cstring>
#include <ucontext.h>
#include <functional>
#include "machine_specific.h"

#if CD_COMM_LOG_ENABLED
//SZ
#include "cd_comm_log.h"
#endif

#if CD_LIBC_LOG_ENABLED
#include "cd_malloc.h"
#endif

using namespace packer;

namespace packer {

//class CDEntry;
//class CDPacker;

struct RemoteCDEntry : public CDEntry {
  int task_id_;
  RemoteCDEntry(void) {}
  RemoteCDEntry(uint64_t id, uint64_t size, uint64_t offset) 
    : CDEntry(id, size, offset), task_id_(cd::myTaskID) {}
  RemoteCDEntry(const RemoteCDEntry &that) 
    : CDEntry(that.id_, that.size_.code_, that.offset_, that.src_), task_id_(that.task_id_) {}
  RemoteCDEntry(const CDEntry &that) {
    copy_cdentry(that);
  }
  void copy_cdentry(const CDEntry &that) {
    task_id_ = cd::myTaskID;
    id_      = that.id_;
    size_    = that.size_;
    offset_  = that.offset_;
    src_     = that.src_;
  }
  void copy_rcdentry(const RemoteCDEntry &that) {
    task_id_ = that.task_id_;
    id_      = that.id_;
    size_    = that.size_;
    offset_  = that.offset_;
    src_     = that.src_;
  }
  RemoteCDEntry &operator=(const CDEntry &that) {
    copy_cdentry(that);
    return *this;
  }
  RemoteCDEntry &operator=(const RemoteCDEntry &that) {
    copy_rcdentry(that);
    return *this;
  }
};

}


namespace cd {

  namespace internal {

    // Initialize basepath for preservation, and 
    // create directory if necessary.
    void InitFileHandle(bool make_dir);
    
    //class Internal 
    //brief This class contains static internal routines 
    //      regarding initialization and finalization of CD runtime.
    void Initialize(void);
    
    void Finalize(void);
    class EventHandler;

/**@class CD 
 * @brief Core data structure of CD runtime. It has all the information and controls of a specific level of CDs.
 * This object will not be exposed to users and be managed internally by CD runtime.
 *
 * \todo Implement serialize and deserialize of CD object.
 */ 
class CD : public Serializable {
    /// The friends of CD class
    friend class cd::CDHandle;  
    friend class cd::RegenObject;   
    friend class cd::RecoverObject;
    friend class HandleAllReexecute;
    friend class HandleErrorOccurred;
    friend class HandleAllResume;
    friend class HandleAllPause;
    friend class HandleEntrySearch;
    friend class HandleEntrySend;
    friend class HeadCD;
    friend class cd::logging::RuntimeLogger;
    friend CDHandle *cd::CD_Init(int numTask, int myTask, PrvMediumT prv_medium);
    friend void cd::CD_Finalize(void);
#if CD_MPI_ENABLED
    friend class cd::logging::CommLog;
    friend int MPI_Win_fence(int assert, MPI_Win win);
#endif
    friend void Initialize(void);
    friend void Finalize(void);
#if CD_TEST_ENABLED
    friend class cd_test::Test;
#endif

//    friend CD* IsLogable(bool *logable_);
//    friend CDEntry *CDHandle::InternalGetEntry(std::string entry_name);
    enum { 
      CD_PACKER_NAME=0,
      CD_PACKER_CDID,
      CD_PACKER_LABEL,
      CD_PACKER_CDTYPE,
      CD_PACKER_NUMREEXEC,
      CD_PACKER_BITVECTOR,
      CD_PACKER_JMPBUFFER,
      CD_PACKER_CTXT,
      CD_PACKER_CTXTMOD,
      CD_PACKER_CTXTOPT,
      CD_PACKER_DETECTFUNC,
      CD_PACKER_RECOVERFUNC
    };
  protected:
//  public:

//    enum CtxtPrvMode { kExcludeStack=0, 
//                       kIncludeStack
//                     };

    enum CDInternalErrT { kOK=0, //!< No error 
                          kExecModeError,  //!< Internal error during execution
                          kEntryError,     //!< Internal error in CD entries
                          kErrorReported   //!< Error reported from hardware or application.
                        };  
  protected: 
    CDID cd_id_;
    RecoverObject *recoverObj_;

    bool recreated_;
    bool reexecuted_;
    bool is_window_reused_;
    bool reported_error_;
    bool begin_;
//  public:

    static CDFlagT *rollback_point_;
#if CD_MPI_ENABLED
    // This flag is unique for each process. 
    static CDFlagT *pendingFlag_;
    static CDMailBoxT pendingWindow_;
    static CDMailBoxT rollbackWindow_;
    static bool head_in_levels;
    // Every mailbox resides in head 
    CDFlagT *event_flag_;
    CDMailBoxT mailbox_;
#endif

    // Label for Begin/Complete pair. It is mainly for Loop interation.
    // The Begin/Complete pair that has the same computation will have the same label_
    // and we can optimize CD with this label_ later.
    std::string     label_;
    
    // Name of this CD
    std::string     name_;

    // Flag for Strict or Relexed CD
    CDType          cd_type_;

    // Flag to imform the medium type for preservation
    PrvMediumT      prv_medium_;

    /// Set rollback point and options
    CtxtPrvMode     ctxt_prv_mode_;
    ucontext_t      ctxt_;
    jmp_buf         jmp_buffer_;
    int             jmp_val_;
    uint64_t        option_save_context_; 
//    StackEntry     *stack_entry_;
    /// Flag for normal execution or reexecution.
    CDExecMode      cd_exec_mode_;
    uint64_t        preserve_count_;
    uint64_t        restore_count_;
    int             num_reexecution_;
    
#if 1
  public:
//    static bool need_reexec;
    static bool need_escalation;
//    static int handled_event_count;
//    static uint32_t reexec_level;
#endif
  protected:

    /// Detection-related meta data
    // bit vector that will mask error types that can be handled in this CD
    uint64_t        sys_detect_bit_vector_;

    // Registered user-defined error detection routine
    std::list<std::function<int(void)> > usr_detect_func_list_;  //custom_detect_func;


    /// FIXME later this should be map or list, 
    /// sometimes it might be better to have map structure 
    /// Sometimes list would be beneficial depending on 
    /// how frequently search functionality is required.
    /// Here we define binary search tree structure... 
    /// Then insert only the ones who has ref names.

    /// 09.23.2014
    /// There are two data structure for each CD entry. 
    /// entry_directory_ is a super set of entry_directory_map_
    /// entry_directory_map_ is for preservation via reference
    /// If ref_name is passed by preservation call, it means it allows this entry to be referred to by children CDs.
    //std::list<CDEntry> entry_directory_;
    packer::CDPacker entry_directory_;

    /// 10.12.2016
    /// shawdow entry directory is newly introduced for two purposes.
    /// 1. Buffer to preserve the data that will be needed in future.
    ///    One problem of preservation of large volume is that
    ///    there may be non-negligible contention on storage medium.
    ///    It becomes more severe at large scale, therefore one way to
    ///    resolve it is to distribute the preservation overhead before 
    ///    checkpoint boundary as much as possible.
    /// 2. Enable "Advance semantics" which mark a rollback point by
    ///    updating preservation entries, not deleting all, then
    ///    re-preservation all. Advance sematics are very appealing in
    ///    some cases that child CD update a partial array iteratively,
    ///    but parent CD can advance its rollback point by updating the
    ///    preservation entries which is modified by children level.
    ///    This case, shadow entry is needed for child CD to push modified 
    ///    entry to parent for preservation, and child CD may preserve it
    ///    via reference in future. Therefore, parent CD need an additional
    ///    entry directory that contains child's updates for preservation, 
    ///    but should not be involved in parent-level state restoration 
    ///    in the case of escalation. 
    ///    When parent CD advances the rollback point, then the shadow entries
    ///    will add or replace/update corresponding entries in entry_directory,
    ///    make the shadow entry directory empty for the next.
//    std::list<CDEntry> shadow_entry_directory_;

    static std::list<CommInfo> entry_req_;
    static std::map<ENTRY_TAG_T, CommInfo> entry_request_req_;
//    static std::map<ENTRY_TAG_T, CommInfo> entry_send_req_;
    static std::map<ENTRY_TAG_T, CommInfo> entry_recv_req_;
//    static std::map<ENTRY_TAG_T, CommInfo> entry_search_req_;
//    static std::map<std::string, uint32_t> exec_count_;
    static std::map<std::string, CDHandle *> access_store_;
    static std::map<uint32_t, CDHandle *> delete_store_;

    // Only CDEntries that has refname will be pushed into this data structure for later quick search.
//    EntryDirType entry_directory_map_;   
   
    // These are entry directory for preservation via reference 
    // in the case that the preserved copy resides in a remote node. 
    EntryDirType remote_entry_directory_map_;   
    
    static std::list<EventHandler *> cd_event_;
    
//    Packer packer_;
/*  
09.23.2014 
It is not complete yet. I am thinking of some way to implement like cd_advance semantic which should allow
update the preserved data. 
 __________________________________________________________________________________________________________
|_case_|_entry_name_|_ref_name_|______________Meaning__________________|___________Property________________|
|      |            |          |I copied app data for preservation     | Modification in my preserved      |
|  0   |      X     |     X    |but don't allow reference from         | copy of data is fine.             |
|______|____________|__________|my_children.___________________________|___________________________________| 
|      |            |          |I do not have a copy of app data       | Modification in preserved data    |
|  1   |      X     |     O    |but will refer to my ancestor's entry  | is not allowed.                   |
|______|____________|__________|_______________________________________|___________________________________|
|      |            |          |I copied app data for preservation     | Modification is not fine becuase  | 
|  2   |      O     |     X    |and also allow reference from          | my children CD may refer to my    |
|______|____________|__________|my_children.___________________________|_preserved_data.___________________| 
|      |            |          |I do not have a copy of app data       | Modification is not allowed       |
|  3   |      O     |     O    |but will refer to my ancestor's entry  |                                   |
|______|____________|__________|and_also_allow_reference_from_children_|___________________________________|
*/

    /// This shall be used for re-execution. 
    /// We will restore the value one by one.
    /// Should not be CDEntry*. 
//    std::list<CDEntry>::iterator iterator_entry_;   
  
//    CDFileHandle file_handle_;
    // PFS
//    PFSHandle *pfs_handle_;

#if CD_LIBC_LOGGING
    uint64_t libc_log_begin_;
    uint64_t libc_log_end_;
    uint64_t libc_log_id_;
#endif

#if CD_COMM_LOG_ENABLED
  public:
    //SZ
    CommLog *comm_log_ptr_;
    uint32_t child_seq_id_;
    //KYU: FIXME later I want to merge this flag and exec_mode flag.
    CDLoggingMode cd_logging_mode_;
    //KYU
    IncompleteLogStore incomplete_log_;
#endif

#if CD_LIBC_LOG_ENABLED
    //GONG
    CommLog *libc_log_ptr_;
    //GONG
    std::vector<IncompleteLogEntry> mem_alloc_log_;
    unsigned long cur_pos_mem_alloc_log;
    unsigned long replay_pos_mem_alloc_log;
#endif

  protected:
//  public:
    CD();

    CD(CDHandle *cd_parent, 
       const char *name, 
       const CDID& cd_id, 
       CDType cd_type, 
       PrvMediumT prv_medium, 
       uint64_t sys_bit_vector);

    virtual ~CD();

    void Initialize(CDHandle *cd_parent, 
                    const char *name, 
                    const CDID &cd_id, 
                    CDType cd_type, 
                    PrvMediumT prv_medium, 
                    uint64_t sys_bit_vector);
    void InternalInitialize(CDHandle *cd_parent);
#if CD_MPI_ENABLED
    void InitializeMailBox(void); 
    void FinalizeMailBox(void);
#endif
    inline void Init(void);

    virtual CDHandle *Create(CDHandle *parent, 
                     const char *name, 
                     const CDID& child_cd_id, 
                     CDType cd_type, 
                     uint64_t sys_bit_vector, 
                     CDInternalErrT* cd_err=0);

    static CDHandle *CreateRootCD(const char *name, 
                     const CDID& child_cd_id, 
                     CDType cd_type,
                     uint64_t sys_bit_vector, 
                     CD::CDInternalErrT *cd_internal_err);

    virtual CDErrT Destroy(bool collective=true, bool need_destroy=false);

    CDErrT Begin(const char *label=NO_LABEL,
                 bool collective=true);

    CDErrT Complete(bool update_preservations=false,
                    bool collective=true);

    CDErrT Advance(bool collective=true);

    CDErrT Preserve(void *data,                   // address in the local process 
                    uint64_t &len_in_bytes,        // data size to preserve
                    CDPrvType preserve_mask, // preservation method
                    std::string my_name,          // data name      
                    std::string ref_name,         // reference name
//                    const char *my_name=NO_LABEL,        // data name
//                    const char *ref_name=NO_LABEL,       // reference name
                    uint64_t ref_offset=0,        // reference offset
                    const RegenObject *regen_object=0, // regen object
                    PreserveUseT data_usage=kUnsure);   // for optimization
  

    // Collective version
    // Kyushick : Why do we need collective Preserve call as an API?
    // I think we need collective method when CDs want to preserve data to Global file system
    // So user set some medium type
    CDErrT Preserve(CDEvent &cd_event,            // Event object to synch
                    void *data_ptr, 
                    uint64_t &len, 
                    CDPrvType preserve_mask, 
                    std::string my_name, 
                    std::string ref_name, 
                    uint64_t ref_offset=0, 
                    const RegenObject *regen_object=0, 
                    PreserveUseT data_usage=kUnsure);
  
    CDInternalErrT Detect(uint32_t &rollback_point); 
    CDErrT RestoreAll(void);
    CDInternalErrT Restore(char *data, uint64_t len_in_bytes, CDPrvType preserve_mask, 
                           const string &my_name, const string &ref_name);
    CDInternalErrT Restore(const string &entry_name);
  
//  [DISCUSSION]
//  Jinsuk: About longjmp setjmp: By running some experiement, 
//  I confirm that this only works when stack is just there. 
//  That means, if we want to jump to a function context 
//  which does not exist in stack anymore, 
//  you will eventually get segfault. 
//  Thus, CD begin and complete should be in the same function context,
//  or at least complete should be deper in the stack 
//  so that we can jump back to the existing stack pointer. 
//  I think that Just always pairing them in the same function is much cleaner. 
  
    CDInternalErrT Assert(bool test);
 
    CDInternalErrT RegisterDetection(uint32_t system_name_mask, 
                                     uint32_t system_loc_mask);

    CDInternalErrT RegisterRecovery(uint32_t error_name_mask, 
                                    uint32_t error_loc_mask, 
                                    RecoverObject *recover_object=0);
  
    CDInternalErrT RegisterRecovery(uint32_t error_name_mask, 
                                    uint32_t error_loc_mask, 
                                    CDErrT(*recovery_func)(std::vector< SysErrT > errors)=0);
public:
    CDID     &GetCDID(void)            { return cd_id_; }
    CDNameT  &GetCDName(void)          { return cd_id_.cd_name_; }
    NodeID   &GetNodeID(void)          { return cd_id_.node_id_; }
    uint32_t level(void)         const { return cd_id_.cd_name_.level_; }
    uint32_t phase(void)         const { return cd_id_.cd_name_.phase_; }
    uint32_t rank_in_level(void) const { return cd_id_.cd_name_.rank_in_level_; }
    uint32_t sibling_num(void)   const { return cd_id_.cd_name_.size_; }
    ColorT  &color(void)               { return cd_id_.node_id_.color_; }
    GroupT  &group(void)               { return cd_id_.node_id_.task_group_; }
    int      task_in_color(void) const { return cd_id_.node_id_.task_in_color_; }
    int      head(void)          const { return cd_id_.node_id_.head_; }
    int      task_size(void)     const { return cd_id_.node_id_.size_; }
    bool     IsHead(void)        const { return cd_id_.IsHead(); }
    bool     recreated(void)     const { return recreated_; }
    bool     reexecuted(void)    const { return reexecuted_; }
    int      num_reexec(void)    const { return num_reexecution_; }
    char    *name(void)          const { return (char *)name_.c_str(); }
    char    *label(void)         const { return (char *)label_.c_str(); }
    CDType   GetCDType(void)     const { return static_cast<CDType>(MASK_CDTYPE(cd_type_)); }
    CDExecMode GetExecMode(void) const { return cd_exec_mode_; }
#if CD_LIBC_LOG_ENABLED
    CommLog *libc_log_ptr()      const { return libc_log_ptr_; }
#endif
    // These should be virtual because I am going to use polymorphism for those methods.
    virtual CDErrT Stop(CDHandle *cdh=NULL);
    virtual CDErrT Resume(void);
    virtual CDErrT AddChild(CDHandle *cd_child);
    virtual CDErrT RemoveChild(CDHandle *cd_child);

    void PrintDebug();
  protected:
 
/**@addtogroup internal_recovery 
 * @{ 
 */


/**
 * @brief Reexecute CD from the beginning. 
 * 
 * It set the CD execution mode as kReexecute, 
 * and increment reexecution counter by one.
 * Then it long jump to the point of setjump marked by CD_Begin.
 * The logging framework for communication message and libc will be replayed in the reexecution mode.
 *
 * @return Returns error code.
 */
    virtual CDErrT Reexecute(void);

///** @brief Escalate error/failure to parent
// *
// * Internal method used by Recover() to escalate
// * errors/failures that cannot be handled.
// * 
// */
//    virtual void Escalate(
//        uint64_t error_name_mask,     //!< [in] Mask of all error/fail types that require recovery
//        uint64_t error_location_mask, //!< [in] Mask of all error/fail locations that require recovery
//        std::vector<SysErrT> errors   //!< [in] Errors/failures to recover from (typically just one).
//        );

/** @brief Invoke a registered recovery handler. 
 *  If user does not register it, CD::Reexecute() is called by default. 
 *
 */
    void Recover(uint32_t level, bool collective=true);

/** @brief Method to test if this CD can recover from an error/location mask
 *
 * \return Returns `true` is can recover and `false` otherwise.
 *
 * \sa Create(), SysErrNameT, SysErrLocT
 */
    virtual bool CanRecover(
          uint64_t error_name_mask,  //!< [in] Mask of all error/fail types that require recovery
          uint64_t error_location_mask //!< [in] Mask of all error/fail locations that require recovery
          );

  /** @} */ // End internal_recovery group ===========================================================================


    // Utilities -----------------------------------------------------
    // GetPlaceToPreserve() decides actual medium to preserve data.
    // TODO Currently, it preserves in memory for everything.
    // we can change it to preserve file by flag when we compile cd runtime. (with MEMORY=0)
    // We need some good strategy to decide the most efficient medium of the CD for preservation.
    PrvMediumT GetPlaceToPreserve(void);
    bool CheckToReuseCD(const std::string &cd_obj_key);
    static CDInternalErrT InternalCreate(CDHandle *parent, 
                     const char *name, 
                     const CDID& child_cd_id, 
                     CDType cd_type, 
                     uint64_t sys_bit_vector, 
                     CDHandle** new_cd_handle);

    CDInternalErrT InternalDestroy(bool collective, bool need_destroy=false);
    CDInternalErrT InternalPreserve(void *data, 
                                    uint64_t &len_in_bytes,
                                    CDPrvType preserve_mask, 
                                    const std::string &my_name, 
                                    const std::string &ref_name, 
                                    uint64_t ref_offset, 
                                    const RegenObject *regen_object, 
                                    PreserveUseT data_usage);
    CDEntry *PreserveCopy(void *data, 
                          uint64_t &len_in_bytes, 
                          CDPrvType preserve_mask, 
                          const std::string &my_name, uint64_t id);
  
    CDErrT InternalReexecute(void);

    void FreeCommResource(void);
    // copy should happen for the part that is needed.. 
    // so serializing entire CDEntry class does not make sense. 

  protected:
    // Search CDEntry with entry_name given. 
    // It is needed when its children preserve data via reference 
    // and search through its ancestors. 
    // If it cannot find in current CD object, it outputs NULL 
    CDEntry *InternalGetEntry(ENTRY_TAG_T entry_name, uint16_t attr);
    virtual CDEntry *GetEntry(ENTRY_TAG_T entry_name, uint16_t attr);
    virtual CDEntry *SearchEntry(ENTRY_TAG_T tag_to_search, uint32_t &found_level, uint16_t attr=0);
    void AddEntryToSend(const ENTRY_TAG_T &entry_tag_to_search);
  private:
    // This comment is previous one, so may be confusing for current design. 
    //
    // When Restore is called, it should be copied the only part ... smartly. 
    // It means we need to send a copy request to remote node. 
    // If it is in PGAS this might be a little easier to handle. 
    // For MPI, it could be a little tricky but anyways possible.
    // When it is serialized they should not do the deep copy for datahandle... 
    // so basically address information is transfered but not the actual data... 
    // FIXME for now let's just consider single process or single thread environment.. 
    // so we can always return CDEntry without worries. 
    // But later we will need to trasfer this object 
    // and then new CDEntry and then return with proper value. 
    
    // Actual malloced data or created file for the preservation is deleted in this routine. 
    virtual void DeleteEntryDirectory(void);
    

    CDInternalErrT InvokeErrorHandler(void);
    CDInternalErrT InvokeAllErrorHandler(void);
    // Test the completion of internal-CD communications
#if CD_MPI_ENABLED
    bool TestComm(bool test_until_done=false);
    bool TestReqComm(bool is_all_valid=true);
    bool TestRecvComm(bool is_all_valid=true);
    void GetRemoteEntry(void); 
#endif

  protected:
    CDInternalErrT Sync(ColorT color); 
    CDInternalErrT SyncFile(void); 
    static uint32_t SyncCDs(CD *cd_lv_to_sync, bool for_recovery=false);
    void *SerializeRemoteEntryDir(uint64_t &len_in_bytes);
//    void DeserializeRemoteEntryDir(EntryDirType &remote_entry_dir, void *object, uint64_t task_count, uint64_t unit_size);
    virtual void *Serialize(uint64_t &len_in_bytes);
    virtual void Deserialize(void *object);
    static CDHandle *GetCDToRecover(CDHandle *cd_level, bool collective=true);
    CDInternalErrT CompleteLogs(bool is_rollback=false);
#if CD_LIBC_LOG_ENABLED
public:
    //GONG: duplicated for libc logging
    CommLogErrT CommLogCheckAlloc_libc(unsigned long length);
    void *MemAllocSearch(CD *curr_cd, unsigned int level, unsigned long index, void *p_update = NULL);
    bool PushedMemLogSearch(void *p, CD *curr_cd);
    unsigned int PullMemLogs(void);
#endif

#if CD_COMM_LOG_ENABLED
public:
    //KL
    bool DeleteIncompleteLog(int64_t flag);
    //bool DeleteIncompleteLog(void* request);
    CommLogErrT InvalidateIncompleteLogs(void);
    CommLogErrT ProbeIncompleteLogs(void);
    //SZ
    //FIXME: no function should return CommLogErrT, change to CD error types...
    CommLogErrT CommLogCheckAlloc(unsigned long length);
    //SZ 
    bool IsParentLocal();
    //SZ
    CDHandle *GetParentHandle();
    //SZ
    CommLogErrT ProbeAndLogData(int64_t flag);
    //SZ
    CommLogErrT LogData(const void *data_ptr, 
                        unsigned long length, 
                        uint32_t task_id=0,
                        bool completed=true, int64_t flag=0,
                        bool isrecv=0, bool isrepeated=0, 
                        // KL : added these three things
                        bool intra_cd_msg=false, 
                        int tag=INVALID_MSG_TAG, 
                        ColorT comm=INVALID_COLOR);
    //SZ
    CommLogErrT ProbeData(const void *data_ptr, unsigned long length);
    //SZ
    CommLogErrT ReadData(void *data_ptr, unsigned long length);
    //SZ
    CommLogMode GetCommLogMode();
    //GONG: duplicated for libc
    CommLogMode GetCommLogMode_libc();
    //SZ
    bool IsNewLogGenerated();
    //GONG: duplicated for libc
    bool IsNewLogGenerated_libc();
    //SZ
    //SZ
    void PrintIncompleteLog();
    //SZ
    CDLoggingMode GetCDLoggingMode() {return cd_logging_mode_;}
    void SetCDLoggingMode(CDLoggingMode cd_logging_mode) {
      cd_logging_mode_ = cd_logging_mode;
    }
#endif

  private:
    uint32_t SetRollbackPoint(const uint32_t &rollback_lv, bool remote);
    uint32_t CheckRollbackPoint(bool remote);
#if CD_MPI_ENABLED
    CDErrT CheckMailBox(void);
    CDErrT SetMailBox(const CDEventT &event);
    CDInternalErrT RemoteSetMailBox(const CDEventT &event);
    CDEventHandleT ReadMailBox(const CDFlagT &event);
    virtual CDInternalErrT InternalCheckMailBox(void);
    inline void     UnsetEventFlag(CDFlagT &event_flag, CDFlagT event_mask);
    inline uint32_t GetPendingCounter(void);
    uint32_t DecPendingCounter(void);
    inline uint32_t IncPendingCounter(void);
    inline CDFlagT GetEventFlag(void);
//    inline void CheckError(bool collective, uint32_t &orig_rollback_point, uint32_t &new_rollback_point);
    inline void CheckReexecution(void);
    inline void ForwardToLowerLevel(CD *cdp, const CDEventT &event);
    inline bool GetBegin() const { return begin_; } 
    inline void SetBegin()       { CD_ASSERT_STR(begin_ == false, "Lv%u: %s\n", level(), label()); begin_ = true; } 
    inline void UnsetBegin()     { CD_ASSERT_STR(begin_ == true, "Lv%u: %s\n", level(), label()); begin_ = false; } 
  public:
    int  BlockUntilValid(MPI_Request *request, MPI_Status *status);
    int  BlockallUntilValid(int count, MPI_Request array_of_request[], MPI_Status array_of_status[]);
    bool CheckIntraCDMsg(int target_id, MPI_Group &target_group);
#endif
    void Escalate(CDHandle *leaf, bool need_sync_to_reexec);

    CD *GetCoarseCD(CD* curr_cd);
}; // class CD ends


/**@class HeadCD 
 * @brief A representer of CD objects among the task group corresponding to the current CD.
 *        Head task which has HeadCD object has actual control over the non-head tasks which has CD object.
 *
 * \todo Implement serialize and deserialize of HeadCD object.
 */ 
class HeadCD : public CD {
  friend class Internal;
  friend class cd::CDHandle;
  friend class HandleEntrySearch;
  friend class HandleErrorOccurred;
  friend class RegenObject;   
//  friend class CDEntry;  
  friend class RecoverObject;
  friend class CD;
//    friend class HandleAllReexecute;
//    friend class HandleEntrySearch;
//    friend class HandleEntrySend;
  friend class CommLog;

  friend CDInternalErrT CD::InternalCreate(CDHandle *parent, 
                     const char *name, 
                     const CDID& child_cd_id, 
                     CDType cd_type, 
                     uint64_t sys_bit_vector, 
                     CDHandle** new_cd_handle);
#if CD_TEST_ENABLED
  friend class cd_test::Test;
#endif
  private:
    /// Link information of CD hierarchy   
    /// This is a kind of CD object but represents the corresponding task group of each CD
    /// Therefore, CDTree is generated with CD object. 
    /// parent CD object is always in the same process.
    /// But Head's parent CD object is in the Head process of parent CD.
    /// CDHandle cd_parent_ should be an accessor for this Head process.
    /// Regarding children CDs, 
    /// this Head's CD has actual copy of the Head children's CD object.
    /// If children CD is gone, this Head CD sends children CD.
    /// So, when we create CDs, 
    /// we should send Head CDHandle and its CD to its parent CD
    std::list<CDHandle *> cd_children_;

    TableStore<RemoteCDEntry> remote_entry_table_;

    CDHandle *cd_parent_;

//    std::map<ENTRY_TAG_T, CommInfo> entry_search_req_;
    // event related

    virtual CDEntry *GetEntry(ENTRY_TAG_T entry_name, uint16_t attr);
    bool error_occurred;
//    bool need_reexec;


    HeadCD();
    HeadCD(CDHandle *cd_parent, 
             const char *name, 
             const CDID& cd_id, 
             CDType cd_type, 
             PrvMediumT prv_medium, 
             uint64_t sys_bit_vector);
    virtual ~HeadCD();

    CDHandle *cd_parent(void);
    void set_cd_parent(CDHandle *cd_parent);
    virtual CDHandle *Create(CDHandle *parent, 
                             const char *name, 
                             const CDID& child_cd_id, 
                             CDType cd_type, 
                             uint64_t sys_bit_vector, 
                             CDInternalErrT* cd_err=0);
    virtual CDErrT Destroy(bool collective=true, bool need_destroy=false);
    virtual CDErrT Reexecute(void);
    virtual CDErrT Stop(CDHandle *cdh=NULL);
    virtual CDErrT Resume(void); // Does this make any sense?
    virtual CDErrT AddChild(CDHandle *cd_child); 
    virtual CDErrT RemoveChild(CDHandle *cd_child); 

    void DeserializeRemoteEntryDir(void *object, uint64_t totsize);
    virtual void DeleteEntryDirectory(void);

    void *Serialize(uint64_t &len_in_bytes);
    void Deserialize(void *object);
#if CD_MPI_ENABLED
//    virtual CDFlagT *event_flag();
    inline CDFlagT *GetEventFlag(void);
    CDErrT SetMailBox(const CDEventT &event, int task_id);
    CDInternalErrT LocalSetMailBox(const CDEventT &event);
//    virtual CDErrT SetMailBox(const CDEventT &event);
    CDEventHandleT ReadMailBox(CDFlagT *p_event, int idx=0);
    virtual CDInternalErrT InternalCheckMailBox(void);

//    CDInternalErrT InvokeErrorHandler(void);
//    virtual bool TestComm(bool test_untile_done=false);
#endif    
};

  } // namespace internal ends
uint64_t GetCDEntryID(const std::string &str);
const char *GetCDEntryStr(uint64_t id);
} // namespace cd ends
#endif
