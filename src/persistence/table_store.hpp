#ifndef _TABLE_STORE_H
#define _TABLE_STORE_H

#include "base_store.h"
#include <type_traits>
#define TABLE_ID_OFFSET 0xFFFFFFFF00000000
namespace packer {
class BaseTable {
  protected:
    uint64_t size_;
    uint64_t tail_;
    uint64_t head_;
    uint64_t advance_point_;
    uint64_t grow_unit_;
    uint32_t allocated_;
  public:
    BaseTable(void) : advance_point_(0), grow_unit_(32), allocated_(0) {}
    virtual ~BaseTable(void) {}
    void SetGrowUnit(uint32_t grow_unit) { grow_unit_ = grow_unit; }
    void Print(void)
    {
      MYDBG("%lu/%lu, grow:%lu, alloc:%u\n", tail_, size_, grow_unit_, allocated_);
    }

    inline bool NeedRealloc(size_t entrysize, uint64_t cnt=1)
    {
      bool ret = false;
//      MYDBG("[%s] entrysize:%zu, used:%lu, size:%lu\n", __func__, entrysize, tail_, size_);
      while( (tail_ + cnt) * entrysize > size_ ) {
        grow_unit_ <<= 1;
        size_ = grow_unit_ * entrysize;
        ret = true;
      }
      return ret;
    }

  public:
    virtual void Init(void)=0;
//    virtual void *Find(uint64_t id)=0;
//    virtual uint64_t FindWithAttr(uint16_t attr, uint64_t begin, BaseTable *that=NULL)=0;
    virtual CDErrType Find(uint64_t id, uint64_t &ret_size, uint64_t &ret_offset)=0;
    virtual CDErrType GetAt(uint64_t idx, uint64_t &ret_id, uint64_t &ret_size, uint64_t &ret_offset)=0;
    virtual CDErrType Reallocate(void)=0;
    virtual CDErrType Free(bool reuse)=0;
    virtual void Read (char *pto, uint64_t size, uint64_t pos)=0;
    virtual uint64_t GetChunkToFetch(uint64_t fetchsize, uint64_t &idx)=0; 
    virtual int64_t  used(void)     const = 0;
    virtual int64_t  buf_used(void) const = 0;
    virtual uint64_t size(void)     const = 0;
    virtual int64_t  usedsize(void) const = 0;
    virtual int64_t  tablesize(void) const = 0;
    virtual char    *GetPtr(void)   const = 0;
    uint64_t tail(void) const { return tail_; }
};

//extern uint64_t BaseTable::table_id; = TABLE_ID_OFFSET;
//
// Packer(new TableStore<EntryT>, new DataStore)
template <typename EntryT>
class TableStore : public BaseTable {
//    static uint64_t data_used;
  protected:
    EntryT *ptr_;
    EntryT prv_entry_;
  public:
    TableStore<EntryT>(uint64_t alloc=BASE_ENTRY_CNT) : ptr_(0), prv_entry_(table_id++, 0UL, INVALID_NUM) { 
      MYDBG("\n");
//      if(alloc) AllocateTable();//sizeof(EntryT));
      AllocateTable(alloc);//sizeof(EntryT));
    }
    
    TableStore<EntryT>(EntryT *ptr_table, uint32_t len_in_byte) : prv_entry_(table_id++, 0UL, INVALID_NUM) {
      MYDBG("\n");
      if(ptr_table != NULL) {
        ptr_ = ptr_table;
        PACKER_ASSERT(len_in_byte % sizeof(EntryT) == 0);
        size_ = len_in_byte;
//        tail_ = len_in_byte / sizeof(EntryT);
        head_ = 0;
        tail_ = head_;//0;//len_in_byte / sizeof(EntryT);
        advance_point_ = head_;
        if(size_ > 0) {
          grow_unit_ = size_ * 2;
        }
      } else {
        AllocateTable(BASE_ENTRY_CNT);
      }
    }
    void Reset(uint64_t tail) { tail_ = tail; advance_point_ = tail; }
    virtual void Init(void) {
        prv_entry_.id_++;
        prv_entry_.offset_ = INVALID_NUM;
        head_ = 0;
        tail_ = head_;
        advance_point_ = head_;
        if(size_ > 0) {
          grow_unit_ = size_ * 2;
        }
    }
    virtual ~TableStore<EntryT>(void) {
      MYDBG("\n");
      Free(false); 
    }

    EntryT &operator[](uint64_t idx) { return ptr_[idx % size_]; }

