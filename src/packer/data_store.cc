//#define _GNU_SOURCE
#include <cstdlib>
#include <cstdio>
#include <string.h>
//#include <utility>
#include "define.h"
#include "data_store.h"

#include "cd_file_handle.h"
#include "buffer_consumer.h"

using namespace cd;

#include <pthread.h>
ActiveBuffer cd::active_buffer;

#ifdef _DEBUG_ENABLED
std::map<pthread_t, unsigned int> tid2str;
int indent_cnt = 0; 
#endif

int64_t chunksize_threshold = CHUNKSIZE_THRESHOLD_BASE;

DataStore::DataStore(bool alloc)
{
  MYDBG("\n");
  pthread_mutex_lock(&mutex);
  ptr_ = 0;
  head_ = 0;
  tail_ = 0;//sizeof(MagicStore);
  grow_unit_ = DATA_GROW_UNIT;
  size_ = grow_unit_;
  allocated_ = 0;
  mode_ = kGrowingMode | kPosixFile;
  ptr_ = NULL;
  if(alloc) AllocateData();
  /** Orer matters *************************/
  fh_ = GetFileHandle(ftype()); 
  BufferConsumer::Get()->InsertBuffer(this);
  /*****************************************/
  printf("BeforeWrite ft:%u fh:%p ptralloc:%p magic:%lu\n",ftype(),  fh_, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
  if(fh_->GetFileSize() < sizeof(MagicStore)) {
    fh_->Write(0, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
    fh_->FileSync();
  }
  chunksize_ = fh_->GetBlkSize();
  written_len_ = fh_->GetFileSize();
  printf("blksize:%u filesize:%lu (%lu)\n", chunksize_, written_len_, sizeof(MagicStore)); //getchar();
  pthread_mutex_unlock(&mutex);
}

DataStore::~DataStore(void)
{ 
  MYDBG("\n");
  FreeData(false); 
  //BufferConsumer::Get()->RemoveBuffer(this); 
}

CDErrType DataStore::Free(char *ptr)
{ 
  MYDBG("%p\n", ptr - sizeof(MagicStore));
  free(ptr - sizeof(MagicStore)); 
  return kOK; 
}

CDErrType DataStore::Alloc(void **ptr, uint64_t size)
{ 
  MYDBG("%lu\n", size);
  posix_memalign(ptr, CHUNK_ALIGNMENT, size + sizeof(MagicStore)); 
  MYDBG("done %d %p\n", size, *ptr); //getchar();
  *(char **)ptr = (char *)(*ptr) + sizeof(MagicStore);
  return kOK; 
}


CDErrType DataStore::Copy(void *dst, char *src, int64_t len)
{ 
  MYASSERT(len >= 0, "len:%ld\n", len);
  MYASSERT(dst != NULL && src != NULL, "Read failed:%p<-%p\n", dst, src); 
  if(len > 0) {
    memcpy(dst, src, len); 
  } 
  return kOK; 
}

CDErrType DataStore::AllocateData(void)
{
  CDErrType err = kOK;
  MYDBG("grow:%lu ptr:%p, used:%ld | ", grow_unit_, ptr_, buf_used());
  if(ptr_ == NULL) {
    MYDBG("alloc\n");
    //ptr_ = (char *)malloc(grow_unit_);
    void *ptr = (void *)ptr_;
    Alloc(&ptr, size_);
    ptr_ = (char *)ptr;
    if(ptr_ != NULL) {
      size_ = grow_unit_;
      allocated_++;
    } else {
      err = kMallocFailed;
    }
  } else {
    err = kAlreadyAllocated;
  }
  MYDBG("ptr:%p, size:%lu, used:%ld, allocate:%u\n", ptr_, size_, buf_used(), allocated_);
  return err;
}

CDErrType DataStore::Reallocate(uint64_t len) 
{
  CDErrType err = kOK;
  MYDBG("data_size:%ld/%lu   (%lu, %lu)\n", buf_used(), size_, head_, tail_);
  uint64_t orig_size = size_;
  while( len + buf_used() > size_ ) {
    if( size_ > TWO_GIGABYTE ) {
      size_ += grow_unit_; // Increase linearly
      grow_unit_ += grow_unit_;
    } else if( size_ > 0x10000000 ) { // 256 MiB
      size_ <<= 1; // doubling
      grow_unit_ = size_;
    } else {
      size_ <<= 2; // x4
      grow_unit_ = size_;
    }
  }
  void *newptr = NULL;
  Alloc(&newptr, size_);
  MYDBG("Alloc large [%p - %p - %p] data_size:%ld/%lu\n", 
         (char *)newptr, (char *)newptr+buf_used(), (char *)newptr+size_, buf_used(), size_);
  //getchar();
  if(newptr != NULL) {
    uint64_t tail = tail_ % orig_size;
    uint64_t first = 0;
    /******* Critical section begin: update ptr_ *********/
    pthread_mutex_lock(&mutex);
    uint64_t head = head_ % orig_size;
    memcpy((char *)newptr - sizeof(MagicStore), ptr_ - sizeof(MagicStore), sizeof(MagicStore));
    char *newptr_eff = (char *)newptr;// + sizeof(MagicStore);
    if(tail_ > head_) {
      if(head >= tail) {
        first = orig_size - head;
        MYDBG("%lu %lu %lu offset1:%lu, offset2:%lu, offset3:%lu\n", size_, head_, tail_, (size_ - first), head, first);
        Copy(newptr_eff + head, ptr_ + head, first);
        Copy(newptr_eff, ptr_, buf_used() - first);
      } else {
        Copy(newptr_eff + head, ptr_ + head, buf_used());
      }
    }
    Free(ptr_);
    ptr_ = newptr_eff;
    //size_ = newsize;
    allocated_++;
    pthread_mutex_unlock(&mutex);
    /***********************************************************/
  } else {
    err = kMallocFailed;
  }
  return err;
}

//CDErrType DataStore::SafeReallocate(uint64_t len) 
//{
//  MYDBG("\n");
//  pthread_mutex_lock(&mutex);
//  CDErrType ret = Reallocate(len);
//  pthread_mutex_unlock(&mutex);
//  return ret;
//}

CDErrType DataStore::FreeData(bool reuse)
{
  MYDBG("reuse:%d\n", reuse);
  CDErrType err = kOK;

  /********** Critical section *************/
  pthread_mutex_lock(&mutex);

  // Init
  tail_ = 0;//sizeof(MagicStore);
  head_ = 0;
  allocated_ = 0;

  if(reuse == false) {
    size_ = 0;
    grow_unit_ = DATA_GROW_UNIT;
    if(ptr_ != NULL) {
      MYDBG("free:%d\n", reuse);
      Free(ptr_);
      ptr_ = NULL;
    } else {
      err = kAlreadyFree;
    }
  }
  BufferConsumer::Get()->RemoveBuffer(this); 
  pthread_mutex_unlock(&mutex);
  /*****************************************/
  return err;
}

void DataStore::UpdateMagic(const MagicStore &magic)
{
  MYDBG("\n");
  MagicStore *magic_ = reinterpret_cast<MagicStore *>(ptr_ - sizeof(MagicStore));
  *magic_ = magic;
  //magic_->Print();
}

void DataStore::UpdateMagic(MagicStore &&magic)
{
  MYDBG("\n");
  MagicStore *magic_ = reinterpret_cast<MagicStore *>(ptr_ - sizeof(MagicStore));
  *magic_ = std::move(magic);
  //magic_->Print();
}

//static inline
//bool CheckCond(void) 
//{
//  return buf_used() >= chunksize_threshold;
//}

// Bounded buffer mode: write from the ptr
uint64_t DataStore::Write(char *pfrom, uint64_t len)//, uint64_t pos)
{
  MYDBG("%lu\n", len);

  // deprecated
  active_buffer.id_ = this;
  active_buffer.state_ = mode_;

  uint64_t ret = 0;
  if( CHECK_FLAG(mode_, kBoundedMode) ) {
    // Reallocate buffer or wait until completing write
    // If WriteBuffer() is still not sufficient to
    // reserve space for incoming data size (len),
    // Do reallocate
    if( len > size_ ) {
      Reallocate(len);
    }

    ret = WriteBuffer(pfrom, len);

  } else { // Non-buffering mode

    if( len + buf_used() > size_ ) {
      Reallocate(len);
    }
//    bool is_empty = IsEmpty();
    ret = WriteMem(pfrom, len);

    if( ftype() != kVolatile ) {
      if( CHECK_FLAG(mode_, kConcurrent) ) {
        if( buf_used() >= chunksize_threshold ) {
          SetActiveBuffer();
        }
      } else if(buf_used() >= chunksize_threshold) {
        MYDBG("Flush file for unbounded buffer mode\n"); //getchar();
        WriteFile(buf_used());
        MYDBG("Flush done\n"); //getchar();
      }
        // if buffer contents is not sufficient to write,
        // do file write() at the next available time.
    }
//    else { // Concurrent buffer consumer enabled
//      // Initially empty, and still less than chunksize
//
//      //if(is_empty && IsEmpty() == false) { 
//      if(is_empty && IsEmpty() == false) { 
//        pthread_mutex_lock(&mutex);
//        CD_UNSET(active_buffer.state_, kBufferEmpty);
//        pthread_mutex_unlock(&mutex);
//      }     
//    }
  }
  return ret;
}

////////////////////////////////////////////////////////////
//
// Buffer space will be always available,
// because it is reallocated in the case that 
// the space is not sufficient for the incoming data.
//
// Note that buffer is still circular in non-buffering mode.
// Therefore, it is necessary to write the upper segment and lower segment,
// if tail + len becomes larger than size.
uint64_t DataStore::WriteMem(char *src, int64_t len)
{
  MYDBG("\n");
  uint64_t ret = 0;
  if( len > 0 && ptr_ != NULL && src != NULL ) {  
    const uint64_t tail  = tail_ % size_;
    const uint64_t first = size_ - tail;
    const int64_t  rest  = len - first;
    MYDBG("%lu + %lu > %lu (head:%lu)\n", tail, len, size_, head_ % size_);
    if( rest > 0 ) {
      MYDBG("First copy  %p <- %p, %lu\n", ptr_ + (size_ - first), src, first);
      MYDBG("Second copy %p <- %p, %lu\n", ptr_, src + first, rest);
      Copy(ptr_ + tail, src, first);
      Copy(ptr_, src + first, rest);
      tail_ += len;
      ASSERT((tail_ % size_) == (uint64_t)rest);
    } else {
      MYDBG("Copy %p <- %p, %lu\n", ptr_ + tail, src, len);
      Copy(ptr_ + tail, src, len);
      tail_ += len;
    }
    ret = buf_used();
  } else {
    ERROR_MESSAGE("Write failed: ptr_:%p %p\n", ptr_, src);
  }
  return ret;
}

///////////////////////////////////////////////////////////////
//
// tail_ can grow in any size, but head always can grow in chunksize_
// But these phases are necessary to allow ReadBuffer() to correctly
// consume data in chunksize_ with alignment.
// In this case, it does not need to consider segments like WriteMem()
// because it always writes in aligned chunksize_.
uint64_t DataStore::WriteBuffer(char *src, int64_t len)
{
  MYDBG("\n");
  if(len > 0) {
    // Phase1: write until the next alignment
    const uint32_t available_in_chunk = chunksize_ - (tail_ % chunksize_);
    uint32_t len_to_write = (len < available_in_chunk)? len : available_in_chunk;
    MYDBG("Phase1: %p <- %p (%lu)\n", ptr_ + (tail_ % size_), src, len_to_write);

    // Actually does not need a lock.
    // It is safe to overlap writing data to not-yet-filled chunk in any case.
    // If there is no other chunks, it is still empty.
    if(available_in_chunk > 0) {
      ASSERT(len_to_write < chunksize_);
//      pthread_mutex_lock(&mutex);
      Copy(ptr_ + (tail_ % size_), src, len_to_write);
      tail_ += len_to_write;
//      pthread_mutex_unlock(&mutex);
      len -= len_to_write;
    }

    // Phase2: write in the unit of chunksize_ with alignment
    int64_t i = len_to_write;
    while(len >= chunksize_) {
      MYDBG("Phase2:");
      WriteInternal(src, chunksize_, i);
      i += chunksize_;
      len -= chunksize_;
    }
  
    // Phase3: write the rest of length
    if(len > 0) {
      MYDBG("Phase3:");
      ASSERT(len < chunksize_);

      WriteInternal(src, len, i);
      len = 0;
      //getchar();
    }
  }
  return tail_;
}

void DataStore::WriteInternal(char *src, uint32_t len_to_write, int64_t &i) 
{
  MYDBG("%p %u %ld: %p <- %p (%u) \n", src, len_to_write, i, ptr_ + (tail_ % size_), src + i, len_to_write);
  pthread_mutex_lock(&mutex);
  while( IsFull() ) {
    pthread_cond_wait(&full, &mutex);
  }

  Copy(ptr_ + (tail_ % size_), src + i, len_to_write);
  tail_ += len_to_write;

  MYDBG("%p + %ld (%u)\n", src, i, len_to_write);
  pthread_cond_signal(&empty);
  pthread_mutex_unlock(&mutex);
}

void DataStore::WriteInternal2(char *src, uint32_t len_to_write, int64_t &i) 
{
  MYDBG("%p %u %ld: %p <- %p (%u) \n", src, len_to_write, i, ptr_ + (tail_ % size_), src + i, len_to_write);

  pthread_mutex_lock(&mutex);
  Copy(ptr_ + (tail_ % size_), src + i, len_to_write);
  tail_ += len_to_write;
  pthread_mutex_unlock(&mutex);

  MYDBG("%p + %ld (%u)\n", src, i, len_to_write);
}


void DataStore::Read(char *pto, uint64_t len, uint64_t pos)
{
  //char *src = ptr_ + sizeof(MagicStore) + pos;
  char *src = ptr_ + (head_ % size_) + pos;
  MYDBG("%p <- %p (%lu)\n", pto, src, len);
  if( ptr_ != NULL && pto != NULL ) {  
    Copy(pto, src, len);
  } else {
    ERROR_MESSAGE("Read failed: src:%p, pto:%p\n", src, pto);
  }
}

//void DataStore::ReadAll(char *pto, uint64_t len, uint64_t pos)

// In-place read
void DataStore::ReadAll(char *pto)
{
  printf("[%s] written:%lu head:%lu tail:%lu\n", __func__, written_len_, head_, tail_);
  if(head_ != 0) { // read from file
    fh_->ReadTo(pto, head_, written_len_);
  }

  const int64_t rest = buf_used();
  MYDBG("%p <- %p (%lu)\n", pto, ptr_, rest);
  if( ptr_ != NULL && pto != NULL && rest != 0) {  
    MYDBG("%p <- %p (%lu)\n", pto, ptr_, rest);
    Copy(pto + (head_ % size_), ptr_ + (head_ % size_), rest);
  } else {
//    ERROR_MESSAGE("Read failed: src:%p, pto:%p\n", ptr_, pto);
  }
}


char *DataStore::ReadAll(uint64_t &totsize)
{
  void *ret_ptr = NULL;
  //MYDBG("\n\n\n##############\n\n\n\n\n\ntail: %lu\n", tail_);
  //printf("written:%lu head:%lu tail:%lu\n", written_len_, head_, tail_);
  posix_memalign(&ret_ptr, CHUNK_ALIGNMENT, tail_);
  //ret_ptr  = malloc(tail_);
  //printf("written:%lu head:%lu tail:%lu\n", written_len_, head_, tail_);
  ReadAll((char *)ret_ptr);
  totsize = tail_;
  return (char *)ret_ptr;
}

CDErrType DataStore::WriteFile(void) 
{
  return WriteFile(chunksize_);
}

CDErrType DataStore::WriteFile(int64_t len)
{
  ASSERT(len > 0);
  uint64_t mask_len = ~(chunksize_ - 1);
  MYDBG(" len:%lu\n", len);
  printf("[DataStore::%s] head:%lu written:%lu, used:%ld, len:%ld, inc:%lu\n", 
      __func__, head_, written_len_, buf_used(), len, mask_len & len); //getchar();
  CDErrType ret = fh_->Write(head_ + written_len_, begin(), len);
  head_ += mask_len & len; 

  return ret;
}

void DataStore::FileSync(void)
{
  fh_->FileSync();
}

void DataStore::Flush(void)
{
  if(buf_used() > 0) {
    MYDBG("\n\n##### [%s] %ld %zu ###\n", __func__, buf_used(), sizeof(MagicStore));
    WriteFile(buf_used());
    FileSync();
  }
}


void DataStore::SetActiveBuffer(bool high_priority) 
{
//  if( CHECK_FLAG(mode_, kEagerWriting) ) { 
    pthread_mutex_lock(&mutex);
  
    if(high_priority) {
      active_buffer.id_ = this;
      CD_SET(mode_, kEagerWriting); 
      active_buffer.state_ = mode_;
    }
  
    pthread_cond_signal(&empty); 
    pthread_mutex_unlock(&mutex);
//  }
}




MagicStore::MagicStore(void) : total_size_(0), table_offset_(0), entry_type_(0) {}
MagicStore::MagicStore(uint64_t total_size, uint64_t table_offset, uint32_t entry_type) 
  : total_size_(total_size), table_offset_(table_offset), entry_type_(entry_type) {}
void MagicStore::Print(void) 
{
  printf("[%s] %lu %lu %u\n", __func__, total_size_, table_offset_, entry_type_);
}
