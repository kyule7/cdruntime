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

#include <stdint.h>
#include <vector>
#include <string>
#include <array>
#include <setjmp.h>
#include <ucontext.h>
#include <functional>

#include "cd_global.h"
#include "node_id.h"
#include "sys_err_t.h"
#include "cd_name_t.h"

#if _PROFILER
#include "cd_profiler.h"
#endif


using namespace cd;








namespace cd {
  /** \addtogroup cd_init_funcs CD Init Functions
   *  The \ref cd_init_funcs are used for initialization.
   * @{
   */

/** 
 * @brief Initialize the CD runtime.
 * 
 * CD_Init()
 * Create the data structure that CD object and CDHandle object are going to use.
 * For example, CDEntry object or CDHandle object or CD object itself.
 * These objects are very important data for CDs and 
 * managed separately from the user-level data structure
 * All the meta data objects and preserved data are managed internally.
 * Register Root CD.
 * Creates all necessary CD runtime components and data structures,
 * initializes the CD runtime, and creates and begins the root
 * CD. At this point, the current CD is the root. `cd::Init()`
 * __should only be called once per application.__
 *
 * There are two variants of this function. The first is a
 * collective operation across all SPMD tasks currently in the
 * application. All tasks get a handle to the single root and begin
 * the CD in a synchronized manner. The second variant is called
 * locally by a single task and synchronization, as well as
 * broadcasting the handle are up to the programmer. Use of this second
 * variant is discouraged.
 *
 * @return Returns a handle to the root CD; Returns `kOK` if successful, `AlreadyInit` if called more
 * than once, and `kError` if initialization is unsuccessful for
 * some reason.
 *
 * The handle to the root is also registered in some globally
 * accessible variable so that it can be accessed by
 * cd::GetCurrentCD() and cd::GetRootCD().
 */
  CDHandle* CD_Init(int numTask, int myRank);
  
/** 
 * @brief Finalize the CD runtime.
 * 
 *
 * @return Returns void
 *
 */
  void CD_Finalize(DebugBuf *debugBuf);

  /** @} */ // End cd_init_funcs group ===========================================================================
} // namespace cd ends


