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
#include "cd_config.h"

#if comm_log
#include "cd_def_debug.h"
#include "cd_comm_log.h"
#include "cd_internal.h"
#include <string.h>

using namespace cd;
using namespace cd::logging;
using namespace cd::internal;

//CommLog::CommLog()
//  :queue_size_unit_(1024), table_size_unit_(100), child_log_size_unit_(1024)
//{
//  InitInternal(1);
//}

CommLog::CommLog(CD* my_cd, 
                 CommLogMode comm_log_mode)
  :queue_size_unit_(1024*1024), table_size_unit_(1024), child_log_size_unit_(1024*1024)
{
  my_cd_ = my_cd;
  comm_log_mode_ = comm_log_mode;
  InitInternal();
}

//CommLog::CommLog(CD* my_cd, 
//                 CommLogMode comm_log_mode)
//  :queue_size_unit_(1024), table_size_unit_(100), child_log_size_unit_(1024)
//{
//  my_cd_ = my_cd;
//  comm_log_mode_ = comm_log_mode;
//  InitInternal();
//}
//

CommLog::CommLog(CD* my_cd, 
                 CommLogMode comm_log_mode, 
                 uint64_t queue_size_unit, 
                 uint64_t table_size_unit)
  :child_log_size_unit_(1024*1024)
{
  my_cd_ = my_cd;
  comm_log_mode_ = comm_log_mode;
  queue_size_unit_ = queue_size_unit;
  table_size_unit_ = table_size_unit;
  InitInternal();
}


CommLog::CommLog(CD* my_cd, 
                 CommLogMode comm_log_mode, 
                 uint64_t queue_size_unit, 
                 uint64_t table_size_unit, 
                 uint64_t child_log_size_unit)
{
  my_cd_ = my_cd;
  comm_log_mode_ = comm_log_mode;
  queue_size_unit_ = queue_size_unit;
  table_size_unit_ = table_size_unit;
  child_log_size_unit_ = child_log_size_unit;
  InitInternal();
}


CommLog::~CommLog()
{
  //destroy all allocated memory
  //log_table_
  if (log_table_.base_ptr_ != NULL) 
  {
    LOG_DEBUG("delete log_table_.base_ptr_ (%p)\n", log_table_.base_ptr_);
    delete log_table_.base_ptr_;
  }

  //log_queue_
  if (log_queue_.base_ptr_ != NULL)
  {
    LOG_DEBUG("delete log_queue_.base_ptr_ (%p)\n", log_queue_.base_ptr_);
    delete log_queue_.base_ptr_;
  }

  //child_log_
  if (child_log_.base_ptr_ != NULL)
  {
    LOG_DEBUG("delete child_log_.base_ptr_ (%p)\n", child_log_.base_ptr_);
    delete child_log_.base_ptr_;
  }
  
  LOG_DEBUG("Reach the end of CommLog destructor...\n");
}


void CommLog::InitInternal()
{
  new_log_generated_ = false;

  log_queue_.queue_size_ = 0;
  log_queue_.cur_pos_ = 0;
  log_queue_.base_ptr_ = NULL;

  log_table_.table_size_ = 0;
  log_table_.cur_pos_ = 0;
  log_table_.base_ptr_ = NULL;

  child_log_.size_ = 0;
  child_log_.cur_pos_ = 0;
  child_log_.base_ptr_ = NULL;

  log_table_reexec_pos_ = 0;

  new_log_generated_ = false;

  // if forward execution
  if (comm_log_mode_ == kGenerateLog)
  {
    // allocate space for log table and log queue
    CommLogErrT ret = InitAlloc();
    if (ret == kCommLogInitFailed)
    {
      ERROR_MESSAGE("Communication Logs Initialization Failed!\n");
    }
  }
  //// in kReplayLog mode, no allocation because unpack will allocate data...
  //else if (comm_log_mode_ == kReplayLog)
  //{
  //}

}


// ReInit is called when a CD wants to reexecute
void CommLog::ReInit()
{
  comm_log_mode_ = kReplayLog;
  log_table_reexec_pos_ = 0;
}


