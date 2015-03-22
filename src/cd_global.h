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
#include <cstdlib>
#include <csetjmp>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <functional>
// This could be different from MPI program to PGAS program
// key is the unique ID from 0 for each CD node.
// value is the unique ID for mpi communication group or thread group.
// For MPI, there is a communicator number so it is the group ID,
// For PGAS, there should be a group ID, or we should generate it. 

#if _MPI_VER 
#include <mpi.h>
#define ROOT_COLOR MPI_COMM_WORLD
#define ROOT_HEAD_ID 0
#define INITIAL_COLOR MPI_COMM_NULL
typedef MPI_Comm ColorT;
typedef MPI_Group GroupT;
typedef MPI_Request ReqestT;
#else

#define ROOT_COLOR 0 
#define ROOT_HEAD_ID 0
#define INITIAL_COLOR 0
typedef int ColorT;
typedef int GroupT;
typedef int ReqestT;

#endif

#if _MPI_VER
#if _KL
typedef int CDFlagT;
typedef MPI_Win CDMailBoxT;
#endif
#endif

typedef uint32_t ENTRY_TAG_T;

#define MAX_ENTRY_BUFFER_SIZE 1024

#define MSG_TAG_ENTRY_TAG 1073741824 // 2^30
#define MSG_TAG_ENTRY     2147483648 // 2^31
#define MSG_TAG_DATA      3221225472 // 2^31 + 2^30
#define MSG_MAX_TAG_SIZE  1073741824 // 2^30. TAG is
//#define GEN_MSG_TAG(X) 

#define INIT_TAG_VALUE   0
#define INIT_ENTRY_SRC   0
#define INVALID_TASK_ID -1
#define INVALID_HEAD_ID -1
#define NUM_FLAGS 1024


#define INITIAL_ERR_VAL kOK
#define DATA_MALLOC malloc
#define DATA_FREE free

#define CHECK_EVENT_NO_EVENT(X) (X == 0)
#define CHECK_PRV_TYPE(X,Y) ((X & Y) == Y)
#define CHECK_EVENT(X,Y) ((X & Y) == Y)
#define CHECK_NO_EVENT(X) (X == 0)
#define SET_EVENT(X,Y) (X |= Y)

#define TAG_MASK(X) ((2<<(X-1)) - 1)
#define TAG_MASK2(X) ((2<<(X-1)) - 2)

//GONG: global variable to represent the current context for malloc wrapper
//extern bool app_side;

namespace cd {
  class DebugBuf;
#if _DEBUG
//  extern std::ostringstream dbg;
  extern DebugBuf dbg;
#define dbgBreak nullFunc
#else
#define dbg std::cout
#define dbgBreak nullFunc
#endif

  class CD;
  class HeadCD;
  class CDHandle;
  class CDPath;
  class CDEntry;
  class DataHandle;
  class Serializable;
  class NodeID;
  class CDID;
  class Packer; 
  class Unpacker; 
//  class Util;
  class CDEvent;
  class RegenObject;  
  class RecoverObject;
  class CD_Parallel_IO_Manager;
  class SysErrT;
#ifdef comm_log
  //SZ
  class CommLog;
#endif

  /** \addtogroup internal_error_types Internal Error Types 
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
                kExecutionModeError,
                kOutOfMemory,
                kFileOpenError
              };

  /** @} */ // end of internal_error_types


  /** \addtogroup error_reporting 
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
  enum SysErrNameT  { kNoHWError=0,   //!< No errors/failures.
                      kSoftMem=1,     //!< Soft memory error.
                                      //!< (info includes address range and perhaps syndrome)
                      kDegradedMem=2, //!< Hard memory error that disabled some memory capacity.
                                      //!< (info includes address range(s))
                      kSoftComm=4,    //!< Soft communication error.
		                                  //!< (info includes message info)
                      kDegradedComm=8, //!< Some channel loss.
                      kSoftComp=16,    //!< Soft compute error.
		                                   //!< (info includes affected PC and perhaps bounds on the error?)
                      kDegradedResource=32, //!< Resource lost __some__ functionality.
                      kHardResource=64, //!< Resource entirely lost.
		                                    //!< (control/reachability failure).
                      kFileSys=128      //!< Some file system error.
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
  enum SysErrLocT   { kNoSysErr=0,      //!< No errors/failures
                      kIntraCore=1,     //!< Within a part of a core
                      kCore=2,          //!< A core
                      kProc=4,          //!< Processor
                      kNode=8,          //!< Same as processor?
                      kModule=16,       //!< Module
                      kCabinet=32,      //!< A cabinet
                      kCabinetGroup=64, //!< Some grouping of cabinets
                      kSystem=128       //!< Entire system
                    };

  /** \todo Missing error injection API */
  /** @} */ // end of error_reporting