    EntryT *At(uint64_t idx) { return (ptr_ + (idx % size_)); }

    CDErrType AllocateTable(uint64_t entry_cnt=BASE_ENTRY_CNT)
    {
      MYDBG("\n");
      CDErrType err = kOK;
      head_ = 0;
      tail_ = head_;
      tail_ = advance_point_;
      if(entry_cnt != 0) {
        grow_unit_ = entry_cnt;
        MYDBG("[%s] grow:%lu ptr:%p, used:%lu | ", __func__, 
              grow_unit_, ptr_, tail_*sizeof(EntryT));
        if(ptr_ == NULL) {
          ptr_ = new EntryT[grow_unit_];
          if(ptr_ != NULL) {
            size_ = grow_unit_ * sizeof(EntryT);
            allocated_++;
          } else {
            err = kMallocFailed;
          }
        } else {
          err = kAlreadyAllocated;
        }
      } else {
        err = kMallocFailed;
      }
      MYDBG("[%s] ptr:%p, size:%lu, used:%lu, allocate:%u\n", __func__, ptr_, size_, tail_*sizeof(EntryT), allocated_);
      //getchar();
      return err;
    }

    virtual CDErrType Find(uint64_t id, uint64_t &ret_size, uint64_t &ret_offset) 
    {
      CDErrType ret = kNotFound;
      for(uint32_t i=0; i<tail_; i++) {
        // The rule for entry is that the first element in object layout is always ID.
        if( ptr_[i].id_ == id ) {
          MYDBG("%lu == %lu\n", ptr_[i].id_, id);
          ret_size = ptr_[i].size();
          ret_offset = ptr_[i].offset_;
          ret = kOK;
          break;
        }
      }
      return ret;
    }
//    virtual uint64_t Insert(EntryT &newentry)
//    {
////      MYDBG("[%s] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+tail_, ptr_, tail_, sizeof(EntryT));
//      if(NeedRealloc(sizeof(EntryT))) {
//        Reallocate();
//      }
//      ptr_[tail_] = newentry;
//      tail_++;
////      MYDBG("[%s done] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+tail_, ptr_, tail_, sizeof(EntryT));
//      return tail_*sizeof(EntryT);
//    }
//    virtual uint64_t FindWithAttr(uint16_t attr, BaseTable *that=NULL)
//    {
//      uint64_t num_entry = 0;
//      TableStore<EntryT> *table = NULL;
//      if(that == NULL) {
//        table = this;
//      } else {
//        table = static_cast<TableStore<EntryT> *>(that);
//      }
//
//      uint64_t orig_tail = tail_;
//      for(uint32_t i=0; i<tail_; i++) {
//        if(ptr_[i].size_.CheckAny(attr)) {
//          table->Insert(ptr_[i]);
//        }
//      }
//      num_entry = tail_ - orig_tail;
//      return num_entry;
//    }
    
    // Find an entry with kNeedPushed from the offset, 
    // and copy the entry at the dst offset to the offset.
    // return the dst offset for the next search.
    virtual uint64_t FindWithAttr(uint16_t attr, uint64_t begin, TableStore<EntryT> *table=NULL)
    {
      if(table == NULL) { // Insert entries with the attr to this table from begin offset
//        uint64_t orig_tail = tail_;
//        uint64_t orig_begin = begin;
        for(uint32_t i=begin; i<tail_; i++) {
          if(ptr_[i].size_.CheckAny(attr)) {
            ptr_[begin++] = ptr_[i];
          }
        }
        tail_ = begin;
        return tail_;
//        num_entry = begin - orig_begin;
      } else { // Search entries with the attr from the begin offset of this table
               // Insert those entries with the attr to target table. 
//        uint64_t orig_tail = table->tail_;
        for(uint32_t i=begin; i<tail_; i++) {
          if(ptr_[i].size_.CheckAny(attr)) {
            table->Insert(ptr_[i]);
          }
        }
        return table->tail_;
//        num_entry = table->tail_ - orig_tail;
      }
    }