CommLogErrT CommLog::InitAlloc()
{
  //TODO: should check if Init is called more than once??
  if (log_queue_.base_ptr_ == NULL)
  {
    log_queue_.base_ptr_ = new char [queue_size_unit_];
    if (log_queue_.base_ptr_ == NULL) 
    {
      return kCommLogInitFailed;
    }
    log_queue_.queue_size_ = queue_size_unit_;
  }

  if (log_table_.base_ptr_ == NULL)
  {
    log_table_.base_ptr_ = new struct LogTableElement [table_size_unit_];
    if (log_table_.base_ptr_ == NULL) 
    {
      return kCommLogInitFailed;
    }
    log_table_.table_size_ = table_size_unit_;
  }

  // TODO: why need the following check again? This follows how Jinsuk implemented Packer class
  if (log_queue_.base_ptr_==NULL || log_table_.base_ptr_==NULL)
  {
    return kCommLogError;
  }

  return kCommLogOK;
}


CommLogErrT CommLog::Realloc()
{
  log_queue_.queue_size_ = 0;
  log_queue_.cur_pos_ = 0;
  log_queue_.base_ptr_ = NULL;

  log_table_.table_size_ = 0;
  log_table_.cur_pos_ = 0;
  log_table_.base_ptr_ = NULL;

  child_log_.size_ = 0;
  child_log_.cur_pos_ = 0;
  child_log_.base_ptr_ = NULL;

  log_table_reexec_pos_ = 0;

  new_log_generated_ = false;

  CommLogErrT ret = InitAlloc();
  if (ret != kCommLogOK)
  {
    ERROR_MESSAGE("Comm Log reallocation failed!\n");
  }

  return kCommLogOK;
}


bool CommLog::ProbeAndLogData(void *addr, 
                              uint64_t length,
                              int64_t flag,
                              bool isrecv)
{
  uint64_t pos;
  bool found = false;
  if(isrecv) {
    // ??
  }
  // change entry incompleted entry to completed if there is any
  // and associate it with the data
  for (pos=0; pos<log_table_.cur_pos_; pos++)
  {
    if (log_table_.base_ptr_[pos].flag_ == flag)
    {
      // change log table
      // length stored is length from isend/irecv, so length should match here...
      assert(log_table_.base_ptr_[pos].length_ == length);
      log_table_.base_ptr_[pos].completed_ = true;

      // change log queue
      if (length!=0)
      {
        // copy data into the place that reserved..
        memcpy(log_queue_.base_ptr_+log_table_.base_ptr_[pos].pos_, addr, length);
      }

      found = true;
      break;
    }
  }

  return found;
}


bool CommLog::ProbeAndLogDataPacked(void *addr, 
                                    uint64_t length,
                                    int64_t flag,
                                    bool isrecv)
{
  if(isrecv) {
    // ??
  }
  // 1) find the corresponding log entry in child_log_
  // 2) assert length 
  // 3) find corresponding log queue address and copy data in
  // 4) change incomplete flag to complete

  bool found = false;
  uint64_t inner_index=0, tmp_index, index=0;
  uint64_t tmp_length=0;
  struct LogTable tmp_log_table;
  struct LogTableElement *tmp_table_ptr;
  char *dest_addr;

  while (index < child_log_.cur_pos_)
  {
    inner_index = index;
    memcpy(&tmp_length, child_log_.base_ptr_+inner_index, sizeof(uint64_t));
    inner_index += sizeof(uint64_t);
    inner_index += sizeof(CDID);
    memcpy(&tmp_log_table, child_log_.base_ptr_+inner_index, sizeof(struct LogTable));
    // find corresponding entry information
    inner_index += sizeof(struct LogTable);
    tmp_index = 0;
    tmp_table_ptr = (struct LogTableElement *) (child_log_.base_ptr_+inner_index);
    while (tmp_index < tmp_log_table.cur_pos_)
    {
      if (tmp_table_ptr->flag_ == flag){
        // if found the corresponding log entry
        assert(tmp_table_ptr->length_ == length);
        found = true;
        break;
      }

      tmp_index++;
      tmp_table_ptr++;
    }

    if (found)
    {
      // find corresponding queue space to store data
      inner_index += sizeof(struct LogTableElement)*tmp_log_table.cur_pos_;
      inner_index += sizeof(struct LogQueue);
      dest_addr = (char*)(child_log_.base_ptr_+inner_index+tmp_table_ptr->pos_);

      // copy data in
      memcpy(dest_addr, addr, sizeof(char)*length);

      // change incomplete status to complete
      tmp_table_ptr->completed_ = true;
    
      break;
    }

    // jump to next packed log
    index += tmp_length;
  }

  return found;
}

