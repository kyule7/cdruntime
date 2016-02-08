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

#include <cstdio>
#include <csetjmp>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <errno.h>
#include "cd_features.h"
#define EntryDirType std::unordered_map<ENTRY_TAG_T,CDEntry*>

namespace cd {
//  namespace internal {
//
//    class CD;
//    class HeadCD;
//    class CDPath;
//    class CDEntry;
//    class DataHandle;
//    class NodeID;
//    class CDNameT;
//    class CDID;
//    class CDEvent;
//    class PFSHandle;
//  }
//  namespace logging {
//    class CommLog;
//    class RuntimeLogger;
//  }
  namespace interface {

  }
}

using namespace cd;
using namespace cd::internal;
using namespace cd::interface;
using namespace cd::logging;

#if CD_MPI_ENABLED 

#include <mpi.h>
//#define ROOT_COLOR    MPI_COMM_WORLD
//#define INITIAL_COLOR MPI_COMM_NULL
//#define ROOT_HEAD_ID  0
//typedef MPI_Comm      ColorT;
//typedef MPI_Group     CommGroupT;
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
//typedef int           CommGroupT;
typedef int           CommRequestT;
typedef int           CommStatusT;
// FIXME
typedef int           CDFlagT;
typedef int           CDMailBoxT;
typedef int           COMMLIB_Offset;
typedef int           COMMLIB_File;

#endif


typedef uint32_t ENTRY_TAG_T;

#define CD_FLAG_T uint32_t
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
#define MASK_CDTYPE(X)          (X & 0x03)
#define MASK_MEDIUM(X)          (X & 0xFC)
#define CHECK_EVENT(X,Y)        ((X & Y) == Y)
#define CHECK_NO_EVENT(X)       (X == 0)
#define SET_EVENT(X,Y)          (X |= Y)

#define TAG_MASK(X) ((2<<(X-1)) - 1)
#define TAG_MASK2(X) ((2<<(X-1)) - 2)

//GONG: global variable to represent the current context for malloc wrapper
namespace cd {

  class CDHandle;
//  class Serializable;
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




/** \addtogroup profiler-related
 *@{
 */

/** @brief Profile format
 *
 */
  enum ProfileFormat    { PRV, 
                          REC, 
                          BODY, 
                          MAX_FORMAT };

/** @} */ // end profiler-related group ===========================================


#if CD_COMM_LOG_ENABLED
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
  //SZ
  enum CDLoggingMode {kOutOfCD=0,
                      kStrictCD,
                      kRelaxedCDGen,
                      kRelaxedCDRead};
#endif

//  enum CDEventT { kNoEvent=0,
//                  // Head -> Non-Head
//                  kAllPause=1,
//                  kAllResume=2,
//                  kAllReexecute=4,
//                  kEntrySend=8, 
//                  // Non-Head -> Head
//                  kEntrySearch=16,
//                  kErrorOccurred=32,
//                  kReserved=64 };

  enum CDEventHandleT { kEventNone = 0,
                        kEventResolved,
                        kEventPending };


  // Local CDHandle object and CD object are managed by CDPath (Local means the current process)

  extern int myTaskID;
#if CD_MPI_ENABLED
  extern MPI_Group whole_group;
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
//  extern std::hash<std::string> str_hash;
//  extern EntryDirType::hasher str_hash;
  extern std::unordered_map<std::string,CDEntry*>::hasher str_hash;
  extern std::unordered_map<std::string,CDEntry*> str_hash_map;

  extern uint64_t gen_object_id;

  //GONG: global variable to represent the current context for malloc wrapper
//  extern bool app_side;

  static inline void nullFunc(void) {}