  /** \addtogroup preservation_funcs Preservation/Restoration Types and Methods
   *  The \ref preservation_funcs module contains all
   *  preservation/restoration related types and methods.
   *
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
  enum CDPreserveT  { kCopy=1, //!< Prevervation via copy copies
                               //!< the data to be preserved into
                               //!< another storage/mem location
                               //!< Preservation via reference
											kRef=2, //!< Preservation via reference     
                              //!< indicates that restoration can
                              //!< occur by restoring data that is
                              //!< already preserved in another
                              //!< CD. __Restriction:__ in the
                              //!< current version of the API only
                              //!< the parent can be used as a
                              //!< reference. 
											kRegen=4, //!< Preservation via regenaration
                                //!< is done by calling a
                                //!< user-provided function to
                                //!< regenerate the data during
                                //!< restoration instead of copying
                                //!< it from preserved storage.
                      kShared=8 
										};
  /** @} */ // end of preservation_funcs




  /** \addtogroup cd_defs 
   *@{
   *
   * @brief Type for specifying whether a CD is strict or relaxed
   *
   * This type is used to specify whether a CD is strict or
   * relaxed. The full definition of the semantics of strict and
   * relaxed CDs can be found in the semantics document under
   * <http://lph.ece.utexas.edu/public/CDs>. In brief, concurrent
   * tasks (threads, MPI ranks, ...) in two different strict CDs
   * cannot communicate with one another and must first complete inner
   * CDs so that communicating tasks are at the same CD context. Tasks
   * in two different relaxed CDs may communicate (verified data
   * only). Relaxed CDs typically incur additional runtime overhead
   * compared to strict CDs.
   */
    enum CDType  { kStrict=0, ///< A strict CD
              		 kRelaxed   ///< A relaxed CD
                 };

  /** @brief Type to indicate whether preserved data is from read-only
   * or potentially read/write application data
   *
   * \sa CDHandle::Preserve(), CDHandle::Complete()
   */
    enum PreserveUseT { kUnsure =0, //!< Not sure whether data being preserved will be written 
                                    //!< by the CD (treated as Read/Write for now, but may be optimized later)
                        kReadOnly = 1, //!< Data to be preserved is read-only within this CD
                        kReadWrite = 2 //!< Data to be preserved will be modified by this CD
    };
  
  /** @brief Type to indicate where to preserve data
   *
   * \sa CD::GetPlaceToPreserve()
   */
    enum PrvMediumT { kMemory=0,
                      kHDD,
                      kSSD,
                      kPFS};

  /** @} */ // End group cd_defs ===========================================

  /** \addtogroup PGAS_funcs PGAS-Specific Methods and Types
   *
   * @{
   */

  /** @brief Different types of PGAS memory behavior for relaxed CDs.
   *
   * Please see the CD semantics document at
   * <http://lph.ece.utexas.edu/public/CDs> for a full description of
   * PGAS semantics. In brief, because of the logging/tracking
   * requirements of relaxed CDs, it is important to identify which
   * memory accesses may be for communication between relaxed CDs
   * vs. memory accesses that are private (or temporarily privatized)
   * within this CD. 
   *
   */
  enum PGASUsageT {
    kSharedVar = 0,      //!< Definitely shared for actual communication
    kPotentiallyShared, //!< Perhaps used for communication by this
			//!< CD, essentially equivalent to kShared for CDs.
    kPrivatized,     //!< Shared in general, but not used for any
		      //!< communication during this CD.
    kPrivate          //!< Entirely private to this CD.
  };

  /** @} */ // end PGAS_funcs ===========================================





  /** \addtogroup profiler-related Profiler-related Methods and Types
   *
   * @{
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
                          MAX_PROFILE_DATA };

  /** @brief Profile format
   *
   */
  enum ProfileFormat    { PRV, 
                          REC, 
                          BODY, 
                          MAX_FORMAT };

  /** @} */ // end profiler-related group ===========================================


#ifdef comm_log
  //SZ
  enum CommLogErrT {kCommLogOK=0, 
                    kCommLogInitFailed, 
                    kCommLogAllocFailed,
                    kCommLogChildLogAllocFailed, 
                    kCommLogReInitFailed,
                    kCommLogCommLogModeFlip,
                    kCommLogChildLogNotFound,
                    kCommLogError};
  
  //SZ
  enum CommLogMode { kGenerateLog=0,
                       kReplayLog
                    };
#endif

  enum CDEventT { kNoEvent=0,
                  // Head -> Non-Head
                  kAllPause=1,
                  kAllResume=2,
                  kAllReexecute=4,
                  kEntrySend=8, 
                  // Non-Head -> Head
                  kEntrySearch=16,
                  kErrorOccurred=32,
                  kReserved=64 };