bool CommLog::FoundRepeatedEntry(const void *data_ptr, uint64_t data_length, 
                                 bool completed, int64_t flag)
{
  //LOG_DEBUG("Inside FoundRepeatedEntry:\n");
  //LOG_DEBUG("isrepeated_=%d\n", log_table_.base_ptr_[log_table_.cur_pos_-1].isrepeated_);
  //LOG_DEBUG("length_=%ld vs data_length=%ld\n", log_table_.base_ptr_[log_table_.cur_pos_-1].length_, data_length);
  //LOG_DEBUG("completed_=%d vs completed=%d\n", log_table_.base_ptr_[log_table_.cur_pos_-1].completed_, completed);
  //LOG_DEBUG("flag_=%ld vs flag=%ld\n", log_table_.base_ptr_[log_table_.cur_pos_-1].flag_, flag);

  return log_table_.base_ptr_[log_table_.cur_pos_-1].isrepeated_ &&
      log_table_.base_ptr_[log_table_.cur_pos_-1].length_ == data_length &&
      log_table_.base_ptr_[log_table_.cur_pos_-1].completed_ == completed &&
      log_table_.base_ptr_[log_table_.cur_pos_-1].flag_ == flag;

}


CommLogErrT CommLog::LogData(const void *data_ptr, uint64_t data_length, uint32_t taskID,
                          bool completed, int64_t flag, 
                          bool isrecv, bool isrepeated, 
                          bool intra_cd_msg, int tag, ColorT comm)
{
  LOG_DEBUG("LogData of address (%p) and length(%ld)\n", data_ptr, data_length);

  CommLogErrT ret;

  if (isrepeated)
  {
    // check if repeated log entry exists
    // if exists, just increment the counter_ and return
    if (FoundRepeatedEntry(data_ptr, data_length, completed, flag))
    {
      LOG_DEBUG("Found current repeating log entry matching!\n");
      log_table_.base_ptr_[log_table_.cur_pos_-1].counter_++;
      return kCommLogOK;
    }
    LOG_DEBUG("Current repeating log entry NOT matching!\n");
  }

  ret = WriteLogTable(taskID, data_ptr, data_length, completed, flag, isrepeated);
  if (ret == kCommLogAllocFailed) 
  {
    ERROR_MESSAGE("Log Table Realloc Failed!\n");
    return ret;
  }

  // write data into the queue
  if (data_length!=0)
  {
    ret = WriteLogQueue(data_ptr, data_length, completed);
    if (ret == kCommLogAllocFailed) 
    {
      ERROR_MESSAGE("Log Queue Realloc Failed!\n");
      return ret;
    }
  }

  // insert one incomplete_log_entry_ to incomplete_log_
  if (!completed)
  {
    // append one entry at the end of my_cd_->incomplete_log_
    IncompleteLogEntry tmp_log_entry;
    tmp_log_entry.addr_ = const_cast<void *>(data_ptr);
    tmp_log_entry.length_ = (uint64_t) data_length;
    tmp_log_entry.flag_ =  flag;
    tmp_log_entry.complete_ = false;
    tmp_log_entry.isrecv_ = isrecv;
    tmp_log_entry.taskID_ = taskID;
    tmp_log_entry.intra_cd_msg_ = intra_cd_msg;
    tmp_log_entry.tag_ = tag;
    tmp_log_entry.comm_ = comm;

#ifdef libc_log
    //GONG
    tmp_log_entry.p_ = NULL;
#endif

    my_cd_->incomplete_log_.push_back(tmp_log_entry);
#if _DEBUG
    my_cd_->PrintIncompleteLog();
#endif
  }

  new_log_generated_ = true;

  return kCommLogOK;
}