    void UnsetAttr(uint16_t attr, uint64_t start=INVALID_NUM, uint64_t finish=0)
    {

      PACKER_ASSERT(head_ == 0);
      EntryT *ret = NULL; 
      // INVALID_NUM is always false
      const int64_t begin = (start   > tail_)? tail_-1 : start;
      const int64_t end   = (finish <= head_)? head_   : end;
      const int64_t tail  = tail_;
      assert(end == 0); // for now
      
      if(begin >= 0) {
        for(int64_t i=begin; i>=end; i--) {
          if(ptr_[i].size_.CheckAny(attr)) {
            ret = &(ptr_[i]);
            ptr_[i].size_.Unset(attr);
          }
        }
    
        // check
        if(ret == NULL) {
          if(packerTaskID == 0) {
            MYDBG("[UnsetAttr] Find %x, tail:%lu\n", attr, tail_); //getchar();
            for(int64_t i=0; i<tail; i++) {
              // The rule for entry is that the first element in object layout is always ID.
              MYDBG("TEST %lu == %lu\n", ptr_[i].id_, id);
//              if( ptr_[i].id_ == id ) {
//                MYDBG("TEST %lu == %lu\n", ptr_[i].id_, id);
//              }
            }
          } 
          PACKER_ASSERT(0);
        }
      }
      return ret;
    }

    EntryT *FindReverse(uint64_t &id, uint16_t attr, uint64_t start=INVALID_NUM, uint64_t finish=0)
    {
      PACKER_ASSERT(head_ == 0);
      EntryT *ret = NULL; 
      // INVALID_NUM is always false
      const int64_t begin = (start   > tail_)? tail_-1 : start;
      const int64_t end   = (finish <= head_)? head_   : end;
      const int64_t tail  = tail_;
      assert(end == 0); // for now
      
//      if(packerTaskID == 0 && attr != 0) {printf("findreverse(%x): id:%lx > ", attr, id); }
      if(begin >= 0) {
        for(int64_t i=begin; i>=end; i--) {
          // The rule for entry is that the first element in object layout is always ID.
          if( ptr_[i].id_ == id ) {
            MYDBG("%lu == %lu\n", ptr_[i].id_, id);
            //if(ptr_[i].size_.CheckAny(attr) && attr != Attr::koutput) 
            //CDEntry tentry = ptr_[i];
//            if(packerTaskID == 0) {printf("%lx,", tentry.attr());}
            if(ptr_[i].attr() == attr && attr != Attr::knoattr) {
              continue;
            } else {
              ret = &(ptr_[i]);
              id = i;
              if(ptr_[i].size_.Check(attr) && attr != Attr::knoattr) {
//                char tmp[32];
//                sprintf(tmp, "ERROR %x", attr);
//                ptr_[i].Print(tmp); 
                ret = NULL;
              }
              break;
            }
          }
        }
    
        // check
        if(ret == NULL) {
          if(packerTaskID == 0) {
            MYDBG("[FindReverse] Find %lu, tail:%lu\n", id, tail_); //getchar();
            for(int64_t i=0; i<tail; i++) {
    //        for(int64_t i=begin; i>=0; i--) {
              // The rule for entry is that the first element in object layout is always ID.
              MYDBG("TEST %lu == %lu\n", ptr_[i].id_, id);
              if( ptr_[i].id_ == id ) {
                MYDBG("TEST %lu == %lu\n", ptr_[i].id_, id);
              }
            }
          } 
  //        else {
  //          MYDBG("my task id:%d\n", packerTaskID);
  //        }
          PACKER_ASSERT(0);
        }
      }
//      if(packerTaskID == 0) {printf(" ret:%p\n", ret); }
      return ret;
    }

    EntryT *Find(uint64_t id)
    {
      PACKER_ASSERT(head_ == 0);
      EntryT *ret = NULL;
      for(uint32_t i=0; i<tail_; i++) {
        // The rule for entry is that the first element in object layout is always ID.
        if( ptr_[i].id_ == id ) {
          MYDBG("%lu == %lu\n", ptr_[i].id_, id);
          ret = &(ptr_[i]);
          break;
        }
      }

      // check
      if(ret == NULL) {
        if(packerTaskID == 0) {
          MYDBG("[Find] tail:%lu\n", tail_); //PACKER_ASSERT(0);
          for(uint32_t i=0; i<tail_; i++) {
            // The rule for entry is that the first element in object layout is always ID.
            if( ptr_[i].id_ == id ) {
              MYDBG("TEST %lu == %lu\n", ptr_[i].id_, id);
            }
          }
        } 
//        else {
//          MYDBG("my task id:%d\n", packerTaskID);
//        }
        PACKER_ASSERT(0);
      }
      //return (void *)ret;
      return ret;
    }

