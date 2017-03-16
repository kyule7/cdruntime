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

/**
 * @file cd_handle.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * \brief Containment Domains namespace for global functions, types, and main interface
 *
 * All user-visible CD API calls and definitions are under the CD namespace.
 * Internal CD API implementation components are under a separate cd_internal
 * namespace; the CDInternal class of namespace cd serves as an interface where necessary.
 *
 */

#include <ucontext.h>
#include <setjmp.h>
#include <functional>
#include "cd_features.h"
#include "cd_global.h"
#include "node_id.h"
#include "profiler_interface.h"
#include "error_injector.h"
#include "sys_err_t.h"
#include "serdes.h"

//#define FUNC_ATTR inline __attribute__((always_inline))
using namespace cd::interface;
using namespace cd::internal;


/**@addtogroup cd_init_funcs
 * User must exeplicitly initialize and finalize CD runtime 
 * by calling CD_Init() and CD_Finalize().
 * For MPI-version of CD runtime, 
 * CD_Init() should be called after MPI_Init(), and
 * CD_Finalize() should be called before MPI_Finalize().
 * In current version of CD runtime, user can explicitly control 
 * the medium of preservation by CD_Init() for root CD,
 * and CDHandle::Create() for non-root CDs.
 * @{
 */

namespace cd {
class SysErrT;
typedef int(*RecoveryFuncType)(std::vector<SysErrT> &);

/**@}
 * @addtogroup cd_init_funcs
 * @{
 */
/**@brief Initialize the CD runtime.
 * 
 * Creates all necessary CD runtime components and data structures,
 * initializes the CD runtime, and creates the root CD. 
 * These internal metadata are very important for CDs and 
 * managed separately from the user-level data structure.
 * At this point, the current CD is the root. 
 * The storage medium for preservation through CD runtime 
 * can be explicitly controlled by users.
 * This medium can be set per CD level, 
 * so users can set a specific medium at CD_Init() or CDHandle::Create().
 * If users want to set a specific filepath for preservations,
 * they can set it by defining environmetal variable (CD_PRV_FILEPATH)
 * before building CD-enabled executable. 
 * If it is not defined, the default filepath is 
 * {current location running the executable}/{medium_name_such_as_HDD}.
 * In initialization phase, it creates a directory for preservation files, and
 * this filepath is broadcasted to every process in SPMD program.
 *
 * `CD_Init()` __should only be called once per application.__
 *
 *
 * @return Returns a handle to the root CD. 
 * Returns `kOK` if successful, 
 * `AlreadyInit` if called more than once, 
 * and `kError` if initialization is unsuccessful for some reason.
 * The handle to the root is also registered in some globally
 * accessible variable so that it can be accessed by
 * cd::GetCurrentCD() and cd::GetRootCD().
 * 
 * @sa GetCurrentCD(), GetRootCD(), Internal::Initialize()
 */
CDHandle *CD_Init(int numTask=1, //!< [in] number of task. For single task version, 
                                 //!< this should be 1. Default is 1.
                  int myTask=0,  //!< [in] Task ID which is global for all tasks.
                                 //!< Default is 0.
                  PrvMediumT prv_medium=DEFAULT_MEDIUM //!< Preservation medium for root CD.
                                              //!< User can set the medium for root through CD_Init's argument.
                                              //!< Default is kDRAM.
                 );
  
/**@brief Finalize the CD runtime.
 *  
 * This call destroys root CD object, which indicates the termination of CD-enabled program.
 * In this finalization routine, CD-related global data structures are all destroyed. 
 * This call is recommended to be called before MPI_Finalize() in MPI-version of CD runtime.
 * 
 *
 * `CD_Finalize()` __should only be called once per application.__
 *
 * @return Returns nothing.
 *
 * @sa Internal::Finalize(), sec_example_test_hierarchy
 */
void CD_Finalize(void);
  

/** @} */ // End cd_init_funcs group ===========================================================================
} // namespace cd ends



/**@addtogroup cd_handle
 * @brief Interfaces that user need to know.
 * @{
 */
namespace cd {

/**@defgroup cd_split CD split interface
 * @brief Method to split CDs to children CDs.
 * @ingroup user_interfaces
 * @addtogroup cd_split
 * @{
 */
   /** @var typedef SplitFuncT
    *  @brief A function object type for splitting method to create children CDs.
    *  @param[in] my_task_id   Current task ID.
    *  @param[in] my_size      The number of task of the current CD. (original task group)
    *  @param[in] num_children The number of childrens. (new task groups)
    *  @param[out] new_color   New color (task group ID) generated from split method.
    *  @param[out] new_task    New task ID generated from split method.                   
    *  @typedef function pointer/object type for splitting CDs.
    */ 
    typedef std::function< int(const int& my_task_id,
                               const int& my_size,       
                               const int& num_children,  
                               int& new_color,           
                               int& new_task             
                               ) > SplitFuncT; 
 
/** @} */ // End cd_split and user_interfaces

/**@class CDHandle 
 * @brief The accessor class that provides a handle to a specific CD instance.
 *
 * This class is the interface that users will actuall use to express resiliency-related information.
 * All usage of CDs (other than \ref cd_init_funcs and \ref cd_accessor_funcs) is done by
 * utilizing a handle to a particular CD instance within the CD tree. 
 * The CDHandle provides an implementation- and
 * location-independent accessor for CD operation.
 *
 * __Most calls currently only have blocking versions.__ 
 */ 
class CDHandle {
/**@} */ // End cd_handle
  friend class internal::CD;
  friend class internal::HeadCD;
  friend class interface::Profiler;
  friend class RegenObject;
  friend CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium);
  friend void CD_Finalize(void);
#if CD_TEST_ENABLED
  friend class cd_test::Test;
#endif


#if CD_ERROR_INJECTION_ENABLED
  private:
    CDErrorInjector *cd_error_injector_;                //!< Error injector interface.
    static MemoryErrorInjector *memory_error_injector_; //!< Error injector interface.
  public:
    static SystemErrorInjector *system_error_injector_; //!< Error injector interface.
#endif
  private:
    CD    *ptr_cd_;       //!< Pointer to CD object which will not exposed to users.
    NodeID node_id_;      //!< NodeID contains the information to access to the task.
    SplitFuncT SplitCD;   //!<function object that will be set to some appropriate split strategy.
//    uint64_t count_;
//    uint64_t interval_;
//    uint64_t error_mask_;
//    bool     active_;
  public:
#if CD_PROFILER_ENABLED 
    Profiler* profiler_;  //!< Pointer to the profiling-related handler object.
                          //!< It will be valid when `CD_PROFILER_ENABLED` compile-time flag is on.
#endif
    int     jmp_val_;     //!< Temporary flag related to longjmp/setjmp
    jmp_buf jmp_buffer_;  //!< Temporary buffer related to longjmp/setjmp
    ucontext_t &ctxt_;    //!< Temporary buffer related to setcontext/getcontext
  private:
    CDHandle(void);       //!< Default constructor of CDHandle. 
    CDHandle(CD *ptr_cd); //!< Normally this constructor will be called when CD is created. 
                          //!<CDHandle pointer is returned when `CDHandle::Create()` is called.
   ~CDHandle(void);
  public:
//    struct Internal {
/**@addtogroup cd_hierarchy 
 * This Module is about functions and types to create CD hierarchy in the application.
 * There are three kinds of Create() method for CD hierarchy. 
 * One is non-collective manner, and the other two are collective calls.
 * Users can map each task to belong to a specific task group corresponding to a CD,
 * and also they can register a split method to easily map parent-level tasks to children-level tasks.
 * By default, 3 dimensionally splitting method is provided, so the number of tasks provided should be
 * n^3 (for example, 8, 27, 64, ...). In future, there will be more default split functions provided. 
 * And users can also define this split function and register it.
 * Another important point in this module is CD_Begin() and CD_Complete() pair.
 * Explanation about these are following.
 * 
 * @sa cd_split 
 * @{
 */
      
