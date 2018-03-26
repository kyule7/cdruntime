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

#ifndef _CD_GLOBAL_H
#define _CD_GLOBAL_H

/**
 * @file cd_global.h
 * @author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * @date March 2015
 *
 * @brief Containment Domains API v0.2 (C++)
 */

/** 
 * \brief Declarations/Definitions for global usages in CD runtime.
 *
 * All user-visible CD API calls and definitions are under the CD namespace.
 * Internal CD API implementation components are under a separate CDInternal namespace.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include "cd_features.h"
#include "cd_def_common.h"
#include "libc_wrapper.h" // defined here due to tuned
// This could be different from MPI program to PGAS program
// key is the unique ID from 0 for each CD node.
// value is the unique ID for mpi communication group or thread group.
// For MPI, there is a communicator number so it is the group ID,
// For PGAS, there should be a group ID, or we should generate it. 

//#define NO_LABEL ""

#if CD_MPI_ENABLED 
#include <mpi.h>
#define ROOT_COLOR    MPI_COMM_WORLD
#define INITIAL_COLOR MPI_COMM_NULL
#define ROOT_HEAD_ID  0

///@addtogroup cd_defs 
///@{
///@var typedef ColorT
///@brief ColorT is a type that signifies a task group entity. 
///       For MPI version, it corresponds to MPI_Comm.
///@typedef ColorT is used for non-single task program.OR
typedef MPI_Comm      ColorT;
///@}
typedef MPI_Group     GroupT;

#else

#define ROOT_COLOR    0 
#define INITIAL_COLOR 0
#define ROOT_HEAD_ID  0
typedef int           ColorT;
typedef int           GroupT;
#endif

namespace packer {
  extern bool orig_disabled;
  extern bool orig_appside;
}
namespace tuned {
  extern bool orig_disabled;
  extern bool orig_appside;
}

namespace interface {
  class Profiler;
  class ErrorInjector; 
  class ErrorProb;
  class UniformRandom;
  class LogNormal;
  class Exponential;
  class Normal;
  class Poisson;
  class ErrorInjector;
  class CDErrorInjector;
  class MemoryErrorInjector;
  class NodeFailureInjector;
}

namespace cd {
  namespace internal {

    class CD;
    class HeadCD;
    class CDPath;
//    class CDEntry;
//    class DataHandle;
    class NodeID;
    class CDNameT;
    class CDID;
//    class PFSHandle;
  }
  namespace logging {
    class CommLog;
    class RuntimeLogger;
  }
  namespace cd_test {
    class Test;
  }

  class CDEvent;
  class SysErrT;
  class RegenObject;
  class RecoverObject;
  class CDHandle;
  class DebugBuf;
}

namespace common {
/**@addtogroup internal_error_types  
 * @{
 *
 * @brief Type for specifying error return codes from an API call --
 * signifies some failure of the API call itself, not a system 
 * failure.
 *
 * Unlike SysErrNameT, CDErrT is not for system errors, but
 * rather errors originating from the CD framework itself.
 *
 * For now, only returning OK or error, but will get more elaborate
 * in future versions of this API.
 */

  enum CDErrT { kOK=0,        //!< Call executed without error       
                kAlreadyInit, //!< Init called more than once
                kError,       //!< Call did not execute as expected
                kExecutionModeError, //!< Errors during execution
                kOutOfMemory, //!< Error due to not enough memory
                kFileOpenError, //!< error during opening file
                kFailedMakeFileDir,
                kAppError,
                kCompleteError,
              };
/** @} */ // end of internal_error_types
}

using namespace common;

