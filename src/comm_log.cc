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

#include "comm_log.h"
#include "cd.h"
#include <string.h>

using namespace cd;

CommLog::CommLog()
  :queue_size_unit_(1024), table_size_unit_(100), child_log_size_unit_(1024)
{
}

void CommLog::Init(CD* my_cd, unsigned long num_threads_in_cd)
{
  my_cd_ = my_cd;
  InitInternal(num_threads_in_cd);
}


void CommLog::Init(CD* my_cd, unsigned long num_threads_in_cd, 
    unsigned long queue_size_unit, unsigned long table_size_unit)
{
  my_cd_ = my_cd;
  queue_size_unit_ = queue_size_unit;
  table_size_unit_ = table_size_unit;
  InitInternal(num_threads_in_cd);
}


void CommLog::Init(CD* my_cd, unsigned long num_threads_in_cd, 
    unsigned long queue_size_unit, unsigned long table_size_unit, 
    unsigned long child_log_size_unit)
{
  my_cd_ = my_cd;
  queue_size_unit_ = queue_size_unit;
  table_size_unit_ = table_size_unit;
  child_log_size_unit_ = child_log_size_unit;
  InitInternal(num_threads_in_cd);
}


//void CommLog::ReInit(CD* my_cd, unsigned long num_threads_in_cd)
//{
//  my_cd_ = my_cd;
//  ReInitInternal(num_threads_in_cd);
//}
//
//
//void CommLog::ReInit(CD* my_cd, unsigned long num_threads_in_cd, 
//    unsigned long queue_size_unit, unsigned long table_size_unit)
//{
//  my_cd_ = my_cd;
//  queue_size_unit_ = queue_size_unit;
//  table_size_unit_ = table_size_unit;
//  ReInitInternal(num_threads_in_cd);
//}
//
//
//void CommLog::ReInit(CD* my_cd, unsigned long num_threads_in_cd, 
//    unsigned long queue_size_unit, unsigned long table_size_unit, 
//    unsigned long child_log_size_unit)
//{
//  my_cd_ = my_cd;
//  queue_size_unit_ = queue_size_unit;
//  table_size_unit_ = table_size_unit;
//  child_log_size_unit_ = child_log_size_unit;
//  ReInitInternal(num_threads_in_cd);
//}
//

CommLog::~CommLog()
{
  
}


void CommLog::InitInternal(unsigned long num_threads_in_cd)
{
  num_threads_in_cd_ = num_threads_in_cd;

  log_queue_.queue_size_ = 0;
  log_queue_.cur_pos_ = 0;
  log_queue_.base_ptr_ = NULL;

  log_table_.table_size_ = 0;
  log_table_.cur_pos_ = 0;
  log_table_.base_ptr_ = NULL;

  child_log_ptr_ = NULL;

  // if forward execution
  if (my_cd_->cd_exec_mode_ == CD::kExecution)
  {
    // space for log_table_reexec_pos_ will be allocated in reexecution
    log_table_reexec_pos_ = NULL;

    // allocate space for log table and log queue
    CommLogErrT ret = InitAlloc();
    if (ret == kCommLogInitFailed)
    {
      ERROR_MESSAGE("Communication Logs Initialization Failed!");
    }
  }
  else if (my_cd_->cd_exec_mode_ == CD::kReexecution)
  {
    // in reexecution mode, allocate space and initialize log_table_reexec_pos_
    log_table_reexec_pos_ = new unsigned long [num_threads_in_cd_];
    if (log_table_reexec_pos_ == NULL) 
    {
      ERROR_MESSAGE
        ("Error: Cannot allocate space for log_table_reexec_pos_ in ReInitInternal() function!\n");
      //return kCommLogInitFailed;
    }

    for (unsigned long ii=0; ii<num_threads_in_cd_;ii++)
    {
      log_table_reexec_pos_[ii] = 0;
    }
  }

}