      /**@brief Non-collective Create (Single task)
       * 
       * Creates a new CD as a child of this CD. The new CD does
       * not begin until Begin() is called explicitly.
       * This version of Create() is intended to be called by only a
       * single task and the value of the returned handle explicitly
       * communicated between all tasks contained within the new child. An
       * alternate collective version is also provided. It is expected
       * that this non-collective version will be mostly used within a
       * single task or, at least, within a single process address space.
       *
       * @return Returns a pointer to the handle of the newly created
       * child CD.\n Returns 0 on an error (error code returned in a parameter).
       *
       */
       CDHandle *Create(const char *name=NO_LABEL, //!< [in] Optional user-specified
                                            //!< name that can be used to "re-create" the same CD object
                                            //!< if it was not destroyed yet; useful for resuing preserved
                                            //!< state in CD trees that are not loop based.
                        int cd_type=kDefaultCD, //!< [in] Strict or relaxed. 
                                                //!< User can also set preservation medium for children CDs 
                                                //!< when creating them. (ex. kStrict | kHDD)
                        uint32_t error_name_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                                    //!< should be able to recover from that error type.
                        uint32_t error_loc_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                                   //!< should be able to recover from that error type.
                        CDErrT *error=0 //!< [in,out] Pointer for error return value 
                                        //!< (kOK on success and kError on failure) 
                                        //!< no error value returned if error=0.
                        );
    
      /**
       * @brief Collective Create
       * 
       * Creates a new CD as a child of the current CD. 
       * The new CD does not begin until `CDHandle::Begin()` is called explicitly.
       * This version of Create() is intended to be called by all
       * tasks that will be contained in the new child CD. It functions as
       * a collective operation in a way that is analogous to
       * MPI_comm_split, but only those tasks that are contained in the
       * new child synchronize with one another.
       *
       * @return Returns a pointer to the handle of the newly created child CD.\n
       *         Returns 0 on an error.
       *
       */
       CDHandle *Create(uint32_t  numchildren,    //!< [in] The total number of CDs that 
                                                  //!< will be collectively created by the current CD object.
                                                  //!< This collective CDHandle::Create() waits for 
                                                  //!< all tasks in the current CD to arrive before creating new children.
                        const char *name,         //!< [in] Optional user-specified name that can be used 
                                                  //!< to "re-create" the same CD object
                                                  //!< if it was not destroyed yet; useful for resuing preserved state 
                                                  //!< in CD trees that are not loop based.
                        int cd_type=kDefaultCD,   //!< [in] Strict or relaxed. 
                                                  //!< User can also set preservation medium for children CDs 
                                                  //!< when creating them. (ex. kStrict | kDRAM)
                        uint32_t err_name_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                                  //!< should be able to recover from that error type.
                        uint32_t err_loc_mask=0,  //!< [in] each `1` in the mask indicates that this CD
                                                  //!< should be able to recover from that error type.
                        CDErrT *error=0           //!< [in,out] Pointer for error return value 
                                                  //!< (kOK on success and kError on failure) 
                                                  //!< no error value returned if error=0.
                        );
    
    
    
      
      /**
       * @brief Collective Create
       * 
       * Creates a new CD as a child of the current CD. 
       * This version of Create() intended to allow users to explicitly provide 
       * supgroup ID (color), task ID in a supgroup (task_in_color), and number of subgroup.
       * (See also \ref example_user_defined_hierarchy)
       *
       * The new CD does not begin until CDHandle::Begin() is called explicitly.
       * This API as a collective operation is analogous to MPI_comm_split, 
       * but only those tasks that are contained in the new child synchronize with one another.
       *
       * @return Returns a pointer to the handle of the newly created
       * child CD; returns 0 on an error.
       *
       */
       CDHandle *Create(uint32_t color,         //!< [in] The "color" of the new child to which this task will belong to.
                        uint32_t task_in_color, //!< [in] The total number of tasks that are collectively creating
                                                //!< the child numbered "color"; the collective waits for this number
                                                //!< of tasks to arrive before creating the child.
                        uint32_t  numchildren,  //!< [in] The total number of CDs that will be collectively created 
                                                //<! by the current CD object.
                                                //!< This collective CDHandle::Create() waits for all tasks 
                                                //!< in the current CD to arrive before creating new children.
                        const char *name,       //!< [in] Optional user-specified name that can be used to 
                                                //!< "re-create" the same CD object if it was not destroyed yet; 
                                                //!< useful for resuing preserved state in CD trees that are not loop based.
                        int cd_type=kDefaultCD, //!< [in] Strict or relaxed. 
                                                //!< User can also set preservation medium for children CDs 
                                                //!< when creating them. (ex. kStrict | kDRAM)
                        uint32_t err_name_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                                  //!< should be able to recover from that error type.
                        uint32_t err_loc_mask=0,  //!< [in] each `1` in the mask indicates that this CD
                                                  //!< should be able to recover from that error type.
                        CDErrT *error=0           //!< [in,out] Pointer for error return value 
                                                  //!< (kOK on success and kError on failure) 
                                                  //!< no error value returned if error=0.
                        );
    
     
      /**
       * @brief Collective Create and Begin
       *
       * Creates a new CD as a child of the current CD. The new CD then
       * immediately begins with a single collective call.
       * This version of is intended to be called by all
       * tasks that will be contained in the new child CD. It functions as
       * a collective operation in a way that is analogous to
       * MPI_comm_split, but only those tasks that are contained in the
       * new child synchronize with one another. To avoid unnecessary
       * collectives, CreateAndBegin() then immediately begins the new CD.
       *
       * @return Returns a pointer to the handle of the newly created
       * child CD; returns 0 on an error.
       *
       */
       CDHandle *CreateAndBegin(uint32_t numchildren, //!< [in] The total number of CDs that will be 
                                                      //!< collectively created by the current CD object.
                                                      //!< This collective CDHandle::Create() waits for 
                                                      //!< all tasks in the current CD to arrive before creating new children.
                                const char *name, //!< [in] Optional user-specified name that can be used to 
                                                  //!< "re-create" the same CD object if it was not destroyed yet; 
                                                  //!< useful for resuing preserved state in CD trees that are not loop based.
                                int cd_type=kDefaultCD, //!< [in] Strict or relaxed. 
                                                        //!< User can also set preservation medium for children CDs 
                                                        //!< when creating them. (ex. kStrict | kDRAM)
                                uint32_t error_name_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                                            //!< should be able to recover from that error type.
                                uint32_t error_loc_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                                           //!< should be able to recover from that error type.
           
                                CDErrT *error=0 //!< [in,out] Pointer for error return value 
                                                //!< (kOK on success and kError on failure) 
                                                //!< no error value returned if error=0.
                                );
    
