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
#include <stdint.h>
#include <csetjmp>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <errno.h>
#include "cd_features.h"
#include "cd_def_common.h"
#include "cd_def_interface.h"
#include "cd_global.h"
//#include "libc_wrapper.h"

namespace packer {
  class CDEntry;
}

typedef uint64_t ENTRY_TAG_T;
typedef uint32_t CD_FLAG_T;
typedef std::unordered_map<ENTRY_TAG_T, packer::CDEntry*> EntryDirType;

//// This is like MappingConfig
//class PhaseMap : public std::map<std::string, uint32_t> {
//  public:
//    //std::string prev_phase_;
//    uint32_t phase_gen_;
//};
//
//typedef std::map<uint32_t, PhaseMap> PhaseMapType;

namespace cd {
  // phaseMap generates phase ID for CDNameT at each level.
  // Different label can create different phase,
  // therefore CD runtime compares the label of current CD being created
  // with that of preceding sequential CD at the same level. See GetPhase()
  // 
  // The scope of phase ID is limited to the corresponding level,
  // which means that the same label at different levels are different phases.
//  extern PhaseMapType phaseMap;
//  extern PhasePathType phasePath;
//  extern std::vector<PhaseNode *> phaseNodeCache;

  namespace internal {

    class CD;
    class HeadCD;
    class CDPath;
    //class CDEntry;
    class DataHandle;
    class NodeID;
    class CDNameT;
    class CDID;
    //class PFSHandle;
  }
  namespace logging {
    class CommLog;
    class RuntimeLogger;
  }
//  namespace interface {
//
//  }
  class CDEvent;
}

using namespace common;
using namespace cd::internal;
using namespace interface;
using namespace cd::logging;

#if CD_MPI_ENABLED 

#include <mpi.h>
#if 0
//#define ROOT_COLOR    MPI_COMM_WORLD
//#define INITIAL_COLOR MPI_COMM_NULL
//#define ROOT_HEAD_ID  0
//typedef MPI_Comm      ColorT;
//typedef MPI_Group     GroupT;
#endif
typedef MPI_Request   CommRequestT;
typedef MPI_Status    CommStatusT;
typedef uint32_t      CDFlagT;
typedef MPI_Win       CDMailBoxT;
typedef MPI_Offset    COMMLIB_Offset;
typedef MPI_File      COMMLIB_File;
#define INVALID_COLOR MPI_COMM_NULL

#else

#if 0
//#define ROOT_COLOR    0 
//#define INITIAL_COLOR 0
//#define ROOT_HEAD_ID  0
//typedef int           ColorT;
//typedef int           GroupT;
#endif
typedef int           CommRequestT;
typedef int           CommStatusT;
// FIXME
typedef uint32_t      CDFlagT;
typedef int           CDMailBoxT;
typedef int           COMMLIB_Offset;
typedef int           COMMLIB_File;
#define INVALID_COLOR -1

#endif

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


#define INITIAL_ERR_VAL common::kOK
#define DATA_MALLOC malloc
#define DATA_FREE free

#define CHECK_EVENT_NO_EVENT(X) (X == 0)
#define CHECK_TYPE(X,Y)         (((X) & (Y)) == (Y))
#define CHECK_ANY(X,Y)          (((X) & (Y)) != 0)
#define CHECK_PRV_TYPE(X,Y)     (((X) & (Y)) == (Y))
#define MASK_TYPE(X,Y)          ((X) & (Y))
#define MASK_CDTYPE(X)          ((X) & 0x03)
#define MASK_MEDIUM(X)          ((X) & 0xFC)
#define CHECK_EVENT(X,Y)        (((X) & (Y)) == (Y))
#define CHECK_NO_EVENT(X)       (((X) == 0))
#define SET_EVENT(X,Y)          ((X) |= (Y))

#define TAG_MASK(X) ((2<<((X)-1)) - 1)
#define TAG_MASK2(X) ((2<<((X)-1)) - 2)

#define ROOT_SYS_DETECT_VEC 0xFFFFFFFFFFFFFFFF

#define INITIAL_CDOBJ_NAME "INITIAL_NAME"
#define INITIAL_CDOBJ_LABEL "INITIAL_CDOBJ_LABEL"
#define DEFAULT_INCOMPL_LOG_SIZE 64
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



