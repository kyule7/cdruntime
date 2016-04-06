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
#include <cstdint>
#include <csetjmp>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <errno.h>
#include "cd_features.h"
#include "cd_def_debug.h"
#include "cd_def_interface.h"
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
//typedef MPI_Group     GroupT;
typedef MPI_Request   CommRequestT;
typedef MPI_Status    CommStatusT;
typedef uint32_t      CDFlagT;
typedef MPI_Win       CDMailBoxT;
typedef MPI_Offset    COMMLIB_Offset;
typedef MPI_File      COMMLIB_File;
#define INVALID_COLOR MPI_COMM_NULL

#else

//#define ROOT_COLOR    0 
//#define INITIAL_COLOR 0
//#define ROOT_HEAD_ID  0
//typedef int           ColorT;
//typedef int           GroupT;
typedef int           CommRequestT;
typedef int           CommStatusT;
// FIXME
typedef uint32_t      CDFlagT;
typedef int           CDMailBoxT;
typedef int           COMMLIB_Offset;
typedef int           COMMLIB_File;
#define INVALID_COLOR -1

#endif


typedef uint32_t ENTRY_TAG_T;

#define CD_FLAG_T uint32_t
#define MAX_ENTRY_BUFFER_SIZE 1024

#define MSG_TAG_ENTRY_TAG 1073741824 // 2^30
#define MSG_TAG_ENTRY     2147483648 // 2^31
#define MSG_TAG_DATA      3221225472 // 2^31 + 2^30
#define MSG_MAX_TAG_SIZE  1073741824 // 2^30. TAG is
//#define GEN_MSG_TAG(X) 

#define CD_INT32_MAX  (0x0FFFFFFF)
#define CD_INT32_MIN  (0x10000000)
#define CD_UINT32_MAX (0xFFFFFFFF)
#define CD_UINT64_MAX (0xFFFFFFFFFFFFFFFF)
#define MPI_ERR_NEED_ESCALATE (0xFF00)

#define INIT_TAG_VALUE   0
#define INIT_ENTRY_SRC   0
#define INVALID_TASK_ID -1
#define INVALID_HEAD_ID -1
#define INVALID_ROLLBACK_POINT 0xFFFFFFFF
#define INVALID_MSG_TAG -1
#define NUM_FLAGS 1024


#define INITIAL_ERR_VAL kOK
#define DATA_MALLOC malloc
#define DATA_FREE free

#define CHECK_EVENT_NO_EVENT(X) (X == 0)
#define CHECK_PRV_TYPE(X,Y)     ((X & Y) == Y)
#define MASK_CDTYPE(X)          (X & 0x03)
#define MASK_MEDIUM(X)          (X & 0xFC)
#define CHECK_EVENT(X,Y)        ((X & Y) == Y)
#define CHECK_NO_EVENT(X)       (((X) == 0))
#define SET_EVENT(X,Y)          (X |= Y)

#define TAG_MASK(X) ((2<<(X-1)) - 1)
#define TAG_MASK2(X) ((2<<(X-1)) - 2)



#define ROOT_SYS_DETECT_VEC 0xFFFFFFFFFFFFFFFF

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
                    kCommLogMissing,
                    kCommLogError};
  
  //SZ
  enum CommLogMode { kGenerateLog=1,
                     kReplayLog=2,
                     kInvalidLogMode=-1
                    };
  //SZ
  enum CDLoggingMode { kOutOfCD=1, 
                       kStrictCD=2,
                       kRelaxedCDGen=4, // kRelaxed and kExec
                       kRelaxedCDRead=8, // kRelaxed and kRexec
                       kInvalidLoggingMode=-1
                     };
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
//  extern int handled_event_count;
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
      case BIT_0:
        return "kAllPause";
      case BIT_1:
        return "kAllResume";
      case BIT_2:
        return "kAllReexecute";
      case BIT_3:
        return "kAllEscalate";
      case BIT_4:
        return "kEntrySend";
      case BIT_5:
        return "kEntrySend";
      case BIT_6:
        return "kEntrySearch";
      case BIT_7:
        return "kErrorOccurred";
      case BIT_8:
        return "kEscalationDetected";
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

//#if CD_MPI_ENABLED
//#define CD_CLOCK_T double
//#define CLK_NORMALIZER (1.0)
//#define CD_CLOCK MPI_Wtime
//#else
//#include <ctime>
//#define CD_CLOCK_T clock_t
//#define CLK_NORMALIZER CLOCKS_PER_SEC
//#define CD_CLOCK clock
//#endif

extern CD_CLOCK_T tot_begin_clk;
extern CD_CLOCK_T tot_end_clk;

extern CD_CLOCK_T msg_begin_clk;
extern CD_CLOCK_T msg_end_clk;
extern CD_CLOCK_T msg_elapsed_time;

extern CD_CLOCK_T log_begin_clk;
extern CD_CLOCK_T log_end_clk;
extern CD_CLOCK_T log_elapsed_time;

extern CD_CLOCK_T prof_begin_clk;
extern CD_CLOCK_T prof_end_clk;
extern CD_CLOCK_T prof_sync_clk;