CommLogErrT CommLog::WriteLogTable (uint32_t taskID, const void *data_ptr, uint64_t data_length, 
                                  bool completed, int64_t flag, bool isrepeated)
{
  CommLogErrT ret;
  if (log_table_.cur_pos_ >= log_table_.table_size_) 
  {
    ret = IncreaseLogTableSize();
    if (ret == kCommLogAllocFailed) return ret;
  }

  if(data_ptr == NULL) {
    //??
  }

  log_table_.base_ptr_[log_table_.cur_pos_].pos_ = log_queue_.cur_pos_;
  log_table_.base_ptr_[log_table_.cur_pos_].length_ = data_length;
  log_table_.base_ptr_[log_table_.cur_pos_].completed_ = completed;
  log_table_.base_ptr_[log_table_.cur_pos_].flag_ = flag;
  log_table_.base_ptr_[log_table_.cur_pos_].counter_ = 1;
  log_table_.base_ptr_[log_table_.cur_pos_].reexec_counter_ = 0;
  log_table_.base_ptr_[log_table_.cur_pos_].isrepeated_ = isrepeated;
  log_table_.base_ptr_[log_table_.cur_pos_].taskID_ = taskID;
  log_table_.cur_pos_++;

  return kCommLogOK;
}


CommLogErrT CommLog::WriteLogQueue (const void *data_ptr, uint64_t data_length, bool completed)
{
  CommLogErrT ret;
  if (log_queue_.cur_pos_ + data_length > log_queue_.queue_size_)
  {
    ret = IncreaseLogQueueSize(data_length);
    if (ret == kCommLogAllocFailed) return ret;
  }

  // TODO: check memcpy success??
  if (completed && data_length!=0)
  {
    memcpy(log_queue_.base_ptr_+log_queue_.cur_pos_, data_ptr, data_length);
  }
  log_queue_.cur_pos_ += data_length;

  return kCommLogOK;
}


CommLogErrT CommLog::IncreaseLogTableSize()
{
  struct LogTableElement *tmp_ptr 
    = new struct LogTableElement [log_table_.table_size_*2];
    //= new struct LogTableElement [log_table_.table_size_ + table_size_unit_];
  if (tmp_ptr == NULL) return kCommLogAllocFailed;

  // copy old data in
  memcpy (tmp_ptr, log_table_.base_ptr_, log_table_.table_size_ *sizeof(struct LogTableElement));

  // free log_table_.base_ptr_
  delete log_table_.base_ptr_;

  // change log_table_.base_ptr_ to tmp_ptr
  log_table_.base_ptr_ = tmp_ptr;

  // increase table_size_
  log_table_.table_size_ *= 2;
  //log_table_.table_size_ += table_size_unit_;

  return kCommLogOK;
}


// increase log queue size to hold data of size of "length"
CommLogErrT CommLog::IncreaseLogQueueSize(uint64_t length)
{
  //uint64_t required_length = ((log_queue_.cur_pos_+length)/queue_size_unit_+1)*queue_size_unit_;
  uint64_t required_length = (log_queue_.cur_pos_+length)*2;
  if (required_length <= log_queue_.queue_size_)
  {
    ERROR_MESSAGE("Inside IncreaseLogQueueSize, required_length (%ld) is smaller than log_queue_.queue_size_ (%ld)!!\n",
                  required_length, log_queue_.queue_size_);
    return kCommLogError;
  }
  char *tmp_ptr = new char [required_length];
  if (tmp_ptr == NULL) return kCommLogAllocFailed;

  // copy old data in
  memcpy (tmp_ptr, log_queue_.base_ptr_, log_queue_.queue_size_ *sizeof(char));

  // free log_queue_.base_ptr_
  delete log_queue_.base_ptr_;

  // change log_queue_.base_ptr_ to tmp_ptr
  log_queue_.base_ptr_ = tmp_ptr;

  // increase queue_size_
  log_queue_.queue_size_ = required_length;

  return kCommLogOK;
}


CommLogErrT CommLog::CheckChildLogAlloc(uint64_t length)
{
  // if first time copy data into child_log_.base_ptr_
  if (child_log_.base_ptr_ == NULL) 
  {
    uint64_t required_length = (length/child_log_size_unit_ + 1)*child_log_size_unit_;
    child_log_.base_ptr_ = new char [required_length];
    if (child_log_.base_ptr_ == NULL) return kCommLogChildLogAllocFailed;
    child_log_.size_ = required_length;
    child_log_.cur_pos_ = 0;
  }
  // if not enough space in child_log_.base_ptr_
  else if (child_log_.cur_pos_ + length >= child_log_.size_)
  {
    uint64_t required_length = 
      ((child_log_.cur_pos_+length)/child_log_size_unit_ + 1)*child_log_size_unit_;

    char *tmp_ptr = new char [required_length];
    if (tmp_ptr == NULL) return kCommLogChildLogAllocFailed;
    memcpy (tmp_ptr, child_log_.base_ptr_, child_log_.cur_pos_ *sizeof(char));
    delete child_log_.base_ptr_;
    child_log_.base_ptr_ = tmp_ptr;
    child_log_.size_ = required_length;
  }

  return kCommLogOK;
}


