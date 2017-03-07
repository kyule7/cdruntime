//#define _GNU_SOURCE
#include <cstdlib>
#include <cstdio>
#include <string.h>
//#include <utility>
#include "define.h"
#include "data_store.h"

#include "cd_file_handle.h"
#include "buffer_consumer.h"

using namespace packer;

#include <pthread.h>
ActiveBuffer packer::active_buffer;

#ifdef _DEBUG_ENABLED
std::map<pthread_t, unsigned int> tid2str;
int indent_cnt = 0; 
#endif

uint64_t packer::table_id = TABLE_ID_OFFSET;
int64_t chunksize_threshold = CHUNKSIZE_THRESHOLD_BASE;

//DataStore::DataStore(bool alloc) 
//{
//  MYDBG("\n");
//  assert(0);
//  // Should be thread-safe
//  Init(NULL);
//
//  printf("BeforeWrite ft:%u fh:%p ptralloc:%p magic:%lu\n",
//        ftype(), fh_, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
//  if(fh_->GetFileSize() < (int64_t)sizeof(MagicStore)) {
//    fh_->Write(0, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
//    fh_->FileSync();
//  }
//
//  written_len_ = fh_->GetFileSize();
//  printf("blksize:%u filesize:%lu (%lu)\n", 
//      chunksize_, written_len_, sizeof(MagicStore)); //getchar();
//
//}
DataStore::DataStore(char *ptr)
{
  MYDBG("\nptr:%p", ptr);

  // Should be thread-safe
  Init(ptr);

  printf("BeforeWrite ft:%u fh:%p ptralloc:%p magic:%lu\n",
        ftype(), fh_, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
  if(fh_->GetFileSize() < (int64_t)sizeof(MagicStore)) {
    fh_->Write(0, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
    fh_->FileSync();
  }

  written_len_ = fh_->GetFileSize();
  printf("blksize:%u filesize:%lu (%lu)\n", 
      chunksize_, written_len_, sizeof(MagicStore)); //getchar();
}

void DataStore::Init(char *ptr) 
{
  pthread_mutex_lock(&mutex);
  ptr_ = NULL;
  grow_unit_ = DATA_GROW_UNIT;
  size_ = grow_unit_;
  head_ = 0;//size_ - size_ / 4;
  tail_ = head_;//sizeof(MagicStore);
  allocated_ = 0;
  mode_ = kGrowingMode | kPosixFile;
  r_tail_ = 0;
  r_head_ = 0;
  if(ptr == NULL) 
    AllocateData();
  else 
    ptr_ = ptr + sizeof(MagicStore);
  /** Order matters *************************/
  fh_ = GetFileHandle(ftype()); 
  BufferConsumer::Get()->InsertBuffer(this);
  /*****************************************/
  chunksize_ = fh_->GetBlkSize();

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
  PACKER_ASSERT_STR(len >= 0, "len:%ld\n", len);
  PACKER_ASSERT_STR(dst != NULL && src != NULL, "Read failed:%p<-%p\n", dst, src); 
  if(len > 0) {
    memcpy(dst, src, len); 
  } 
  return kOK; 
}

// Initialize ptr_, size_, head_, tail_
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
  fh_->SetOffset(written_len_);

  if(reuse == false) {
    size_ = 0;
    grow_unit_ = DATA_GROW_UNIT;
    // free buffer
    if(ptr_ != NULL) {
      MYDBG("free:%d\n", reuse);
      Free(ptr_);
      ptr_ = NULL;
    } else {
      err = kAlreadyFree;
    }
    // free file
    fh_->Truncate(written_len_);
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

MagicStore &DataStore::GetMagicStore(uint64_t offset)
{
  if(offset != INVALID_NUM)
    return *reinterpret_cast<MagicStore *>(ptr_ + (offset % size_));
  else 
    return *reinterpret_cast<MagicStore *>(ptr_ - sizeof(MagicStore));
}

CDErrType DataStore::FlushMagic(const MagicStore *magic)
{
  MYDBG("\n");
  CDErrType ret = fh_->Write(0, (char *)magic, sizeof(MagicStore), 0);
  return ret;
}

//static inline
//bool CheckCond(void) 
//{
//  return buf_used() >= chunksize_threshold;
//}

uint64_t DataStore::WriteBufferMode(char *pfrom, uint64_t len)
{
  // Reallocate buffer or wait until completing write
  // If WriteBuffer() is still not sufficient to
  // reserve space for incoming data size (len),
  // Do reallocate
  if( len > size_ ) {
    Reallocate(len);
  }

  return WriteBuffer(pfrom, len);
}

uint64_t DataStore::WriteCacheMode(char *pfrom, uint64_t len)
{
  if( len + buf_used() > size_ ) {
    Reallocate(len);
  }
  return WriteMem(pfrom, len);
}


// Bounded buffer mode: write from the ptr
uint64_t DataStore::Write(char *pfrom, uint64_t len)//, uint64_t pos)
{
  MYDBG("%lu\n", len);

  // deprecated
  active_buffer.id_ = this;
  active_buffer.state_ = mode_;

  uint64_t ret = INVALID_NUM;
  if( CHECK_FLAG(mode_, kBoundedMode) ) {

    ret = WriteBufferMode(pfrom, len);

  } else { // Non-buffering mode
//    bool is_empty = IsEmpty();
    ret = WriteCacheMode(pfrom, len);

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
  // FIXME
  return ret;
//  return (ret + written_len_);
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
  uint64_t ret = INVALID_NUM;
  if( len > 0 && ptr_ != NULL && src != NULL ) {  
    const uint64_t tail  = tail_ % size_;
    const uint64_t first = size_ - tail;
    const int64_t  rest  = len - first;
    ret = used();
    MYDBG("%lu + %lu > %lu (head:%lu)\n", tail, len, size_, head_ % size_);
    if( rest > 0 ) {
      MYDBG("First copy  %p <- %p, %lu\n", ptr_ + (size_ - first), src, first);
      MYDBG("Second copy %p <- %p, %lu\n", ptr_, src + first, rest);
      Copy(ptr_ + tail, src, first);
      Copy(ptr_, src + first, rest);
      tail_ += len;
      PACKER_ASSERT((tail_ % size_) == (uint64_t)rest);
    } else {
      MYDBG("Copy %p <- %p, %lu\n", ptr_ + tail, src, len);
      Copy(ptr_ + tail, src, len);
      tail_ += len;
    }
  } else {
    ERROR_MESSAGE_PACKER("Write failed: len: %ld ptr_:%p %p\n", len, ptr_, src);
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
  uint64_t ret = INVALID_NUM;
  if( len > 0 && ptr_ != NULL && src != NULL ) { 
    ret = used();
    // Phase1: write until the next alignment
    const uint32_t available_in_chunk = chunksize_ - (tail_ % chunksize_);
    uint32_t len_to_write = (len < available_in_chunk)? len : available_in_chunk;
    MYDBG("Phase1: %p <- %p (%lu)\n", ptr_ + (tail_ % size_), src, len_to_write);

    // Actually does not need a lock.
    // It is safe to overlap writing data to not-yet-filled chunk in any case.
    // If there is no other chunks, it is still empty.
    if(available_in_chunk > 0) {
      PACKER_ASSERT(len_to_write < chunksize_);
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
      PACKER_ASSERT(len < chunksize_);

      WriteInternal(src, len, i);
      len = 0;
      //getchar();
    }
  } else {
    ERROR_MESSAGE_PACKER("Write failed: ptr_:%p %p\n", ptr_, src);
  }
  return ret;
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
  //if(len > (uint64_t)buf_used()) { // some portion is written to file
  const uint64_t file_offset = written_len_ + head_;

  int64_t chunk_in_file = 0;
  int64_t chunk_in_memory = 0;
  if(pos < file_offset) { // some portion is written to file
    PACKER_ASSERT(buf_used() >= 0);

    const uint64_t data_offset = len + pos;
    if(data_offset <= file_offset) { 
      chunk_in_file = len;
    } else {
      chunk_in_file = file_offset - pos;
      chunk_in_memory = len - chunk_in_file;
    }
    const int64_t chunk_written_before_len = tail_ - len;
    PACKER_ASSERT(chunk_written_before_len >= 0);
    PACKER_ASSERT(buf_used() >= 0);
    
    MYDBG("DataStore: %p, %lu, %lu\n", pto, chunk_in_file, pos);

    fh_->ReadTo(pto, chunk_in_file, pos); 

#if 0
    const int64_t chunk_in_file = len - buf_used();
    //fh_->ReadTo(pto, chunk_in_file, written_len_ + head_ - chunk_in_file); 
    const int64_t chunk_written_before_len = tail_ - len;
    PACKER_ASSERT(chunk_written_before_len <= 0);
    PACKER_ASSERT(buf_used() < 0);
    fh_->ReadTo(pto, chunk_in_file, written_len_ + chunk_written_before_len); 
#endif
  }

  //char *src = ptr_ + (head_ % size_) + pos;
  char *src = NULL;
  if(chunk_in_file != 0) {
    pto += chunk_in_file;
    src = ptr_ + (head_ % size_);
  } else {
    src = ptr_ + ((pos - file_offset) % size_);
    chunk_in_memory = len;
  }
  MYDBG("%p <- %p (%lu + %lu = %lu)\n", pto, src, chunk_in_file, chunk_in_memory, len);
  if(chunk_in_memory != 0) {
    MYDBG("$$ Read From Memory $$ %d == %d\n", *(int *)pto, *(int *)src);
    Copy(pto, src, chunk_in_memory);
  }
}

//void DataStore::ReadAll(char *pto, uint64_t len, uint64_t pos)

// In-place read
void DataStore::ReadAll(char *pto)
{
  printf("[%s] written:%lu head:%lu tail:%lu\n", __func__, written_len_, head_, tail_);
  if(head_ != 0) { // read from file
    fh_->ReadTo(pto, head_, written_len_);
//    fh_->ReadTo(pto, head_ + sizeof(MagicStore), written_len_);
  }

  const int64_t rest = buf_used();
  MYDBG("%p <- %p (%lu)\n", pto, ptr_, rest);
  Copy(pto + (head_ % size_), ptr_ + (head_ % size_), rest);
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

char *DataStore::ReadOpt(uint64_t len, uint64_t pos)
{
  char *ret = NULL;
  if( CHECK_FLAG(mode_, kReadMode) == false) {
    Flush();
    CD_SET(mode_, kReadMode); 
    PACKER_ASSERT(head_ == tail_);
    r_tail_ = tail_;
    r_head_ = tail_;
  }
  PACKER_ASSERT(CHECK_FLAG(mode_, kReadMode));
  const int64_t offset = head_ - written_len_;
  const uint64_t offset_in_buffer = pos + offset;
  const int64_t file_len_to_read  = offset_in_buffer + len - r_tail_;
  int64_t buffer_len_to_read      = len;
  if(file_len_to_read > 0) { // need to read data from file
    buffer_len_to_read -= file_len_to_read;
    MYDBG("DataStore: %p, %lu, %lu\n", 
        ptr_ + (r_tail_ % size_), file_len_to_read, r_tail_ - offset);
    fh_->ReadTo(ptr_ + (r_tail_ % size_), file_len_to_read, r_tail_ - offset);
    PACKER_ASSERT(r_tail_ - offset == pos + buffer_len_to_read); 
    // r_tail_ - offset - pos - len + offset + pos + len - r_tail_ == 0
  }
  
  ret = ptr_ + ((pos + offset) % size_); 

  if(r_tail_ - head_ == head_ - written_len_) {
    CD_UNSET(mode_, kReadMode); 
  }

  return ret;
}

CDErrType DataStore::WriteFile(void) 
{
  return WriteFile(chunksize_);
}

CDErrType DataStore::WriteFile(int64_t len)
{
  CDErrType ret = kOK;
  PACKER_ASSERT(len > 0);
  uint64_t mask_len = ~(chunksize_ - 1);
  MYDBG("[DataStore::%s] head:%lu written:%lu, used:%ld, len:%ld, inc:%lu\n", 
      __func__, head_, written_len_, buf_used(), len, mask_len & len); //getchar();
  const int64_t first = size_ - (head_ % size_);
  PACKER_ASSERT(first >= 0);

  const uint64_t file_offset = head_ + written_len_;
  if(len > first) {
    CDErrType iret = fh_->Write(file_offset, begin(), first);

    PACKER_ASSERT_STR( (((head_ % size_) + first) % size_) == 0, 
        "((%lu + %lu) %% %lu) == %lu\n", (head_ % size_), first, size_, (((head_ % size_) + first) % size_));

    const int64_t second = len - first;
    ret = fh_->Write(file_offset + first, ptr_, second, mask_len & second);

    ret = (CDErrType)((uint32_t)iret | (uint32_t)(ret));
  } else {
    ret = fh_->Write(file_offset, begin(), len, mask_len & len);
  }
  head_ += mask_len & len; 

  return ret;
}


void DataStore::FileSync(void)
{
  fh_->FileSync();
}

CDErrType DataStore::Flush(void)
{
  CDErrType ret = kOK;
  if(buf_used() > 0) {
    MYDBG("\n\n##### [%s] %ld %zu ###\n", __func__, buf_used(), sizeof(MagicStore));
    ret = WriteFile(buf_used());
    FileSync();
  }
  return ret;
}

uint64_t DataStore::WriteFlushMode(char *src, uint64_t len)
{
  static uint64_t tot_write_len = 0;
  if(len <= 0) {
    return used();
  }
  else {
  
    if( (uint64_t)(len + buf_used()) > size_ ) {
      MYDBG("##### [%s] buf:%ld len:%zu #####\n", __func__, buf_used(), len);
      CDErrType err = WriteFile(buf_used());
      if(err != kOK) {
        MYDBG("Error while WriteFile(%lu)\n", len);
      }
    }

    const uint64_t size_to_write = (len <= size_)? len : size_;
    const uint64_t offset = WriteMem(src, size_to_write);

    tot_write_len += size_to_write;

    // Writing src.
    // Main usage of this is to write tail associated with data
//    PACKER_ASSERT((uint64_t)(len + buf_used()) < size_);
  
    // The return value is only used for the first recursive function.
    // The return value of following recursions are descarded.
    if(len - size_to_write > 0) {
      tot_write_len += WriteFlushMode(src, len - size_to_write);
    } 

    return offset;
  }
}

uint64_t DataStore::Flush(char *src, int64_t len) 
{
  CDErrType err = kOK;
  PACKER_ASSERT(len < 0);

  // The reason for not reusing Write(src,len) for writing table
  // is that it is likely to just flush data and table to get persistency at
  // some point, not keeping writing some data to data store.
  // Therefore, it will be good to just make the data sture empty, then copy
  // table store to buffer, then perform file write.
  // The reason for not performing file write from table store itself, but
  // copying to the tail of data store is that table store may be not aligned
  // for file write, and there may be some remaining data after the alignment in
  // data store. 
  uint64_t table_offset = WriteFlushMode(src, len);

  err = WriteFile( buf_used() );
  if(err != kOK) {
    MYDBG("Error while writing something before flush\n");
  }
  err = fh_->Write(0, GetPtrAlloc(), sizeof(MagicStore), 0);
  if(err != kOK) {
    MYDBG("Error in flush\n");
  }
  FileSync();

  return table_offset;  
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




MagicStore::MagicStore(void) : total_size_(0), table_offset_(0), entry_type_(0) {
  memset(pad_, 0, sizeof(pad_));
}

MagicStore::MagicStore(uint64_t total_size, uint64_t table_offset, uint32_t entry_type) 
  : total_size_(total_size), table_offset_(table_offset), entry_type_(entry_type) {
  memset(pad_, 0, sizeof(pad_));
}

MagicStore::MagicStore(const MagicStore &that) 
  : total_size_(that.total_size_), table_offset_(that.table_offset_), entry_type_(that.entry_type_) {
  memcpy(pad_, that.pad_, sizeof(pad_));
}

void MagicStore::Print(void) 
{
  printf("[%s] %lu %lu %u\n", __func__, total_size_, table_offset_, entry_type_);
}

