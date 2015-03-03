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

#ifndef _CD_HANDLE_H 
#define _CD_HANDLE_H
#include "cd_global.h"
#include "node_id.h"
#include "sys_err_t.h"
#include "cd_name_t.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <array>
#include <setjmp.h>
#include <ucontext.h>
#include <functional>

#if _PROFILER
#include "cd_profiler.h"
#endif

// CDHandle is a global accessor to the CD object. 
// CDHandle must be valid regardless of MPI rank or threads
// That means that once I know CDHandle object, 
// I can access to the ptr_cd_ anyhow.
// Sometimes, CDHandle::node_id_.second and CDID::node_id_.second can be different. 
// (If node_id_.first is different, that means there is something wrong!)
// That means that CD object was newly given from somewhere,
// and in that case, we should be careful of synchorizing the CD object to CDHandle
// 09.22.2014 Kyushick: The above comment is written long time ago, so may be confusing to understand the current design.

using namespace cd;

class cd::CDHandle {
  friend class cd::RegenObject;
  friend class cd::CD;
  friend class cd::CDEntry;
  private:
    CD*    ptr_cd_;
    NodeID node_id_;

    // task ID of head in this CD.
    // If current task is head of current CD, than it is the same as node_id_.task_


  public:
//#if _MPI_VER
//#if _KL
//    // This flag is unique for each process. 
//    static CDFlagT *pendingFlag_;
//    CDMailBoxT pendingWindow_;
//#endif
//#endif

#if _PROFILER 
    Profiler* profiler_;
#endif

    //TODO copy these to CD async 
    ucontext_t ctxt_;
    jmp_buf    jmp_buffer_;
    int jmp_val_;
    // Default Constructor
    CDHandle();

    // Constructor
    CDHandle(CD* ptr_cd, const NodeID& node_id);

   ~CDHandle(); 

    // Non-collective
    CDHandle* Create(const char* name=0, 
                     CDType cd_type=kStrict, 
                     uint32_t error_name_mask=0, 
                     uint32_t error_loc_mask=0, 
                     CDErrT *error=0 );
  
    // Collective
    CDHandle* Create(const ColorT& color, 
                     uint32_t num_tasks_in_color, 
                     const char* name=0, 
                     CDType cd_type=kStrict, 
                     uint32_t error_name_mask=0, 
                     uint32_t error_loc_mask=0, 
                     CDErrT *error=0 );
    // Collective
    CDHandle* Create(uint32_t  numchildren,
                     const char* name, 
                     CDType cd_type=kStrict, 
                     uint32_t error_name_mask=0, 
                     uint32_t error_loc_mask=0, 
                     CDErrT *error=0 );
 
    // Collective
    CDHandle* CreateAndBegin(const ColorT& color, 
                             uint32_t num_tasks_in_color=0, 
                             const char* name=0, 
                             CDType cd_type=kStrict, 
                             uint32_t error_name_mask=0, 
                             uint32_t error_loc_mask=0, 
                             CDErrT *error=0 );

    CDErrT Destroy(bool collective=false);
    
    CDErrT Begin(bool collective=true, 
                 const char* label=0);

    CDErrT Complete(bool collective=true, 
                    bool update_preservations=false);
  
    // Non-collective
    CDErrT Preserve(void *data_ptr=0, 
                    uint64_t len=0, 
                    uint32_t preserve_mask=kCopy, 
                    const char *my_name=0, 
                    const char *ref_name=0, 
                    uint64_t ref_offset=0,
                    const RegenObject *regen_object=0, 
                    PreserveUseT data_usage=kUnsure );
    // Collective (not implemented yet)
    CDErrT Preserve(CDEvent &cd_event, 
                    void *data_ptr=0, 
                    uint64_t len=0, 
                    uint32_t preserve_mask=kCopy, 
                    const char *my_name=0, 
                    const char *ref_name=0, 
                    uint64_t ref_offset=0, 
                    const RegenObject *regen_object=0, 
                    PreserveUseT data_usage=kUnsure );
  
// if Regen were to registered from a remote node to a actual CD Object, 
// it will need to serialize the Regen object and then finally send the object wait... 
// we can't send the "binary" to the remote node.. this will be too much to support... 
// so we have to always assume this preservation happens local...  
// Basically preserve function will get called from local..
  
    CDErrT CDAssert(bool test_true, 
                    const SysErrT *error_to_report=0);

    CDErrT CDAssertFail(bool test_true, 
                        const SysErrT *error_to_report=0);

    CDErrT CDAssertNotify(bool test_true, 
                          const SysErrT *error_to_report=0);
  
