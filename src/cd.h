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
#include "cd_global.h"
#include "cd_handle.h"
#include "cd_entry.h"
#include "util.h"
#include "cd_id.h"

#if _WORK
#include "transaction.h"
#endif

#include <list>
#include <vector>
#include <stdint.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <setjmp.h>
#include <ucontext.h>
#include <map>
#include <unordered_map>
//#include <array>

//class cd::CDEntry;
//class cd::CDHandle;
using namespace cd;

/// TODO Implement serialize and deserialize of this instance
class cd::CD {
    /// The friends of CD class
    friend class cd::RegenObject;   
    friend class cd::CDEntry;   
  public:
    /// CD class internal enumerators 
    enum CDExecMode  {kExecution=0, kReexecution, kSuspension};
    enum CtxtPrvMode {kExcludeStack=0, kIncludeStack};
//    enum CDErrT { kOK=0, kExecutionModeError };  
  protected: 
    CDID cd_id_;
  public:
    std::vector<std::string> label_;
    char*           name_;
    CDType          cd_type_;

    /// Set rollback point and options
    CtxtPrvMode     ctxt_prv_mode_;
    ucontext_t       ctxt_;
    jmp_buf         jump_buffer_;
    uint64_t        option_save_context_; 

    /// Flag for normal execution or reexecution.
    /// TODO: What if the pointer of the object has changed since it was created? 
    /// This could be desaster if we are just relying on the address of the pointer to access CD object.
    CDExecMode      cd_exec_mode_;
    int             num_reexecution_;

    /// Detection-related meta data
    uint64_t        sys_detect_bit_vector_;
    std::list<int*> usr_detect_func_list_;  //custom_detect_func;
//     int (*AddDetectFunc)(void);

    /// FIXME later this should be map or list, 
    /// sometimes it might be better to have map structure 
    /// Sometimes list would be beneficial depending on how frequently search functionality is required.
    /// Here we define binary search tree structure... 
    /// Then insert only the ones who has ref names.
    std::list<CDEntry> entry_directory_; 
    
    // Only CDEntries that has refname will be pushed into this data structure for later quick search.
    std::map<std::string, CDEntry> entry_directory_map_;   
    
    /// This shall be used for re-execution. 
    /// We will restore the value one by one.
    /// Should not be CDEntry*. 
    std::list<CDEntry>::iterator iterator_entry_;   
  
    //  std::vector<cd_log> log_directory_;
    //  pthread_mutex_t mutex_;      //
    //  pthread_mutex_t log_directory_mutex_;

#if _WORK
  protected:
    bool _OpenHDD, _OpenSSD;
    Path path;
  public:
    tsn_log_struct HDDlog;
    tsn_log_struct SSDlog;
    void InitOpenHDD() { _OpenHDD = false; }
    void InitOpenSSD() { _OpenSSD = false; }
    bool isOpenHDD()   { return _OpenHDD;  }
    bool isOpenSSD()   { return _OpenSSD;  }
    void OpenHDD()     { _OpenHDD = true;  }
    void OpenSSD()     { _OpenSSD = true;  }
    void Body();
    PrvMediumT GetPlaceToPreserve(void);
#endif

  public:
    CD();

    CD(cd::CDHandle* cd_parent, 
       const char* name, 
       CDID cd_id, 
       CDType cd_type, 
       uint64_t sys_bit_vector);

    virtual ~CD();

    void Initialize(cd::CDHandle* cd_parent, 
                    const char* name, 
                    CDID cd_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector);
    void Init(void);
  
    CDID GetCDID() { return cd_id_; }

// TODO This should be moved to outside and inside the namespace cd
// Kyushick : Do we need this static Create method?
//    static CDHandle Create(CDType cd_type, CDHandle &parent);
//    static void Destroy(CD *cd);

    CD* Create(cd::CDHandle* cd_parent, 
               const char* name, 
               const CDID& cd_id, 
               CDType cd_type, 
               uint64_t sys_bit_vector, 
               CDErrT* cd_err);
    CDErrT Destroy();

    CDErrT Begin(bool collective=true, 
                 const char* label=0);
    CDErrT Complete(bool collective=true, 
                    bool update_preservations=true);


  /// Jinsuk: Really simple preservation function. 
  /// This will use default storage. 
  /// It can be set when creating this instance, 
  /// or later by calling set_ something function. 
  
