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

#ifndef _COMM_LOG_H 
#define _COMM_LOG_H

#include "cd_global.h"
#include "cd_id.h"

    
struct LogTableElement {
  //unsigned long id_;  // this id_ is thread/task id related
  unsigned long pos_; // starting position of logged data in log_queue_
  unsigned long length_; // length of logged data
};
    
class cd::CommLog {
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

    // wrapper for LogData and ReadData
    CommLogErrT AccessLog(void * data_ptr, unsigned long data_length);

    // log new data into the queue
    // need to check if running out queues
    CommLogErrT LogData(void * data_ptr, unsigned long data_length);

    CommLogErrT ReadData(void * buffer, unsigned long length);
    //CommLogErrT FindNextTableElement(unsigned long * index);

    // push logs to parent
    CommLogErrT PackAndPushLogs(CD * parent_cd);
    CommLogErrT PackLogs(CommLog * parent_cl_ptr, unsigned long length);
    CommLogErrT CheckChildLogAlloc(unsigned long length);

    // copy logs to children cds
    CommLogErrT UnpackLogsToChildCD(CD* child_cd);
    CommLogErrT FindChildLogs(CDID child_cd_id, char** src_ptr);
    CommLogErrT UnpackLogs(char * src_ptr);

    void ReInit();

    CommLogMode GetCommLogMode()
    {
      return comm_log_mode_;
    }

    void SetCommLogMode(CommLogMode comm_log_mode)
    {
      comm_log_mode_ = comm_log_mode;
    }

    void Print();

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
    CommLogErrT IncreaseLogQueueSize();

    CommLogErrT WriteLogTable (void * data_ptr, unsigned long data_length);
    CommLogErrT WriteLogQueue (void * data_ptr, unsigned long data_length);
    

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
      char * base_ptr_ = NULL; 
      unsigned long cur_pos_;

      // queue size by default is queue_size_unit_, and can be increased during runtime
      // but need to be multiples of queue_size_unit_ 
      // TODO: may need to consider the size growth method of C++ vector
      unsigned long queue_size_;
    } log_queue_;

    // this level of indirection is used to cover multiple threads/tasks within one CD
    struct LogTable {
      struct LogTableElement * base_ptr_ = NULL;

      unsigned long cur_pos_;
      
      // table size by default is table_size_unit_, and can be increased during runtime
      // but need to be multiples of table_size_unit_ 
      unsigned long table_size_;
    } log_table_;

    unsigned long log_table_reexec_pos_;


  public:
    // TODO: is there a way not to make this public?? 
    // allocate child_log_ptr_ when pushing data to parents
    // pack all children's log data and then copy into child_log_ptr array
    struct ChildLogQueue{
      char * base_ptr_ = NULL;
      unsigned long size_;
      unsigned long cur_pos_;
    } child_log_;
};

#endif