//void CommLog::ReInitInternal(unsigned long num_threads_in_cd)
//{
//  num_threads_in_cd_ = num_threads_in_cd;
//
//  log_queue_.queue_size_ = 0;
//  log_queue_.cur_pos_ = 0;
//  log_queue_.base_ptr_ = NULL;
//
//  log_table_.table_size_ = 0;
//  log_table_.cur_pos_ = 0;
//  log_table_.base_ptr_ = NULL;
//
//  child_log_ptr_ = NULL;
//
//  log_table_reexec_pos_ = new unsigned long [num_threads_in_cd_];
//  if (log_table_reexec_pos_ == NULL) 
//  {
//    ERROR_MESSAGE
//      ("Error: Cannot allocate space for log_table_reexec_pos_ in ReInitInternal() function!\n");
//    //return kCommLogInitFailed;
//  }
//
//  for (unsigned long ii=0; ii<num_threads_in_cd_;ii++)
//  {
//    log_table_reexec_pos_[ii] = 0;
//  }
//}


CommLogErrT CommLog::InitAlloc()
{
  //TODO: should check if Init is called more than once??
  if (log_queue_.base_ptr_ == NULL)
  {
    log_queue_.base_ptr_ = new char [queue_size_unit_];
    if (log_queue_.base_ptr_ == NULL) return kCommLogInitFailed;
    log_queue_.queue_size_ = queue_size_unit_;
  }

  if (log_table_.base_ptr_ == NULL)
  {
    log_table_.base_ptr_ = new struct LogTableElement [table_size_unit_];
    if (log_table_.base_ptr_ == NULL) return kCommLogInitFailed;
    log_table_.table_size_ = table_size_unit_;
  }

  if (log_table_reexec_pos_ == NULL)
  {
    log_table_reexec_pos_ = new unsigned long [num_threads_in_cd_];
    if (log_table_reexec_pos_ == NULL) return kCommLogInitFailed;

    // Init array to all 0s
    for (unsigned long ii=0; ii<num_threads_in_cd_;ii++)
    {
      log_table_reexec_pos_[ii] = 0;
    }
  }

  // TODO: why need the following check again? This follows how Jinsuk implemented Packer class
  if (log_queue_.base_ptr_==NULL || log_table_.base_ptr_==NULL || log_table_reexec_pos_==NULL)
  {
    return kCommLogError;
  }

  return kCommLogOK;
}


// need programmer to provide a thread related id when logging data
CommLogErrT CommLog::LogData(unsigned long id, void * data_ptr, unsigned long data_length)
{
  CommLogErrT ret;
  //TODO: should also log accessed address??
  // add one element in the log_table_
  ret = WriteLogTable(id, data_ptr, data_length);
  if (ret == kCommLogAllocFailed) 
  {
    ERROR_MESSAGE("Log Table Realloc Failed!");
  }

  // write data into the queue
  ret = WriteLogQueue(id, data_ptr, data_length);
  if (ret == kCommLogAllocFailed) 
  {
    ERROR_MESSAGE("Log Queue Realloc Failed!");
  }

  return kCommLogOK;
}


CommLogErrT CommLog::WriteLogTable (unsigned long id, void * data_ptr, unsigned long data_length)
{
  CommLogErrT ret;
  if (log_table_.cur_pos_ >= log_table_.table_size_) 
  {
    ret = IncreaseLogTableSize();
    if (ret == kCommLogAllocFailed) return ret;
  }

  log_table_.base_ptr_[log_table_.cur_pos_].id_ = id;
  log_table_.base_ptr_[log_table_.cur_pos_].pos_ = log_queue_.cur_pos_;
  log_table_.base_ptr_[log_table_.cur_pos_].length_ = data_length;
  log_table_.cur_pos_++;

  return kCommLogOK;
}


CommLogErrT CommLog::WriteLogQueue (unsigned long id, void * data_ptr, unsigned long data_length)
{
  CommLogErrT ret;
  if (log_queue_.cur_pos_ + data_length >= log_queue_.queue_size_)
  {
    ret = IncreaseLogQueueSize();
    if (ret == kCommLogAllocFailed) return ret;
  }

  // TODO: check memcpy success??
  memcpy (log_queue_.base_ptr_+log_queue_.cur_pos_, data_ptr, data_length);
  log_queue_.cur_pos_ += data_length;

  return kCommLogOK;
}


CommLogErrT CommLog::IncreaseLogTableSize()
{
  struct LogTableElement * tmp_ptr 
    = new struct LogTableElement [log_table_.table_size_ + table_size_unit_];
  if (tmp_ptr == NULL) return kCommLogAllocFailed;

  // copy old data in
  memcpy (tmp_ptr, log_table_.base_ptr_, log_table_.table_size_ * sizeof(struct LogTableElement));

  // free log_table_.base_ptr_
  delete log_table_.base_ptr_;

  // change log_table_.base_ptr_ to tmp_ptr
  log_table_.base_ptr_ = tmp_ptr;

  // increase table_size_
  log_table_.table_size_ += table_size_unit_;

  return kCommLogOK;
}