  extern inline ENTRY_TAG_T cd_hash(const std::string &str)
  { return static_cast<ENTRY_TAG_T>(cd::str_hash(str)); }

//  static inline ENTRY_TAG_T cd_hash(const std::string &str)


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
//    enum CtxtPrvMode { kExcludeStack=0, 
//                       kIncludeStack
//                     };



/**@addtogroup runtime_logging 
 * @{
 */

/**@brief Set current context as non-application side. 
 * @return true/false
 */
#if _PROFILER

#define CDPrologue() \
  app_side = false;\
  profiler_->RecordClockBegin();

#else


#define CDPrologue() \
  app_side = false;

#endif
//  static inline void CDPrologue(void) 
//  { 
//    app_side = false; 
//#if _PROFILER
//    profiler_->RecordClockBegin();
//#endif
//  }



/**@brief Set current context as application side. 
 * @return true/false
 */
#if _PROFILER

#define CDEpilogue() \
  app_side = true;\
  profiler_->RecordClockEnd();

#else

#define CDEpilogue() \
  app_side = true;

#endif
//  static inline void CDEpilogue(void) 
//  { 
//    app_side = true; 
//#if _PROFILER
//    profiler_->RecordClockEnd();
//#endif
//    
//  }

/**@brief Check current context is application side. 
 * @return true/false
 */
  static inline bool CheckAppSide(void) { return app_side; }

 /** @} */ // End runtime_logging group =====================================================

#if CD_MPI_ENABLED
  extern inline void IncHandledEventCounter(void)
  { handled_event_count++; }
#endif



  namespace logging {

    // data structure to store incompleted log entries
    struct IncompleteLogEntry {
      uint32_t thread_;
      //void * addr_;
      unsigned long addr_;
      unsigned long length_;
      unsigned long flag_;
      bool complete_;
      bool isrecv_;
      bool intra_cd_msg_;
      //GONG
      void* p_;
      bool pushed_;
      unsigned int level_;
      //bool valid_;
      //SZ
      IncompleteLogEntry(void) {
        thread_ = 0;
        //void * addr_;
        addr_ = 0;
        length_ = 0;
        flag_ = 0;
        complete_ = 0;
        isrecv_ = 0;
        intra_cd_msg_ = false;
        //GONG
        p_ = 0;
        pushed_ = 0;
        level_ = 0;
      }
    };

  }

} // namespace cd ends

#define ERROR_MESSAGE(...) \
  { fprintf(stderr, __VA_ARGS__); assert(0); }


#if CD_DEBUG_DEST == CD_DEBUG_SILENT  // No printouts 

#define CD_DEBUG(...) 
#define LOG_DEBUG(...) 
#define LIBC_DEBUG(...)
 
#elif CD_DEBUG_DEST == CD_DEBUG_TO_FILE  // Print to fileout

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



#elif CD_DEBUG_DEST == CD_DEBUG_STDOUT  // print to stdout 

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


#elif CD_DEBUG_DEST == CD_DEBUG_STDERR  // print to stderr

#define CD_DEBUG(...) \
  fprintf(stderr, __VA_ARGS__)

#else  // -------------------------------------

#define CD_DEBUG(...) \
  fprintf(stderr, __VA_ARGS__)

#endif




#define INITIAL_CDOBJ_NAME "INITIAL_NAME"
#define INITIAL_CDOBJ_LABEL "INITIAL_CDOBJ_LABEL"

#define MAX_FILE_PATH 256
#define CD_FILEPATH_INVALID "./error_logs/"
#define INIT_FILE_PATH "INITIAL_FILE_PATH"
#define CD_FILEPATH_PFS "PFS/"
#define CD_FILEPATH_HDD "HDD/"
#define CD_FILEPATH_SSD "SSD/"
#define CD_DEFAULT_PRV_BASEPATH "./"
#define CD_DEFAULT_PRV_FILENAME "prv_files_%s_XXXXXX"
#define CD_DEFAULT_FILEPATH "./prv_files_XXXXXX"
#define CD_DEFAULT_DEBUG_OUT "./debug_logs/"

#define CD_SHARING_DEGREE 64
#define dout clog

//#define DEFAULT_MEDIUM kDRAM

#define CheckHere() \
  if(cd::app_side) assert(0);


#endif
