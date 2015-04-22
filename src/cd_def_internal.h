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

#ifndef _CD_DEF_INTERNAL_H
#define _CD_DEF_INTERNAL_H

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
  namespace logging {

#ifdef comm_log
  //SZ
  class CommLog;
#endif
  }
  namespace interface {

  }
}

using namespace cd;
using namespace cd::internal;
using namespace cd::interface;
using namespace cd::logging;
#include <string>
#include <vector>
#include <map>
#include <csetjmp>
#include <cstdio>

#if _MPI_VER 

#include <mpi.h>
//#define ROOT_COLOR    MPI_COMM_WORLD
//#define INITIAL_COLOR MPI_COMM_NULL
//#define ROOT_HEAD_ID  0
//typedef MPI_Comm      ColorT;
typedef MPI_Group     CommGroupT;
typedef MPI_Request   CommRequestT;
typedef MPI_Status    CommStatusT;
typedef int           CDFlagT;
typedef MPI_Win       CDMailBoxT;
typedef MPI_Offset    COMMLIB_Offset;
typedef MPI_File      COMMLIB_File;

#else

//#define ROOT_COLOR    0 
//#define INITIAL_COLOR 0
//#define ROOT_HEAD_ID  0
//typedef int           ColorT;
typedef int           CommGroupT;
typedef int           CommRequestT;
typedef int           CommStatusT;
// FIXME
typedef int           CDFlagT;
typedef int           CDMailBoxT;
typedef int           COMMLIB_Offset;
typedef int           COMMLIB_File;

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
#define CHECK_PRV_TYPE(X,Y)     ((X & Y) == Y)
#define MASK_CDTYPE(X)         (X & 0x3)
#define MASK_MEDIUM(X)          (X & 0xFFFFFFFC)
#define CHECK_EVENT(X,Y)        ((X & Y) == Y)
#define CHECK_NO_EVENT(X)       (X == 0)
#define SET_EVENT(X,Y)          (X |= Y)

#define TAG_MASK(X) ((2<<(X-1)) - 1)
#define TAG_MASK2(X) ((2<<(X-1)) - 2)

//GONG: global variable to represent the current context for malloc wrapper
namespace cd {

  class CDHandle;
  class Serializable;
  class Packer; 
  class Unpacker; 
//  class Util;
  class RegenObject;  
  class RecoverObject;
  class SysErrT;



/** \addtogroup cd_defs 
 *@{
 */
/** 
 * @brief Type for specifying whether the current CD is executing
 * for the first time or is currently reexecuting as part of recovery.
 *
 * During reexecution, data is restored instead of being
 * preserved. Additionally, for relaxed CDs, some communication and
 * synchronization may not repeated and instead preserved (logged)
 * values are used. 
 * See <http://lph.ece.utexas.edu/public/CDs> for a detailed
 * description. Note that this is not part of the cd_internal
 * namespace because the application programmer may want to manipulate
 * this when specifying specialized recovery routines.
 */
    enum CDExecMode  {kExecution=0, //!< Execution mode 
                      kReexecution, //!< Reexecution mode
                      kSuspension   //!< Suspension mode (not active)
                     };

/** @} */ // End group cd_defs



/** @brief Profile-related enumerator
 *
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


  // Local CDHandle object and CD object are managed by CDPath (Local means the current process)

  extern int myTaskID;
#if _MPI_VER
  extern int handled_event_count;
  extern int max_tag_bit;
  extern int max_tag_level_bit;
  extern int max_tag_rank_bit;
  extern int max_tag_task_bit;
#endif



/**@class cd::CommInfo
 * @brief Data structure that contains the communication message information generated in CD runtime.
 *
 *  There are some communications among task group in a CD, and some of them are non-blocking calls.
 *  So, this class is used to contain and track the completion of the messages.
 */ 
  class CommInfo { 
  public:
    void *addr_;
    CommRequestT req_;   //<! MPI_Request
    CommStatusT  stat_;  //<! MPI_Status

    int valid_;         //<! Valid bit for the message completion.
    CommInfo(void *addr = NULL) : addr_(addr) {
      valid_ = 0;  
    }
    ~CommInfo() {}

/**@brief Copy operator for class CommInfo.
 */
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