CommLogErrT CommLog::IncreaseLogQueueSize()
{
  char * tmp_ptr = new char [log_queue_.queue_size_ + queue_size_unit_];
  if (tmp_ptr == NULL) return kCommLogAllocFailed;

  // copy old data in
  memcpy (tmp_ptr, log_queue_.base_ptr_, log_queue_.queue_size_ * sizeof(char));

  // free log_queue_.base_ptr_
  delete log_queue_.base_ptr_;

  // change log_queue_.base_ptr_ to tmp_ptr
  log_queue_.base_ptr_ = tmp_ptr;

  // increase queue_size_
  log_queue_.queue_size_ += queue_size_unit_;

  return kCommLogOK;
}


CommLogErrT CommLog::CheckChildLogAlloc(unsigned long length)
{
  // if first time copy data into child_log_ptr_
  if (child_log_ptr_ == NULL) 
  {
    unsigned long required_length = (length/child_log_size_unit_ + 1)*child_log_size_unit_;
    child_log_ptr_ = new char [required_length];
    if (child_log_ptr_ == NULL) return kCommLogChildLogAllocFailed;
    child_log_size_ = required_length;
    child_log_cur_pos_ = 0;
  }
  // if not enough space in child_log_ptr_
  else if (child_log_cur_pos_ + length >= child_log_size_)
  {
    unsigned long required_length = 
      ((child_log_cur_pos_+length)/child_log_size_unit_ + 1)*child_log_size_unit_;

    char * tmp_ptr = new char [required_length];
    if (tmp_ptr == NULL) return kCommLogChildLogAllocFailed;
    memcpy (tmp_ptr, child_log_ptr_, child_log_cur_pos_ * sizeof(char));
    delete child_log_ptr_;
    child_log_ptr_ = tmp_ptr;
    child_log_size_ = required_length;
  }

  return kCommLogOK;
}


CommLogErrT CommLog::PackAndPushLogs(CD* parent_cd)
{
  // calculate length of data copy
  // pack format is [SIZE][CDID][Table][Queue]
  unsigned long length;
  length = sizeof(unsigned long) + sizeof(CDID) // SIZE + CDID
         + sizeof(struct LogTable) + sizeof(struct LogTableElement)*log_table_.cur_pos_ //Table
         + sizeof(struct LogQueue) + sizeof(char)*log_queue_.cur_pos_; // Queue

  // check parent's queue availability
  // TODO: check alloc??
  parent_cd->CommLogCheckAlloc(length);

  // TODO: this is ugly, what about a tmp_ptr to pack and then copy to parent??
  // pack logs to parent's child_log_ptr_
  PackLogs(parent_cd->comm_log_ptr_, length);

  return kCommLogOK;
}


