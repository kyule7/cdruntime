//#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <utility>
#include "define.h"
#include "data_store.h"

#include "cd_file_handle.h"
#include "buffer_consumer.h"
#include "packer_prof.h"

using namespace packer;
//packer::Time packer::time_write("time_write"); 
//packer::Time packer::time_read("time_read"); 

#include <pthread.h>
ActiveBuffer packer::active_buffer;

#ifdef _DEBUG_ENABLED
std::map<pthread_t, unsigned int> tid2str;
int indent_cnt = 0; 
#endif

FILE *packer_stream = NULL;

uint64_t packer::table_id = TABLE_ID_OFFSET;
int64_t chunksize_threshold = CHUNKSIZE_THRESHOLD_BASE;

static uint64_t chunk_mask = CHUNK_ALIGNMENT - 1;
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
DataStore::DataStore(char *ptr, int filemode)
{
  MYDBG("\nptr:%p", ptr);

  // Should be thread-safe
  Init(ptr, filemode);

  MYDBG("BeforeWrite ft:%u fh:%p ptralloc:%p magic:%lu\n",
        ftype(), fh_, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
  
  if(fh_ != NULL) {
    if(fh_->GetFileSize() < (int64_t)sizeof(MagicStore)) {
      fh_->Write(0, ptr_ - sizeof(MagicStore), sizeof(MagicStore));
      fh_->FileSync();
    }
  
    written_len_ = fh_->GetFileSize();
  } else {
    written_len_ = CHUNK_ALIGNMENT;
  }
  MYDBG("blksize:%u filesize:%lu (%lu)\n", 
      chunksize_, written_len_, sizeof(MagicStore)); //getchar();
}

void DataStore::Init(char *ptr, int filemode) 
{
  pthread_mutex_lock(&mutex);
  ptr_ = NULL;
  buf_preserved_ = NULL;
  tail_preserved_ = 0;
  grow_unit_ = DATA_GROW_UNIT;
  size_ = grow_unit_;
  head_ = 0;//size_ - size_ / 4;
  tail_ = head_;//sizeof(MagicStore);
  allocated_ = 0;
  mode_ = kBoundedMode | filemode;
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
  if(fh_ != NULL)
    chunksize_ = fh_->GetBlkSize();
  else
    chunksize_ = CHUNK_ALIGNMENT;

  pthread_mutex_unlock(&mutex);
}

void DataStore::ReInit(void)
{
  buf_preserved_ = NULL;
  tail_preserved_ = 0;
  tail_ = 0;
  head_ = 0;
  allocated_ = 0;
  r_tail_ = 0;
  r_head_ = 0;
  BufferConsumer::Get()->InsertBuffer(this);
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
  if(fh_ != NULL)
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
    if(fh_ != NULL)
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
    return *reinterpret_cast<MagicStore *>(ptr_ + ((offset - written_len_) % size_));
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

//uint64_t DataStore::WriteBufferMode(char *pfrom, uint64_t len)
//{
//  // Reallocate buffer or wait until completing write
//  // If WriteBuffer() is still not sufficient to
//  // reserve space for incoming data size (len),
//  // Do reallocate
//  if( len > size_ ) {
//    Reallocate(len);
//  }
//
//  return WriteBuffer(pfrom, len);
//}

//uint64_t DataStore::WriteCacheMode(char *pfrom, uint64_t len)
//{
//  if( len + buf_used() > size_ ) {
//    Reallocate(len);
//  }
//  return WriteMem(pfrom, len);
//}

uint64_t DataStore::Write(char *pfrom, int64_t len, uint64_t pos)
{
  PACKER_ASSERT(len > 0);
  CDErrType ret = kOK;
  if(head_ > pos) { // already data was written to file.
    PACKER_ASSERT(len % CHUNK_ALIGNMENT == 0);
    if(align_check(pfrom)) { 
//      ret = CopyBufferToFile(pfrom, len, pos + written_len_);
      ret = fh_->Write(pos + written_len_, pfrom, len, 0);
    } else { // pfrom is misaligned.
      void *ptr;
      const uint64_t len_up = align_up(len);
      posix_memalign(&ptr, CHUNK_ALIGNMENT, len_up);
      memcpy(ptr, pfrom, len);
//      ret = CopyBufferToFile(ptr, len, pos + written_len_);
      ret = fh_->Write(pos + written_len_, (char *)ptr, len, 0);
      free(ptr);
    }
  } else {
    CopyToBuffer(pfrom, len, pos); 
  }
  if(ret != kOK) assert(0);
  return 0;
}

// Bounded buffer mode: write from the ptr
uint64_t DataStore::Write(char *pfrom, int64_t len)
{
  time_write.Begin();
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
  time_write.End(len);
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
  PACKER_ASSERT_STR( len > 0 && ptr_ != NULL && src != NULL,
      "Write failed: len: %ld ptr_:%p %p\n", len, ptr_, src );
  uint64_t  ret = used();
  CopyToBuffer(src, len);
  tail_ += len;
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

  const uint64_t data_offset = len + pos;
  if(data_offset <= file_offset) { 
    chunk_in_file = len;
  } else {
    chunk_in_file = file_offset - pos;
    chunk_in_memory = len - chunk_in_file;
  }
  if(pos < file_offset) { // some portion is written to file
    const int64_t chunk_written_before_len = tail_ - len;
    PACKER_ASSERT(chunk_written_before_len >= 0);
    FileRead(pto, chunk_in_file, pos);
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

void DataStore::FileRead(char *pto, uint64_t chunk_in_file, uint64_t pos)
{
  PACKER_ASSERT(buf_used() >= 0);
  const uint64_t pos_aligned_down = pos & ~chunk_mask;
  const uint64_t redundant_len    = pos & chunk_mask;
  const uint64_t len_to_read = chunk_in_file + redundant_len;
  const uint64_t len_to_read_aligned = align_up(len_to_read);
  MYDBG("DataStore: %p, %lu, %lu\n", pto, chunk_in_file, pos);
#if 0
  fh_->Read(pto, chunk_in_file, pos);
#else
  void *tmp;
  posix_memalign(&tmp, CHUNK_ALIGNMENT, len_to_read_aligned); 
  memset(tmp, 0, len_to_read_aligned); 
  fh_->Read((char *)tmp, len_to_read_aligned, pos_aligned_down);
  memcpy(pto, (char *)tmp + redundant_len, chunk_in_file);

  if(pos == 66432) {
    printf("@@@@ READ: check this out!!!!##########33\n\n\n");
    for(uint32_t i=0; i<len_to_read/(16*4); i++) {
      for(uint32_t j=0; j<16; j++) {
        printf("%5u ", *((int *)tmp + i*16 + j));
      }
      printf("\n");
    } 
    printf("@@@@ READ: check this out!!!!##########33\n\n\n");
  }
  free(tmp);
#endif

#if 0
  const int64_t chunk_in_file = len - buf_used();
  //fh_->Read(pto, chunk_in_file, written_len_ + head_ - chunk_in_file); 
  const int64_t chunk_written_before_len = tail_ - len;
  PACKER_ASSERT(chunk_written_before_len <= 0);
  PACKER_ASSERT(buf_used() < 0);
  fh_->Read(pto, chunk_in_file, written_len_ + chunk_written_before_len); 
#endif
}

// Figure out all the hassel regarding chunk continuation.
// Assumed pto is contiguous buffer.
//void DataStore::FileRead(char *pto, uint64_t chunk_in_file, uint64_t pos)
//{
//  PACKER_ASSERT(buf_used() >= 0);
//  const uint64_t pos_aligned_down = align_down(pos);
//  const uint64_t pos_aligned_up   = align_up(pos);
////  const uint64_t redundant_len    = pos & chunk_mask;
//  const uint64_t len_to_read      = pos_aligned_up - pos_aligned_down;
//
////  const uint64_t len_to_read_aligned = align_up(len_to_read, CHUNK_ALIGNMENT);
//  fh_->Read((char *)tmp, len_to_read_aligned, pos_aligned_down);
//  MYDBG("DataStore: %p, %lu, %lu\n", pto, chunk_in_file, pos);
//}

// In-place read
void DataStore::ReadAll(char *pto)
{
  MYDBG("[%s] written:%lu head:%lu tail:%lu\n", __func__, written_len_, head_, tail_);
  if(head_ != 0) { // read from file
    fh_->Read(pto, head_, written_len_);
//    fh_->Read(pto, head_ + sizeof(MagicStore), written_len_);
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

// This is used to copy buffer contents to a contiguous data,
// which is the opposite-direction copy operation of WriteMem.
void DataStore::CopyFromBuffer(char *dst, uint64_t len, uint64_t pos)
{
  MYDBG("Copy %p <- %lu + %lu\n", dst, len, pos);
  const uint64_t offset  = (pos == INVALID_NUM)? (head_ % size_) : (pos % size_);
  const uint64_t first = size_ - offset;
  const int64_t  rest  = len - first;
  MYDBG("%lu + %lu > %lu (offset:%lu)\n", first, len, size_, offset);
  if( rest > 0 ) {
    MYDBG("First copy  %p <- %p, %lu\n", dst,         ptr_ + offset, first);
    MYDBG("Second copy %p <- %p, %lu\n", dst + first, ptr_,        rest);
    Copy(dst,         ptr_ + offset, first);
    Copy(dst + first, ptr_,        rest);
  } else {
    MYDBG("Copy %p <- %p, %lu\n", ptr_ + offset, len);
    Copy(dst, ptr_ + offset, len);
  }
}

// This is used to copy buffer contents to a contiguous data,
// which is the opposite-direction copy operation of WriteMem.
void DataStore::CopyToBuffer(char *src, uint64_t len, uint64_t pos)
{
  const uint64_t offset = (pos == INVALID_NUM)? tail_ % size_ : pos % size_;
  MYDBG("Copy %p -> %lu\n", src, len);
//  const uint64_t tail  = buf_pos % size_;
  const uint64_t first = size_ - offset;
  const int64_t  rest  = len - first;
  MYDBG("%lu + %lu > %lu (head:%lu)\n", offset, len, size_, head_ % size_);
  if( rest > 0 ) {
    MYDBG("First copy  %p <- %p, %lu\n", ptr_ + offset, src,         first);
    MYDBG("Second copy %p <- %p, %lu\n", ptr_,          src + first, rest);
    Copy(ptr_ + offset, src,         first);
    Copy(ptr_,          src + first, rest);
  } else {
    MYDBG("Copy %p <- %p, %lu\n", ptr_ + offset, src, len);
    Copy(ptr_ + offset, src, len);
  }
}

// This is used to copy buffer contents to a contiguous data,
// which is the opposite-direction copy operation of WriteMem.
CDErrType DataStore::CopyFileToBuffer(uint64_t buf_pos, int64_t len, uint64_t pos)
{
  CDErrType ret = kOK;
  const uint64_t tail = buf_pos % size_;
  const uint64_t first = size_ - tail;
  const int64_t  rest  = len - first;
  MYDBG("%lu + %lu > %lu (head:%lu)\n", buf_pos, len, size_, r_head_ % size_);

  if( rest > 0 ) {
    MYDBG("First copy  %p <- %p, %lu\n", ptr_ + tail, pos,         first);
    MYDBG("Second copy %p <- %p, %lu\n", ptr_,        pos + first, rest);
    CDErrType iret = fh_->Read(ptr_ + tail, first, pos);
    CDErrType jret = fh_->Read(ptr_,        rest,  pos + first);
    ret = (CDErrType)((uint64_t)iret | (uint64_t)jret);
  } else {
    MYDBG("Copy %p <- %p, %lu\n", ptr_ + tail, pos, len);
    ret = fh_->Read(ptr_ + tail, len, pos);
  }

  return ret;
}

// This is used to copy buffer contents to a contiguous data,
// which is the opposite-direction copy operation of WriteMem.
#if 0
  CDErrType ret = kOK;
  PACKER_ASSERT(len > 0);
  uint64_t mask_len = ~(chunksize_ - 1);
  MYDBG("[DataStore::%s] head:%lu written:%lu, used:%ld, len:%ld, inc:%lu\n", 
      __func__, head_, written_len_, buf_used(), len, mask_len & len); //getchar();
  const int64_t first = size_ - (head_ % size_);
  const uint64_t first_aligndown = mask_len & first;
  const uint64_t file_offset = head_ + written_len_;
  PACKER_ASSERT(first >= 0);
  PACKER_ASSERT(first % CHUNK_ALIGNMENT == 0);
  PACKER_ASSERT(file_offset % CHUNK_ALIGNMENT == 0);
  if(len > first) {
    char *pbegin = begin();
    CDErrType iret = fh_->Write(file_offset, pbegin, first);

    PACKER_ASSERT_STR( (((head_ % size_) + first) % size_) == 0, 
        "((%lu + %lu) %% %lu) == %lu\n", (head_ % size_), first, size_, (((head_ % size_) + first) % size_));

    const int64_t second = len - first;
    const uint64_t second_up   = align_up(second, CHUNK_ALIGNMENT);
    const uint64_t second_down = second & mask_len;
    printf("foffset:%lx, total:%lu, %p actual writesize:%lu %lu, %p second:%lu %lu %lu\n", file_offset, len,
        pbegin, first, first_aligndown, 
        ptr_, second, second_up, second_down);
    ret = fh_->Write(file_offset + first, ptr_, second_up, second_down);

    ret = (CDErrType)((uint32_t)iret | (uint32_t)(ret));
  } else {
    printf("foffset:%lx, total:%lu\n", file_offset, len);
    //ret = fh_->Write(file_offset, begin(), len, mask_len & len);
    ret = fh_->Write(file_offset, begin(), align_up(len, CHUNK_ALIGNMENT), mask_len & len);
  }
#endif
// WriteFile(head_, len, head_ + written_len_);
CDErrType DataStore::CopyBufferToFile(uint64_t buf_pos, int64_t len, uint64_t file_offset)
{
  CDErrType ret = kOK;
  const uint64_t mask_len = ~chunk_mask;
  const uint64_t pos_in_buf = buf_pos % size_;
  const int64_t  first = size_ - pos_in_buf;
//  const uint64_t file_offset = buf_pos + written_len_;
  PACKER_ASSERT(first >= 0);
  PACKER_ASSERT(first % CHUNK_ALIGNMENT == 0);
  PACKER_ASSERT(file_offset % CHUNK_ALIGNMENT == 0);
  char *pbegin = ptr_ + pos_in_buf;
  if(len > first) {
    CDErrType iret = fh_->Write(file_offset, pbegin, first);
    PACKER_ASSERT_STR( ((pos_in_buf + first) % size_) == 0, 
        "((%lu + %lu) %% %lu) == %lu\n", pos_in_buf, first, size_, ((pos_in_buf + first) % size_));

    const int64_t second = len - first;
    const uint64_t second_up   = align_up(second, CHUNK_ALIGNMENT);
    const uint64_t second_down = second & mask_len;
    ret = fh_->Write(file_offset + first, ptr_, second_up, second_down);
    ret = (CDErrType)((uint64_t)iret | (uint64_t)(ret));
  } else {
#ifdef _DEBUG_0402        
    if(packerTaskID == 0) printf("foffset:%lx, total:%lu\n", file_offset, len);
#endif
    //ret = fh_->Write(file_offset, begin(), len, mask_len & len);
    ret = fh_->Write(file_offset, pbegin, align_up(len, CHUNK_ALIGNMENT), mask_len & len);
  }
  return ret;
#if 0
  const uint64_t r_tail = r_tail_ % size_;
  const uint64_t first  = size_ - r_tail;
  const int64_t  rest   = len - first;
  MYDBG("%lu + %lu > %lu (head:%lu)\n", r_tail_, len, size_, r_head_ % size_);
  if( rest > 0 ) {
    MYDBG("First copy  %p <- %p, %lu\n", ptr_ + tail, pos,         first);
    MYDBG("Second copy %p <- %p, %lu\n", ptr_,        pos + first, rest);
    fh_->Read(ptr_ + r_tail, first, pos);
    fh_->Read(ptr_,          rest,  pos + first);
  } else {
    MYDBG("Copy %p <- %p, %lu\n", ptr_ + tail, pos, len);
    fh_->Read(ptr_ + r_tail, len, pos);
  }
#endif
}

void DataStore::InitReadMode(void) 
{
  if(CHECK_FLAG(mode_, kReadMode) == false) {
    CD_SET(mode_, kReadMode); 
    if(buf_used() > 0) {
      PadZeros(0);
      Flush();
      CD_SET(mode_, kReadMode); 
      PACKER_ASSERT(buf_preserved_ == NULL);
      // Preserve the remaining buffer state
      PACKER_ASSERT_STR(buf_used() >= 0, "head:%lu,tail:%lu\n", head_, tail_);
      tail_preserved_ = buf_used();
      void *palloc = NULL;
      posix_memalign(&palloc, CHUNK_ALIGNMENT, align_up(buf_used()));
      buf_preserved_ = (char *)palloc;
      CopyFromBuffer(buf_preserved_, tail_preserved_);
      tail_ = head_;
//      printf("\n>>>>>> [%s]:%ld, tail:%lu, head:%lu\n", __func__, buf_used(), tail_, head_); //getchar();
      // r_tail_ and r_head_ only can increase by CHUNK_ALIGNMENT size
      r_tail_ = head_ + written_len_;
      r_head_ = head_ + written_len_;
    }
  }
}

void DataStore::FinReadMode(void) 
{
  if( tail_preserved_ > 0 && (head_ + written_len_ <= r_tail_ - head_) //&& last_rtail <= head_ + pos + CHUNK_ALIGNMENT)
//  if(r_tail_ >= head_ + used() && (r_tail_ <= head_ + used() + CHUNK_ALIGNMENT)
    && CHECK_FLAG(mode_, kReadMode)) {
//  if(r_tail_ == (head_<<1) && CHECK_FLAG(mode_, kReadMode)) {
//    printf("\n<<<<<<< [%s] final!! %lu <= %ld, %d\n----------------", __func__,
//   head_ + written_len_, r_tail_ - head_, CHECK_FLAG(mode_, kReadMode));
    //getchar();
//  if(r_tail_ - (head_ + written_len_) == head_ + written_len_) {// - written_len_) {
    // Restore the remaining buffer state
    CopyToBuffer(buf_preserved_, tail_preserved_);
    tail_ = head_ + tail_preserved_;
    free(buf_preserved_);
    buf_preserved_ = NULL;
    tail_preserved_ = 0;
    CD_UNSET(mode_, kReadMode); 
//    printf("Checkthis:%d at %p\n", CHECK_FLAG(mode_, kReadMode), this);
  }
}
///////////////////////////////////////////////////////////////////////////////
// Assummed that dst is a contiguous memory space at least larger than len.
void DataStore::GetData(char *dst, int64_t len, uint64_t pos, bool keep_reading)
{
  time_read.Begin(len);
  PACKER_ASSERT(len >= 0);
  MYDBG("%p <- %ld, %lu\n", dst, len, pos);
  InitReadMode();
  // FIXME: const int64_t  offset_diff = head_ - written_len_;
  const int64_t  offset_diff = head_;
  const uint64_t until = pos + len;
  uint64_t fetch_pos = pos;
  int64_t tail_inc = 0; 
  int64_t head_inc = 0;
//  int64_t orig_len = len; 
//  uint64_t orig_rtail = r_tail_;
  while(len > 0) {
    int64_t consumed = FetchInternal(tail_inc, head_inc, len, fetch_pos, false);
    int64_t readsize = (len < consumed)? len : consumed;
//    printf("readsize, len, consumed:%ld %ld %ld, pos:%lu\n", readsize, len, consumed, pos);
    CopyFromBuffer(dst, readsize, pos + offset_diff);
    len  -= readsize;
    r_head_ += head_inc;
    r_tail_ += tail_inc;
//    printf("consumed:%lu,readsize:%lu,fetchpos:%lu,len:%lu\n", consumed, readsize, fetch_pos,len);
    fetch_pos += readsize;
  }

  PACKER_ASSERT_STR(fetch_pos == until, "[Error %s]:%lu != %lu\n", __func__, fetch_pos, until);
  PACKER_ASSERT(len == 0);
  //PACKER_ASSERT(r_tail_ - head_ <= head_ - written_len_);
  //PACKER_ASSERT_STR((r_tail_ >= head_ && r_tail_ < head_ + CHUNK_ALIGNMENT),  
//  uint64_t last_rtail = r_tail_ - tail_inc;
#if 0
  PACKER_ASSERT_STR(orig_rtail == r_tail_ 
      || r_tail_ - orig_rtail >= align_up(pos + orig_len) - align_down(pos) - CHUNK_ALIGNMENT,
      "%lu (%lu - %lu) >= %lu\n",
      r_tail_ - orig_rtail, r_tail_, orig_rtail, align_up(pos + orig_len) - align_down(pos) - CHUNK_ALIGNMENT);
#endif
//  PACKER_ASSERT_STR( head_ + pos <= last_rtail && last_rtail <= head_ + pos + CHUNK_ALIGNMENT,
//      "%lu <= %lu <= %lu (rtail:%lu, head:%lu)\n", 
//      head_ + pos, last_rtail, head_ + pos + CHUNK_ALIGNMENT, r_tail_, head_);
//      r_tail_, pos + len + head_, pos, len, head_);
//  if(keep_reading == false) {
    FinReadMode();

  time_read.End();
//  }
//  while(fetch_pos <= until) {
//    uint64_t pos_in_buf = Fetch(len, fetch_pos);
//
//    if(rbuf_used() >= len) {
//      CopyFromBuffer(dst, len, pos_in_buf);
//    } else { 
//      // In the case that buffer is not sufficient,
//      // Fetch() multiple times until completely reading from file.
//      uint64_t next_read_size = rbuf_used();
//      CopyFromBuffer(dst, rbuf_used(), pos_in_buf);
//    }
//    r_head_   += align_down(pos_in_buf + len) - r_head_;
//    fetch_pos += rbuf_used();
//  }


}

uint64_t DataStore::FakeWrite(uint64_t len)
{
  head_preserved_ = head_; 
  uint64_t len_to_write = len & ~chunk_mask;
  if(len_to_write > 0) {
    head_ += len_to_write;
    fh_->SetOffset(len_to_write);//offset_ += len_to_write; 
  }
  return head_preserved_ + written_len_;
}
/***********************************************************
 * Buffer window and data to cache.
 *
 *  Case 0     Case 1     Case 2     Case 3     Case 4     Case 5
 *  _|___|_     |   |      |   |      |   |      |   |      |   |   
 * | |   | |    |   |      |   |      |   |      |   |      |   |  
 * | |   | |   _|___|_    _|___|_     |   |      |   |      |   |  
 * |_|___|_|  | |   | |  | |   | |    |   |      |   |      |   |  
 *   |___|    | |___| |  | |___| |    |___|      |___|      |___|
 *   | A |    | | A | |  |_|_A_|_|   _|_A_|_     | A |      | A |
 *   | B |    | | B | |    | B |    | | B | |    | B |      | B |
 *   | C |    | | C | |    | C |    | | C | |   _|_C_|_     | C |
 *   |_D_|    | |_D_| |    |_D_|    |_|_D_|_|  | |_D_| |    |_D_|
 *   |   |    |_|___|_|    |   |      |   |    | |   | |   _|___|_ 
 *   |   |      |   |      |   |      |   |    |_|___|_|  | |   | |
 *   |   |      |   |      |   |      |   |      |   |    | |   | |
 *   |   |      |   |      |   |      |   |      |   |    |_|___|_|
 *   |   |      |   |      |   |      |   |      |   |      |   |  
 *   |   |      |   |      |   |      |   |      |   |      |   |  
 *   |___|      |___|      |___|      |___|      |___|      |___|  
 *   |   |      |   |      |   |      |   |      |   |      |   |  
 *
 * Important this is to determine r_tail_next.
 * Depending on r_tail_next, r_head_next may move or not.
 * Returns alined up lenth to read
 */
uint64_t DataStore::CalcOffset(uint64_t &len_to_read, uint64_t &buf_offset, 
                               uint64_t &r_tail_next, uint64_t &r_head_next,
                               uint64_t buf_pos_low, uint64_t buf_pos_high, bool locality)
{
  uint64_t total_len = buf_pos_high - buf_pos_low;
  bool overflow = total_len > size_;
  uint64_t max_len_to_read = (overflow)? size_ : total_len;
  uint64_t len_in_buff = 0; 
  uint64_t len_in_file = total_len;
  len_to_read = 0;
  r_tail_next = r_tail_;
  r_head_next = r_head_;
  buf_offset      = 0; // if offset is 0, do not perform file write
  if(buf_pos_high > r_tail_) {   
    if(buf_pos_low < r_head_) { // case 1 moving backward first
                                 // or realloc will be good
      MYDBG("[Case 1] ");
      assert(0); // implement later
      //int64_t buf_inc = buf_pos_high - r_tail_;
//      int64_t buf_inc = buf_pos_low - r_tail_;
//      r_tail_ += buf_inc;
//      r_head_ += buf_inc;


    } 
    else if(buf_pos_low < r_tail_) { // case 2 moving forward (common)
      // partially read data from file for upper part
      len_in_buff = r_tail_ - buf_pos_low;
      len_in_file = max_len_to_read - len_in_buff;
//      uint64_t avail_len_to_read = size_ - len_in_buff; // size_ - r_tail_ + buf_pos_low
      r_tail_next = (overflow)? size_ + buf_pos_low : buf_pos_high;
      len_to_read = r_tail_next - r_tail_;
      buf_offset = r_tail_;
      MYDBG("[Case 2] ");
    } 
    else { // case 0 moving forward
      if(locality) {
        // read whole data from file.
        // Read as much as possible
        r_tail_next = (overflow)? buf_pos_low + size_ : buf_pos_high;
        buf_offset  = (overflow)? buf_pos_low : buf_pos_high - size_;
        if(overflow) {
          r_tail_next = buf_pos_low + size_;
          buf_offset  = buf_pos_low;
          len_to_read = size_;
          r_head_next = buf_pos_low;
        } else { // buffer is sufficient
          r_tail_next = buf_pos_high;
          // we know buf_pos_high - buf_pos_low <= size_
          if(buf_pos_high - r_tail_ > size_) { 
            // does not fit if it reads from r_tail.
            // In this case, it is not need to read the space between r_tail_ and
            // buf_pos_low. just read from buf_pos_low expecting forward read next
            // time.
            buf_offset  = buf_pos_low;
            r_head_next = buf_pos_low;
            len_to_read = max_len_to_read;
          }
          else { // still fit
            // read from r_tail_
            buf_offset  = r_tail_;
            r_head_next = (r_tail_next - r_head_ > size_)? r_tail_next - size_ : r_head_;
            len_to_read = r_tail_next - buf_offset;
          }
//          buf_offset      = (buf_pos_high - r_tail_ > size_)? buf_pos_high - size_ : r_tail_;
//          len_to_read = size_ : r_tail_next - r_tail_; 
//          r_head_next = r_tail_; 
//          r_tail_ = r_tail_next;
//          r_head_ = r_head_next;
        }
        MYDBG("[Case 0 w/opt] ");
      } else { // just read required size
        len_in_file = max_len_to_read;
        len_to_read = max_len_to_read;
        r_tail_next = (overflow)? buf_pos_low + size_ : buf_pos_high;
        r_head_next = buf_pos_low;
        buf_offset  = buf_pos_low;
        MYDBG("[Case 0 noopt] ");
      }
    }

  } // case 0, 1, 2 ends
  else if(buf_pos_high > r_head_) { // buffer window moving backward

    if(buf_pos_low < r_head_) { // case 4. moving backward
      len_in_buff = buf_pos_high - r_head_;
      len_in_file = max_len_to_read - len_in_buff;
      //uint64_t avail_len_to_read = size_ - len_in_buff; // size_ - r_tail_ + buf_pos_low
      //r_head_next = (overflow)? r_head_ - (avail_len_to_read) : buf_pos_low;
      r_head_next = (overflow)? buf_pos_high - size_ : buf_pos_low;
      len_to_read = r_head_ - r_head_next;
      buf_offset = r_head_;
      MYDBG("[Case 4] ");
    } 
    else { // case 3 (fully cached) do not move.
      // Just read from buffer
      len_in_buff = total_len;

      MYDBG("[Case 3] ");//%lu %lu %lu", r_head_next, r_tail_next, len_to_read);
    }

  } 
  else { // case 5 moving backward
    if(locality) {
      assert(0);
    } else { // just read required size
      len_in_file = max_len_to_read;
      len_to_read = len_in_file;
      r_head_next = (overflow)? buf_pos_high - size_ : buf_pos_low;
      r_tail_next = buf_pos_high;
      buf_offset  = buf_pos_low;
      MYDBG("[Case 5] ");
    }

  }
//  printf("len_in_file:%lu, len_to_read:%lu, r_head_next:%lu, r_tail_next:%lu, buf_offset:%lu\n",
//        len_in_file, len_to_read, r_head_next, r_tail_next, buf_offset);

//  uint64_t offset_in_buf = buf_pos_low; // return
      MYDBG("size:%lu,total_len:%lu,len_in_file:%lu,len_to_read:%lu,"
              "r_head_next:%lu,r_tail_next:%lu,head:%lu,wl:%lu,"
              "buf_offset:%lu,pos_low:%lu,pos_high:%lu\n",
              size_, total_len, len_in_file, len_to_read, 
              r_head_next, r_tail_next, head_, written_len_, 
              buf_offset, buf_pos_low, buf_pos_high);
//      getchar();

  return max_len_to_read;
}

///////////////////////////////////////////////////////////////////////////////
// Semantic of Fetch
///////////////////////////////////////////////////////////////////////////////
// It fetches from file to buffer space (cache) for len in byte, 
// and retruns the offset in buffer to let users to read from buffer.
//
// If the data that users try to read is already in buffer (cache), 
// it just returns the offset.
//
// If the data is partially cached in buffer, the remaining data in file are
// appended from file to buffer space, then return offset to be read from
// buffer.
//
// It is designed to keep invoked until reading from written_len_ to head_.
// It is recommended to use if after flushing the buffer first.
// The same buffer space is reused for reading from file for current DataStore,
// assuming that the DataStore is operating in either kWriteMode or kReadMode.
//
// uint64_t consumed = Fetch(tail_inc, head_inc, len, pos);
// CopyFromBuffer(dst, consumed, pos + offset_diff);
// len  -= consumed;
//
// head += head_inc;
// tail += tail_inc;
uint64_t DataStore::Fetch(int64_t len, uint64_t pos, bool locality)
{
  time_read.Begin(len);
  int64_t tail_inc = 0;
  int64_t head_inc = 0;
  int64_t consumed = FetchInternal(tail_inc, head_inc, len, pos, locality);
  r_tail_ += tail_inc;
  r_head_ += head_inc;
  time_read.End();
  return consumed;
}

uint64_t DataStore::Fetch(int64_t &tail_inc, int64_t &head_inc, int64_t len, uint64_t pos, bool locality)
{
  time_read.Begin(len);
  uint64_t consumed = FetchInternal(tail_inc, head_inc, len, pos, locality);
  time_read.End();
  return consumed;
}

uint64_t DataStore::FetchInternal(int64_t &tail_inc, int64_t &head_inc, int64_t len, uint64_t pos, bool locality)
{
  MYDBG("len:%lu, pos:%lu, rhead:%lu,rtail:%lu\n", len, pos, r_head_, r_tail_);
  InitReadMode();
  PACKER_ASSERT(CHECK_FLAG(mode_, kReadMode));
  PACKER_ASSERT(len > 0);
  const uint64_t pos_align_down = align_down(pos);
  const uint64_t pos_align_up   = align_up(pos + len);
  const int64_t  offset_diff    = head_;// - written_len_; 
  const uint64_t buf_pos_low   = pos_align_down + offset_diff; // must be aligned.
  const uint64_t buf_pos_high  = pos_align_up   + offset_diff; // must be aligned.
//  const int64_t  uncached_fpos  = r_tail_ - offset_diff; // must be aligned.
//  const uint64_t redundant_len  = pos & chunk_mask;
//  const uint64_t total_len      = pos_align_up - pos_align_down;
  PACKER_ASSERT_STR(pos_align_up - pos_align_down > 0, "%lu %lu\n", pos, pos+len);
  // Outputs
  uint64_t len_to_read = 0;
  uint64_t buf_offset  = 0;
  uint64_t r_tail_next = 0;
  uint64_t r_head_next = 0;
  // aligned up - aligned down
  uint64_t aligned_len_to_read = CalcOffset(len_to_read, buf_offset, r_tail_next, r_head_next,
                                     buf_pos_low, buf_pos_high, false);
#ifdef _DEBUG_0402        
  if(packerTaskID == 0) {
    printf("len_to_read:%lx, buf_offset:%lx, r_tail_next:%lx, r_head_next:%lx, buf_pos_low:%lx, buf_pos_high:%lx\n",
          len_to_read, buf_offset, r_tail_next, r_head_next, buf_pos_low, buf_pos_high);
  }
#endif
  // actual len_to_read may less than aligned_len_to_read because
  // some portion may be cached to buffer.
  if(len_to_read != 0) { 
    CopyFileToBuffer(buf_offset, len_to_read, buf_offset - offset_diff);
  }
  PACKER_ASSERT_STR(len_to_read <= aligned_len_to_read, 
      "len_to_read:%lu, consumed:%lu, rhead:%lu,rtail:%lu\n",
       len_to_read, aligned_len_to_read, r_head_, r_tail_);
  PACKER_ASSERT(buf_offset  % CHUNK_ALIGNMENT == 0);
  PACKER_ASSERT(offset_diff % CHUNK_ALIGNMENT == 0);
  tail_inc = r_tail_next - r_tail_;
  head_inc = r_head_next - r_head_;

  FinReadMode();
  return aligned_len_to_read;
}

#if 0
/////////////// OLD fetch /////////////////////////////////////
//  ////////////////////////////////////////////////////////////////
//    if(len_to_read <= 0) { // every data is in buffer
//      uint64_t overwritten_len = r_head_ - buff_pos; 
//      if(overwritten_len > 0) { 
//        // the lower part was overwritten.
//  
//        if(len <= buf_used()) {
//          r_head_ -= overwritten_len;
//          CopyFileToBuffer(ptr_ + (r_head_ % size_), overwritten_len, overwritten_len);
//        } else { // read the overwritten lower part next time.
//          offset_in_buf = r_head_;
//          direction = REVERSE
//          // this should copy data to dst like 
//          // copy(dst + read_size, ptr_ + offset_in_buf, read_size);
//          // Then, in the next time, 
//          // copy(dst, ptr_ + offset_in_buf, read_size);
//        }
//  
//      } else { // every data is in buffer AND no overwritten part (fully cached to buffer)
//  
//        offset_in_buf = pos + offset_diff;
//  
//      }
//    }
//    else { // some data is in file
//  //    const uint64_t r_tail = r_tail_ % size_;
//  //    const uint64_t first  = size_ - r_tail;
//  //    const int64_t  rest   = len_to_read - first;
//  //    MYDBG("%lu + %lu > %lu (head:%lu)\n", r_tail_, len_to_read, size_, r_head_ % size_);
//  //    if( rest > 0 ) {
//  //      MYDBG("First copy  %p <- %p, %lu\n", ptr_ + tail, uncached_fpos,         first);
//  //      MYDBG("Second copy %p <- %p, %lu\n", ptr_,        uncached_fpos + first, rest);
//  //      fh_->Read(ptr_ + r_tail, first, uncached_fpos);
//  //      fh_->Read(ptr_,          rest,  uncached_fpos + first);
//  //    } else {
//  //      MYDBG("Copy %p <- %p, %lu\n", ptr_ + tail, uncached_fpos, len_to_read);
//  //      fh_->Read(ptr_ + r_tail, len_to_read, uncached_fpos);
//  //    }
//      CopyFileToBuffer(len_to_read, uncached_fpos);
//      r_tail_ += len_to_read;
//    }

//  const uint64_t offset_in_buf = pos + offset_diff;
//  PACKER_ASSERT(align_down(offset_in_buf - offset_diff) > pos); // this means need to read lower part overwritten
//  const uint64_t len_to_read    = pos_align_up - uncached_fpos;
//  const uint64_t offset_in_buf   = pos + offset_diff;
//  const int64_t file_len_to_read = offset_in_buf + len - r_tail_;
//  int64_t buffer_len_to_read     = len;
//
//  if(file_len_to_read > 0) { // need to read data from file
//    buffer_len_to_read -= file_len_to_read;
//    MYDBG("DataStore: %p, %lu, %lu\n", 
//        ptr_ + (r_tail_ % size_), file_len_to_read, r_tail_ - offset_diff); // r_tail_ - head_ + written_len_
//    fh_->Read(ptr_ + (r_tail_ % size_), file_len_to_read, r_tail_ - offset_diff);
//    PACKER_ASSERT(r_tail_ - offset_diff == pos + buffer_len_to_read); 
//    // r_tail_ - offset_diff - pos - len + offset_diff + pos + len - r_tail_ == 0
//
//  }
//  ret = ptr_ + ((pos + offset_diff) % size_); 
/////////////////////////////////////////////////////////////////
//  const int64_t first = size_ - (head_ % size_);
//  const uint64_t first_aligndown = mask_len & first;
//  const uint64_t file_offset = head_ + written_len_;
//  PACKER_ASSERT(first >= 0);
//  PACKER_ASSERT(first % CHUNK_ALIGNMENT == 0);
//  PACKER_ASSERT(file_offset % CHUNK_ALIGNMENT == 0);
//  if(len > first) {
//    char *pbegin = begin();
//    CDErrType iret = fh_->Write(file_offset, pbegin, first);
//    PACKER_ASSERT_STR( (((head_ % size_) + first) % size_) == 0, 
//        "((%lu + %lu) %% %lu) == %lu\n", (head_ % size_), first, size_, (((head_ % size_) + first) % size_));
//
//    const int64_t second = len - first;
//    const uint64_t second_up   = align_up(second, CHUNK_ALIGNMENT);
//    const uint64_t second_down = second & mask_len;
//    ret = fh_->Write(file_offset + first, ptr_, second_up, second_down);
//
//MYDBG("\n");
//uint64_t ret = INVALID_NUM;
//if( len > 0 && ptr_ != NULL && src != NULL ) {  
//  const uint64_t tail  = tail_ % size_;
//  const uint64_t first = size_ - tail;
//  const int64_t  rest  = len - first;
//  ret = used();
//    if(used() >= 0x10880 && used() <= 0x10ac0) {
//      printf("(src:%d at %lx, dst:%p), %lu + %lu > %lu (head:%lx, wl:%lu)\n", *(int *)src, used(), ptr_ + tail,
//          tail, len, size_, head_ + written_len_, written_len_);
//      getchar();
//    }
//  MYDBG("%lu + %lu > %lu (head:%lu)\n", tail, len, size_, head_ % size_);
//  if( rest > 0 ) {
//    MYDBG("First copy  %p <- %p, %lu\n", ptr_ + (size_ - first), src, first);
//    MYDBG("Second copy %p <- %p, %lu\n", ptr_, src + first, rest);
//    Copy(ptr_ + tail, src, first);
//    Copy(ptr_, src + first, rest);
//    tail_ += len;
//    PACKER_ASSERT((tail_ % size_) == (uint64_t)rest);
//  } else {
//    MYDBG("Copy %p <- %p, %lu\n", ptr_ + tail, src, len);
//    Copy(ptr_ + tail, src, len);
//    tail_ += len;
//  }
//} else {
//  ERROR_MESSAGE_PACKER("Write failed: len: %ld ptr_:%p %p\n", len, ptr_, src);
//}
//return ret;
#endif

CDErrType DataStore::WriteFile(void) 
{
  return WriteFile(chunksize_);
}

CDErrType DataStore::WriteFile(int64_t len)
{

  PACKER_ASSERT(len > 0);
  CDErrType ret = CopyBufferToFile(head_, len, head_ + written_len_);
  head_ += ~chunk_mask & len; 
#if 0
  uint64_t mask_len = ~(chunksize_ - 1);
  MYDBG("[DataStore::%s] head:%lu written:%lu, used:%ld, len:%ld, inc:%lu\n", 
      __func__, head_, written_len_, buf_used(), len, mask_len & len); //getchar();
  const int64_t first = size_ - (head_ % size_);
  const uint64_t first_aligndown = mask_len & first;
  const uint64_t file_offset = head_ + written_len_;
  PACKER_ASSERT(first >= 0);
  PACKER_ASSERT(first % CHUNK_ALIGNMENT == 0);
  PACKER_ASSERT(file_offset % CHUNK_ALIGNMENT == 0);
  if(len > first) {
    char *pbegin = begin();
    CDErrType iret = fh_->Write(file_offset, pbegin, first);

    PACKER_ASSERT_STR( (((head_ % size_) + first) % size_) == 0, 
        "((%lu + %lu) %% %lu) == %lu\n", (head_ % size_), first, size_, (((head_ % size_) + first) % size_));

    const int64_t second = len - first;
    const uint64_t second_up   = align_up(second, CHUNK_ALIGNMENT);
    const uint64_t second_down = second & mask_len;
    printf("foffset:%lx, total:%lu, %p actual writesize:%lu %lu, %p second:%lu %lu %lu\n", file_offset, len,
        pbegin, first, first_aligndown, 
        ptr_, second, second_up, second_down);
//    if(file_offset == 0x10800) {
//      for(uint32_t i=0; i<first/(16*4); i++) {
//        for(uint32_t j=0; j<16; j++) {
//          printf("%4u ", *((int *)pbegin + i*16 + j));
//        }
//        printf("\n");
//      }
//     exit(-1); 
//    }
    ret = fh_->Write(file_offset + first, ptr_, second_up, second_down);
//    if(file_offset == 0x10800) {
//      for(uint32_t i=0; i<second_up/(16*4); i++) {
//        for(uint32_t j=0; j<16; j++) {
//          printf("%4u ", *((int *)ptr_ + i*16 + j));
//        }
//        printf("\n");
//      } 
//    }

    ret = (CDErrType)((uint32_t)iret | (uint32_t)(ret));
  } else {
    printf("foffset:%lx, total:%lu\n", file_offset, len);
    //ret = fh_->Write(file_offset, begin(), len, mask_len & len);
    ret = fh_->Write(file_offset, begin(), align_up(len, CHUNK_ALIGNMENT), mask_len & len);
  }
  head_ += ~chunk_mask & len; 
#endif
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
//    printf("\n\n##### [%s] %ld %zu ###\n", __func__, buf_used(), sizeof(MagicStore));
    ret = WriteFile(buf_used());
    FileSync();
  }
  return ret;
}

uint64_t DataStore::WriteFlushMode(char *src, uint64_t len)
{
#if 0
  if( (uint64_t)(len + buf_used()) > size_ ) {
    MYDBG("##### [%s] buf:%ld len:%zu #####\n", __func__, buf_used(), len);
    CDErrType err = WriteFile(buf_used());
    if(err != kOK) {
      MYDBG("Error while WriteFile(%lu)\n", len);
    }
  }
  const uint64_t table_offset = WriteMem(src, len);
  char *ret = ptr_ + ((table_offset - written_len_) % size_);
  ret -= 4;
  assert((table_offset % size_) + len < size_);
  printf("check this out!!!!##########33\n\n\n");
        for(uint32_t i=0; i<len/(16*4); i++) {
          for(uint32_t j=0; j<16; j++) {
            printf("%4u ", *((int *)ret + i*16 + j));
          }
          printf("\n");
        } 

  printf("check this out!!!!##########33\n\n\n");
        for(uint32_t i=0; i<len/(16*4); i++) {
          for(uint32_t j=0; j<16; j++) {
            printf("%4u ", *((int *)src + i*16 + j));
          }
          printf("\n");
        } 

  printf("check this out!!!!##########33\n\n\n");
        getchar();
  return table_offset;
#else
  static uint64_t tot_write_len = 0;
  if(len <= 0) {
    return used();
  }
  else {
  
    time_write.Begin();
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

    time_write.End(len);
    return offset;
  }
#endif
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
#if 1
  uint64_t table_offset = WriteFlushMode(src, len);
#else
  if( (uint64_t)(len + buf_used()) > size_ ) {
    MYDBG("##### [%s] buf:%ld len:%zu #####\n", __func__, buf_used(), len);
    CDErrType err = WriteFile(buf_used());
    if(err != kOK) {
      MYDBG("Error while WriteFile(%lu)\n", len);
    }
  }
  const uint64_t table_offset = WriteMem(src, len);
  char *ret = ptr_ + (table_offset % size_);
  assert((table_offset % size_) + len < size_);
  printf("check this out!!!!##########33\n\n\n");
        for(int i=0; i<len/(16*4); i++) {
          for(int j=0; j<16; j++) {
            printf("%4lu ", *((uint64_t *)ret + i*16 + j));
          }
          printf("\n");
        } 

  printf("check this out!!!!##########33\n\n\n");
        getchar();
  
#endif
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


void DataStore::SetFileOffset(uint64_t offset) { fh_->SetOffset(offset); }


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

