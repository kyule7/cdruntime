#ifndef _TABLE_STORE_H
#define _TABLE_STORE_H

#include "base_store.h"
#include <type_traits>
namespace cd {
class BaseTable {
  protected:
    uint64_t size_;
    uint64_t used_;
    uint64_t grow_unit_;
    uint32_t allocated_;
  public:
    BaseTable(void) : allocated_(0) {}
    virtual ~BaseTable(void) {}
    void SetGrowUnit(uint32_t grow_unit) { grow_unit_ = grow_unit; }
    void Print(void)
    {
      MYDBG("%lu/%lu, grow:%lu, alloc:%u\n", used_, size_, grow_unit_, allocated_);
    }

    inline uint64_t NeedRealloc(size_t entrysize)
    {
//      MYDBG("[%s] entrysize:%zu, used:%lu, size:%lu\n", __func__, entrysize, used_, size_);
      if( (used_ + 1) * entrysize > size_ ) {
        grow_unit_ <<= 1;
        size_ = grow_unit_ * entrysize;
        return true;
      } else {
        return false;
      }
    }

  public:
    virtual CDErrType Find(uint64_t id, uint64_t &ret_size, uint64_t &ret_offset)=0;
    virtual CDErrType GetAt(uint64_t idx, uint64_t &ret_id, uint64_t &ret_size, uint64_t &ret_offset)=0;
    virtual CDErrType Reallocate(void)=0;
    virtual CDErrType Free(bool reuse)=0;
    virtual void Read (char *pto, uint64_t size, uint64_t pos)=0;
    virtual uint64_t GetChunkToFetch(uint64_t fetchsize, uint64_t &idx)=0; 
    virtual uint64_t usedsize(void)=0;
    virtual uint32_t type(void)=0;
    virtual char *GetPtr(void)=0;
};

//
// Packer(new TableStore<EntryT>, new DataStore)
template <typename EntryT>
class TableStore : public BaseTable {
//    static uint64_t data_used;
  protected:
    EntryT *ptr_;
  public:
    TableStore<EntryT>(uint64_t alloc=BASE_ENTRY_CNT) : ptr_(0) { 
      MYDBG("\n");
//      if(alloc) AllocateTable();//sizeof(EntryT));
      AllocateTable(alloc);//sizeof(EntryT));
    }
    
    TableStore<EntryT>(EntryT *ptr_table, uint32_t len_in_byte) {
      MYDBG("\n");
      if(ptr_table != NULL) {
        ptr_ = ptr_table;
        ASSERT(len_in_byte % sizeof(EntryT) == 0);
        size_ = len_in_byte;
        used_ = len_in_byte / sizeof(EntryT);
        grow_unit_ = used_ * 2;
        printf("Created!!!!!!\n"); //getchar();
      } else {
        AllocateTable(BASE_ENTRY_CNT);
      }
    }

    virtual ~TableStore<EntryT>(void) {
      MYDBG("\n");
      Free(false); 
    }

    EntryT &operator[](uint64_t idx) { return ptr_[idx]; }