      /** @brief Destroys a CD
       *
       * Destroys a CD by removing it from the CD tree and deleting all
       * its data structures. Once Destroy() is called, __additional
       * attempts to destroy the same CD instance may result in undefined
       * behavior__. Destroy() need not be a collective operation because
       * it typically comes after Complete(). However, a single task must
       * call `Destroy(cd_object=true)` while the rest call
       * `Destroy(cd_object=false)`. 
       *
       * @return May return kError if instance was already destroyed, but
       * may also return kOK in such a case.
       *
       */
       CDErrT Destroy(bool collective=false //!< [in] if `true`, destroy is done as a collective across all tasks that
                                            //!< created the CD; otherwise the behavior is that only one task destroys 
                                            //!< the actual object while the rest just delete the local CDHandle.
                     );
    
       CDErrT InternalDestroy(bool collective, bool need_destroy=false);

      /** @brief Begins a CD 
       *
       * Begins the CD and sets it as the current CD for the calling
       * task. If `collective=true` then the Begin() call is a collective
       * across all tasks that collectively created this CD. It is illegal
       * to call a collective Begin() on a CD that was created without a
       * collective Create(). If `collective=false`, the user is
       * responsible for first synchronizing all tasks contained within
       * this CD and then updating the current CD in each task using
       * cd::SetCurrentCD(). 
       *
       * __Important constraint: Begin() and Complete() must be called
       * from within the same program scope (i.e., same degree of scope
       * nesting).__
       *
       * @return Returns kOK when successful and kError otherwise.
       * @sa Complete()
       */
       CDErrT Begin(bool collective=true,//!< [in] Specifies whether this call is a collective across all tasks 
                                         //!< contained by this CD or whether its to be run by a single task 
                                         //!< only with the programmer responsible for synchronization. 
                    const char *label=NO_LABEL,
                    const uint64_t &sys_err_vec=0
                   )
       {
         assert(ptr_cd_);
         if(ctxt_prv_mode() == kExcludeStack) 
           setjmp(*jmp_buffer());
         else 
           getcontext(ctxt());
       
         CommitPreserveBuff();
         return InternalBegin(collective, label, sys_err_vec);
       }
  
    
      /** @brief Completes a CD 
       *
       * Completes the CD and sets its parent as the current CD for the calling
       * task. If `collective=true` then the Complete() call is a collective
       * across all tasks that collectively created this CD. It is illegal
       * to call a collective Complete() on a CD that was created without a
       * collective Create(). If `collective=false`, the user is
       * responsible for first synchronizing all tasks contained within
       * this CD and then updating the current CD in each task using
       * cd::SetCurrentCD(). 
       *
       * __Important constraint: Begin() and Complete() must be called
       * from within the same program scope (i.e., same degree of scope
       * nesting).__
       *
       * \warning The update preservation (advance) optimization semantics
       * may not yet be supported.
       *
       * @return Returns kOK when successful and kError otherwise.
       * @sa Begin()
       */
       CDErrT Complete(bool collective=true, //!< [in] Specifies whether
                                            //!< this call is a collective
                                            //!< across all tasks contained
                                            //!< by this CD or whether its to
                                            //!< be run by a single task only
                                            //!< with the programmer
                                            //!< responsible for
                                            //!< synchronization
                       bool update_preservations=false //!< [in] Flag that
                                                   //!< indicates whether preservation should be
                                                   //!< updated on Complete rather than discarding all 
                                                   //!< preserved state. If `true` then Complete
                                                   //!< followed by Begin can be much more efficient
                                                   //!< if the majority of preserved data overlaps
                                                   //!< between these two consecutive uses of the CD
                                                   //!< object (this enables the Cray CD
                                                   //!< AdvancePointInTime functionality).
                        );
    
