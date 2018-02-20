#ifndef _DATA_STORE_H
#define _DATA_STORE_H

#include "base_store.h"
//#define CHUNK_ALIGNMENT 512
// Eliminate file write operation here.
// It just writes to buffer space.
// 
namespace packer {
#define FILE_TYPE_MASK 0x00000F00UL
#define BUFF_MODE_MASK 0x000000FFUL
#define GET_BUFF_MODE(X)       ((X) & BUFF_MODE_MASK)
#define GET_FILE_TYPE(X)       ((X) & FILE_TYPE_MASK)

#define CHECK_FLAG(X,Y)    (((X) & (Y)) == (Y))
#define SET_FILE_TYPE(X,Y) (((X) &= ~(FILE_TYPE_MASK)) |= (Y))
#define SET_BUFF_MODE(X,Y) (((X) &= ~(BUFF_MODE_MASK)) |= (Y))
#define CD_UNSET(STATE,Y) ((STATE) &= ~(Y))
#define CD_SET(STATE,Y)   ((STATE) |=  (Y))
//#define CHUNKSIZE_THRESHOLD_BASE 0x1000 // 4KB
//#define CHUNKSIZE_THRESHOLD_BASE 0x40000000 // 16KB
//#define DATA_GROW_UNIT           0x40000000
//#define CHUNKSIZE_THRESHOLD_BASE 0x200 // 16KB

//enum {
//  kGrowingMode=0x00,
//  kPersistent=0x01,
//  kBoundedMode=0x02,
//  kBufferFreed=0x04,
//  kBufferEmpty=0x08
//};
class CDPacker;
class FileHandle;
class DataStore;
const uint32_t inv_state = (kBufferFreed);

struct ActiveBuffer {
  DataStore *id_;
  uint64_t state_;
};

extern ActiveBuffer active_buffer;

class DataStore {
  private: // 8*6 = 48 Byte
    uint64_t size_;
    uint64_t head_; // length written to file
    uint64_t tail_;
    uint64_t grow_unit_;
    uint32_t allocated_;
    uint64_t written_len_;
    uint32_t mode_; // state
    uint32_t chunksize_;
    char    *ptr_;
    char    *buf_preserved_;
    uint64_t tail_preserved_; // used for kReadMode
    uint64_t head_preserved_; // used for FakeWrite
    FileHandle *fh_;
  private:
    uint64_t r_tail_;
    uint64_t r_head_;
  public:
//    DataStore(bool alloc);
    DataStore(void);
    DataStore(char *ptr, uint64_t init_size, int filemode);
    //DataStore(char *ptr=NULL, uint64_t init_size=GROW_UNIT, int filemode=DEFAULT_FILEMODE);
    virtual ~DataStore(void);
    void InitFile(uint32_t filetype=kVolatile);
    void Init(char *ptr, uint64_t init_size, int filemode);
    void ReInit(void);
    CDErrType AllocateData();
    CDErrType FreeData(bool reuse);
    CDErrType Reallocate(uint64_t len);
//    CDErrType SafeReallocate(uint64_t len);
    // Write
    uint64_t Write(char *pfrom, int64_t len, uint64_t pos);
    uint64_t Write(char *pfrom, int64_t len);
    ///@brief Write data to bounded in-memory buffer. 
    ///       If buffer is full, wait until buffer is available.
    inline uint64_t WriteBufferMode(char *pfrom, uint64_t len)
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
    ///@brief Write data to unbounded in-memory buffer.
    ///       If buffer is full, realloc buffer for larger space.
    inline uint64_t WriteCacheMode(char *pfrom, uint64_t len) 
    {
      if( len + buf_used() > size_ ) {
        Reallocate(len);
      }
      return WriteMem(pfrom, len);
    }
    ///@brief Write data to bounded in-memory buffer.
    ///       If buffer is full, flush current data in buffer,
    ///       then write new data in buffer.
    ///       If new data is larger then buffer size,
    ///       iterate this step until completely writing all data.
    uint64_t WriteFlushMode(char *pfrom, uint64_t len);
    CDErrType WriteFile(int64_t len);
    CDErrType WriteFile(void);
    uint64_t FakeWrite(uint64_t len);