//SZ: FIXME: what if parent_cd and cd are in different address space???
CommLogErrT CommLog::PackAndPushLogs(CD* parent_cd)
{
  // calculate length of data copy
  // pack format is [SIZE][CDID][Table][Queue][ChildLogQueue]
  uint64_t length;
  length = sizeof(uint64_t) + sizeof(CDID) // SIZE + CDID
         + sizeof(struct LogTable) + sizeof(struct LogTableElement)*log_table_.cur_pos_ //Table
         + sizeof(struct LogQueue) + sizeof(char)*log_queue_.cur_pos_ // Queue
         + sizeof(struct ChildLogQueue) + sizeof(char)*child_log_.cur_pos_; // ChildLogQueue

  // check parent's queue availability
  parent_cd->CommLogCheckAlloc(length);

  // TODO: this is ugly, what about a tmp_ptr to pack and then copy to parent??
  // pack logs to parent's child_log_.base_ptr_
  PackLogs(parent_cd->comm_log_ptr_, length);

  return kCommLogOK;
}

#ifdef libc_log
//GONG
CommLogErrT CommLog::PackAndPushLogs_libc(CD* parent_cd)
{
  // calculate length of data copy
  // pack format is [SIZE][CDID][Table][Queue][ChildLogQueue]
  uint64_t length;
  length = sizeof(uint64_t) + sizeof(CDID) // SIZE + CDID
         + sizeof(struct LogTable) + sizeof(struct LogTableElement)*log_table_.cur_pos_ //Table
         + sizeof(struct LogQueue) + sizeof(char)*log_queue_.cur_pos_ // Queue
         + sizeof(struct ChildLogQueue) + sizeof(char)*child_log_.cur_pos_; // ChildLogQueue

  // check parent's queue availability
  parent_cd->CommLogCheckAlloc_libc(length);

  // TODO: this is ugly, what about a tmp_ptr to pack and then copy to parent??
  // pack logs to parent's child_log_.base_ptr_
  PackLogs(parent_cd->libc_log_ptr_, length);

  return kCommLogOK;
}
#endif