      /** @brief Advances a CD 
       *   Advance() is the other way to marks a rollback point 
       *   by updating preservation entries, not deleting all, 
       *   then re-preservation all like Complete/Begin pair.
       *   Advance sematics are very appealing in
       *   some cases that child CD update a partial array iteratively,
       *   but parent CD can advance its rollback point by updating the
       *   preservation entries which is modified by children level.
       *
       * __Important constraint: Advance() should not be called at parent level
       * if child CD is still active.__
       *
       * @return Returns kOK when successful and kError otherwise.
       * @sa Begin(), Complete()
       */
       CDErrT Advance(bool collective=true  //!< [in] Specify whether this call is collective
                                            //!< across all tasks contained by this CD.
                      );
      /** @} */ // End cd_hierarchy ===================================================================
  
      /**@addtogroup preservation_funcs
       * The \ref preservation_funcs module contains all preservation/restoration related types and methods.
       * With CDs, a variety of preservation methods are supported, and they are configured in CDHandle::Preserve().
       * Users can preserve via reference, but it is the responsibility of users to make sure if the tag of preserving data 
       * is matched to the target preservation entry that is referred to.
       * If the tag is not matched, CD runtime will not be able to search for 
       * the right entry preserved via copy at the parent CD level. (Currently, this case cause calling internal assert call.)
       * 
       * __Users must make sure that all the data which are touched in a CD computation body are preserved.
       * The data are modified by computation, but not preserved for the original states, it will cause a critical problem.__
       * 
       * @{
       */
      
      /**@brief (Non-collective) Preserve data to be restored when recovering. 
       *        (typically reexecuting the CD from right after its Begin() call.)
       * 
       * Preserves application data so that it can be restored for correct
       * CD recovery (typically reexecution).
       *
       * Preserve() preserves data on the first execution of the CD
       * (`kExec`) but acts to restore data when a CD is reexecuted
       * (`kReexec`). Success is defined as successfully preserving or
       * restoring `len` bytes of contiguous data starting at address
       * `data_ptr`.
       *
       * In many cases there is more than one way to preserve data and the
       * best way to do depends on machine-specific characteristics. It is
       * therefore possible to call Preserve() with a set of possible
       * correct and legal preservation methods and have an autotuner
       * select the best one. This is done by setting the appropriate bits
       * in the `preserve_mask` parameter based on the constants defined
       * by PreserveMethodT.
       *
       * @return kOK on success and kError otherwise.
       *
       * \sa Complete()
       */
       CDErrT Preserve(void *data_ptr,                    //!< [in] pointer to data to be preserved;
                                                          //!< __currently must be in same address space
                                                          //!< as calling task, but will extend to PGAS fat pointers later 
                       uint64_t len,                      //!< [in] Length of preserved data (Bytes)
                       uint32_t preserve_mask=kCopy,      //!< [in] Allowed types of preservation 
                                                          //!< (e.g., kCopy|kRef|kRegen), default only via copy
                       const char *my_name=NO_LABEL,      //!< [in] Optional C-string representing the name of this
                                                          //!< preserved data for later preserve-via-reference
                       const char *ref_name=NO_LABEL,     //!< [in] Optional C-string representing
                                                          //!< a user-specified name that was set 
                                                          //!< by a previous preserve call at the parent;
                                                          //!< __Do we also need an offset into parent preservation?__
                       uint64_t ref_offset=0,             //!< [in] explicit offset within the named region at the other CD 
                                                          //!< (for restoration via reference)
                       const RegenObject *regen_object=0, //!< [in] optional user-specified function for
                                                          //!< regenerating values instead of restoring by copying
                       PreserveUseT data_usage=kUnsure    //!< [in] This flag is used
                                                          //!< to optimize consecutive Complete/Begin calls
                                                          //!< where there is significant overlap in
                                                          //!< preserved state that is unmodified (see Complete()).
                       );
    
    
      /**@brief (Non-collective) Preserve user-defined class object
       * 
       * User need to define which member variables will be de/serialized at constructor with serdes object in it.
       * This will be passed through Preserve().
       * In execution mode, serializer will be invoked in CD runtime side.
       * When failure happens, deserializer will be invoked to restore class object. 
       * To selectively preserve/restore user-defined class member variables, 
       * users might want to register those members of interest before passing through Preserve().
       * They can register them by serdes.RegisterTarget({MemberID0, MemberID1, ...}).
       * At Serdes interface module, it is explained more in detail with example. 
       *
       * @return kOK on success and kError otherwise.
       *
       * \sa Serdes, Complete()
       */
       CDErrT Preserve(Serializable &serdes,     //!< [in] Serdes object in user-defined class.
                                                      //!< This will be invoked in CD runtime
                                                      //!< to de/serialize class object
                       uint32_t preserve_mask=kCopy,  //!< [in] Allowed types of preservation 
                                                      //!< (e.g., kCopy|kRef|kRegen), default only via copy
                       const char *my_name=NO_LABEL,  //!< [in] Optional C-string representing the name of this
                                                      //!< preserved data for later preserve-via-reference
                       const char *ref_name=NO_LABEL, //!< [in] Optional C-string representing
                                                      //!< a user-specified name that was set 
                                                      //!< by a previous preserve call at the parent.; 
                                                      //!< __Do we also need an offset into parent preservation?__
                       uint64_t ref_offset=0,         //!< [in] explicit offset within the named region 
                                                      //!< at the other CD (for restoration via reference)
                       const RegenObject *regenObj=0, //!< [in] optional user-specified function for
                                                      //!< regenerating values instead of restoring by copying
                       PreserveUseT dataUsage=kUnsure //!< [in] This flag is used
                                                      //!< to optimize consecutive Complete/Begin calls
                                                      //!< where there is significant overlap in
                                                      //!< preserved state that is unmodified (see Complete()).
                       );
    
