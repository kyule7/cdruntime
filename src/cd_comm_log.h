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

#ifndef _CD_COMM_LOG_H 
#define _CD_COMM_LOG_H

#include "cd_global.h"
#include "cd_def_internal.h"
#include "cd_id.h"

using namespace cd::internal;

namespace cd {
  namespace logging {
    
struct LogTableElement {
#if CD_TEST_ENABLED
  friend class cd::cd_test::Test;
#endif
  unsigned long pos_; // starting position of logged data in log_queue_
  unsigned long length_; // length of logged data
  bool completed_;
  unsigned long flag_;
  unsigned long counter_;
  unsigned long reexec_counter_;
  bool isrepeated_;
  uint32_t thread_; // src-thread for read op and dst-thread for write op
#if _PGAS_VER
  bool iswriteop_;
  uint32_t sync_counter_; // target-side SC
  uint32_t incoming_write_counter_; // target-side IWC
#endif
 
  LogTableElement(void) {
    counter_=0;
    reexec_counter_=0;
    isrepeated_=false;
#if _PGAS_VER
    iswriteop_=false;
#endif
  }
};
    
class CommLog {
    friend class cd::internal::CD;
    friend class cd::internal::HeadCD;
#if CD_TEST_ENABLED
    friend class cd::cd_test::Test;
#endif
//    friend CD* IsLogable(bool *logable_);
  public:
    //CommLog();

    CommLog(CD* my_cd, CommLogMode comm_log_mode);
    //CommLog(CD* my_cd, CommLogMode comm_log_mode, unsigned long num_threads_in_cd);
    CommLog(CD* my_cd, CommLogMode comm_log_mode, 
        unsigned long queue_size_unit, unsigned long table_size_unit);
    CommLog(CD* my_cd, CommLogMode comm_log_mode, unsigned long queue_size_unit, 
        unsigned long table_size_unit, unsigned long child_log_size_unit);

    ~CommLog();

    // add cd_id
    void SetCD(CD * my_cd)
    {
      my_cd_ = my_cd;
    }

    bool ProbeAndLogData(void* addr, 
                         unsigned long length, 
                         unsigned long flag,
                         bool isrecv);
    bool ProbeAndLogDataPacked(void* addr, 
                               unsigned long length, 
                               unsigned long flag,
                               bool isrecv);
    // log new data into the queue
    // need to check if running out queues
    CommLogErrT LogData(const void * data_ptr, 
                        unsigned long data_length, 
                        uint32_t thread=0,
                        bool completed=true,
                        unsigned long flag=0,
                        bool isrecv=0,
                        bool isrepeated=0,
                        bool intra_cd_msg=false);

    CommLogErrT ReadData(void * buffer, unsigned long length);
    CommLogErrT ProbeData(const void * buffer, unsigned long length);
    //CommLogErrT FindNextTableElement(unsigned long * index);

    // push logs to parent
    CommLogErrT PackAndPushLogs(CD * parent_cd);
    //GONG: duplicated for libc logging
    CommLogErrT PackAndPushLogs_libc(CD * parent_cd);
    CommLogErrT PackLogs(CommLog * parent_cl_ptr, unsigned long length);
    CommLogErrT CheckChildLogAlloc(unsigned long length);

    // copy logs to children cds
    CommLogErrT UnpackLogsToChildCD(CD* child_cd);
    //GONG: duplicated for libc logging
    CommLogErrT UnpackLogsToChildCD_libc(CD* child_cd);
    CommLogErrT FindChildLogs(CDID child_cd_id, char** src_ptr);
    CommLogErrT UnpackLogs(char * src_ptr);

    bool IsNewLogGenerated_()
    {
      return new_log_generated_;
    }

    void SetNewLogGenerated(bool new_log_generated)
    {
      new_log_generated_ = new_log_generated;
    }

    void ReInit();

    // Reset is called when a CD completes, so next CD_Begin can reuse all the allocated space
    void Reset();

    CommLogMode GetCommLogMode()
    {
      return comm_log_mode_;
    }

    void SetCommLogMode(CommLogMode comm_log_mode)
    {
      comm_log_mode_ = comm_log_mode;
    }

    void Print();

    CommLogErrT Realloc();

    //// In re-executation, when a CD is created, need to trigger this init
    //// This init will not allocate any space for table and queue,
    //// because when unpacking data, the space will be allocated..
    //void ReInit(CD* my_cd, unsigned long num_threads_in_cd);
    //void ReInit(CD* my_cd, unsigned long num_threads_in_cd, 
    //    unsigned long queue_size_unit, unsigned long table_size_unit);
    //void ReInit(CD* my_cd, unsigned long num_threads_in_cd, unsigned long queue_size_unit, 
    //    unsigned long table_size_unit, unsigned long child_log_size_unit);

  private:
    CommLogErrT InitAlloc();

    // internal function, called by constructor, and used to allocate first log queue
    // and initialize all parameters
    void InitInternal();

    CommLogErrT IncreaseLogTableSize();
    CommLogErrT IncreaseLogQueueSize(unsigned long length);
    bool FoundRepeatedEntry(const void * data_ptr, 
                            unsigned long data_length, 
                            bool completed, 
                            unsigned long flag);

    CommLogErrT WriteLogTable (uint32_t thread,
                              const void * data_ptr, 
                              unsigned long data_length, 
                              bool completed,
                              unsigned long flag,
                              bool isrepeated);
    CommLogErrT WriteLogQueue (const void * data_ptr, 
                               unsigned long data_length,
                               bool completed);
    

  private:
    CD* my_cd_;
    //SZ: as we have multiple CD objects for multi threads CDs,
    //    so we not need to consider the case that multiple threads are in the same CD
    //unsigned long num_threads_in_cd_;
    unsigned long queue_size_unit_;
    unsigned long table_size_unit_;
    unsigned long child_log_size_unit_;

    CommLogMode comm_log_mode_;


    // struct to describe current address and bound address of a log queue
    struct LogQueue {
      char * base_ptr_; 
      unsigned long cur_pos_;

      // queue size by default is queue_size_unit_, and can be increased during runtime
      // but need to be multiples of queue_size_unit_ 
      // TODO: may need to consider the size growth method of C++ vector
      unsigned long queue_size_;
      LogQueue(void) : base_ptr_(NULL) {}
    }; 
    LogQueue log_queue_;

    // this level of indirection is used to cover multiple threads/tasks within one CD
    struct LogTable {
      LogTableElement * base_ptr_;

      unsigned long cur_pos_;
      
      // table size by default is table_size_unit_, and can be increased during runtime
      // but need to be multiples of table_size_unit_ 
      unsigned long table_size_;

      LogTable(void) : base_ptr_(NULL) {}
    };
    LogTable log_table_;

    unsigned long log_table_reexec_pos_;

    // to state if new logs are generated in current CD
    bool new_log_generated_;


  public:
    // TODO: is there a way not to make this public?? 
    // allocate child_log_ptr_ when pushing data to parents
    // pack all children's log data and then copy into child_log_ptr array
    struct ChildLogQueue{
      char * base_ptr_;
      unsigned long size_;
      unsigned long cur_pos_;
      ChildLogQueue(void) : base_ptr_(NULL) {}
    };
    ChildLogQueue child_log_;
};

  } // namespace logging ends
} // namespace cd ends
#endif