///** \addtogroup cd_defs 
// *@{
// */
///** 
// * @brief Type for specifying whether the current CD is executing
// * for the first time or is currently reexecuting as part of recovery.
// *
// * During reexecution, data is restored instead of being
// * preserved. Additionally, for relaxed CDs, some communication and
// * synchronization may not repeated and instead preserved (logged)
// * values are used. 
// * See <http://lph.ece.utexas.edu/public/CDs> for a detailed
// * description. Note that this is not part of the cd_internal
// * namespace because the application programmer may want to manipulate
// * this when specifying specialized recovery routines.
// */
//  enum CDExecMode  {kExecution=0, //!< Execution mode 
//                    kReexecution, //!< Reexecution mode
//                    kSuspension   //!< Suspension mode (not active)
//                     };
//
///** @} */ // End group cd_defs




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
  extern std::unordered_map<std::string,packer::CDEntry*>::hasher str_hash;
  extern std::unordered_map<std::string,packer::CDEntry*> str_hash_map;

  extern uint64_t gen_object_id;

  static inline void nullFunc(void) {}


  extern inline ENTRY_TAG_T cd_hash(const std::string &str)
  { return static_cast<ENTRY_TAG_T>(cd::str_hash(str)); }

//  static inline ENTRY_TAG_T cd_hash(const std::string &str)


  extern inline std::string event2str(int event) {
    switch(event) {
      case kNoEvent :
        return "kNoEvent";
      case kAllPause :
        return "kAllPause";
      case kAllResume :
        return "kAllResume";
      case kAllReexecute :
        return "kAllReexecute";
      case kAllEscalate :
        return "kAllEscalate";
      case kEntrySend :
        return "kEntrySend";
      case kReserved :
        return "kReserved";
      case kEntrySearch :
        return "kEntrySearch";
      case kErrorOccurred :
        return "kErrorOccurred";
      case kEscalationDetected :
        return "kEscalationDetected";
      default:
        return "UNDEFINED EVENT";
    }
  }

extern CD_CLOCK_T tot_begin_clk;
extern CD_CLOCK_T tot_end_clk;

extern CD_CLOCK_T msg_begin_clk;
extern CD_CLOCK_T msg_end_clk;
extern CD_CLOCK_T msg_elapsed_time;

extern CD_CLOCK_T log_begin_clk;
extern CD_CLOCK_T log_end_clk;
extern CD_CLOCK_T log_elapsed_time;

//extern CD_CLOCK_T prof_begin_clk;
//extern CD_CLOCK_T prof_end_clk;
//extern CD_CLOCK_T prof_sync_clk;
//
//extern CD_CLOCK_T begin_clk;
//extern CD_CLOCK_T end_clk;
//extern CD_CLOCK_T elapsed_time;
//extern CD_CLOCK_T normal_sync_time;
//extern CD_CLOCK_T reexec_sync_time;
//extern CD_CLOCK_T recovery_sync_time;
//extern CD_CLOCK_T prv_elapsed_time;
//extern CD_CLOCK_T rst_elapsed_time;
//extern CD_CLOCK_T create_elapsed_time;
//extern CD_CLOCK_T destroy_elapsed_time;
//extern CD_CLOCK_T begin_elapsed_time;
//extern CD_CLOCK_T compl_elapsed_time;
//extern CD_CLOCK_T advance_elapsed_time;
//
//extern CD_CLOCK_T mailbox_elapsed_time;
//
extern uint64_t state;
extern int64_t failed_phase;
extern int64_t failed_seqID;
extern bool just_reexecuted;
extern bool orig_app_side;
extern bool orig_disabled;
extern bool orig_msg_app_side;
extern bool orig_msg_disabled;
/**@addtogroup runtime_logging 
 * @{
 */

#define CD_LIBC_LOGGING 1
#define MsgPrologue() \
  orig_msg_app_side = app_side; \
  orig_msg_disabled = logger::disabled; \
  app_side = false; \
  logger::disabled = true; \
  msg_begin_clk = CD_CLOCK(); 

#define MsgEpilogue() \
  msg_end_clk = CD_CLOCK(); \
  msg_elapsed_time += msg_end_clk - msg_begin_clk; \
  app_side = orig_msg_app_side; \
  logger::disabled = orig_msg_disabled; 