      /** @brief (Not supported yet) Non-blocking preserve data to be restored when recovering 
       * (typically reexecuting the CD from right after its Begin() call.
       * 
       * 
       * Preserves application data so that it can be restored for correct
       * CD recovery (typically reexecution).
       *
       * Preserve() preserves data on the first execution of the CD
       * (`kExec`) but acts to restore data when a CD is reexecuted
       * (`kReexec`). Success is defined as successfully preserving or
       * restoring `len` bytes of contiguous data starting at address
       * `data_ptr`.
       *
       * In many cases there is more than one way to preserve data and the
       * best way to do depends on machine-specific characteristics. It is
       * therefore possible to call Preserve() with a set of possible
       * correct and legal preservation methods and have an autotuner
       * select the best one. This is done by setting the appropriate bits
       * in the `preserve_mask` parameter based on the constants defined
       * by PreserveMethodT.
       *
       * __The CDEvent object__ will be initialized to wait on this
       * particular call if it is empty. If the CDEvent already contains
       * an event, then this new event is chained to it -- in this way,
       * the user can use a single CDEvent::Wait() call to wait on a
       * sequence of non-blocking calls.
       *
       * @return kOK on success and kError otherwise.
       *
       * \sa Complete()
       */
       CDErrT Preserve(CDEvent &cd_event, //!< [in,out] enqueue this call onto the cd_event
                       void *data_ptr, //!< [in] pointer to data to be preserved;
                                         //!< __currently must be in same address space as calling task, 
                                         //!< but will extend to PGAS fat pointers later
                       uint64_t len,   //!< [in] Length of preserved data (Bytes)
                       uint32_t preserve_mask=kCopy, //!< [in] Allowed types of preservation 
                                                     //!< (e.g.,kCopy|kRef|kRegen), default only via copy
                       const char *my_name=NO_LABEL, //!< [in] Optional C-string representing the name of this
                                              //!< preserved data for later preserve-via-reference
                       const char *ref_name=NO_LABEL, //!< [in] Optional C-string representing a user-specified name 
                                               //!< that was set by a previous preserve call at the parent.; 
                                               //!< __Do we also need an offset into parent preservation?__
                       uint64_t ref_offset=0, //!< [in] explicit offset within the named region at the other CD
                       const RegenObject *regen_object=0, //!< [in] optional user-specified function for
                                                          //!< regenerating values instead of restoring by copying
                       PreserveUseT data_usage=kUnsure //!< [in] This flag is used to optimize consecutive Complete/Begin calls
                                                       //!< where there is significant overlap in preserved state 
                                                       //!< that is unmodified (see Complete()).
                       );
   
   
     /** @} */ // End preservation_funcs ======================================================================
   
    
      // if Regen were to registered from a remote node to a actual CD Object, 
      // it will need to serialize the Regen object and then finally send the object wait... 
      // we can't send the "binary" to the remote node.. this will be too much to support... 
      // so we have to always assume this preservation happens local...  
      // Basically preserve function will get called from local..
      
      /** \addtogroup cd_detection 
       * @{
       */
     
      /** @brief User-provided detection function for failing a CD
       * 
       * A user may call CDAssert() at any time during CD execution to
       * assert correct execution behavior. If the test fails, the CD
       * fails and must recover. 
       *
       * The CD runtime implementation may choose whether the CD fails at the point
       * that the CDAssert() fails or whether the assertion failure is
       * registered but only acted upon during the CD detection phase.
       *
       * @return kOK when the assertion call completed successfully
       * (regardless of whether the test was true or false) and kError if
       * the action taken by the runtime on CDAssert() failure did not
       * succeed. 
       *
       */
       CDErrT CDAssert(bool test_true, //!< [in] Boolean to be asserted as true.
              const SysErrT* error_to_report=0 //!< [in,out] An optional error report that will be
                                               //!< used during recovery and for system diagnostics. 
                       );
   
     /** @brief User-provided detection function for failing a CD
      * 
      * A user may call CDAssertFail() at any time during CD execution to
      * assert correct execution behavior. If the test fails, the CD
      * fails and must recover. 
      *
      * CDAssertFail() fails immediately and calls recovery.
      *
      * @return kOK when the assertion call completed successfully
      * (regardless of whether the test was true or false) and kError if
      * the action taken by the runtime on CDAssert() failure did not
      * succeed. 
      *
      * \warning May not be implemented yet.
      */
       CDErrT CDAssertFail(bool test_true, //!< [in] Boolean to be asserted
              //!< as true.
              const SysErrT* error_to_report=0
              //!< [in,out] An optional error report that will be
              //!< used during recovery and for system
              //!< diagnostics. 
              );
   
     /** @brief User-provided detection function for failing a CD
      * 
      * A user may call CDAssert() at any time during CD execution to
      * assert correct execution behavior. If the test fails, the CD
      * fails and must recover. 
      *
      * CDAssertNotify() registers the assertion failure, which is
      * only acted upon during the CD detection phase.
      *
      * @return kOK when the assertion call completed successfully
      * (regardless of whether the test was true or false) and kError if
      * the action taken by the runtime on CDAssert() failure did not
      * succeed. 
      *
      */
       CDErrT CDAssertNotify(bool test_true, //!< [in] Boolean to be asserted
              //!< as true.
              const SysErrT* error_to_report=0
              //!< [in,out] An optional error report that will be
              //!< used during recovery and for system
              //!< diagnostics. 
              );
   
     
     /** @brief Check whether any errors occurred while CD the executed
      *
      * Only checks for those errors that the CD registered for, This
      * function is only used for those errors that are logged during
      * execution and not those that require immediate recovery.
      *
      * __This requires more thought and a more precise description.__
      *
      * @return any errors or failures detected during this CDs execution.
      */
       std::vector<SysErrT> Detect(CDErrT* err_ret_val=0 //!< [in,out] Pointer to a variable 
               //!<for optionally returning a CD runtime error code indicating some bug with Detect().
                                  );
     
