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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "cd_features.h"
#include "cd_def_common.h"
// This could be different from MPI program to PGAS program
// key is the unique ID from 0 for each CD node.
// value is the unique ID for mpi communication group or thread group.
// For MPI, there is a communicator number so it is the group ID,
// For PGAS, there should be a group ID, or we should generate it. 

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


namespace cd {
  namespace internal {

    class CD;
    class HeadCD;
    class CDPath;
    class CDEntry;
    class DataHandle;
    class NodeID;
    class CDNameT;
    class CDID;
    class CDEvent;
    class PFSHandle;
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
  namespace logging {
    class CommLog;
    class RuntimeLogger;
  }
  namespace cd_test {
    class Test;
  }

  class SysErrT;
  class RegenObject;
  class RecoverObject;
  class CDHandle;
  class DebugBuf;
}

namespace cd {

#define BIT_0     0x0000000000000001  
#define BIT_1     0x0000000000000002                       
#define BIT_2     0x0000000000000004                       
#define BIT_3     0x0000000000000008                       
#define BIT_4     0x0000000000000010                       
#define BIT_5     0x0000000000000020                       
#define BIT_6     0x0000000000000040                       
#define BIT_7     0x0000000000000080                       
#define BIT_8     0x0000000000000100                       
#define BIT_9     0x0000000000000200                       
#define BIT_10    0x0000000000000400                       
#define BIT_11    0x0000000000000800                       
#define BIT_12    0x0000000000001000                       
#define BIT_13    0x0000000000002000                       
#define BIT_14    0x0000000000004000                       
#define BIT_15    0x0000000000008000                       
#define BIT_16    0x0000000000010000                       
#define BIT_17    0x0000000000020000                       
#define BIT_18    0x0000000000040000                       
#define BIT_19    0x0000000000080000                       
#define BIT_20    0x0000000000100000                       
#define BIT_21    0x0000000000200000                       
#define BIT_22    0x0000000000400000                       
#define BIT_23    0x0000000000800000                       
#define BIT_24    0x0000000001000000                       
#define BIT_25    0x0000000002000000                       
#define BIT_26    0x0000000004000000                       
#define BIT_27    0x0000000008000000                       
#define BIT_28    0x0000000010000000                       
#define BIT_29    0x0000000020000000                       
#define BIT_30    0x0000000040000000                       
#define BIT_31    0x0000000080000000                       
#define BIT_32    0x0000000100000000                       
#define BIT_33    0x0000000200000000                       
#define BIT_34    0x0000000400000000                       
#define BIT_35    0x0000000800000000                       
#define BIT_36    0x0000001000000000                       
#define BIT_37    0x0000002000000000                       
#define BIT_38    0x0000004000000000                       
#define BIT_39    0x0000008000000000                       
#define BIT_40    0x0000010000000000                       
#define BIT_41    0x0000020000000000                       
#define BIT_42    0x0000040000000000                       
#define BIT_43    0x0000080000000000                       
#define BIT_44    0x0000100000000000                       
#define BIT_45    0x0000200000000000                       
#define BIT_46    0x0000400000000000           
#define BIT_47    0x0000800000000000                       
#define BIT_48    0x0001000000000000                       
#define BIT_49    0x0002000000000000                       
#define BIT_50    0x0004000000000000                       
#define BIT_51    0x0008000000000000                       
#define BIT_52    0x0010000000000000           
#define BIT_53    0x0020000000000000                       
#define BIT_54    0x0040000000000000                       
#define BIT_55    0x0080000000000000                       
#define BIT_56    0x0100000000000000                       
#define BIT_57    0x0200000000000000                       
#define BIT_58    0x0400000000000000           
#define BIT_59    0x0800000000000000                       
#define BIT_60    0x1000000000000000                       
#define BIT_61    0x2000000000000000                       
#define BIT_62    0x4000000000000000                       
#define BIT_63    0x8000000000000000                       

//#if CD_DEBUG_ENABLED
////  extern std::ostringstream dbg;
//  extern DebugBuf cddbg;
//#define dbgBreak nullFunc
//#else
//#define cddbg std::cout
//#define dbgBreak nullFunc
//#endif


  //GONG: global variable to represent the current context for malloc wrapper
  extern bool app_side;

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
                kAppError 
              };

/** @} */ // end of internal_error_types


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



/**@addtogroup preservation_funcs 
 * @{
 */

/** 
 * @brief Type for specifying preservation methods
 *
 * See <http://lph.ece.utexas.edu/public/CDs> for a detailed description. 
 *
 * The intent is for this to be used as a mask for specifying
 * multiple legal preservation methods so that the autotuner can
 * choose the appropriate one.
 *
 * \sa RegenObject, CDHandle::Preserve()
 */
  enum CDPreserveT  { kCopy=BIT_8, //!< Prevervation via copy copies
                                   //!< the data to be preserved into
                                   //!< another storage/mem location
                                   //!< Preservation via reference
                      kRef=BIT_9, //!< Preservation via reference     
                                  //!< indicates that restoration can
                                  //!< occur by restoring data that is
                                  //!< already preserved in another
                                  //!< CD. __Restriction:__ in the
                                  //!< current version of the API only
                                  //!< the parent can be used as a
                                  //!< reference. 
                      kRegen=BIT_10, //!< Preservation via regenaration
                                    //!< is done by calling a
                                    //!< user-provided function to
                                    //!< regenerate the data during
                                    //!< restoration instead of copying
                                    //!< it from preserved storage.
                      kCoop=BIT_11,  //!< This flag is used for preservation-via-reference 
                                    //!< in the case that the referred copy is in remote task.
                                    //!< This flag can be used with kCopy
                                    //!< such as kCopy | kCoop.
                                    //!< Then, this entry can be referred by lower level.
                      kSerdes=BIT_12, //!< This flag indicates the preservation is done by
                                      //!< serialization, which mean it does not need to 
                                      //!< duplicate the data because serialized data is
                                      //!< already another form of preservation.
                                      //!< This can be used such as kCopy | kSerdes
                      kReservedPrvT0=BIT_13,
                      kReservedPrvT1=BIT_14
                    };
/** @} */ // end of preservation_funcs

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


  extern CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium);
  extern void CD_Finalize(void);

}


#endif