    // Find with offset. It sounds wierd, but it is used for matching malloc/free.
    // malloc creates an entry with [TYPE|COUNTER] as id, and has a pointer in [OFFSET]
    // free call should search for the pointer in the table
    EntryT *FindWithOffset(uint64_t offset, uint32_t &table_offset)
    {
      EntryT *ret = NULL;
      uint32_t found_idx = -1U;
      for(uint32_t i=table_offset; i<tail_; i++) {
        MYDBG("Find %lu at %u, (target: %lu)\n", offset, i, ptr_[i].offset());
        // The rule for entry is that the first element in object layout is always ID.
        if( ptr_[i].offset() == offset ) {
          MYDBG("%lu == %lu\n", ptr_[i].offset(), offset);
          ret = &(ptr_[i]);
          found_idx = i;
          break;
        }
      }
      // checking
      if(ret == NULL) {
        if(packerTaskID == 0) {
          MYDBG("[FindWithOffset] tail:%lu\n", tail_); PACKER_ASSERT(0);
          for(uint32_t i=table_offset; i<tail_; i++) {
            // The rule for entry is that the first element in object layout is always ID.
            if( ptr_[i].offset() == offset ) {
              MYDBG("TEST %lu == %lu\n", ptr_[i].offset(), offset);
            }
          }
        } 
//        else {
//          MYDBG("my task id:%d\n", packerTaskID);
//        }
        //PACKER_ASSERT(0);
      }
      table_offset = found_idx;
      return ret;
    }

    virtual CDErrType GetAt(uint64_t idx, uint64_t &ret_id, uint64_t &ret_size, uint64_t &ret_offset) 
    {
      MYDBG("[%s] idx:%lu\n", __func__, idx);
      // bound check
      CDErrType ret = kOK;
      if((int64_t)idx < used()) {
        uint64_t i = (head_ + idx) % size_;
        ret_id = ptr_[i].id_;
        ret_size = ptr_[i].size();
        ret_offset = ptr_[i].offset_;
      } else {
        ret = kOutOfTable;
      }
      MYDBG("[%s] idx:%lu id:%lu size:%lu offset:%lu\n", __func__, idx, ret_id, ret_size, ret_offset);
      return ret;
    }

//    EntryT *GetAt(uint64_t idx)
//    {
//      // bound check
//      if((int64_t)idx < used()) {
//        uint64_t i = (head_ + idx) % size_;
//        return ptr_ + i;
//      } else {
//        return NULL;
//      }
//    }
    
    // idx should be an index in buffer space
    // It is used in IsPushedLog
    EntryT *GetAt(uint64_t idx)
    {
      PACKER_ASSERT(idx == 0 || idx < tail_);
      PACKER_ASSERT(idx >= head_);
      PACKER_ASSERT(head_ == 0);
      const uint64_t i = (head_ + idx) % size_;
      return ptr_ + i;
    }

    EntryT *GetLast(void)
    {
      return ptr_ + ((tail_ - 1) % size_);
    }

    virtual uint64_t Advance(uint64_t offset)
    {
      Insert(prv_entry_);
      const uint64_t ret = tail_;
      const uint64_t table_size = tail_ - advance_point_;
      advance_point_ = tail_;
      prv_entry_.id_      = table_id++;
      prv_entry_.offset_  = offset;
      prv_entry_.size_.attr_.size_  = table_size; 
      prv_entry_.size_.attr_.table_ = 1;
      return ret; 
    }

    virtual void Advance(void)
    {
      advance_point_ = tail_;
    }

//    static inline 
//    uint64_t IterateChunks(uint64_t fetchsize, uint64_t &idx)
//    {
//      uint64_t datasize = 0;
//      uint64_t orig_idx = idx;
//      uint64_t i = (head_ + idx) % size_;
//      while(idx < used()) {
//        const uint64_t chunksize = ptr_[i].size_;
//        if(chunksize + datasize < fetchsize) { 
//          datasize += chunksize;
//          idx++;
//        } else {
//          break;
//        }
//      }
//      return orig_idx;
//    }

    virtual uint64_t GetChunkToFetch(uint64_t fetchsize, uint64_t &idx) 
    {
      uint64_t datasize = 0;
      uint64_t ret = idx;
      while((int64_t)idx < used()) {
        const uint64_t chunksize = ptr_[idx].size();
        if(chunksize + datasize < fetchsize) { 
          datasize += chunksize;
          idx++;
        } else {
          break;
        }
      }
 //     uint64_t ret = IterateChunks(fetchsize, idx);
      // if ret == idx, fetchsize (buffer size) is insufficient
      ret = (ret != idx)? datasize : BUFFER_INSUFFICIENT;
      return ret; 
    }

//    uint64_t DeleteChunks(uint64_t idx)
//    {
//      head_ = 0;
//      while(idx < used()) {
//        const uint64_t chunksize = ptr_[idx].size_;
//        if(chunksize + datasize < fetchsize) { 
//          datasize += chunksize;
//          idx++;
//        } else {
//          break;
//        }
//      }
//
//      uint64_t ret = IterateChunks(MAX_UINT64_NUM, idx);
//      if(ret == 0) {}
//    }