       /** @} */ // End cd_detection group =========================================
//     }; // end Internal


  public:
   CDErrT InternalBegin(bool collective=true,//!< [in] Specifies whether this call is a collective across all tasks 
                                             //!< contained by this CD or whether its to be run by a single task 
                                             //!< only with the programmer responsible for synchronization. 
                        const char *label=NO_LABEL,
                        const uint64_t &sys_err_vec=0
                       );

   /** @ingroup cd_detection */
   /** @ingroup register_detection_recovery */
   /**
    *  @{
    */

   /** @brief Declare that this CD can detect certain errors/failures by user-defined detectors.
    *
    * The intent of this method is to specify to the autotuner that
    * detection is possible. This is needed in order to balance between
    * fine-grained and coarse-grained CDs and associated recovery.
    *
    * @return kOK on success.
    */
    CDErrT RegisterDetection(uint32_t system_name_mask, //!< [in] each `1` in
                                                        //!< the mask indicates that this CD
                                                        //!< should be able to detect any errors
                                                        //!< that are meaningful to the application
                                                        //!< (in the error type mask).
                             uint32_t system_loc_mask   //!< [in] each `1` in
                                                        //!< the mask indicates that this CD
                                                        //!< should be able to detect any errors
                                                        //!< that are meaningful to the application
                                                        //!< (in the error type mask).
                            );

   /** @brief Register that this CD can recover from certain errors/failures
    *
    * This method serves two purposes:
    *  It extends the specification of intent to recover provided in
    *  Create(). 
    *  It enables registering a customized recovery routine by
    *  inheriting from the RecoverObject class.
    *
    * @return kOK on success.
    *
    * \sa Create(), RecoverObject
    *
    *\todo Does registering recovery also imply turning on detection?
    *Or is that done purely through RequireErrorProbability()? 
    */

    CDErrT RegisterRecovery(uint32_t error_name_mask,   //!< [in] each `1` in
                                                        //!< the mask indicates that this CD
                                                        //!< should be able to recover from that
                                                        //!< error type.
                            uint32_t error_loc_mask,    //!< [in] each `1` in
                                                        //!< the mask indicates that this CD
                                                        //!< should be able to recover from that
                                                        //!< error type.
                            RecoverObject* recoverObj=0 //!< [in] pointer
                                                        //!< to an object that contains the customized
                                                        //!< recovery routine; if unspecified,
                                                        //!< default recovery is used.
                            );
  
    CDErrT RegisterRecovery(
             uint32_t error_name_mask, //!< [in] each `1` in
                                       //!< the mask indicates that this CD
                                       //!< should be able to recover from that
                                       //!< error type.
             uint32_t error_loc_mask,  //!< [in] each `1` in
                                       //!< the mask indicates that this CD
                                       //!< should be able to recover from that
                                       //!< error type.
             RecoveryFuncType recovery_func=0 
                                       //!< [in] function pointer for customized
                                       //!< recovery routine; if unspecified, default recovery is used.
             );
    /** \todo What about specifying leniant communication-related errors
     *  for relaxed-CDs context?
     */

    /** @} */ // Ends register_detection_recovery group


/**@addtogroup cd_detection */

   /** @brief Recover method to be specialized by inheriting and overloading
    *
    * Recover uses methods that are internal to the CD itself and should
    * only be called by customized recovery routines that inherit from
    * RecoverObject and modify the Recover() method.
    *
    */
    void Recover(uint32_t error_name_mask, 
                 uint32_t error_loc_mask, 
                 std::vector< SysErrT > errors);

/** @} */ // End cd_detection group =========================================

 
/**@addtogroup cd_split
 * @{
 */
   /**@brief Register a method to split CDs to create chidlren CDs. 
    * 
    * __SPMD programming model specific__
    * When CD runtime create children CDs, it is necessary to split the task group of the current CD,
    * then re-index each task in new task groups for children CDs. 
    * By default, it splits the task group of the current CD in three dimension.
    * In general, how many children group to split, 
    * how may tasks there are in current task group, current task ID are required.
    * With these three information, split method populates new color (task group ID), new task ID appropriately.
    *
    * @return Returns CD-related error code.
    * @param [in] function pointer or object to split CD.
    */
    CDErrT RegisterSplitMethod(SplitFuncT split_func);
/** @} */ // Ends cd_split

/** \addtogroup cd_error_probability Methods for Interacting with the CD Framework and Tuner
 *
 * @{
 */

   /** @brief Ask the CD framework to estimate error/fault rate.
    *
    * Each CD will experience a certain rate of failure/error for
    * different failure/error mechanisms. These rates depend on the
    * system, the duration of the CD, its memory footprint, etc. The CD
    * framework can estimate the expected rate of each fault mechanism
    * given its knowledge of the CD and system properties.
    *
    * @return probability that the queried number of error/failure of the type
    * queried will occur during the execution of this CD.
    *
    * \note Should this be some rate instead? Seems like it would be
    * easier for the programmer to deal with number and probability,
    * but is it?
    *
    * \todo Decide on rate vs. number+probability
    */
    float GetErrorProbability(SysErrT error_type, //!< [in] Type of
                //!error/failure
                //!queried
            uint32_t error_num //!< [in] Number of
                //!< errors/failures queried.
            );