  enum CDEventHandleT { kEventNone = 0,
                        kEventResolved,
                        kEventPending };

  class CDNameT;

  // Local CDHandle object and CD object are managed by CDPath (Local means the current process)

  extern int myTaskID;
  extern int handled_event_count;
  extern int max_tag_bit;
  extern int max_tag_level_bit;
  extern int max_tag_rank_bit;
  extern int max_tag_task_bit;

  class DebugBuf: public std::ostringstream {
    std::ofstream ofs_;
  public:
    ~DebugBuf() {}
    DebugBuf(void) {}
    DebugBuf(const char *filename) 
      : ofs_(filename) {}
  
    void open(const char *filename)
    {
      ofs_.open(filename);
    }
    
    void close(void)
    {
      ofs_.close();
    }
    
    void flush(void) {
      ofs_ << str();
      ofs_.flush();
      str("");
      clear();
    }
      
  };



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
  
  class CommInfo { 
  public:
    void *addr_;
    MPI_Request req_;
    MPI_Status  stat_;
    int valid_;
    CommInfo(void *addr = NULL) : addr_(addr) {
      valid_ = 0;  
    }
    ~CommInfo() {}
    CommInfo &operator=(const CommInfo &that) {
      addr_  = that.addr_;
      req_   = that.req_;
      stat_  = that.stat_;
      valid_ = that.valid_;
      return *this;
    }
  };

  int GenMsgTag(ENTRY_TAG_T entry_tag, bool any_source=false); 

//uint32_t GenMsgTag(uint64_t tag, uint16_t cd_tag) 
  extern uint16_t CDTag16(void);
  extern uint32_t CDTag32(void); 
  extern uint64_t CDTag64(void);
  extern void ReadCDTag64(uint64_t cd_tag, uint32_t &level, uint32_t &rank_in_level, uint32_t &task_in_color);
  extern void ReadCDTag32(uint32_t cd_tag, uint32_t &level, uint32_t &rank_in_level, uint32_t &task_in_color);
  extern void ReadCDTag16(uint16_t cd_tag, uint32_t &level, uint32_t &rank_in_level, uint32_t &task_in_color);
  extern void ReadMsgTag(uint32_t msg_tag);

  extern std::map<ENTRY_TAG_T, std::string> tag2str;
  extern std::hash<std::string> str_hash;

  extern CDHandle* CD_Init(int numproc=1, int myrank=0);
  extern void CD_Finalize(DebugBuf *debugBuf=NULL);
  extern void WriteDbgStream(DebugBuf *debugBuf=NULL);
  extern uint64_t gen_object_id;
  //GONG: global variable to represent the current context for malloc wrapper
  extern bool app_side;

  static inline void nullFunc(void) {}

 /** \addtogroup runtime_logging Runtime logging-related functionality
  *  The \ref runtime_logging is supported in current CD runtime.
  * @{
  *
  *  @brief Set current context as non-application side. 
  *  @return true/false
  */
  static inline void CDPrologue(void) { app_side = false; }
 /** @brief Set current context as application side. 
  *  @return true/false
  */
  static inline void CDEpilogue(void) { app_side = true; }
 /** @brief Check current context is application side. 
  *  @return true/false
  */
  static inline bool CheckAppSide(void) { return app_side; }

  /** @} */ // End runtime_logging group =====================================================


  /** \addtogroup cd_accessor_funcs Global CD Accessor Functions
   *  The \ref cd_accessor_funcs are used to get the current and root
   *  CD handles if these are not explicitly tracked.
   *
   * These methods are globally accessible without a CDHandle object.
   *
   * @{
   *
   */
 /**
  * @brief Accessor function to current active CD.
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

 /**
  * @brief Accessor function to root CD of the application.
  * @return returns a pointer to the handle of the root CD; Returns
  * 0 if CD runtime is not yet initialized or because of a CD
  * implementation bug.
  */
  CDHandle *GetRootCD(void);

 /** @brief Accessor function to a CDHandle at a specific level in CDPath 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(void);

 /** @brief Accessor function to a CDHandle at a specific level in CDPath 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(int current_level);

  /** @} */ // End cd_accessor_funcs group =====================================================

  static inline ENTRY_TAG_T cd_hash(const std::string &str)
  { return static_cast<ENTRY_TAG_T>(cd::str_hash(str)); }

//  static inline ENTRY_TAG_T cd_hash(const std::string &str)

//static inline  ENTRY_TAG_T cd_hash(const std::string &str)
//{ return static_cast<ENTRY_TAG_T>(cd::str_hash(str)); }