extern CD_CLOCK_T begin_clk;
extern CD_CLOCK_T end_clk;
extern CD_CLOCK_T elapsed_time;
extern CD_CLOCK_T normal_sync_time;
extern CD_CLOCK_T reexec_sync_time;
extern CD_CLOCK_T recovery_sync_time;
extern CD_CLOCK_T prv_elapsed_time;
extern CD_CLOCK_T create_elapsed_time;
extern CD_CLOCK_T destroy_elapsed_time;
extern CD_CLOCK_T begin_elapsed_time;
extern CD_CLOCK_T compl_elapsed_time;

extern CD_CLOCK_T mailbox_elapsed_time;
/**@addtogroup runtime_logging 
 * @{
 */

#define MsgPrologue() \
  app_side = false; \
  msg_begin_clk = CD_CLOCK(); 

#define MsgEpilogue() \
  app_side = true; \
  msg_end_clk = CD_CLOCK(); \
  msg_elapsed_time += msg_end_clk - msg_begin_clk; 

#define LogPrologue() \
  log_begin_clk = CD_CLOCK(); 

#define LogEpilogue() \
  log_end_clk = CD_CLOCK(); \
  log_elapsed_time += log_end_clk - log_begin_clk; 


/**@brief Set current context as non-application side. 
 * @return true/false
 */

#define CDPrologue() \
  app_side = false; \
  begin_clk = CD_CLOCK(); 


/**@brief Set current context as application side. 
 * @return true/false
 */

#define CDEpilogue() \
  app_side = true; \
  end_clk = CD_CLOCK(); \
  elapsed_time += end_clk - begin_clk; 


/**@brief Check current context is application side. 
 * @return true/false
 */
  static inline bool CheckAppSide(void) { return app_side; }

 /** @} */ // End runtime_logging group =====================================================

//#if CD_MPI_ENABLED
//  extern inline void IncHandledEventCounter(void)
//  { handled_event_count++; }
//#endif


#if 1
  namespace logging {

    // data structure to store incompleted log entries
    struct IncompleteLogEntry {
      void    *addr_;
      uint64_t length_;
      uint32_t taskID_;
      uint32_t tag_;
      MPI_Comm comm_;
      void    *flag_;
      bool     complete_;
      bool     isrecv_;
      bool     intra_cd_msg_;
      //GONG
      void    *p_;
      bool     pushed_;
      uint32_t level_;
      IncompleteLogEntry(void) {
        taskID_ = 0;
        addr_ = NULL;
        length_ = 0;
        flag_ = NULL;
        complete_ = 0;
        isrecv_ = 0;
        intra_cd_msg_ = false;
        comm_ = MPI_COMM_NULL;
        tag_  = INVALID_MSG_TAG;
        //GONG
        p_ = 0;
        pushed_ = 0;
        level_ = 0;
      }
      IncompleteLogEntry(const void *addr, 
                         uint64_t length, 
                         uint32_t taskID, 
                         uint32_t tag, 
                         const MPI_Comm &comm, 
                         void    *flag, 
                         bool complete) 
        : addr_(const_cast<void *>(addr)), length_(length), taskID_(taskID), tag_(tag), 
          comm_(comm), flag_(flag), complete_(complete) 
      {
        CD_DEBUG("pushed back:%p\n", flag);
        p_ = NULL;
        pushed_ = 0;
        level_ = 0;
        isrecv_ = 0;
        intra_cd_msg_ = false;
      }
      void Print(void) {
        printf("\n== Incomplete Log Entry ==\ntaskID:%u\nlength:%lu\naddr:%p\ntag:%u\ncomm:%u\nflag:%p\ncomplete:%d\nisrecv:%d\nintra_msg:%d\np:%p\npushed:%d\nlevel:%u\n==========================\n", taskID_, length_, addr_, tag_, comm_, flag_, complete_, isrecv_, intra_cd_msg_, p_, pushed_, level_);
      }

    };

    class IncompleteLogStore : public std::vector<IncompleteLogEntry> {
      uint32_t unit_size_;
    public:
      IncompleteLogStore(){}
      IncompleteLogStore(uint32_t unit_size) : unit_size_(unit_size) {}
      std::vector<IncompleteLogEntry>::iterator find(void *flag) {
        std::vector<IncompleteLogEntry>::iterator it = begin();
        for(; it!=end(); ++it) {
          if(it->flag_ == flag) 
            break;
        }
        return it;
      }
    };

//    class MsgHandle {
//      public:
//        static int  BlockUntilValid(CD *cd_p, MPI_Request *request, MPI_Status *status) {
//          return cd_p->BlockUntilValid(request, status);
//        }
//    
//        static void Escalate(CDHandle *leaf, bool need_sync_to_reexec) {
//          (leaf->ptr_cd()->GetCDToRecover(leaf, need_sync_to_reexec))->ptr_cd()->Recover();
//        }
//    
//        // TODO
//        static bool CheckIntraCDMsg(CD *cd_p, int target_id, MPI_Group &target_group) {
//          return cd_p->CheckIntraCDMsg(target_id, target_group);
//        }
//    };
  } // namespace logging ends
#endif

} // namespace cd ends





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

#define DEFAULT_INCOMPL_LOG_SIZE 64

#define CD_SHARING_DEGREE 64
#define dout clog

//#define DEFAULT_MEDIUM kDRAM

#define CheckHere() \
  if(cd::app_side) assert(0);


#endif