   /** @brief Request the CD framework to reach a certain error/failure probability.
    *
    * Each CD will experience a certain rate of failure/error for
    * different failure/error mechanisms. These rates depend on the
    * system, the duration of the CD, its memory footprint, etc. The CD
    * framework may be able to apply automatic resilience techniques,
    * such as replication, to ensure certain errors/failures will be
    * tolerated (or simply) detected.
    *
    * @return probability that at least `error_num` errors/failurse of the type
    * queried will occur during the execution of this CD, _after_ the 
    * automatic techniques are applied. Should be less than or equal to
    * requested `probability` if successful.
    *
    * \note Should this be some rate instead? Seems like it would be
    * easier for the programmer to deal with number and probability,
    * but is it?
    *
    * \todo Decide on rate vs. number+probability
    *
    */
    float RequireErrorProbability(SysErrT error_type, //!< [in] Type of
          //!< error/failure
          //!< queried.
          uint32_t error_num, //!< [in] Number of
          //!< errors/failures queried.
          float probability, //!< [in] Requested
          //!< maximum probability of `num_errors` errors/failures
          //!< not being detected or even occurring during
          //!< CD execution.
          bool fail_over=true //!< [in] Should
          //!< redundancy be added just to detect the
          //!< specified  error type (`false`) or should
          //!< enough redundancy be added to tolerate the
          //!< error (fail-over/forward-error-correction/...)
          );

/** @} */ // End cd_error_probability group  =======================================


/**@addtogroup error_injector
 * @{
 */
   /**@brief Register memory error injection method into CD runtime system.
    *
    * This is registered once in a program generally, and will be invoked at Preserve().
    * Then, some location in the application data can be corrupted (manipulated) by CD runtime,
    * and application will perform its computation with this corrupted value.
    * The wrong results from wrong inputs will be supposed to be detected at the end of CD.
    * This error injection is used for development phase, not in production run.
    * 
    * So, ERROR_INJECTION_ENABLED=0 to turn off this error injection functinality.
    * Even though there is some code lines regarding error injection, 
    * it will not affect any errors with this compile-time flag off.
    *
    * (See \ref sec_example_error_injection)
    *
    * @param [in] created memory error injector object.
    */

#if CD_ERROR_INJECTION_ENABLED
    void RegisterMemoryErrorInjector(MemoryErrorInjector *memory_error_injector);
#else
    void RegisterMemoryErrorInjector(MemoryErrorInjector *memory_error_injector) {}
#endif

/**@brief Register error injection method into CD runtime system.
 * 
 * This is registered per CDHandle::Create(),
 * otherwise error injection information will be passed from parent. 
 * (Just task list to fail, not CD list to fail. 
 * It is because CD hierarchy could be changed by CDHandle::Create(), 
 * but task ID (mpi rank in MPI version) will be invariant.)
 * 
 * This error injection is used for development phase, not in production run.
 * So, ERROR_INJECTION_ENABLED=0 to turn off this error injection functinality.
 * Even though there is some code lines regarding error injection, 
 * it will not affect any errors with this compile-time flag off.
 * -\ref sec_example_error_injection
 * @param [in] newly created error injector object.
 */
#if CD_ERROR_INJECTION_ENABLED
    void RegisterErrorInjector(CDErrorInjector *cd_error_injector);
#else
    void RegisterErrorInjector(CDErrorInjector *cd_error_injector) {}
#endif

 /** @} */ // Ends cd_split


#if CD_PGAS_ENABLED
/** \addtogroup PGAS_funcs
 *
 * @{
 */
   /** @brief Declare how a region of memory behaves within this CD (for Relaxed CDs) 
    *
    * Declare the behavior of a region of PGAS/GAS memory within this
    *  CD to minimize logging/tracking overhead. Ideally, only those
    *  memory accesses that really are used to communicate between this
    *  relaxed CD and another relaxed CD are logged/tracked.
    *
    * \warning For now, we are using an address to mark the type, but it is
    * quite possible that using actual types and casting is
    * better. Unfortunately types cannot be done through an API
    * interface and require a change to the language. It is not clear
    * how much overhead will be saved through just this API technique
    * and we will explore language changes (see discussion below).
    *
    * In the UPC++ implementation, this API call should not be used, and a cast from 
    * `shared_array` to `privatized_array` (or `shared_var` to `privatized_var`) is
    * preferred. UPC++ implementation should be quite straight-forward
    *
    * \todo Do we want to expose explicit logging functions?
    *
    * Just in UPC runtime, perhaps cram a field that says log
    * vs. unlogged into the pointer representation (steal one
    * bit). That we can perhaps do just in the runtime. A problem is
    * that all pointer operations (e.g., comparisons) need to know
    * about this bit.
    * 
    * If adding to the compiler, then should be done at same point as
    * strict and relaxed are done.
    *
    * There is also a pragma that can be used for changing the default
    * behavior from shared to privatized (assuming that all or at least
    * vast majority of accesses) within a code block are such. This
    * might be easier than casting.
    */
    CDErrT SetPGASType(void *data_ptr,    //!< [in] pointer to data to be "Typed";
                                          //!< __currently must be in same address space
                                          //!< as calling task, but will extend to
                                          //!< PGAS fat pointers later  
            uint64_t len,                 //!< [in] Length of preserved data (Bytes)
            PGASUsageT regionType=kShared //!< [in] How is this memory range used.
                                          //!< (shared for comm or not?)
            ) { return kOK; }
  
   /** \brief Simplify optimization of discarding relaxed CD log entries
    *
    * When using relaxed CDs, the CD runtime may log all communication
    * with tasks that are in a different CD context. While privatizing
    * some accesses reduces logging volume, all logged entries must
    * still be propagated and preserved up the CD tree for distributed
    * recovery. Log entries may be discarded when both the task that
    * produced the data and the task that logged it are in the same CD
    * (after descendant CDs complete and "merge"). This can always be
    * guaranteed when the least-common strict ancestor is reached, but
    * may happen sooner. In order to identify the earliest opportunity
    * to discard a log entry, the CD runtime must track producers,
    * which is impractical in general. In the specific and common
    * scenario of "owner computes", however, it is possible to track
    * the producer with low overhead. The SetPGASOwnerWrites() method i
    * used to indicate this behavior.
    *
    * \return kOK on success.
    *
    */
    CDErrT SetPGASOwnerWrites(void *data_ptr,        //!< [in] pointer to data to be "Typed";
                                                     //!< __currently must be in same address space
                                                     //!< as calling task, but will extend to
                                                     //!< PGAS fat pointers later  
                              uint64_t len=0,        //!< [in] Length of preserved data (Bytes)
                              bool owner_writes=true //!< [in] Is the current
                                                     //!< CD the only CD in which this address
                                                     //!< range is written (until strict
                                                     //!< ancestor is reached)?
                              ) { return kOK; }

/** @} */ // End PGAS_funcs =========================================================== 
#endif