namespace cd {



//  /** \addtogroup cd_defs 
//   *@{
//   */
//  /** 
//   * @brief Type for specifying whether the current CD is executing
//   * for the first time or is currently reexecuting as part of recovery.
//   *
//   * During reexecution, data is restored instead of being
//   * preserved. Additionally, for relaxed CDs, some communication and
//   * synchronization may not repeated and instead preserved (logged)
//   * values are used. 
//   * See <http://lph.ece.utexas.edu/public/CDs> for a detailed
//   * description. Note that this is not part of the cd_internal
//   * namespace because the application programmer may want to manipulate
//   * this when specifying specialized recovery routines.
//   */
//    enum CDExecMode  {kExecution=0, //!< Execution mode 
//                      kReexecution, //!< Reexecution mode
//                      kSuspension   //!< Suspension mode (not active)
//                       };
//  
//  /** @} */ // End group cd_defs


/**@addtogroup error_reporting 
 * @{
 *
 * @brief Type for specifying system error/failure names
 *
 * This type represents the interface between the user and the
 * system with respect to errors and failures. The intent is for
 * these error/failure names to be fairly comprehensive with respect
 * to system-based issues, while still providing general/abstract
 * enough names to be useful to the application programmer. The use
 * categories/names are meant to capture recovery strategies that
 * might be different based on the error/failure and be comprehensive in that regard. 
 *
 * \warning The SysErrNameT and SysErrLocT are extensible by a runtime
 * call to generate a new constant, but SysErrInfo is a class
 * hierarchy and extended at compile time by inhereting the
 * interface -- is this a problem? Should we go the GVR way with all
 * extensions done at runtime and all accesses with potential
 * runtime methods and no compile-time typing?
 *
 * \todo Is SysErrNameT comprehensive enough for portability?
 * \todo segv (segmentation violations) can be used as proxy for soft memory errors using the existing kernel infrastructure
 *
 * \sa SysErrLocT, DeclareErrName(), UndeclareErrName()
 */
  enum SysErrNameT  { kNoHWError=0,             //!< No errors/failures.
                      kSoftMem=BIT_0,           //!< Soft memory error.
                                                //!< (info includes address range and perhaps syndrome)
                      kDegradedMem=BIT_1,       //!< Hard memory error that disabled some memory capacity.
                                                //!< (info includes address range(s))
                      kSoftComm=BIT_2,          //!< Soft communication error.
                                                //!< (info includes message info)
                      kDegradedComm=BIT_3,      //!< Some channel loss.
                      kSoftComp=BIT_4,          //!< Soft compute error.
                                                //!< (info includes affected PC and perhaps bounds on the error?)
                      kDegradedResource=BIT_5,  //!< Resource lost __some__ functionality.
                      kHardResource=BIT_6,      //!< Resource entirely lost.
                                                //!< (control/reachability failure).
                      kFileSys=BIT_7            //!< Some file system error.
                    };

/** @brief Type for specifying errors and failure location names
 *
 * Please see SysErrNameT for discussion of intent and defintions
 *
 * This is really not that suitable for all topologies, but the
 * intent really is to be rather comprehensive to maintain
 * portability -- how do we resolve this?
 *
 * \todo is SysErrLocT comprehensive enough for portability?
 *
 *
 * \sa SysErrNameT, DeclareErrLoc(), UndeclareErrLoc()
 */
  enum SysErrLocT   { kNoSysErr=0,           //!< No errors/failures
                      kIntraCore=BIT_16,     //!< Within a part of a core
                      kCore=BIT_17,          //!< A core
                      kProc=BIT_18,          //!< Processor
                      kNode=BIT_19,          //!< Same as processor?
                      kModule=BIT_20,        //!< Module
                      kCabinet=BIT_21,       //!< A cabinet
                      kCabinetGroup=BIT_22,  //!< Some grouping of cabinets
                      kSystem=BIT_23         //!< Entire system
                    };

/** @} */ // end of error_reporting


/** \addtogroup tunable_api 
 * @{
 */
  enum TuningT {
    kON=1,
    kSeq=2,
    kNest=4,
    kAuto=8,
    kOFF=16
  };
  /** @} */ // End tunable_api group 

  extern int myTaskID;
  extern int totalTaskSize;
  extern int app_input_size;
  extern bool app_side;
  extern bool is_error_free;
  extern bool is_koutput_disabled;
  extern bool runtime_activated;

  enum {
    TOTAL_PRF=0,
    REEX_PRF,
    CDOVH_PRF,
    CD_NS_PRF,
    CD_RS_PRF,
    CD_ES_PRF,
    MSG_PRF,
    LOG_PRF,
    PRV_PRF,
    RST_PRF,
    CREAT_PRF,
    DSTRY_PRF,
    BEGIN_PRF,
    COMPL_PRF,
    PROF_GLOBAL_STATISTICS_NUM
  };

  extern double recvavg[PROF_GLOBAL_STATISTICS_NUM];
  extern double recvstd[PROF_GLOBAL_STATISTICS_NUM];
  extern double recvmin[PROF_GLOBAL_STATISTICS_NUM];
  extern double recvmax[PROF_GLOBAL_STATISTICS_NUM];