  extern void WriteDbgStream(DebugBuf *debugBuf=NULL);
  extern uint64_t gen_object_id;

  //GONG: global variable to represent the current context for malloc wrapper
//  extern bool app_side;

  static inline void nullFunc(void) {}


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
    enum CtxtPrvMode { kExcludeStack=0, 
                       kIncludeStack
                     };



/**@addtogroup runtime_logging 
 * @{
 */

/**@brief Set current context as non-application side. 
 * @return true/false
 */
  static inline void CDPrologue(void) { app_side = false; }

/**@brief Set current context as application side. 
 * @return true/false
 */
  static inline void CDEpilogue(void) { app_side = true; }

/**@brief Check current context is application side. 
 * @return true/false
 */
  static inline bool CheckAppSide(void) { return app_side; }

 /** @} */ // End runtime_logging group =====================================================

#if _MPI_VER
  extern inline void IncHandledEventCounter(void)
  { handled_event_count++; }
#endif
}

#define ERROR_MESSAGE(...) \
  { fprintf(stderr, __VA_ARGS__); assert(0); }


#if _CD_DEBUG == 0  // No printouts -----------

#define CD_DEBUG(...) 
#define LOG_DEBUG(...) 
#define LIBC_DEBUG(...)
 
#elif _CD_DEBUG == 1  // Print to fileout -----

extern FILE *cdout;
extern FILE *cdoutApp;

#define CD_DEBUG(...) \
  fprintf(cdout, __VA_ARGS__)


#define LOG_DEBUG(...) /*\
  { if(cd::app_side) {\
      cd::app_side=false;\
      fprintf(stdout, __VA_ARGS__);\
      cd::app_side = true;}\
    else fprintf(stdout, __VA_ARGS__);\
  }*/

#define LIBC_DEBUG(...) /*\
    { if(cd::app_side) {\
        cd::app_side=false;\
        fprintf(stdout, __VA_ARGS__);\
        cd::app_side = true;}\
      else fprintf(stdout, __VA_ARGS__);\
    }*/



#elif _CD_DEBUG == 2  // print to stdout ------

#define CD_DEBUG(...) \
  fprintf(stdout, __VA_ARGS__)


#define LOG_DEBUG(...) /*\
  { if(cd::app_side) {\
      cd::app_side=false;\
      fprintf(stdout, __VA_ARGS__);\
      cd::app_side = true;}\
    else fprintf(stdout, __VA_ARGS__);\
  }*/

#define LIBC_DEBUG(...)/* \
    { if(cd::app_side) {\
        cd::app_side=false;\
        fprintf(stdout, __VA_ARGS__);\
        cd::app_side = true;}\
      else fprintf(stdout, __VA_ARGS__);\
    }*/


#else  // -------------------------------------

#define CD_DEBUG(...) \
  fprintf(stderr, __VA_ARGS__)

#endif




#define INITIAL_CDOBJ_NAME "INITIAL_NAME"

#define MAX_FILE_PATH 2048
#define CD_FILEPATH_INVALID "./error_logs/"
#define INIT_FILE_PATH "INITIAL_FILE_PATH"
#define CD_FILEPATH_DEFAULT "./HDD/"
#define CD_FILEPATH_PFS "./PFS/"
#define CD_FILEPATH_HDD "./HDD/"
#define CD_FILEPATH_SSD "./SSD/"
#define CD_DEFAULT_PRV_FILEPATH "./"
#define CD_DEFAULT_DEBUG_OUT "./output/"

#define CD_SHARING_DEGREE 64
#define dout clog

#define DEFAULT_MEDIUM kHDD

#define CheckHere() \
  if(cd::app_side) assert(0);

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

//#define CD_Begin(X) if((X)->ctxt_prv_mode() == kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin();
////#define CD_Begin(X) (X)->Begin(); if((X)->ctxt_prv_mode() ==CD::kExcludeStack) (X)->jmp_val_=setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff();
//#define CD_Complete(X) (X)->Complete()   

#endif