  extern inline std::string event2str(int event) {
    switch(event) {
      case 0:
        return "kNoEvent";
      case 1:
        return "kAllPause";
      case 2:
        return "kAllResume";
      case 4:
        return "kAllReexecute";
      case 8:
        return "kEntrySend";
      case 16:
        return "kEntrySearch";
      case 32:
        return "kErrorOccurred";
      case 64:
        return "kReserved";
      default:
        return "UNDEFINED EVENT";
    }

//    std::string eventStr;
//    if(CHECK_NO_EVENT(event)) {
//        eventStr = "kNoEvent";
//    }
//    else {
//      if(CHECK_EVENT(event, kAllPause))
//        eventStr += "kAllPause ";
//      if(CHECK_EVENT(event, kAllResume))
//        eventStr += "kAllResume ";
//      if(CHECK_EVENT(event, kEntrySend))
//        eventStr += "kEntrySend ";
//      if(CHECK_EVENT(event, kEntrySearch))
//        eventStr += "kEntrySearch ";
//      if(CHECK_EVENT(event, kErrorOccurred))
//        eventStr += "kErrorOccurred ";
//      if(CHECK_EVENT(event, kReserved))
//        eventStr += "kReserved ";
//      if(eventStr.empty()) 
//        eventStr = "UNDEFINED EVENT";
//    }
//    return eventStr;

  }
}


//SZ: change the following macro to 1) add "Error: " before all error messages,
//    2) accept multiple arguments, and 3) assert(0) in ERROR_MESSAGE
//#define ERROR_MESSAGE(X) printf(X);
#define ERROR_MESSAGE(...) {printf("\nError: ");printf(__VA_ARGS__);assert(0);}
//#ifdef DEBUG
//#define DEBUG_PRINT(...) {printf(__VA_ARGS__);}
//#else
//#define DEBUG_PRINT(...) {}
//#endif

//SZ: when testing MPI functions, print to files is easier
#ifdef comm_log 

#if _DEBUG
extern FILE * cd_fp;
#endif

#endif

#if _DEBUG
  //SZ: change to this if want to compile test_comm_log.cc
  #ifdef comm_log
//    #define PRINT_DEBUG(...) {fprintf(cd_fp,__VA_ARGS__);}
    #define PRINT_DEBUG(...) {if(cd::app_side){cd::app_side=false;printf(__VA_ARGS__);cd::app_side = true;}else printf(__VA_ARGS__);}
  #else
    #define PRINT_DEBUG(...) {printf(__VA_ARGS__);}
  #endif

  #define PRINT_DEBUG2(X,Y) printf(X,Y);
#else
  #define PRINT_DEBUG(...) {}
  #define PRINT_DEBUG2(X,Y) printf(X,Y);
#endif

//SZ temp disable libc printf
#define PRINT_LIBC(...) {printf(__VA_ARGS__);}


#define INITIAL_CDOBJ_NAME "INITIAL_NAME"

#define MAX_FILE_PATH 2048
#define INIT_FILE_PATH "INITIAL_FILE_PATH"
#define CD_FILEPATH_DEFAULT "./HDD/"
#define CD_FILEPATH_INVALID "./error_logs/"
#define CD_FILEPATH_PFS "./PFS/"
#define CD_FILEPATH_HDD "./HDD/"
#define CD_FILEPATH_SSD "./SSD/"
#define dout clog

/* 
ISSUE 1 (Kyushick)
If we do if-else statement here and make a scope { } for that, does it make its own local scope in the stack?

void Foo(void)
{
  double X, Y, Z;
  ...
  if (IsTrue == true) { 
    int A, B, C, D;
    ...
    ...
    A = B + C; B = D * A;
    ...
  } 
  else {
    int E, F;
    ...
    E = F * E;
    ...
  }
  ...
}

there will be Foo's local scope in the stack and what about A, B, C, D, E, F ??
A, B, C, D, E, F's life cycle will be up to the end of if-then statement. 
So, I guess the stack will grow/shrink a bit due to this if-then statement.
So, if we setjmp or get context within this if-then statement's scope,
I think there will be some problem...

ISSUE 2 (Kyushick)
We are increasing the number of reexecution inside Begin(). So, the point of time when we mark rollback point is not after Begin() but before Begin()
*/

//#define CD_Begin(X) (X)->Begin(); if((X)->ctxt_prv_mode() ==CD::kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff()

// Macros for setjump / getcontext
// So users should call this in their application, not call cd_handle->Begin().

#define CD_Begin(X) if((X)->ctxt_prv_mode() ==CD::kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin();
//#define CD_Begin(X) (X)->Begin(); if((X)->ctxt_prv_mode() ==CD::kExcludeStack) (X)->jmp_val_=setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff();
#define CD_Complete(X) (X)->Complete()   

#endif