  void InitDir(int myTask, int numTask);

/** \addtogroup cd_accessor_funcs 
 * These methods are globally accessible without a CDHandle object.
 * So, user can get a handle of CD at any point of program with these methods.
 * This can be used at convenience to map a CD for a function.
 * In this case, users do not need to pass CDHandle as a function argument,
 * but instead, they can get the leaf CD handle by calling GetCurrentCD().
 *
 * @{
 */

/**@brief Accessor function to current active CD.
 * 
 *  At any point after the CD runtime is initialized, each task is
 *  associated with a current CD instance. The current CD is the
 *  deepest CD in the tree visible from the task that has begun but
 *  has not yet completed. In other words, whenever a CD begins, it
 *  becomes the current CD. When a CD completes, its parent becomes
 *  the current CD.
 *
 *  @return returns a pointer to the handle of the current active CD; Returns
 *  0 if CD runtime is not yet initialized or because of a CD
 *  implementation bug.
 */
  CDHandle *GetCurrentCD(void);

/**@brief Accessor function to current leaf CD.
 * 
 * It is the same as GetCurrentCD() except that it returns the leaf CD even though it is not active yet.
 *
 *  @return returns a pointer to the handle of the leaf CD; Returns
 *  0 if CD runtime is not yet initialized or because of a CD
 *  implementation bug.
 */
  CDHandle *GetLeafCD(void);

 /**@brief Accessor function to a CDHandle at a root level in CD hierachy.
  * @return returns a pointer to the handle of the root CD; Returns
  * 0 if CD runtime is not yet initialized or because of a CD
  * implementation bug.
  */
  CDHandle *GetRootCD(void);

 /** @brief Accessor function to a CDHandle at parent level of the leaf CD. 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(void);

 /** @brief Accessor function to a CDHandle at parent level of a specific CD hierarchy level. 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(int current_level);

  /** @} */ // End cd_accessor_funcs group =====================================================


  enum CDEventT { kNoEvent=0,
                  // Head -> Non-Head
                  kAllPause=BIT_0,
                  kAllResume=BIT_1,
                  kAllReexecute=BIT_2,
                  kAllEscalate=BIT_3,
                  kEntrySend=BIT_4, 
                  kReserved=BIT_5, 
                  // Non-Head -> Head
                  kEntrySearch=BIT_6,
                  kErrorOccurred=BIT_7,
                  kEscalationDetected=BIT_8 };

/** \addtogroup profiler-related
 *@{
 */

/** @brief Profile-related enumerator
 *
 */
  enum ProfileType      { LOOP_COUNT, 
                          EXEC_CYCLE, 
                          PRV_COPY_DATA, 
                          PRV_REF_DATA, 
                          OVERLAPPED_DATA, 
                          SYSTEM_BIT_VECTOR,
                          CD_OVERHEAD, 
                          MSG_LOGGING, 
                          RUNTIME_LOGGING, 
                          MAX_PROFILE_DATA };

/** @} */ // end profiler-related group ===========================================


/**@class cd::Tag
 * @brief Utility class to generate tag.
 *
 */ 
  class Tag : public std::string {
    std::ostringstream _oss;
  public:
    Tag() {}
    ~Tag() {}
    template <typename T>
    Tag &operator<<(const T &that) {
      _oss << that;
      return *this;
    }
    std::string str(void) {
      return _oss.str();
    }
  }; 

/*
  struct PhaseNode {
    uint32_t level_;
    uint32_t phase_;
    int64_t interval_;
    int64_t errortype_;
    std::string label_;
    PhaseNode *parent_;
    std::list<PhaseNode *> children_;
    static uint32_t phase_gen;
    PhaseNode(PhaseNode *parent, uint32_t level, const std::string &label) 
    {
      level_ = level;
      phase_ = phase_gen++;
      if(parent != NULL) {
        parent->AddChild(this);
      }
      parent_ = parent;
    }
  
    void Delete(void) 
    {
      // Recursively reach the leaves, then delete from them to root.
      for(auto it=children_.begin(); it!=children_.end(); ++it) {
        (*it)->Delete();
      }
      delete this;
    }
  
    public:
    uint32_t GetPhaseNode(uint32_t level, const std::string &label);
    void AddChild(PhaseNode *child) 
    {
      children_.push_back(child);
    }
    void Print(void); 
    std::string GetPhasePath(void);
  };
  
  struct PhaseTree {
    PhaseNode *root_;
    PhaseNode *current_;
    PhaseTree(void) : root_(NULL), current_(NULL) {}
    ~PhaseTree() {
      root_->Delete();
    }
    void Print(void) {
      root_->Print();
    }
  };

  typedef std::map<std::string, uint32_t> PhasePathType;
  extern PhaseTree phaseTree;
*/
//  extern char output_basepath[512];
  extern CDHandle *null_cd;
  extern CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium);
  extern void CD_Finalize(void);
  extern uint64_t GetCDEntryID(const std::string &str);