CommLogErrT CommLog::PackLogs(CommLog * dst_cl_ptr, unsigned long length)
{
  if (dst_cl_ptr == NULL) return kCommLogError;

  PRINT_DEBUG("dst_cl_ptr=%p\n", dst_cl_ptr);
  PRINT_DEBUG("dst_cl_ptr->child_log_ptr_=%p\n", dst_cl_ptr->child_log_ptr_);
  PRINT_DEBUG("before: child_log_size_= %ld\n", dst_cl_ptr->child_log_size_);
  PRINT_DEBUG("before: child_log_cur_pos_ = %ld\n", dst_cl_ptr->child_log_cur_pos_);

  unsigned long size;
  //[SIZE]
  size = sizeof(unsigned long);
  memcpy (&(dst_cl_ptr->child_log_ptr_[dst_cl_ptr->child_log_cur_pos_]), 
      &length, size);
  dst_cl_ptr->child_log_cur_pos_ += size;
  PRINT_DEBUG("After size: child_log_cur_pos_ = %ld\n", dst_cl_ptr->child_log_cur_pos_);

  //[CDID]
  size = sizeof(CDID);
  CDID tmp_cd_id = my_cd_->GetCDID();
  memcpy (&(dst_cl_ptr->child_log_ptr_[dst_cl_ptr->child_log_cur_pos_]), 
      &(tmp_cd_id), size);
  dst_cl_ptr->child_log_cur_pos_ += size;
  PRINT_DEBUG("After CDID: child_log_cur_pos_ = %ld\n", dst_cl_ptr->child_log_cur_pos_);

  //[Table] meta data
  size = sizeof(struct LogTable);
  memcpy (&(dst_cl_ptr->child_log_ptr_[dst_cl_ptr->child_log_cur_pos_]), 
      &log_table_, size);
  dst_cl_ptr->child_log_cur_pos_ += size;
  PRINT_DEBUG("After table meta data: child_log_cur_pos_ = %ld\n", dst_cl_ptr->child_log_cur_pos_);

  //[Table] data
  size = sizeof(struct LogTableElement)*log_table_.cur_pos_;
  memcpy (&(dst_cl_ptr->child_log_ptr_[dst_cl_ptr->child_log_cur_pos_]), 
      log_table_.base_ptr_, size);
  dst_cl_ptr->child_log_cur_pos_ += size;
  PRINT_DEBUG("After table data: child_log_cur_pos_ = %ld\n", dst_cl_ptr->child_log_cur_pos_);

  //[Queue] meta data
  size = sizeof(struct LogQueue);
  memcpy (&(dst_cl_ptr->child_log_ptr_[dst_cl_ptr->child_log_cur_pos_]), 
      &log_queue_, size);
  dst_cl_ptr->child_log_cur_pos_ += size;
  PRINT_DEBUG("After queue meta data: child_log_cur_pos_ = %ld\n", dst_cl_ptr->child_log_cur_pos_);

  //[Queue] data
  size = sizeof(char)*log_queue_.cur_pos_;
  memcpy (&(dst_cl_ptr->child_log_ptr_[dst_cl_ptr->child_log_cur_pos_]), 
      log_queue_.base_ptr_, size);
  dst_cl_ptr->child_log_cur_pos_ += size;
  PRINT_DEBUG("After queue data: child_log_cur_pos_ = %ld\n", dst_cl_ptr->child_log_cur_pos_);
  PRINT_DEBUG("After: child_log_size_= %ld\n", dst_cl_ptr->child_log_size_);

  return kCommLogOK;
}


// input parameter
CommLogErrT CommLog::ReadData(unsigned long id, void * buffer, unsigned long * length)
{
  CommLogErrT ret;
  unsigned long index;
  ret = FindNextTableElement(id, &index);
  if (ret != kCommLogOK) return ret;

  *length = log_table_.base_ptr_[index].length_;
  memcpy(buffer, 
      log_queue_.base_ptr_+log_table_.base_ptr_[index].pos_,
      log_table_.base_ptr_[index].length_);

  log_table_reexec_pos_[id] = index+1;
  return kCommLogOK;
}


CommLogErrT CommLog::FindNextTableElement(unsigned long id, unsigned long * index)
{
  if (log_table_reexec_pos_[id] == log_table_.cur_pos_)
  {
    ERROR_MESSAGE("Reach end of log_table_ for id(%ld)\n", id);
    return kCommLogError;
  }

  unsigned long ii;
  for (ii=log_table_reexec_pos_[id];ii<log_table_.cur_pos_;ii++)
  {
    if (log_table_.base_ptr_[ii].id_ == id) 
    {
      *index = ii;
      break;
    }
  }
  if (ii == log_table_.cur_pos_)
  {
    ERROR_MESSAGE("No next table element found for id(%ld) in log_table_\n", id);
    return kCommLogError;
  }

  return kCommLogOK;
}


void CommLog::Print()
{
  my_cd_->GetCDID().Print();

  printf("\nUnits:\n");
  printf("queue_size_unit_ = %ld\n", queue_size_unit_);
  printf("table_size_unit_ = %ld\n", table_size_unit_);
  printf("child_log_size_unit_ = %ld\n", child_log_size_unit_);

  printf("\nlog_table_:\n");
  printf("base_ptr_ = %p\n", log_table_.base_ptr_);
  printf("cur_pos_ = %ld\n", log_table_.cur_pos_);
  printf("table_size_ = %ld\n", log_table_.table_size_);
  
  unsigned long ii;
  for (ii=0;ii<log_table_.cur_pos_;ii++)
  {
    printf("log_table_.base_ptr_[%ld]:\n", ii);
    printf("id_=%ld, pos_=%ld, length_=%ld\n\n"
        ,log_table_.base_ptr_[ii].id_
        ,log_table_.base_ptr_[ii].pos_
        ,log_table_.base_ptr_[ii].length_);
  }

  printf ("num_threads_in_cd_ = %ld\n", num_threads_in_cd_);
  printf ("log_table_reexec_pos_ = %p\n", log_table_reexec_pos_);

  if (log_table_reexec_pos_ != NULL)
  {
    for (ii=0;ii<num_threads_in_cd_;ii++)
    {
      printf("log_table_reexec_pos_[%ld]=%ld\n", ii, log_table_reexec_pos_[ii]);
    }
  }

  printf("\nlog_queue_:\n");
  printf("base_ptr_ = %p\n", log_queue_.base_ptr_);
  printf("cur_pos_ = %ld\n", log_queue_.cur_pos_);
  printf("queue_size_ = %ld\n", log_queue_.queue_size_);

  printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}


