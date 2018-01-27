#include "buffer_consumer.h"
#include "buffer_consumer_interface.h"
#include "data_store.h"
using namespace packer;

////////////////////////////////////////////////////////////////////////////////////////
//
// BufferConsumer
//
pthread_t child = -1;
pthread_cond_t  packer::full  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  packer::empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t packer::mutex = PTHREAD_MUTEX_INITIALIZER;

BufferConsumer *BufferConsumer::buffer_consumer_ = NULL;

BufferConsumer *BufferConsumer::Get(void) 
{
  if(buffer_consumer_ == NULL) {
    MYDBG("\n");
    /********* Order matters ***********/
    BufferConsumer *bc = new BufferConsumer;
    buffer_consumer_ = bc;
    bc->Init();
    /***********************************/
  }
  return buffer_consumer_;
}

void BufferConsumer::Init(void) 
{
  MYDBG("\n");
  pthread_create(&child, NULL, BufferConsumer::ConsumeBuffer, (void *)this);
}

BufferConsumer::~BufferConsumer(void)
{
  MYDBG("\n");
  pthread_join(child, NULL);
}

void BufferConsumer::Delete(void) 
{
  delete this;
  buffer_consumer_ = NULL;
}

void BufferConsumer::InsertBuffer(DataStore *ds) 
{
  MYDBG("%p (%p)\n", ds, this); 
//  if(packerTaskID == 0) printf("Insert buffer %p (%p)\n", ds, this); //getchar();

  // PadZero is necessary!
  // Otherwise the data written by previous data_store obj
  // may be overwritten by new object's.
  if(buf_list_.empty() == false) {
    buf_list_.back()->SetFileOffset(buf_list_.back()->PadZeros(0));
  }
  buf_list_.push_back(ds);
}

void BufferConsumer::RemoveBuffer(DataStore *ds) 
{
  MYDBG("%p\n", ds); //getchar();
  bool found = false;
  for(auto it=buf_list_.begin(); it!=buf_list_.end(); ++it) {
    if(*it == ds) { 
      buf_list_.erase(it); 
      found = true;
      break;
    }
  }
  if( found == false ) {
    ERROR_MESSAGE_PACKER("Remove buffer %p which is not existing in buf_list_\n", ds);
  }
}

// Wake up condition
// 1. Eager Consumption mode && there is any non-empty buffer
// 2. Signal mode && there is valid buffer ID
// active_buffer.id_ is the pointer for the active buffer.
// But may be fixed to some ID for buffer. (FIXME)
//
// active_buffer.id_ must be set to NULL when buffer is used as mode 1.
// Assumption(FIXME) : 
// there will be always a single active buffer per a task at a time. 
// If there are concurrent running buffer, active_buffer should be queue or
// condition variable.
//
// NOTE: Once active_buffer is selected, any other buffer cannot be selected
// until the active_buffer complete flushing.
// buffer->SetActiveBuffer() // signal to flush buffer to disk
DataStore *BufferConsumer::GetBuffer(void) 
{
  DataStore *buffer = NULL;
  // active_buffer must be atomic
  if(active_buffer.id_ != NULL) {
    if( CHECK_FLAG(active_buffer.state_, kEagerWriting) ) { // buffer is active
      buffer = active_buffer.id_;
    }
  }
  else {
    // tail_ - head_ must be atomic
    for(auto it=buf_list_.begin(); it!=buf_list_.end(); ++it) {
      if( (*it)->IsEmpty() == false && CHECK_FLAG((*it)->mode(), kEagerWriting) ) {
        MYDBG("empty head:%lu tail:%lu, used:%ld\n", (*it)->head(), (*it)->tail(), (*it)->buf_used());
        buffer = *it; 
        break;
      } else {
        MYDBG("head:%lu tail:%lu, used:%ld\n", (*it)->head(), (*it)->tail(), (*it)->buf_used());
      }
    }
  }
  // If there is no active_buffer and every buffer is not eager buffering mode,
  // just return NULL to let consumer wait. 

  MYDBG("%p, listsize:%zu\n", buffer, buf_list_.size());
  return buffer;
}

/****************************************************
 * Read buffer
 * Write file until buffer is empty
 *   read:  chunk_mask, head_, size_, ptr_, written_len_
 *   write: DataStore::head_, FileHandle::offset_
 * Sync file
 ****************************************************/
void *BufferConsumer::ConsumeBuffer(void *bc) 
{
  MYDBG("\n");
//  BufferConsumer *fhandle = buffer_list;
  BufferConsumer *consumer = (BufferConsumer *)bc;
//  DataStore *buffer = fhandle->IsBu
//  DataStore *buffer = NULL;
  while(1) 
  {
    MYDBG("BufferConsumer::Write\n");
    PACKER_ASSERT(buffer_consumer_ != NULL);
    /******* Critical section begin: Only update head_ *********/
    pthread_mutex_lock(&mutex);  
    MYDBG("111\n");

    // head_, tail_, size_ must not by racy
    // buffer_state cannot be non-zero
    // before BufferConsumer is created, or
    // WriteBuffer is complete, or
    // before the completion of buffer allocation, or
    // after the beginning of buffer deallocation.

    //while( IsInvalidBuffer() ) {
    
    //if(active_buffer_.state_ != kWaiting) 
    {
      DataStore *tbuff = NULL;
      while( tbuff == NULL ) {
        MYDBG("Before going to sleep\n");
        pthread_cond_wait(&empty, &mutex);
        MYDBG("After wake up\n");
        tbuff = consumer->GetBuffer();
      }
    }

    // CD_SET(kEagerWrite)

    DataStore *buffer = buffer_consumer_->GetBuffer();
    buffer->WriteFile();
  
    // Write the rest of chunk, but 
    // leave them in the buffer.
    const int64_t rest = buffer->buf_used();
    if(rest > 0 && rest < buffer->chunksize()) {
      MYDBG("BufferConsumer::WriteFile Rest:%u \n", rest);
      buffer->WriteFile(rest);
      buffer = NULL;
    } else if (rest < 0) { 
      ERROR_MESSAGE_PACKER("rest (%ld) < 0\n", rest);
    }



    buffer->FileSync();

    pthread_cond_signal(&full);
    pthread_mutex_unlock(&mutex);
    /******* Critical section end *********/
  }
  return NULL;
}