//  class SystemConfig;
//  extern SystemConfig config;
} // namespace cd ends


namespace tuned {
//  extern cd::PhaseTree phaseTree;
//  extern cd::PhasePathType phasePath;
  class CDHandle;
  extern CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium);
  extern void CD_Finalize(void);
  extern bool tuning_enabled;
/** \addtogroup cd_accessor_funcs 
 * These methods are globally accessible without a CDHandle object.
 * So, user can get a handle of CD at any point of program with these methods.
 * This can be used at convenience to map a CD for a function.
 * In this case, users do not need to pass CDHandle as a function argument,
 * but instead, they can get the leaf CD handle by calling GetCurrentCD().
 *
 * @{
 */

/**@brief Accessor function to current active CD.
 * 
 *  At any point after the CD runtime is initialized, each task is
 *  associated with a current CD instance. The current CD is the
 *  deepest CD in the tree visible from the task that has begun but
 *  has not yet completed. In other words, whenever a CD begins, it
 *  becomes the current CD. When a CD completes, its parent becomes
 *  the current CD.
 *
 *  @return returns a pointer to the handle of the current active CD; Returns
 *  0 if CD runtime is not yet initialized or because of a CD
 *  implementation bug.
 */
  CDHandle *GetCurrentCD(void);

/**@brief Accessor function to current leaf CD.
 * 
 * It is the same as GetCurrentCD() except that it returns the leaf CD even though it is not active yet.
 *
 *  @return returns a pointer to the handle of the leaf CD; Returns
 *  0 if CD runtime is not yet initialized or because of a CD
 *  implementation bug.
 */
  CDHandle *GetLeafCD(void);

 /**@brief Accessor function to a CDHandle at a root level in CD hierachy.
  * @return returns a pointer to the handle of the root CD; Returns
  * 0 if CD runtime is not yet initialized or because of a CD
  * implementation bug.
  */
  CDHandle *GetRootCD(void);

 /** @brief Accessor function to a CDHandle at parent level of the leaf CD. 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(void);

 /** @brief Accessor function to a CDHandle at parent level of a specific CD hierarchy level. 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(int current_level);

  /** @} */ // End cd_accessor_funcs group =====================================================
//  struct ParamEntry {    
//    int64_t count_;
//    int64_t error_mask_;
//    int64_t interval_;
//    uint32_t merge_begin_;
//    uint32_t merge_end_;
//    ParamEntry(void) : count_(0), error_mask_(-1), interval_(-1), merge_begin_(0), merge_end_(0){}
////    bool     active_;
//  };

#define CD_LIBC_LOGGING 0

#if CD_LIBC_LOGGING

#define TunedPrologue() \
  cd::app_side = false; \
  logger::disabled = true; \
  tuned::begin_clk = CD_CLOCK(); 

#define TunedEpilogue() \
  tuned::end_clk = CD_CLOCK(); \
  tuned::elapsed_time += tuned::end_clk - tuned::begin_clk; \
  cd::app_side = true; \
  logger::disabled = false; 

#define PackerPrologue() \
  packer::orig_appside = cd::app_side; \
  packer::orig_disabled = logger::disabled; \
  cd::app_side = false; \
  logger::disabled = true; 

#define PackerEpilogue() \
  cd::app_side = packer::orig_appside; \
  logger::disabled = packer::orig_disabled;

#else

#define TunedPrologue() \
  cd::app_side = false; \
  tuned::begin_clk = CD_CLOCK(); 

#define TunedEpilogue() \
  tuned::end_clk = CD_CLOCK(); \
  tuned::elapsed_time += tuned::end_clk - tuned::begin_clk; \
  cd::app_side = true; \

#define PackerPrologue() \
  packer::orig_appside = cd::app_side; \
  cd::app_side = false; \

#define PackerEpilogue() \
  cd::app_side = packer::orig_appside; \


#endif

} // namespace tuned ends

#endif
