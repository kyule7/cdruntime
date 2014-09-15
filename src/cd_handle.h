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
#include "cd_name_t.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <array>
#include <setjmp.h>
#include <ucontext.h>

#if _MPI_VER
#include <mpi.h>
#define _SINGLE_VER 0
#elif _PGAS_VER
//#include "cd_pgas.h"
#define _SINGLE_VER 0
#else
#define _SINGLE_VER 1
#endif

//#if _MPI_VER
//#include "cd_mpi.h"
//#define _SINGLE_VER 0
//#elif _PGAS_VER
//#include "cd_pgas.h"
//#define _SINGLE_VER 0
//#else
//#define _SINGLE_VER 1
//#endif


#if _PROFILER
#include "sight.h"
#endif


// CDHandle is a global accessor to the CD object. 
// CDHandle must be valid regardless of MPI rank or threads
// That means that once I know CDHandle object, 
// I can access to the ptr_cd_ anyhow.
// Sometimes, CDHandle::node_id_.second and CDID::node_id_.second can be different. 
// (If node_id_.first is different, that means there is something wrong!)
// That means that CD object was newly given from somewhere,
// and in that case, we should be careful of synchorizing the CD object to CDHandle
using namespace cd;

#if _PROFILER
using namespace sight;
using namespace sight::structure;
#endif

class cd::CDHandle {
  friend class cd::RegenObject;
  friend class cd::CD;

#if _PROFILER

    std::pair<std::string, int> label_;
    uint64_t  sibling_id_;
    uint64_t  level_;
  
    /// Profile-related meta data
    std::map<std::string, std::array<uint64_t, MAX_PROFILE_DATA>> profile_data_;
    bool     is_child_destroyed;
    bool     collect_profile_;
    bool usr_profile_enable;
//    std::vector<std::pair<std::string, long>>  usr_profile;

#if _ENABLE_HIERGRAPH
    HG_context usr_profile_input;
    HG_context usr_profile_output;
#endif


#if _ENABLE_MODULE
    context usr_profile_input;
    context usr_profile_output;
#endif

    /// Timer-related meta data
    uint64_t this_point_;
    uint64_t that_point_;
 

 
    /// sight-related member data
#if _ENABLE_MODULE
    /// All modules that are currently live
    static std::list<module*>    mStack;
    static modularApp*           ma;
#endif

#if _ENABLE_HIERGRAPH
    static std::list<hierGraph*> hgStack;
    static hierGraphApp*         hga;
#endif

#if _ENABLE_SCOPE
    /// All scopes that are currently live
    static std::list<scope*>     sStack;
    static graph*                scopeGraph;
#endif

#if _ENABLE_ATTR
      static attrIf*               attrScope;
      static std::list<attrAnd*>   attrStack;
      static std::list<attr*>      attrValStack;
#endif

#if _ENABLE_CDNODE
    /// All CD Nodes that are currently live
    static std::list<CDNode*>    cdStack;
#endif

#if _ENABLE_COMP
    static std::vector<comparison*> compTagStack;
    
//    static std::vector<int> compKeyStack;
//    static std::list<comparison*> compStack;
#endif

    void InitProfile(std::string label="INITIAL_LABEL");

    void GetLocalAvg(void);
    void GetPrvData(void *data, 
                    uint64_t len_in_bytes,
                    uint32_t preserve_mask, 
                    const char *my_name, 
                    const char *ref_name, 
                    uint64_t ref_offset, 
                    const RegenObject * regen_object, 
                    PreserveUseT data_usage);
   
    void SetUsrProfileInput(std::initializer_list<std::pair<std::string, long>> name_list);
    void SetUsrProfileInput(std::pair<std::string, long> name_list);
    void SetUsrProfileOutput(std::initializer_list<std::pair<std::string, long>> name_list);
    void SetUsrProfileOutput(std::pair<std::string, long> name_list);
    void AddUsrProfile(std::string key, long val, int mode);
   