    /// @brief Commits setjmp buffer or context buffer to CD object.
    void CommitPreserveBuff(void);

  private:  // internal use -------------------------------------------------------------
    /// @brief Initialize for CDHandle object.
    void Init(CD *ptr_cd);

    /// @brief Add children CD to my CD.
    CDErrT AddChild(CDHandle *cd_child);

    /// @brief Delete a child CD in my children CD list
    CDErrT RemoveChild(CDHandle *cd_child);  
    
    /// @brief Stop every task in this CD
    CDErrT Stop(void);
  
#if CD_ERROR_INJECTION_ENABLED
    int CheckErrorOccurred(uint32_t &rollback_point);
#endif
    /// @brief Set bit vector for error types to handle in this CD.
    uint64_t SetSystemBitVector(uint64_t error_name_mask, 
                                uint64_t error_loc_mask=0);

    /// @brief Get NodeID with given new_color and new_task
    static NodeID GenNewNodeID(int new_head, const NodeID &new_node_id, bool is_reuse);

    /// @brief Check mail box.
    CDErrT CheckMailBox(void);

    /// @brief Set mail box.
    CDErrT SetMailBox(CDEventT event);

#if CD_MPI_ENABLED
    /// @brief Get NodeID with given new_color and new_task.
    NodeID GenNewNodeID(const ColorT &my_color, 
                        const int &new_color, 
                        const int &new_task, 
                        int new_head,
                        bool is_reuse
                        );

    /// @brief Collect child CDs' head information.
    void CollectHeadInfoAndEntry(const NodeID &new_node_id);

    /// @brief Synchronize every task of this CD.
    CDErrT Sync(ColorT color);
#endif

  public:
    bool     recreated(void)      const;
    bool     reexecuted(void)     const;
    bool     need_reexec(void)    const;
    uint32_t rollback_point(void) const;

    /// @brief Check the mode of current CD.
    int GetExecMode(void)         const;

/**@defgroup cd_accessor CD accessors
 * @brief The accessors of CDHandle which might be useful.
 * @ingroup user_interfaces
 * @addtogroup cd_accessor
 * @{
 */

   /**@brief Get the string name defined by user.
    *         __It is different from `CDHandle::GetCDName()`.__
    * @return string name for a CD 
    */
    char    *GetName(void)       const;
    std::string  &name(void);

   /**@brief Get the string label defined by user.
    *        Label can be defined at Begin() time.
    *        It is intended to differentiate different phase
    *        of the same CD (object)
    * @return string name for a CD 
    */
    char    *GetLabel(void)      const;
    std::string  &label(void);

    ///@brief Get CDID of the current CD.
    CDID    &GetCDID(void);       

   /**@brief Get the name/location of this CD.
    * The CDName is a (level, rank_in_level) tuple.
    *
    * @return the name/location of the CD
    */
    CDNameT &GetCDName(void);   


    ///@brief Get NodeID of the current CD.
    NodeID  &node_id(void);       
    
    ///@brief Get CD object pointer that this CDHandle corresponds to.
    ///@return CD object's pointer.
    CD      *ptr_cd(void)        const;

    ///@brief Set CD for this CDHandle. 
    void     SetCD(CD *ptr_cd);

    ///@brief Check if the current task int this CD is head task.
    ///@return true if it is head, otherwise false.
    bool     IsHead(void)        const;
    
    ///@brief Get current CDHandle's level.
    uint32_t level(void)         const;

    ///@brief Get current CDHandle's phase.
    uint32_t phase(void)         const;

    ///@brief Get the rank ID of this CD in current CD level.
    uint32_t rank_in_level(void) const;

    ///@brief Get the number of sibling CDs in current CD level.
    uint32_t sibling_num(void)   const;
    
    ///@brief Get color of the current CD.
    ///       In MPI version, color means a communicator.
    ColorT   color(void)         const;

    ///@brief Get task ID in the task group of this CD.
    int      task_in_color(void) const;

    ///@brief Get head task's task ID in this CD.
    int      head(void)          const; 

    ///@brief Get the number of tasks in this CD.
    int      task_size(void)     const; 
    
    ///@brief Get the ID of sequential CD.
    ///       It is incremented by one per begin/complete pair.
    int      GetSeqID(void)      const;

    ///@brief Get CDHandle to this CD's parent
    ///@return Pointer to CDHandle of parent
    CDHandle *GetParent(void)    const;

    ///@brief Operator to check equality between CDHandles.
    bool     operator==(const CDHandle &other) const ;

/** @} */ // End cd_accessor and user_interfaces

    ///@brief Check the current CD's context preservation mode.
    ///       There are two flavors: Include stack and Exclude stack.
    int      ctxt_prv_mode(void);
    jmp_buf *jmp_buffer(void);
    ucontext_t *ctxt(void);

    CDType   GetCDType(void) const;
#if CD_MPI_ENABLED
    int GetCommLogMode(void) const;
    int GetCDLoggingMode(void) const;
#endif
  private:

    GroupT &group(void);
    int SelectHead(uint32_t task_size);

#if CD_TEST_ENABLED
    void PrintCommLog(void) const;
#endif
};

#define CD_Begin(...) FOUR_ARGS_MACRO(__VA_ARGS__, CD_BEGIN3, CD_BEGIN2, CD_BEGIN1, CD_BEGIN0)(__VA_ARGS__)

// Macros for setjump / getcontext
// So users should call this in their application, not call cd_handle->Begin().
#define CD_BEGIN0(X) if((X)->ctxt_prv_mode() == kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin();
#define CD_BEGIN1(X,Y) if((X)->ctxt_prv_mode() == kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin(Y);
#define CD_BEGIN2(X,Y,Z) if((X)->ctxt_prv_mode() == kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin(Y,Z);
#define CD_BEGIN3(X,Y,Z,W) if((X)->ctxt_prv_mode() == kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin(Y,Z,W);

#define CD_Complete(X) (X)->Complete()   


} // namespace cd ends
#endif