CommLogErrT CommLog::PackLogs(CommLog *dst_cl_ptr, uint64_t length)
{
  if (dst_cl_ptr == NULL) return kCommLogError;

  LOG_DEBUG("dst_cl_ptr=%p\n", dst_cl_ptr);
  LOG_DEBUG("dst_cl_ptr->child_log_.base_ptr_=%p\n", dst_cl_ptr->child_log_.base_ptr_);
  LOG_DEBUG("before: child_log_.size_= %ld\n", dst_cl_ptr->child_log_.size_);
  LOG_DEBUG("before: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  uint64_t size;
  //[SIZE]
  size = sizeof(uint64_t);
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      &length, size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After size: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  //[CDID]
  size = sizeof(CDID);
  CDID tmp_cd_id = my_cd_->GetCDID();
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      &(tmp_cd_id), size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After CDID: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  //[Table] meta data
  size = sizeof(struct LogTable);
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      &log_table_, size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After table meta data: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  //[Table] data
  size = sizeof(struct LogTableElement)*log_table_.cur_pos_;
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      log_table_.base_ptr_, size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After table data: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  //[Queue] meta data
  size = sizeof(struct LogQueue);
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      &log_queue_, size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After queue meta data: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  //[Queue] data
  size = sizeof(char)*log_queue_.cur_pos_;
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      log_queue_.base_ptr_, size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After queue data: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  //[Queue] meta data
  size = sizeof(struct ChildLogQueue);
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      &child_log_, size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After child queue meta data: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  //[ChildLogQueue] data
  size = sizeof(char)*child_log_.cur_pos_;
  memcpy (&(dst_cl_ptr->child_log_.base_ptr_[dst_cl_ptr->child_log_.cur_pos_]), 
      child_log_.base_ptr_, size);
  dst_cl_ptr->child_log_.cur_pos_ += size;
  LOG_DEBUG("After child queue data: child_log_.cur_pos_ = %ld\n", dst_cl_ptr->child_log_.cur_pos_);

  LOG_DEBUG("After: child_log_.size_= %ld\n", dst_cl_ptr->child_log_.size_);

  return kCommLogOK;
}


// input parameter
CommLogErrT CommLog::ReadData(void *buffer, uint64_t length)
{
  LOG_DEBUG("ReadData to address (%p) and length(%ld)\n", buffer, length);
  LOG_DEBUG("reexec_pos_: %li\t cur_pos_: %li\n",log_table_reexec_pos_,log_table_.cur_pos_ );
  // reached end of logs, and need to return back and log data again...
  if (log_table_reexec_pos_ == log_table_.cur_pos_)
  {
    comm_log_mode_ = kGenerateLog;

    LOG_DEBUG("Reach end of log_table_ and comm_log_mode_ flip to %d\n", comm_log_mode_);
    return kCommLogCommLogModeFlip;
  }
  
  if (log_table_.cur_pos_ == 0)
  {
    ERROR_MESSAGE("Attempt to read from empty logs!\n");
    return kCommLogError;
  }

  if (length != log_table_.base_ptr_[log_table_reexec_pos_].length_)
  {
    ERROR_MESSAGE("Wrong length when read data from logs: %ld wanted but %ld expected!!\n", length, log_table_.base_ptr_[log_table_reexec_pos_].length_);
    return kCommLogError;
  }

  // if not completed, return kCommLogError, and needs to escalate
  if (log_table_.base_ptr_[log_table_reexec_pos_].completed_==false)
  {
    // Escalation to parent CD which is strict.
    // Report this event to head.
//    ERROR_MESSAGE("Error happens between isend/irecv and wait, needs to escalate...\n");
    return kCommLogMissing;
  }

  if (length != 0)
  {
    memcpy(buffer, 
        log_queue_.base_ptr_+log_table_.base_ptr_[log_table_reexec_pos_].pos_,
        log_table_.base_ptr_[log_table_reexec_pos_].length_);
  }

  // check if current log entry is a repeated log entry
  if (log_table_.base_ptr_[log_table_reexec_pos_].isrepeated_)
  {
    log_table_.base_ptr_[log_table_reexec_pos_].reexec_counter_++;
    if (log_table_.base_ptr_[log_table_reexec_pos_].reexec_counter_ 
        == log_table_.base_ptr_[log_table_reexec_pos_].counter_)
    {
      log_table_.base_ptr_[log_table_reexec_pos_].reexec_counter_ = 0;
      log_table_reexec_pos_++;
    }
  }
  else {
    log_table_reexec_pos_++;
  }

  return kCommLogOK;
}


// input parameter
// differences between ProbeData and ReadData is 
//    1) ProbeData only check log entry, not copy data
//    2) ProbeData's buffer is const void *, since ProbeData does not need to change contents in buffer 
CommLogErrT CommLog::ProbeData(const void *buffer, uint64_t length)
{
  LOG_DEBUG("Inside ProbeData!\n");
  LOG_DEBUG("reexec_pos_: %li\t cur_pos_: %li\n",log_table_reexec_pos_,log_table_.cur_pos_ );
  // reached end of logs, and need to return back and log data again...
  if (log_table_reexec_pos_ == log_table_.cur_pos_)
  {
    comm_log_mode_ = kGenerateLog;

    LOG_DEBUG("Reach end of log_table_ and comm_log_mode_ flip to %d\n", comm_log_mode_);
    return kCommLogCommLogModeFlip;
  }
  
  if (log_table_.cur_pos_ == 0)
  {
    ERROR_MESSAGE("Attempt to read from empty logs!\n");
    return kCommLogError;
  }

  if (length != log_table_.base_ptr_[log_table_reexec_pos_].length_)
  {
    ERROR_MESSAGE("Wrong length when read data from logs: %ld wanted but %ld expected!!\n", length, log_table_.base_ptr_[log_table_reexec_pos_].length_);
    return kCommLogError;
  }

  // if not completed, return kCommLogError, and needs to escalate
  if (log_table_.base_ptr_[log_table_reexec_pos_].completed_==false)
  {
    LOG_DEBUG("Not completed non-blocking send function, needs to escalate...\n");
    return kCommLogError;
  }

  // check if current log entry is a repeated log entry
  if (log_table_.base_ptr_[log_table_reexec_pos_].isrepeated_)
  {
    log_table_.base_ptr_[log_table_reexec_pos_].reexec_counter_++;
    if (log_table_.base_ptr_[log_table_reexec_pos_].reexec_counter_ 
        == log_table_.base_ptr_[log_table_reexec_pos_].counter_)
    {
      log_table_.base_ptr_[log_table_reexec_pos_].reexec_counter_ = 0;
      log_table_reexec_pos_++;
    }
  }
  else {
    log_table_reexec_pos_++;
  }

  return kCommLogOK;
}

void CommLog::Print()
{
  my_cd_->GetCDID().Print();

  LOG_DEBUG("\ncomm_log_mode_=%d\n", comm_log_mode_);

  LOG_DEBUG("\nUnits:\n");
  LOG_DEBUG("queue_size_unit_ = %ld\n", queue_size_unit_);
  LOG_DEBUG("table_size_unit_ = %ld\n", table_size_unit_);
  LOG_DEBUG("child_log_size_unit_ = %ld\n", child_log_size_unit_);

  LOG_DEBUG("\nlog_table_:\n");
  LOG_DEBUG("base_ptr_ = %p\n", log_table_.base_ptr_);
  LOG_DEBUG("cur_pos_ = %ld\n", log_table_.cur_pos_);
  LOG_DEBUG("table_size_ = %ld\n", log_table_.table_size_);
  
  uint64_t ii;
  for (ii=0;ii<log_table_.cur_pos_;ii++)
  {
    LOG_DEBUG("log_table_.base_ptr_[%ld]:\n", ii);
    LOG_DEBUG("pos_=%ld, length_=%ld, completed_=%d, flag_=%ld, counter_=%ld, reexec_counter_=%ld, isrepeated=%d\n\n"
        ,log_table_.base_ptr_[ii].pos_
        ,log_table_.base_ptr_[ii].length_
        ,log_table_.base_ptr_[ii].completed_
        ,log_table_.base_ptr_[ii].flag_
        ,log_table_.base_ptr_[ii].counter_
        ,log_table_.base_ptr_[ii].reexec_counter_
        ,log_table_.base_ptr_[ii].isrepeated_);
  }

  LOG_DEBUG ("log_table_reexec_pos_ = %ld\n", log_table_reexec_pos_);

  LOG_DEBUG("\nlog_queue_:\n");
  LOG_DEBUG("base_ptr_ = %p\n", log_queue_.base_ptr_);
  LOG_DEBUG("cur_pos_ = %ld\n", log_queue_.cur_pos_);
  LOG_DEBUG("queue_size_ = %ld\n", log_queue_.queue_size_);

  LOG_DEBUG("\nchild_log_:\n");
  LOG_DEBUG("base_ptr_ = %p\n", child_log_.base_ptr_);
  LOG_DEBUG("cur_pos_ = %ld\n", child_log_.cur_pos_);
  LOG_DEBUG("size_ = %ld\n", child_log_.size_);

  LOG_DEBUG("\nnew_log_generated_ = %d\n\n", new_log_generated_);
}


CommLogErrT CommLog::UnpackLogsToChildCD(CD* child_cd)
{
  char *src_ptr;
  CommLogErrT ret = FindChildLogs(child_cd->GetCDID(), &src_ptr);
  if (ret != kCommLogOK)
  {
    return ret;
  }

  (child_cd->comm_log_ptr_)->UnpackLogs(src_ptr);

  return kCommLogOK;
}

#ifdef libc_log
//GONG
CommLogErrT CommLog::UnpackLogsToChildCD_libc(CD* child_cd)
{
  char *src_ptr;
  CommLogErrT ret = FindChildLogs(child_cd->GetCDID(), &src_ptr);
  if (ret != kCommLogOK)
  {
    return ret;
  }

  (child_cd->libc_log_ptr_)->UnpackLogs(src_ptr);
  //GONG
//  child_cd->libc_log_ptr_->log_table_reexec_pos_ = 0;
  child_cd->libc_log_ptr_->Print();
  return kCommLogOK;
}
#endif

CommLogErrT CommLog::UnpackLogs(char *src_ptr)
{
  //TODO: too many places return kCommLogError and with error messages
  // Either do it with one error return, and different messages, 
  // or different returns
  
  if (src_ptr == NULL) 
  {
    ERROR_MESSAGE("NULL src_ptr to unpack logs\n");
    return kCommLogError;
  }

  uint64_t length;
  uint64_t index;
  uint64_t size;

  //[SIZE]
  size = sizeof(uint64_t);
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
  // FIXME: just increase table size, not re-allocate spaces...
  struct LogTableElement *tmp_ptr = new struct LogTableElement[log_table_.table_size_];
  if (tmp_ptr == NULL)
  {
    ERROR_MESSAGE("Cannot allocate space for log_table_.base_ptr_!\n");
    return kCommLogError;
  }

  //// should not delete the base_ptr_, in case accidentally delete something else
  //// if running correctly, base_ptr_ should not be allocated any space yet...
  //delete log_table_.base_ptr_;
  log_table_.base_ptr_ = tmp_ptr;

  size = sizeof(struct LogTableElement)*log_table_.cur_pos_;
  memcpy(log_table_.base_ptr_, src_ptr+index, size);
  index += size;

  // [Queue] meta data
  size = sizeof(struct LogQueue);
  memcpy(&log_queue_, src_ptr+index, size);
  index += size;

  // [Queue] data
  // FIXME: just increase queue size, not re-allocate spaces...
  char *tmp_queue_ptr = new char [log_queue_.queue_size_];
  if (tmp_queue_ptr == NULL)
  {
    ERROR_MESSAGE("Cannot allocate space for log_table_.base_ptr_!\n");
    return kCommLogError;
  }
  //delete log_queue_.base_ptr_;
  log_queue_.base_ptr_ = tmp_queue_ptr;

  size = sizeof(char) *log_queue_.cur_pos_;
  memcpy(log_queue_.base_ptr_, src_ptr+index, size);
  index += size;

  // [ChildLogQueue] meta data
  size = sizeof(struct ChildLogQueue);
  memcpy(&child_log_, src_ptr+index, size);
  index += size;

  // [ChildLogQueue] data
  char *tmp_child_log_ptr = new char [child_log_.size_];
  if (tmp_child_log_ptr == NULL)
  {
    ERROR_MESSAGE("Cannot allocate space for log_table_.base_ptr_!\n");
    return kCommLogError;
  }
  //delete child_log_.base_ptr_;
  child_log_.base_ptr_ = tmp_child_log_ptr;

  size = sizeof(char) *child_log_.cur_pos_;
  memcpy(child_log_.base_ptr_, src_ptr+index, size);
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
  uint64_t tmp_index=0;
  uint64_t length;
  CDID cd_id;
  #if _DEBUG
  LOG_DEBUG("Inside FindChildLogs to print CDID wanted:\n");
  child_cd_id.Print();
  #endif

  while(tmp_index < child_log_.cur_pos_)
  {
    memcpy(&length, child_log_.base_ptr_+tmp_index, sizeof(uint64_t));
    memcpy(&cd_id, child_log_.base_ptr_+tmp_index+sizeof(uint64_t), sizeof(CDID));

    #if _DEBUG
    LOG_DEBUG("Temp CDID got:\n");
    cd_id.Print();
    #endif
    if (cd_id == child_cd_id) 
    {
      LOG_DEBUG("Find the correct child logs\n");
      LOG_DEBUG("tmp_index=%ld, child_log_.cur_pos_=%ld\n", tmp_index, child_log_.cur_pos_);
      break;
    }
    tmp_index += length;
  }

  if (tmp_index >= child_log_.cur_pos_)
  {
    LOG_DEBUG("Warning: Could not find correspondent child logs, child may not push any logs yet...\n");
    return kCommLogChildLogNotFound;
  }

  *src_ptr = child_log_.base_ptr_+tmp_index;

  return kCommLogOK;
}


// this function is used when a cd complete to prepare for next begin
void CommLog::Reset()
{
  log_table_.cur_pos_ = 0;
  log_queue_.cur_pos_ = 0;
  child_log_.cur_pos_ = 0;

  log_table_reexec_pos_ = 0;

  new_log_generated_ = false;

  comm_log_mode_ = kGenerateLog;
}

#endif