/** 
 * @brief An object that provides a handle to a specific CD instance.
 *
 * CDHandle is a global accessor to the CD object. 
 * CDHandle must be valid regardless of MPI rank or threads
 * That means that once I know CDHandle object, 
 * I can access to the ptr_cd_ anyhow.
 * Sometimes, CDHandle::node_id_.second and CDID::node_id_.second can be different. 
 * (If node_id_.first is different, that means there is something wrong!)
 * That means that CD object was newly given from somewhere,
 * and in that case, we should be careful of synchorizing the CD object to CDHandle
 * 09.22.2014 Kyushick: The above comment is written long time ago, so may be confusing to understand the current design.
 * All usage of CDs (other than @ref cd_init_funcs and cd_accessor_funcs) is done by
 * utilizing a handle to a particular CD instance within the CD
 * tree. The CDHandle provides an implementation- and
 * location-independent accessor for CD operation.
 *
 * __Most calls currently only have blocking versions.__ Which should
 * also be non-blocking other than Preserve?
 */ 

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

  /** \addtogroup cd_hierarchy How to create CD hierarchy
   *
   *  This Module is about functions and types to create CD hierarchy in the application.
   * @{
   */

  /**
   * @brief Non-collective Create (Single task)
   * 
   * Creates a new CD as a child of this CD. The new CD does
   * not begin until Begin() is called explicitly.
   * 
   * This version of Create() is intended to be called by only a
   * single task and the value of the returned handle explicitly
   * communicated between all tasks contained within the new child. An
   * alternate collective version is also provided. It is expected
   * that this non-collective version will be mostly used within a
   * single task or, at least, within a single process address space.
   *
   * @return Returns a pointer to the handle of the newly created
   * child CD; returns 0 on an error (error code returned in a parameter).
   *
   */
    CDHandle* Create(const char* name=0, //!< [in] Optional user-specified
                                  		   //!< name that can be used to "re-create" the same CD object
                                  		   //!< if it was not destroyed yet; useful for resuing preserved
                                  		   //!< state in CD trees that are not loop based.
                     CDType cd_type=kStrict, //!< [in] Strict or relaxed
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
    * Creates a new CD as a child of the current CD. The new CD does
    * not begin until CDHandle::Begin() is called explicitly.
    * 
    * This version of Create() is intended to be called by all
    * tasks that will be contained in the new child CD. It functions as
    * a collective operation in a way that is analogous to
    * MPI_comm_split, but only those tasks that are contained in the
    * new child synchronize with one another.
    *
    * @return Returns a pointer to the handle of the newly created
    * child CD; returns 0 on an error.
    *
    */
    CDHandle* Create(uint32_t  numchildren, //!< [in] The total number of tasks that are collectively creating
                                     	      //!< the child numbered "color"; the collective waits for this number
		                                        //!< of tasks to arrive before creating the child
                     const char* name, //!< [in] Optional user-specified name that can be used to "re-create" the same CD object
                                		   //!< if it was not destroyed yet; useful for resuing preserved state in CD trees that are not loop based.
                     CDType cd_type=kStrict, //!< [in] Strict or relaxed
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
    * Creates a new CD as a child of the current CD. The new CD does
    * not begin until CDHandle::Begin() is called explicitly.
    * 
    * This version of Create() is intended to be called by all
    * tasks that will be contained in the new child CD. It functions as
    * a collective operation in a way that is analogous to
    * MPI_comm_split, but only those tasks that are contained in the
    * new child synchronize with one another.
    *
    * @return Returns a pointer to the handle of the newly created
    * child CD; returns 0 on an error.
    *
    */
    CDHandle* Create(const ColorT& color, //!< [in] The "color" of the new child to which this task will belong
                     uint32_t num_tasks_in_color, //!< [in] The total number of tasks that are collectively creating
                                            		  //!< the child numbered "color"; the collective waits for this number
                                            		  //!< of tasks to arrive before creating the child
                     const char* name, //!< [in] Optional user-specified name that can be used to "re-create" the same CD object
                                		   //!< if it was not destroyed yet; useful for resuing preserved state in CD trees that are not loop based.
                     CDType cd_type=kStrict, //!< [in] Strict or relaxed
                     uint32_t error_name_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                          		   //!< should be able to recover from that error type.
                     uint32_t error_loc_mask=0, //!< [in] each `1` in the mask indicates that this CD
                                                //!< should be able to recover from that error type.

                     CDErrT *error=0 //!< [in,out] Pointer for error return value 
                                     //!< (kOK on success and kError on failure) 
                                     //!< no error value returned if error=0.
                     );

 
   /**
    * @brief Collective Create and Begin
    *
    * Creates a new CD as a child of the current CD. The new CD then
    * immediately begins with a single collective call.
    * 
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
    CDHandle* CreateAndBegin(const ColorT& color, //!< [in] The "color" of the new child to which this task will belong
                             uint32_t num_tasks_in_color, //!< [in] The total number of tasks that are collectively creating
                                                    		  //!< the child numbered "color"; the collective waits for this number
                                                    		  //!< of tasks to arrive before creating the child
                             const char* name, //!< [in] Optional user-specified name that can be used to "re-create" the same CD object
                                        		   //!< if it was not destroyed yet; useful for resuing preserved state in CD trees that are not loop based.
                             CDType cd_type=kStrict, //!< [in] Strict or relaxed
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
                 const char* label=0
                );
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
  
   /** @} */ // End cd_hierarchy ==================================================================================

  /** \addtogroup preservation_funcs
   *
   * @{
   */

  /** @brief (Non-collective) Preserve data to be restored when recovering (typically reexecuting the CD from right after its Begin() call.
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
    CDErrT Preserve(void *data_ptr=0, //!< [in] pointer to data to be preserved;
                             		      //!< __currently must be in same address space
                             		      //!< as calling task, but will extend to PGAS fat pointers later 
                    uint64_t len=0,   //!< [in] Length of preserved data (Bytes)
                    uint32_t preserve_mask=kCopy, //!< [in] Allowed types of preservation 
                                                  //!< (e.g., kCopy|kRef|kRegen), default only via copy
                    const char *my_name=0,  //!< [in] Optional C-string representing the name of this
                                  		      //!< preserved data for later preserve-via-reference

                    const char *ref_name=0, //!< [in] Optional C-string representing
                                    	      //!< a user-specified name that was set by a previous preserve call at the parent.; 
                                            //!< __Do we also need an offset into parent preservation?__

                    uint64_t ref_offset=0,  //!< [in] explicit offset within the named region at the other CD (for restoration via reference)
                    const RegenObject *regen_object=0, //!< [in] optional user-specified function for
                                              		     //!< regenerating values instead of restoring by copying

                    PreserveUseT data_usage=kUnsure //!< [in] This flag is used
                                            		    //!< to optimize consecutive Complete/Begin calls
                                            		    //!< where there is significant overlap in
                                            		    //!< preserved state that is unmodified (see Complete()).
                    );

  /** @brief (Not supported yet) Non-blocking preserve data to be restored when recovering (typically reexecuting the CD from right after its Begin() call.
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
                    void *data_ptr=0, //!< [in] pointer to data to be preserved;
                            		      //!< __currently must be in same address space as calling task, but will extend to PGAS fat pointers later
                    uint64_t len=0,   //!< [in] Length of preserved data (Bytes)
                    uint32_t preserve_mask=kCopy, //!< [in] Allowed types of preservation 
                                                  //!< (e.g.,kCopy|kRef|kRegen), default only via copy
                    const char *my_name=0, //!< [in] Optional C-string representing the name of this
                                  		     //!< preserved data for later preserve-via-reference
                    const char *ref_name=0, //!< [in] Optional C-string representing a user-specified name 
                                            //!< that was set by a previous preserve call at the parent.; 
                                            //!< __Do we also need an offset into parent preservation?__
                    uint64_t ref_offset=0, //!< [in] explicit offset within the named region at the other CD
                    const RegenObject *regen_object=0, //!< [in] optional user-specified function for
                                              		     //!< regenerating values instead of restoring by copying
                    PreserveUseT data_usage=kUnsure //!< [in] This flag is used to optimize consecutive Complete/Begin calls
                                           		     //!< where there is significant overlap in preserved state that is unmodified (see Complete()).
                    );


  /** @} */ // End preservation_funcs ======================================================================

 
// if Regen were to registered from a remote node to a actual CD Object, 
// it will need to serialize the Regen object and then finally send the object wait... 
// we can't send the "binary" to the remote node.. this will be too much to support... 
// so we have to always assume this preservation happens local...  
// Basically preserve function will get called from local..
 
  /** \addtogroup detection_recovery Detection and Recovery Methods
   *
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
    std::vector<SysErrT> Detect(CDErrT* err_ret_val=0 //!< [in,out] Pointer to
  				 //!< a variable for
  				 //!< optionally returning a
  				 //!< CD runtime error code
  				 //!< indicating some bug
  				 //!< with Detect().
  				 );

  /** @brief Declare that this CD can detect certain errors/failures by user-defined detectors.
   *
   * The intent of this method is to specify to the autotuner that
   * detection is possible. This is needed in order to balance between
   * fine-grained and coarse-grained CDs and associated recovery.
   *
   * @return kOK on success.
   */
    CDErrT RegisterDetection(uint system_name_mask, //!< [in] each `1` in
  			     //!< the mask indicates that this CD
  			     //!< should be able to detect any errors
  			     //!< that are meaningful to the application
  			     //!< (in the error type mask).
  			      uint system_loc_mask //!< [in] each `1` in
  			     //!< the mask indicates that this CD
  			     //!< should be able to detect any errors
  			     //!< that are meaningful to the application
  			     //!< (in the error type mask).
  			      );

  /** @brief Register that this CD can recover from certain errors/failures
   *
   * This method serves two purposes:
   * # It extends the specification of intent to recover provided in
   *   Create(). 
   * # It enables registering a customized recovery routine by
   *   inheriting from the RecoverObject class.
   *
   * @return kOK on success.
   *
   * \sa Create(), RecoverObject
   *
   *\todo Does registering recovery also imply turning on detection?
   *Or is that done purely through RequireErrorProbability()? 
   */
    CDErrT RegisterRecovery(uint error_name_mask, //!< [in] each `1` in
  			     //!< the mask indicates that this CD
  			     //!< should be able to recover from that
  			     //!< error type.
  			     uint error_loc_mask, //!< [in] each `1` in
  			     //!< the mask indicates that this CD
  			     //!< should be able to recover from that
  			     //!< error type.
  			     RecoverObject* recover_object=0 //!< [in] pointer
  			     //!< to an object that contains the customized
  			     //!< recovery routine; if unspecified,
  			     //!< default recovery is used.
  			     );
  
    CDErrT RegisterRecovery(uint error_name_mask, //!< [in] each `1` in
  			     //!< the mask indicates that this CD
  			     //!< should be able to recover from that
  			     //!< error type.
  			     uint error_loc_mask, //!< [in] each `1` in
  			     //!< the mask indicates that this CD
  			     //!< should be able to recover from that
  			     //!< error type.
             CDErrT(*recovery_func)(std::vector< SysErrT > errors)=0 //!< [in] function pointer for customized
  			                                       //!< recovery routine; if unspecified, default recovery is used.
  			     );
  /** \todo What about specifying leniant communication-related errors
   *  for relaxed-CDs context?
   */

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

  /** @} */ // End detection_recovery group =========================================

 

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

   /** @} */ // End cd_error_probability group  ==================================================




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
   * Discussion on 3/11/2014:
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
  CDErrT SetPGASType(void* data_ptr,   //!< [in] pointer to data to be "Typed";
		      //!< __currently must be in same address space
		      //!< as calling task, but will extend to
		      //!< PGAS fat pointers later  
		      uint64_t len, //!< [in] Length of preserved data (Bytes)
		      PGASUsageT region_type=kSharedVar //!< [in] How
		      //!< is this
		      //!< memory range
		      //!< used (shared
		      //!< for comm or not?)
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
  CDErrT SetPGASOwnerWrites(void* data_ptr,
			    //!< [in] pointer to data to be "Typed";
			    //!< __currently must be in same address space
			    //!< as calling task, but will extend to
			    //!< PGAS fat pointers later  
			    uint64_t len=0, //!< [in] Length of preserved data (Bytes)
			    bool owner_writes=true //!< [in] Is the current
			    //!< CD the only CD in which this address
			    //!< range is written (until strict
			    //!< ancestor is reached)?
			    ) { return kOK; }

  /** @} */ // End PGAS_funcs =========================================================== 



  
    void CommitPreserveBuff(void);
//    char *GenTag(const char* tag);
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
    // Accessors

  /** @brief Get the name/location of this CD
   *
   * The CDName is a (level, number_within_level) tuple.
   *
   * @return the name/location of the CD
   */
    char    *GetName(void)       const;

    CDID    &GetCDID(void);       
    CDNameT &GetCDName(void);   
    ColorT  GetNodeID(void);     
    NodeID  &node_id(void);       
    
    CD      *ptr_cd(void)        const;
    void     SetCD(CD* ptr_cd);
    bool     IsHead(void)        const;
    
    uint32_t level(void)         const;
    uint32_t rank_in_level(void) const;
    uint32_t sibling_num(void)   const;
    
    ColorT   color(void)         const;
    int      task_in_color(void) const;
    int      head(void)          const; 
    int      task_size(void)     const; 
    
    int      GetSeqID(void)      const;

  /** @brief Get CDHandle to this CD's parent
   *
   * \return Pointer to CDHandle of parent
   */
    CDHandle *GetParent(void)    const;
    int      ctxt_prv_mode(void);
    bool     operator==(const CDHandle &other) const ;
};


#endif