    void Copy(const TableStore<EntryT> &that) 
    { 
      ptr_ = that.ptr_;
      size_ = that.size_;
      tail_ = that.tail_;
      grow_unit_ = that.grow_unit_;
      allocated_ = that.allocated_;
    }

    virtual int64_t  used(void)     const { return (int64_t)tail_ - (int64_t)head_; }
    virtual int64_t  buf_used(void) const { return ((int64_t)tail_ - (int64_t)head_) * sizeof(EntryT); }
    virtual uint64_t size(void)     const { return size_; }
    virtual uint64_t tail(void)     const { return tail_ % size_; }
    virtual int64_t  usedsize(void) const { return ((int64_t)tail_ - (int64_t)head_) * sizeof(EntryT); }
    virtual int64_t  tablesize(void) const {
      if(((int64_t)tail_ - (int64_t)advance_point_) < 0) { 
        printf("%lu < %lu\n", tail_, advance_point_); assert(0); 
      }
      return ((int64_t)tail_ - (int64_t)advance_point_) * sizeof(EntryT); }
    virtual char    *GetPtr(void)   const { return (char *)ptr_; }
    virtual char    *GetCurrPtr(void)   const { return (char *)(ptr_ + (advance_point_ % size_)); }
    virtual char    *GetTailPtr(void)   const { return (char *)(ptr_ + (tail_ % size_)); }

//    uint64_t used(void) { return tail_ * sizeof(EntryT); }
//    virtual uint64_t usedsize(void) { return tail_ * sizeof(EntryT); }
//    uint64_t used(void) { return tail_; }
    virtual uint32_t type(void) const { 
      if(std::is_same<EntryT, BaseEntry>::value) 
        return kBaseEntry;
      else if(std::is_same<EntryT, CDEntry>::value) 
        return kCDEntry;
      else 
        return 0;
    }

    size_t EntrySize() const { return sizeof(EntryT); }

    virtual void Print(void)
    {
      MYDBG("[Table] %lu/%lu, grow:%lu, alloc:%u\n", tail_*sizeof(EntryT), size(), grow_unit_, allocated_);
    }

    void PrintEntry(const char *str="", uint64_t print_upto=0)
    {
      if(print_upto == 0) print_upto = tail_;
      for(uint64_t i=0; i<print_upto; i++) {
        ptr_[i].Print(str);
      }
    }
  public:
    virtual uint64_t Insert(EntryT *newentry, uint64_t cnt)
    {
      uint64_t entrysize = sizeof(EntryT) * cnt;
      if(NeedRealloc(sizeof(EntryT), cnt)) {
        Reallocate();
      }

      // FIXME:If it is bounded buffer, this should be fixed like data store
      memcpy(ptr_ + (tail_ % size_), newentry, entrysize);
      tail_ += cnt;
      return tail_*sizeof(EntryT);
    }

    virtual uint64_t Insert(EntryT *newentry)
    {
      if(NeedRealloc(sizeof(EntryT))) {
        Reallocate();
      }
      ptr_[tail_] = *newentry;
      tail_++;
      //MYDBG("[%s] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+tail_, ptr_, tail_, sizeof(EntryT));
      delete newentry;
      return tail_*sizeof(EntryT);
    }

    virtual uint64_t Insert(EntryT &newentry)
    {
//      MYDBG("[%s] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+tail_, ptr_, tail_, sizeof(EntryT));
      if(NeedRealloc(sizeof(EntryT))) {
        Reallocate();
      }
      ptr_[tail_] = newentry;
      tail_++;
//      MYDBG("[%s done] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+tail_, ptr_, tail_, sizeof(EntryT));
      return tail_*sizeof(EntryT);
    }

    virtual uint64_t Insert(EntryT &&newentry)
    {
//      MYDBG("[%s] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+tail_, ptr_, tail_, sizeof(EntryT));
      if(NeedRealloc(sizeof(EntryT))) {
        Reallocate();
      }
      ptr_[tail_] = std::move(newentry);
      tail_++;//= sizeof(EntryT);
//      MYDBG("[%s done] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+tail_, ptr_, tail_, sizeof(EntryT));
      return tail_*sizeof(EntryT);
    }
    