CommLogErrT CommLog::UnpackLogsToChildCD(CD* child_cd)
{
  char * src_ptr;
  FindChildLogs(child_cd->GetCDID(), &src_ptr);

  (child_cd->comm_log_ptr_)->UnpackLogs(src_ptr);

  return kCommLogOK;
}


CommLogErrT CommLog::UnpackLogs(char * src_ptr)
{
  //TODO: too many places return kCommLogError and with error messages
  // Either do it with one error return, and different messages, 
  // or differentiate returns
  
  if (src_ptr == NULL) 
  {
    ERROR_MESSAGE("NULL src_ptr to unpack logs\n");
    return kCommLogError;
  }

  unsigned long length;
  unsigned long index;
  unsigned long size;

  //[SIZE]
  size = sizeof(unsigned long);
  memcpy (&length, src_ptr, size);
  index = size;

  // [CDID]
  CDID cd_id;
  size = sizeof(CDID);
  memcpy(&cd_id, src_ptr+index, size);
  if (!(cd_id==my_cd_->GetCDID()))
  {
    ERROR_MESSAGE("Pass wrong log data to unpack!\n");
    return kCommLogError;
  }
  index += size;

  // [Table] meta data
  size = sizeof(struct LogTable);
  memcpy(&log_table_, src_ptr+index, size);
  index += size;

  // [Table] data
  struct LogTableElement * tmp_ptr = new struct LogTableElement[log_table_.table_size_];
  if (tmp_ptr == NULL)
  {
    ERROR_MESSAGE("Cannot allocate space for log_table_.base_ptr_!\n");
    return kCommLogError;
  }
  log_table_.base_ptr_ = tmp_ptr;

  size = sizeof(struct LogTableElement)*log_table_.cur_pos_;
  memcpy(log_table_.base_ptr_, src_ptr+index, size);
  index += size;

  // [Queue] meta data
  size = sizeof(struct LogQueue);
  memcpy(&log_queue_, src_ptr+index, size);
  index += size;

  // [Queue] data
  char * tmp_queue_ptr = new char [log_queue_.queue_size_];
  if (tmp_queue_ptr == NULL)
  {
    ERROR_MESSAGE("Cannot allocate space for log_table_.base_ptr_!\n");
    return kCommLogError;
  }
  log_queue_.base_ptr_ = tmp_queue_ptr;

  size = sizeof(char) * log_queue_.cur_pos_;
  memcpy(log_queue_.base_ptr_, src_ptr+index, size);
  index += size;

  // sanity check
  if (index != length)
  {
    ERROR_MESSAGE("Something went wrong when unpacking data!\n");
    return kCommLogError;
  }
  
  return kCommLogOK;
}


CommLogErrT CommLog::FindChildLogs(CDID child_cd_id, char** src_ptr)
{
  unsigned long tmp_index=0;
  unsigned long length;
  CDID cd_id;
  while(tmp_index < child_log_cur_pos_)
  {
    memcpy(&length, child_log_ptr_+tmp_index, sizeof(unsigned long));
    memcpy(&cd_id, child_log_ptr_+tmp_index+sizeof(unsigned long), sizeof(CDID));

    //SZ: reloaded '==' operator for CDID class
    if (cd_id == child_cd_id) break;
    tmp_index += length;
  }

  if (tmp_index >= child_log_cur_pos_)
  {
    ERROR_MESSAGE("Could not find correspondent child logs!\n");
    return kCommLogError;
  }

  *src_ptr = child_log_ptr_+tmp_index;

  return kCommLogOK;
}