#define LogPrologue() \
  log_begin_clk = CD_CLOCK(); 

#define LogEpilogue() \
  log_end_clk = CD_CLOCK(); \
  log_elapsed_time += log_end_clk - log_begin_clk; 


/**@brief Set current context as non-application side. 
 * @return true/false
 */

#define CDPrologue() \
  orig_app_side = app_side; \
  orig_disabled = logger::disabled; \
  app_side = false; \
  logger::disabled = true; \
  begin_clk = CD_CLOCK(); 


/**@brief Set current context as application side. 
 * @return true/false
 */

#define CDEpilogue() \
  end_clk = CD_CLOCK(); \
  elapsed_time += end_clk - begin_clk; \
  app_side = orig_app_side; \
  logger::disabled = orig_disabled; 

/*
#define TunedPrologue() \
  cd::app_side = false; \
  logger::disabled = true; \
  tuned::begin_clk = CD_CLOCK(); 

#define TunedEpilogue() \
  tuned::end_clk = CD_CLOCK(); \
  tuned::elapsed_time += tuned::end_clk - tuned::begin_clk; \
  cd::app_side = true; \
  logger::disabled = false; 
*/
//end_clk = CD_CLOCK(); 
//  elapsed_time += end_clk - begin_clk; 


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
      ColorT   comm_;
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
        comm_ = INVALID_COLOR;
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
                         const ColorT &comm, 
                         void    *flag, 
                         bool complete) 
        : addr_(const_cast<void *>(addr)), length_(length), taskID_(taskID), tag_(tag), 
          comm_(comm), flag_(flag), complete_(complete) 
      {
        p_ = NULL;
        pushed_ = 0;
        level_ = 0;
        isrecv_ = 0;
        intra_cd_msg_ = false;
      }
      std::string Print(void) {
        char buf[256];
        sprintf(buf, "\n== Incomplete Log Entry ==\ntaskID:%u\nlength:%lu\naddr:%p\ntag:%u\nflag:%p\ncomplete:%d\nisrecv:%d\nintra_msg:%d\np:%p\npushed:%d\nlevel:%u\n==========================\n", taskID_, length_, addr_, tag_, flag_, complete_, isrecv_, intra_cd_msg_, p_, pushed_, level_);
        return std::string(buf);
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
  } // namespace logging ends
#endif
//  extern DebugBuf cddbg;

///**@class cd::DebugBuf
// * @brief Utility class for debugging.
// *
// *  Like other parallel programming models, it is hard to debug CD runtime. 
// *  So, debugging information are printed out to file using this class.
// *  The global object of this class, dbg, is defined.
// * 
// *  The usage of this class is like below.
// *  \n
// *  \n
// *  DebugBuf dbgApp;
// *  dbgApp.open("./file_path_to_write"); 
// *  dbgApp << "Debug Information" << endl;
// *  dbg.flush(); 
// *  dbg.close();
// */ 
//  class DebugBuf: public std::ostringstream {
//    std::ofstream ofs_;
//  public:
//    ~DebugBuf() {}
//    DebugBuf(void) {}
//    DebugBuf(const char *filename) 
//      : ofs_(filename) {}
///**
// * @brief Open a file to write debug information.
// *
// */
//    void open(const char *filename)
//    {
//      bool temp = app_side;
//      app_side = false;
//      ofs_.open(filename);
//      app_side = temp;
//    }
//
///**
// * @brief Close the file where debug information is written.
// *
// */    
//    void close(void)
//    {
//      bool temp = app_side;
//      app_side = false;
//      ofs_.close();
//      app_side = temp;
//    }
//
// 
///**
// * @brief Flush the buffer that contains debugging information to the file.
// *        After flushing it, it clears the buffer for the next use.
// *
// */
//    void flush(void) 
//    {
//      bool temp = app_side;
//      app_side = false;
//      ofs_ << str();
//      ofs_.flush();
//      str("");
//      clear();
//      app_side = temp;
//    }
//     
//
//    template <typename T>
//    std::ostream &operator<<(T val) 
//    {
//      bool temp = app_side;
//      app_side = false;
//      std::ostream &os = std::ostringstream::operator<<(val);
//      app_side = temp;
//      return os;
//    }
// 
//  };
} // namespace cd ends


#endif