    // FIXME
    bool CheckCollectProfile(void);
    void SetCollectProfile(bool flag);
   
    void StartProfile(void);
    void FinishProfile(void);

#if _ENABLE_CDNODE
    void CreateCDNode(void);
    void DestroyCDNode(void);
#endif
#if _ENABLE_SCOPE
    void CreateScope(void);
    void DestroyScope(void);
#endif
#if _ENABLE_ATTR
    void CreateAttr(void);
    void DestroyAttr(void);
#endif
#if _ENABLE_MODULE
    void CreateModule(void);
    void DestroyModule(void);
#endif
#if _ENABLE_HIERGRAPH
    void CreateHierGraph(void);
    void DestroyHierGraph(void);
#endif
#if _ENABLE_COMP
    void CreateComparison(void);
    void DestroyComparison(void);
#endif
  
  public:
    void InitViz();
    void FinalizeViz(void);
#endif

  private:
    CD*  ptr_cd_;
    NodeID node_id_;
//    int  master_; 
    bool IsMaster_; 
  public:


    //TODO copy these to CD async 
    ucontext_t ctxt_;
    jmp_buf    jump_buffer_;
  
    CDHandle();

    CDHandle( CDHandle* parent, 
              const char* name, 
              const NodeID& node_id, 
              CDType cd_type, 
              uint64_t sys_bit_vector);
    CDHandle( CDHandle* parent, 
              const char* name, 
              NodeID&& node_id, 
              CDType cd_type, 
              uint64_t sys_bit_vector);

    void Init(CD* ptr_cd, NodeID node_id); 
   ~CDHandle(); 

  // API List 0.1
  // Should Create/Destroy, Begin/Complete be static??

    // Non-collective
    CDHandle* Create (const char* name=0, 
                      CDType type=kStrict, 
                      uint32_t error_name_mask=0, 
                      uint32_t error_loc_mask=0, 
                      CDErrT *error=0 );
  
    // Collective
    CDHandle* Create (int color, 
                      uint32_t num_tasks_in_color, 
                      const char* name=0, 
                      CDType type=kStrict, 
                      uint32_t error_name_mask=0, 
                      uint32_t error_loc_mask=0, 
                      CDErrT *error=0 );
     // Collective
    CDHandle* Create (uint32_t  numchildren,
                      const char* name, 
                      CDType type=kStrict, 
                      uint32_t error_name_mask=0, 
                      uint32_t error_loc_mask=0, 
                      CDErrT *error=0 );
 
    CDHandle* CreateAndBegin (uint32_t color=0, 
                              uint32_t num_tasks_in_color=0, 
                              const char* name=0, 
                              CDType type=kStrict, 
                              uint32_t error_name_mask=0, 
                              uint32_t error_loc_mask=0, 
                              CDErrT *error=0 );

    CDErrT Destroy (bool collective=false);
    
    void SetColorAndTask(NodeID& new_node, const int& numchildren);
    
    CDErrT Begin   (bool collective=true, 
                    const char* label=0);
    CDErrT Complete(bool collective=true, 
                    bool update_preservations=false);
  
    CDErrT Preserve ( void *data_ptr=0, 
                      uint64_t len=0, 
                      CDPreserveT preserve_mask=kCopy, 
                      const char *my_name=0, 
                      const char *ref_name=0, 
                      uint64_t ref_offset=0, 
                      const RegenObject *regen_object=0, 
                      PreserveUseT data_usage=kUnsure );
  