    EntryT *InsertEntry(EntryT &&newentry)
    {
      if(NeedRealloc(sizeof(EntryT))) {
        Reallocate();
      }
      EntryT *ret = (ptr_ + (tail_ % size_));
      ptr_[tail_] = std::move(newentry);
      tail_++;
      return ret;
    }

    virtual CDErrType Reallocate(void)
    {
      MYDBG("[%s Table 1] newsize:%lu, # elem:%lu\n", __func__, size_, grow_unit_);
      CDErrType err = kOK;
      if(ptr_ != NULL) {
        EntryT *newptr = new EntryT[grow_unit_];
        MYDBG("[%s Table 2] Alloc large [%p - %p] data_size:%lu\n", __func__, 
               newptr, newptr+size_, size_);
        if(newptr != NULL) {
          memcpy(newptr, ptr_, tail_*sizeof(EntryT));
          delete [] ptr_;
          ptr_ = newptr;
          allocated_++;
        } else {
          err = kMallocFailed;
        }
        MYDBG("memcpy?? %lu\n", tail_);
      } else {
        AllocateTable();
      }
      return err;
    }

    virtual CDErrType Free(bool reuse)
    {
      MYDBG("reuse:%d\n", reuse);
      CDErrType err = kOK;
      tail_ = 0;
      advance_point_ = 0;
      allocated_ = 0;
      if(reuse == false) {
        size_ = 0;
        grow_unit_ = BASE_ENTRY_CNT;//initial_grow_unit;
        if(ptr_ != NULL) {
//          MYDBG("%p\n", ptr_);
          delete [] ptr_;
          ptr_ = NULL;
        } else {
          err = kAlreadyFree;
        }
      }
      return err;
    }

    virtual void Read(char *pto, uint64_t size, uint64_t pos)
    {
      uint64_t i = (head_ + pos) % size_;
      MYDBG("[%s] %p <- %p (%lu)\n", __func__, pto, ptr_ + i, size);
      if( ptr_ != NULL && pto != NULL ) {  
        memcpy(pto, ptr_ + i, size);
      } else {
        ERROR_MESSAGE_PACKER("Read failed: to:%p <- from:%p (%p)\n", pto, ptr_+pos, ptr_);
      }
    }

//    // Find an entry with kNeedPushed from the offset, 
//    // and copy the entry at the dst offset to the offset.
//    // return the dst offset for the next search.
//    virtual uint64_t FindWithAttr(uint16_t attr, uint64_t begin, TableStore<EntryT> *table=NULL)
//    {
//      uint64_t num_entry = 0;
//      if(table == NULL) { // Insert entries with the attr to this table from begin offset
////        uint64_t orig_tail = tail_;
//        uint64_t orig_begin = begin;
//        for(uint32_t i=begin; i<tail_; i++) {
//          if(ptr_[i].size_.CheckAny(attr)) {
//            ptr_[begin++] = ptr_[i];
//          }
//        }
//        tail_ = begin;
//        num_entry = begin - orig_begin;
//      } else { // Search entries with the attr from the begin offset of this table
//               // Insert those entries with the attr to target table. 
//        uint64_t orig_tail = table->tail();
//        for(uint32_t i=begin; i<tail_; i++) {
//          if(ptr_[i].size_.CheckAny(attr)) {
//            table->Insert(ptr_[i]);
//          }
//        }
//        num_entry = table->tail() - orig_tail;
//      }
//
//      return num_entry;
//    }
};
    

//BaseTable *GetTable(uint32_t entry_type, char *ptr_entry, uint32_t len_in_byte) 
//{
//  MYDBG("[%s] %u %p %u\n", __func__, entry_type, ptr_entry, len_in_byte);
//  BaseTable *ret = NULL;
//  switch(entry_type) { 
//    case kBaseEntry: {
//      ret = new TableStore<BaseEntry>(reinterpret_cast<BaseEntry *>(ptr_entry), len_in_byte);
//      break;
//    }
//    case kCDEntry: {
//      ret = new TableStore<CDEntry>(reinterpret_cast<CDEntry *>(ptr_entry), len_in_byte);
//      break;
//    }
//    default:
//                   {}
//  }
//  MYDBG("[%s] %u %p %u ret:%p\n", __func__, entry_type, ptr_entry, len_in_byte, ret);
//  return ret;
//}

} // namespace packer ends
#endif