    std::vector<SysErrT> Detect(CDErrT *err_ret_val=0);

    CDErrT RegisterDetection(uint32_t system_name_mask, 
                             uint32_t system_loc_mask);

    CDErrT RegisterRecovery(uint32_t error_name_mask, 
                            uint32_t error_loc_mask, 
                            RecoverObject *recover_object=0);
  
    CDErrT RegisterRecovery(uint32_t error_name_mask, 
                            uint32_t error_loc_mask, 
                            CDErrT(*recovery_func)(std::vector< SysErrT > errors)=0);

    float GetErrorProbability(SysErrT error_type, 
                              uint32_t error_num);

    float RequireErrorProbability(SysErrT error_type, 
                                  uint32_t error_num, 
                                  float probability, 
                                  bool fail_over=true );
    void Recover(uint32_t error_name_mask, 
                 uint32_t error_loc_mask, 
                 std::vector< SysErrT > errors);

//    CDErrT SetPGASType(void *data_ptr, 
//                       uint64_t len, 
//                       CDPGASUsageT region_type=kShared);
  
    void CommitPreserveBuff(void);
//    Tag GenTag(const char* tag);
    char *GenTag(const char* tag);
  private:  // Internal use -------------------------------------------------------------
    // Initialize for CDHandle object.
    void Init(CD* ptr_cd, const NodeID& node_id);

    // Search CDEntry with entry_name given. It is needed when its children preserve data via reference and search through its ancestors. If it cannot find in current CD object, it outputs NULL 
    cd::CDEntry* InternalGetEntry(std::string entry_name);

    
//    void InternalReexecute (void);
//    void InternalEscalate ( uint32_t error_name_mask, 
//                            uint32_t error_loc_mask, 
//                            std::vector< SysErrT > errors);

    // Add children CD to my CD
    CDErrT AddChild(CDHandle* cd_child);

    // Delete a child CD in my children CD list
    CDErrT RemoveChild(CDHandle* cd_child);  
    
    // Stop every task in this CD
    CDErrT Stop(void);
  
    /// Synchronize every task of this CD.
    CDErrT Sync(void);

    // Set bit vector for error types to handle in this CD.
    uint64_t SetSystemBitVector(uint64_t error_name_mask, 
                                uint64_t error_loc_mask);

    // Split CD to create children CD. So, it is basically re-indexing for task group for children CDs. By default it splits the task group of this CD in three dimension.
    // # of children to split
    // Current task ID (NodeID::task_in_level_)
    // Current size of task group (NodeID::size_) 
    // are required to split.
    // It populates new_color, new_task appropriately.
    // By default it splits the task group of this CD in three dimension.
    // This default split function is defined in cd_handle.cc (SplitCD_3D())
    void RegisterSplitMethod(std::function<int(const int& my_task_id,
                                               const int& my_size,
                                               const int& num_children,
                                               int& new_color, 
                                               int& new_task)> split_func);

    // function object that will be set to some appropriate split strategy
    std::function<int(const int& my_task_id, 
                      const int& my_size, 
                      const int& num_children, 
                      int& new_color, 
                      int& new_task)> SplitCD;

    // Get NodeID with given new_color and new_task
    CDErrT GetNewNodeID(NodeID& new_node);
    CDErrT GetNewNodeID(const ColorT& my_color, 
                        const int& new_color, 
                        const int& new_task, 
                        NodeID& new_node);

    /// Do we need this?
    bool IsLocalObject(void);

    // Select Head among task group that are corresponding to one CD.
    void SetHead(NodeID& new_node_id);

    CDErrT CheckMailBox(void);
    CDErrT SetMailBox(CDEventT &event);
//    int InternalCheckMailBox(void);
//    int SetMailBox(CDEventT &event);
//    CDEventHandleT HandleEvent(CDFlagT *event, int idx=0);
//    int ReadMailBox(void);
//    int ReadMailBoxFromRemote(void);

    // Communication routines in CD runtime
    //void CollectHeadInfoAndEntry(void); 
    void CollectHeadInfoAndEntry(const NodeID &new_node_id);


  public:
    bool IsHead(void) const;
    // Accessors
    CD*       ptr_cd(void) const; 
    NodeID&   node_id(void) ;  
    void      SetCD(CD* ptr_cd);
    char*     GetName(void) const; 
    int       GetSeqID(void) const;
    ColorT    GetNodeID(void) const;
    int       GetTaskID(void) const;
    int       GetTaskSize(void) const;
    int       ctxt_prv_mode(void);
    bool      operator==(const CDHandle &other) const ;



};


#endif