    CDErrType AllocateTable(uint64_t entry_cnt=BASE_ENTRY_CNT)
    {
      MYDBG("\n");
      CDErrType err = kOK;
      used_ = 0;
      if(entry_cnt != 0) {
        grow_unit_ = entry_cnt;
        MYDBG("[%s] grow:%lu ptr:%p, used:%lu | ", __func__, 
            grow_unit_, ptr_, used_*sizeof(EntryT));
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
      MYDBG("[%s] ptr:%p, size:%lu, used:%lu, allocate:%u\n", __func__, ptr_, size_, used_*sizeof(EntryT), allocated_);
      //getchar();
      return err;
    }

    virtual CDErrType Find(uint64_t id, uint64_t &ret_size, uint64_t &ret_offset) 
    {
      CDErrType ret = kNotFound;
      for(uint32_t i=0; i<used_; i++) {
        // The rule for entry is that the first element in object layout is always ID.
        if( ptr_[i].id_ == id ) {
          MYDBG("%lu == %lu\n", ptr_[i].id_, id);
          ret_size = ptr_[i].size_;
          ret_size = ptr_[i].offset_;
          ret = kOK;
          break;
        }
      }
      return ret;
    }

    EntryT *Find(uint64_t id)
    {
      EntryT *ret = NULL;
      for(uint32_t i=0; i<used_; i++) {
        // The rule for entry is that the first element in object layout is always ID.
        if( ptr_[i].id_ == id ) {
          MYDBG("%lu == %lu\n", ptr_[i].id_, id);
          ret = &(ptr_[i]);
          break;
        }
      }
      return ret;
    }

    virtual CDErrType GetAt(uint64_t idx, uint64_t &ret_id, uint64_t &ret_size, uint64_t &ret_offset) 
    {
      MYDBG("[%s] idx:%lu\n", __func__, idx);
      // bound check
      CDErrType ret = kOK;
      if(idx < used_) {
        ret_id = ptr_[idx].id_;
        ret_size = ptr_[idx].size_;
        ret_offset = ptr_[idx].offset_;
      } else {
        ret = kOutOfTable;
      }
      MYDBG("[%s] idx:%lu id:%lu size:%lu offset:%lu\n", __func__, idx, ret_id, ret_size, ret_offset);
      return ret;
    }

    EntryT *GetAt(uint64_t idx)
    {
      // bound check
      if(idx < used_) 
        return ptr_ + idx;
      else 
        return NULL;
    }

    virtual uint64_t GetChunkToFetch(uint64_t fetchsize, uint64_t &idx) 
    {
      uint64_t datasize = 0;
      uint64_t ret = idx;
      while(idx < used_) {
        const uint64_t chunksize = ptr_[idx].size_;
        if(chunksize + datasize < fetchsize) { 
          datasize += chunksize;
          idx++;
        } else {
          break;
        }
      }
      // if ret == idx, fetchsize (buffer size) is insufficient
      ret = (ret != idx)? datasize : BUFFER_INSUFFICIENT;
      return ret; 
    }    

    void Copy(const TableStore<EntryT> &that) 
    { 
      ptr_ = that.ptr_;
      size_ = that.size_;
      used_ = that.used_;
      grow_unit_ = that.grow_unit_;
      allocated_ = that.allocated_;
    }

    uint64_t used(void) { return used_; }
    uint64_t size(void) { return size_; }
    virtual uint32_t type(void) { 
      if(std::is_same<EntryT, BaseEntry>::value) 
        return kBaseEntry;
      else if(std::is_same<EntryT, CDEntry>::value) 
        return kCDEntry;
      else 
        return 0;
    }
    virtual uint64_t usedsize(void) { return used_ * sizeof(EntryT); }
    virtual char *GetPtr(void) { return (char *)ptr_; }
//    uint64_t used(void) { return used_ * sizeof(EntryT); }

    size_t EntrySize() { return sizeof(EntryT); }

    virtual void Print(void)
    {
      MYDBG("[Table] %lu/%lu, grow:%lu, alloc:%u\n", used_*sizeof(EntryT), size_, grow_unit_, allocated_);
    }

    void PrintEntry(uint64_t print_upto=0)
    {
      if(print_upto == 0) print_upto = used_;
      for(uint64_t i=0; i<print_upto; i++) {
        ptr_[i].Print();
      }
    }
  public:
    virtual uint64_t Insert(EntryT *newentry)
    {
      if(NeedRealloc(sizeof(EntryT))) {
        Reallocate();
      }
      ptr_[used_] = *newentry;
      used_++;//= sizeof(EntryT); 
      //MYDBG("[%s] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+used_, ptr_, used_, sizeof(EntryT));
      delete newentry;
      return used_*sizeof(EntryT);
    }

    virtual uint64_t Insert(EntryT &newentry)
    {
//      MYDBG("[%s] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+used_, ptr_, used_, sizeof(EntryT));
      if(NeedRealloc(sizeof(EntryT))) {
        Reallocate();
      }
      ptr_[used_] = newentry;
      used_++;//= sizeof(EntryT);
//      MYDBG("[%s done] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+used_, ptr_, used_, sizeof(EntryT));
      return used_*sizeof(EntryT);
    }

    virtual uint64_t Insert(EntryT &&newentry)
    {
//      MYDBG("[%s] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+used_, ptr_, used_, sizeof(EntryT));
      if(NeedRealloc(sizeof(EntryT))) {
        Reallocate();
      }
      ptr_[used_] = std::move(newentry);
      used_++;//= sizeof(EntryT);
//      MYDBG("[%s done] ptr:%p, (%p) used:%lu (entrysize:%zu)\n", __func__, ptr_+used_, ptr_, used_, sizeof(EntryT));
      return used_*sizeof(EntryT);
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
          memcpy(newptr, ptr_, used_*sizeof(EntryT));
          delete ptr_;
          ptr_ = newptr;
          allocated_++;
        } else {
          err = kMallocFailed;
        }
        MYDBG("memcpy?? %lu\n", used_);
      } else {
        AllocateTable();
      }
      return err;
    }

    virtual CDErrType Free(bool reuse)
    {
      MYDBG("reuse:%d\n", reuse);
      CDErrType err = kOK;
      used_ = 0;
      allocated_ = 0;
      if(reuse == false) {
        size_ = 0;
        grow_unit_ = BASE_ENTRY_CNT;//initial_grow_unit;
        if(ptr_ != NULL) {
//          MYDBG("%p\n", ptr_);
          delete ptr_;
          ptr_ = NULL;
        } else {
          err = kAlreadyFree;
        }
      }
      return err;
    }

    virtual void Read(char *pto, uint64_t size, uint64_t pos)
    {
      MYDBG("[%s] %p <- %p (%lu)\n", __func__, pto, ptr_ + pos, size);
      if( ptr_ != NULL && pto != NULL ) {  
        memcpy(pto, ptr_ + pos, size);
      } else {
        ERROR_MESSAGE("Read failed: to:%p <- from:%p (%p)\n", pto, ptr_+pos, ptr_);
      }
    }
};
    

BaseTable *GetTable(uint32_t entry_type, char *ptr_entry, uint32_t len_in_byte) 
{
  MYDBG("[%s] %u %p %u\n", __func__, entry_type, ptr_entry, len_in_byte);
  BaseTable *ret = NULL;
  switch(entry_type) { 
    case kBaseEntry: {
      ret = new TableStore<BaseEntry>(reinterpret_cast<BaseEntry *>(ptr_entry), len_in_byte);
      break;
    }
    case kCDEntry: {
      ret = new TableStore<CDEntry>(reinterpret_cast<CDEntry *>(ptr_entry), len_in_byte);
      break;
    }
    default:
                   {}
  }
  printf("[%s] %u %p %u ret:%p\n", __func__, entry_type, ptr_entry, len_in_byte, ret);
  return ret;
}

} // namespace cd ends
#endif