    // Read
    void Read(char *pto, uint64_t size, uint64_t pos);
    void ReadAll(char *pto);
    char *ReadAll(uint64_t &totsize);
    void FileRead(char *pto, uint64_t chunk_in_file, uint64_t pos);
    inline uint64_t Fetch(int64_t &tail_inc, int64_t &head_inc, int64_t len, uint64_t pos, bool locality=false);
    uint64_t FetchInternal(int64_t &tail_inc, int64_t &head_inc, int64_t len, uint64_t pos, bool locality=false);
    uint64_t Fetch(int64_t len, uint64_t pos, bool locality=false);
    void GetData(char *dst, int64_t len, uint64_t pos, bool keep_reading=true);

    // Flush buffer
    void FileSync(void);
    CDErrType Flush(void);
    uint64_t Flush(char *src, int64_t len);

  private:
    // Internel
    inline void InitReadMode(void);
    inline void FinReadMode(void);
    inline uint64_t CalcOffset(uint64_t &len_to_read, uint64_t &buf_offset, 
                               uint64_t &r_tail_next, uint64_t &r_head_next,
                               uint64_t buf_pos_low, uint64_t buf_pos_high, 
                               bool locality=false);
    void CopyFromBuffer(char *dst, uint64_t len, uint64_t pos=INVALID_NUM);
    void CopyToBuffer(char *src, uint64_t len, uint64_t pos=INVALID_NUM);
    CDErrType CopyFileToBuffer(uint64_t buf_pos, int64_t len, uint64_t pos);
    CDErrType CopyBufferToFile(uint64_t buf_pos, int64_t len, uint64_t file_pos);
    uint64_t WriteBuffer(char *src, int64_t len);
    uint64_t WriteMem(char *src, int64_t len);
  public:
    void copy(const DataStore &that) { 
      ptr_ = that.ptr_;
      size_ = that.size_;
      head_ = that.head_;
      tail_ = that.tail_;
      grow_unit_ = that.grow_unit_;
      allocated_ = that.allocated_;
    }
    MagicStore &GetMagicStore(uint64_t offset=INVALID_NUM);
    DataStore &operator=(const DataStore &that) {
      copy(that);
      return *this;
    }
    char *GetPtr(const uint64_t idx) const { return (ptr_ + (idx % size_)); }
//    uint64_t used(void) { return tail_ - head_; }
    ///@brief Total bytes written to file.
    inline uint64_t used(void)      const { return tail_ + written_len_; }
    ///@brief Offset of file that current data store began.
    ///       Total bytes written to file before beginning current data store.
    inline uint64_t offset(void)    const { return written_len_; }
    ///@brief Total bytes written to file by current data store.
    inline uint64_t tail(void)      const { return tail_; }
    ///@brief Total bytes pushed to the current data store.
    inline uint64_t head(void)      const { return head_; }
    inline uint64_t next_head(void)      const { return head_ + CHUNK_ALIGNMENT; }
    ///@brief Beginning pointer in the current data store.
    inline char    *begin(void)     const { return ptr_ + (head_ % size_); }
    ///@brief The size of data in memory buffer.
    inline int64_t  buf_used(void)  const { return (int64_t)(tail_ - head_); }
    ///@brief Total size of buffer (buffer limit).
    inline uint64_t size(void)      const { return size_; }
    ///@brief Buffer state.
    inline uint32_t mode(void)      const { return GET_BUFF_MODE(mode_); }
    ///@brief File type that buffer writes data to.
    inline uint32_t ftype(void)     const { return GET_FILE_TYPE(mode_); }
    ///@brief The size of chunk to write at a time.
    inline uint32_t chunksize(void) const { return chunksize_; }
    ///@brief Pointer that data store allocated.
    inline char *GetPtrAlloc(void)  const { return ptr_ - sizeof(MagicStore); }
    ///@brief Pointer that will be effectively used.
    inline char *GetPtr(void)       const { return ptr_; }
    inline void IncHead(uint64_t len)     { head_ += len; }
    inline uint64_t PadZeros(uint64_t len) {
//                                             uint64_t orig_tail = tail_; 
                                             uint64_t offset = align_up(tail_) + written_len_; 
                                             tail_ = align_up(tail_ + len);  
//                                             uint64_t padded_size = tail_ - orig_tail;
//                                             if(padded_size + orig_tail % size_ > size_) {
//                                               uint64_t first = size_ - orig_tail % size_;
//                                               memset(ptr_ + orig_tail % size_, 0, first);
//                                               memset(ptr_, 0, padded_size - first);
//
//                                             } else {
//                                               memset(ptr_ + orig_tail % size_, 0, padded_size);
//                                             }
//                                             PACKER_ASSERT_STR(padded_size <= 1024, "padd error:%lu %lx/%lx\n", padded_size, offset, size_);
                                             return offset; 
                                           }
    inline void SetGrowUnit(uint32_t grow_unit) { grow_unit_ = grow_unit; }
    inline uint32_t SetMode(uint32_t mode) { uint32_t orig_mode = mode_; mode_ = mode; return orig_mode; }
    inline uint32_t SetFileType(uint32_t mode) { uint32_t orig_type = GET_FILE_TYPE(mode_); 
                                                 SET_FILE_TYPE(mode_, mode);  return orig_type; }
    void SetFileOffset(uint64_t offset);