    CDErrT Preserve(void *data,                   // address in the local process 
                    uint64_t len_in_bytes,        // data size to preserve
                    uint32_t preserve_mask=kCopy, // preservation method
                    const char *my_name=0,        // data name
                    const char *ref_name=0,       // reference name
                    uint64_t ref_offset=0,        // reference offset
                    const RegenObject * regen_object=0, // regen object
                    PreserveUseT data_usage=kUnsure);   // for optimization
  
    // Collective version
    // Kyushick : Why do we need collective Preserve call as an API?
    // I think we need collective method when CDs want to preserve data to Global file system
    // So user set some medium type
    CDErrT Preserve(CDEvent &cd_event,            // Event object to synch
                    void *data_ptr, 
                    uint64_t len, 
                    uint32_t preserve_mask=kCopy, 
                    const char *my_name=0, 
                    const char *ref_name=0, 
                    uint64_t ref_offset=0, 
                    const RegenObject *regen_object=0, 
                    PreserveUseT data_usage=kUnsure);
  
    CDErrT Detect(); 
    CDErrT Restore();
  
//  DISCUSS: Jinsuk: About longjmp setjmp: By running some experiement, 
//  I confirm that this only works when stack is just there. 
//  That means, if we want to jump to a function context which does not exist in stack anymore, 
//  you will eventually get segfault. 
//  Thus, CD begin and complete should be in the same function context,
//  or at least complete should be deper in the stack so that we can jump back to the existing stack pointer. 
//  I think that Just always pairing them in the same function is much cleaner. 
  
    CDErrT Assert(bool test);
  
    CDErrT Reexecute(void);
  
    // Utilities -----------------------------------------------------
    virtual CDErrT Stop(cd::CDHandle* cdh=NULL);
    virtual CDErrT Resume(void);
  
    CDErrT InternalPreserve(void *data, 
                                    uint64_t len_in_bytes,
                                    uint32_t preserve_mask, 
                                    const char *my_name, 
                                    const char *ref_name, 
                                    uint64_t ref_offset, 
                                    const RegenObject * regen_object, 
                                    PreserveUseT data_usage);
  
    // copy should happen for the part that is needed.. 
    // so serializing entire CDEntry class does not make sense. 
    CDEntry* InternalGetEntry(std::string entry_name); 
  
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
  

    void DeleteEntryDirectory(void);

    virtual CDErrT AddChild(cd::CDHandle* cd_child);
    virtual CDErrT RemoveChild(cd::CDHandle* cd_child);
    
 };


class cd::MasterCD : public cd::CD {
  public:
    /// Link information of CD hierarchy   
    /// This is important data for MASTER among sibling mpi ranks (not sibling CDs) of each CD 
    /// Therefore, CDTree is generated with CD object. 
    /// parent CD object is always in the same process.
    /// But MASTER's parent CD object is in the MASTER process of parent CD.
    /// CDHandle cd_parent_ should be an accessor for this MASTER process.
    /// Regarding children CDs, this MASTER's CD has actual copy of the MASTER children's CD object.
    /// If children CD is gone, this MASTER CD sends children CD.
    /// So, when we create CDs, we should send MASTER CDHandle and its CD to its parent CD
    std::list<cd::CDHandle*> cd_children_;
    cd::CDHandle*            cd_parent_;

    MasterCD();
    MasterCD(cd::CDHandle* cd_parent, 
             const char* name, 
             CDID cd_id, 
             CDType cd_type, 
             uint64_t sys_bit_vector);
    virtual ~MasterCD();

    virtual CDErrT Stop(cd::CDHandle* cdh=NULL);
    virtual CDErrT Resume(void); // Does this make any sense?
    virtual CDErrT AddChild(cd::CDHandle* cd_child); 
    virtual CDErrT RemoveChild(cd::CDHandle* cd_child); 
    cd::CDHandle* cd_parent(void);
    void    set_cd_parent(cd::CDHandle* cd_parent);
};

namespace cd {
//  extern CDHandle* GetCurrentCD(void);
//  extern CDHandle* GetRootCD(void);
//
//  extern CDHandle* CD_Init(int numproc, int myrank);
//  extern void CD_Finalize();
}

#endif