    CDErrT Preserve ( CDEvent &cd_event, 
                      void *data_ptr=0, 
                      uint64_t len=0, 
                      CDPreserveT preserve_mask=kCopy, 
                      const char *my_name=0, 
                      const char *ref_name=0, 
                      uint64_t ref_offset=0, 
                      const RegenObject *regen_object=0, 
                      PreserveUseT data_usage=kUnsure );
  
//    CDErrT Preserve ( void *data_ptr, 
//                      uint64_t len, 
//                      uint32_t preserve_mask=kCopy, 
//                      const char *ref_name=0, 
//                      uint64_t ref_offset=0, 
//                      const Regen *regen_object=0);
//    CDErrT Preserve ( CDEvent &cd_event, 
//                      void *data_ptr, 
//                      uint64_t len, 
//                      uint32_t preserve_mask=kCopy, 
//                      const char *ref_name=0, 
//                      uint64_t ref_offset=0, 
//                      const Regen *regen_object=0);

// if Regen were to registered from a remote node to a actual CD Object, 
// it will need to serialize the Regen object and then finally send the object wait... 
// we can't send the "binary" to the remote node.. this will be too much to support... 
// so we have to always assume this preservation happens local...  
// Basically preserve function will get called from local..
  
    CDErrT CDAssert( bool test_true, 
                     const SysErrT *error_to_report=0);

    CDErrT CDAssertFail( bool test_true, 
                         const SysErrT *error_to_report=0);

    CDErrT CDAssertNotify( bool test_true, 
                           const SysErrT *error_to_report=0);
  
    std::vector< SysErrT > Detect (CDErrT *err_ret_val=0);

    CDErrT RegisterDetection( uint32_t system_name_mask, 
                              uint32_t system_loc_mask);

    CDErrT RegisterRecovery( uint32_t error_name_mask, 
                             uint32_t error_loc_mask, 
                             RecoverObject *recover_object=0);
  
    CDErrT RegisterRecovery( uint32_t error_name_mask, 
                             uint32_t error_loc_mask, 
                             CDErrT(*recovery_func)(std::vector< SysErrT > errors)=0);

    float GetErrorProbability (SysErrT error_type, 
                               uint32_t error_num);

    float RequireErrorProbability ( SysErrT error_type, 
                                    uint32_t error_num, 
                                    float probability, 
                                    bool fail_over=true );
    virtual void Recover (uint32_t error_name_mask, 
                          uint32_t error_loc_mask, 
                          std::vector< SysErrT > errors);

    CDErrT SetPGASType (void *data_ptr, 
                        uint64_t len, 
                        CDPGASUsageT region_type=kShared);
  
    void CommitPreserveBuff(void);
  protected:  // Internal use -------------------------------------------------------------
    cd::CDEntry* InternalGetEntry(std::string entry_name);
    void InternalReexecute (void);
    void InternalEscalate ( uint32_t error_name_mask, 
                            uint32_t error_loc_mask, 
                            std::vector< SysErrT > errors);

    // It returns sibling ID
    // type can be just int because color and task are int type
    CDErrT AddChild(CDHandle* cd_child);
    CDErrT RemoveChild(CDHandle* cd_child);  

    CDErrT Stop(void);
  
    /// Synchronize the CD object in every task of that CD.
    CDErrT Sync(void);

    uint64_t SetSystemBitVector(uint64_t error_name_mask, uint64_t error_loc_mask);
  private:
    int SplitCD(int my_size, int num_children, int& new_color, int& new_task);
    CDErrT GetNewNodeID(NodeID& new_node);
    CDErrT GetNewNodeID(int my_color, int new_color, int new_task, NodeID& new_node);

  public:
    /// Do we need this?
    bool IsLocalObject(void);

    /// Check if this CD object (*ptr_cd_) is the MASTER CD object
    bool IsMaster(void);
    void SetMaster(int task);

    // Accessors
//    CDHandle* GetParent(void);
    CD*       ptr_cd(void); 
    NodeID&   node_id(void);  
    void      SetCD(CD* ptr_cd);
    char*     GetName(void); 
    int       GetSeqID(void);
    int&      GetNodeID(void);
    int       GetTaskID(void);
    int       GetTaskSize(void);
    int       ctxt_prv_mode(void);
    bool      operator==(const CDHandle &other) const ;



};


#endif