    inline void Print(void) const
    { MYDBG("%lu/%lu, grow:%lu, alloc:%u\n", buf_used(), size_, grow_unit_, allocated_); }
    void UpdateMagic(const MagicStore &magic);
    void UpdateMagic(MagicStore &&magic);
    CDErrType FlushMagic(const MagicStore *magic);
  public: // interface
    virtual CDErrType Free(char *ptr);
    virtual CDErrType Copy(void *dst, char *src, int64_t len);
    virtual CDErrType Alloc(void **ptr, uint64_t size);
  public:
    void PadAndInit(int64_t next_head) {
      head_ = next_head;
      tail_ = next_head; 
      written_len_ = next_head;
    }
    inline bool IsEmpty(void)  { PACKER_ASSERT(buf_used() >= 0); return (buf_used() < chunksize_); }
    inline bool IsFull(void) { 
      bool is_full = (buf_used() > (int64_t)(size_ - chunksize_)); 
      if(packerTaskID == 0) {
        printf("%s head:%lu tail:%lu\n", (is_full)? "Full":"NotFull", head_, tail_); 
      }
      PACKER_ASSERT(buf_used() >= 0);
      PACKER_ASSERT(size_ != 0);
      return is_full; 
    }
    inline void SetActiveBuffer(bool high_priority=false);
    inline void WriteInternal(char *src, uint32_t len_to_write, int64_t &i); 
    inline void WriteInternal2(char *src, uint32_t len_to_write, int64_t &i); 
};


// Wake up condition
// 1. Eager Consumption mode && there is any non-empty buffer
// 2. Signal mode && there is valid buffer ID
//inline 
//DataStore *IsInvalidBuffer(void) 
//{
//  bool ret = false;
//  if(active_buffer.id_ != NULL) {
//    return active_buffer.id_;
//  } else {
//    return GetBuffer();
//  }
//}
//  ( (active_buffer.id_ == NULL) || 
//           ((active_buffer.state_ & inv_state) != 0) ); 

#if 0
class FileHandle {
  public:
    static FileHandle *fh_;
    int fdesc_;
    char filepath_[64];
    std::vector<DataStore *> buf_list_;
//    static bool opened;
//    static bool active;
  public:
    FileHandle(DataStore *buffer=NULL) {}
    ~FileHandle(void);
    
    FileHandle *GetFileHandle(void);
    void SetFilepath(char *filepath);
    void Delete(void);

    void InsertBuffer(DataStore *ds);
    void RemoveBuffer(DataStore *ds);
    DataStore *GetBuffer(void); 

    static void *Write(void *parm); 
    void WriteAll(DataStore *buffer);
};

class BufferList : public std::vector<DataStore *> {
    static BufferList *buf_list_;
  public:
    BufferList(void) : buf_list_(NULL) {}
    ~BufferList(void) {}
    BufferList *GetBufferList(void) {
      if(buf_list_ == NULL) {
        buf_list_ = new BufferList;
      }
      return buf_list_;
    }
    DataStore *GetBuffer(void) {
      DataStore *ret = NULL;
      for(auto it=buf_list_.begin(); it!=buf_list_.end(); ++it) {
        if(it->IsEmpty() == false) { 
          ret = *it;
        }
      }
      return ret;
    }

class FileDataStore : public DataStore {
    int fdesc_;
  public:
    uint64_t BeginWrite();
    uint64_t CompleteWrite();
  public:
    FileDataStore(char *filepath) : DataStore(false) {}
    virtual ~FileDataStore(void) {}
    virtual CDErrType Reallocate(uint64_t len);
    virtual CDErrType Free(bool reuse);
    virtual uint64_t Write(char *pfrom, uint64_t len, uint64_t pos);
    virtual uint64_t Write(char *pfrom, uint64_t len);
    virtual void Read (char *pto, uint64_t size, uint64_t pos);
    virtual void ReadAll(char *pto);
//    virtual void ReadAll (char *pto, uint64_t size, uint64_t pos);
};
#endif

} // namespace packer ends
#endif
