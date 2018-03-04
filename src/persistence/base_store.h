#ifndef _BASE_STORE_H
#define _BASE_STORE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include "define.h"
#define ENTRY_TYPE_CNT 8
#define BASE_ENTRY_CNT 128
//#define DATA_GROW_UNIT 65536 // 64KB
//#define DATA_GROW_UNIT 4096 // 4KB
#define TABLE_GROW_UNIT 4096 // 4KB (128 EntryT)
#define BUFFER_INSUFFICIENT 0xFFFFFFFFFFFFFFFF
namespace packer {

extern void SetHead(bool head);
//enum PackerErrT {
//  kOK = 0,
//  kAlreadyAllocated,
//  kMallocFailed,
//  kAlreadyFree,
//  kNotFound,
//  kOutOfTable
//};

enum EntryType {
  kBaseEntry = 1,
  kCDEntry   = 2,
  kLogEntry  = 3,
};



//struct MagicStore {
//  uint64_t total_size_;
//  uint64_t table_offset_;
//  uint32_t entry_type_;
//  uint32_t reserved_;
//  uint64_t reserved2_; // 32 B
//  char pad_[480];
//  MagicStore(void) : total_size_(0), table_offset_(0), entry_type_(0) {}
//  MagicStore(uint64_t total_size, uint64_t table_offset=0, uint32_t entry_type=0) 
//    : total_size_(total_size), table_offset_(table_offset), entry_type_(entry_type) {}
//  void Print(void) {
//    printf("[%s] %lu %lu %u\n", __func__, total_size_, table_offset_, entry_type_);
//  }
//};

//struct SizeFieldType {
//  uint64_t size:48;
//  uint64_t attr:16;
//};
//
//union SizeType { 
//  unsigned long size;
//  SizeFieldType code;
//};


//    uint64_t id_   :64;
//    uint64_t size_  :48;
//    uint64_t attr_  :16;
//    uint64_t offset_:64;
//    char *   src;
// attr should be
// type   3bit(entrytype) 
// dirty  1bit
// medium 2bit (dram, file, remote, regen, ...)
//
#if 1 

struct AttrInternal {
    uint64_t size_:48;
    uint64_t reserved_:7;
    uint64_t output_:1;
    uint64_t table_:1;     // the entry actually points to table chunk
    uint64_t nested_:1;    // the entry points to data+table nested in data chunk
    uint64_t tmpshared_:1; // the etnry is temporarily shared
    uint64_t share_:1;  // the entry is shared
    uint64_t remote_:1; // the entry is remote
    uint64_t refer_:1;  // the entry is refering to another entry. offset is the ID for that.
    uint64_t dirty_:1;  // the entry is dirty. The data chunk that it points to is modified.
    uint64_t invalid_:1;  // the entry is valid
};

union Attr {
    enum {
      koutput    = 0x100,
      ktable     = 0x080,
      knested    = 0x040,  
      ktmpshared = 0x020,
      kshare     = 0x010,
      kremote    = 0x008,
      krefer     = 0x004, 
      kdirty     = 0x002,  
      kinvalid   = 0x001,
      knoattr    = 0x000
    };
    uint64_t code_;
    AttrInternal attr_;
    
    Attr(void) : code_(0) {}
    Attr(uint64_t size) : code_(size) {}
    Attr(uint64_t attr, uint64_t size) {
      code_ = (attr << 48);
      attr_.size_ = size;
    }
    uint64_t operator=(uint64_t that) {
      attr_.size_ = that;
      return attr_.size_;
    } 
    Attr &operator=(const Attr &that) {
      code_ = that.code_;
      return *this;
    } 
  
    void SetAll(uint64_t attr) {
      code_ = (attr << 48);
    }
  
    void Set(uint64_t attr) {
      code_ |= (attr << 48);
    }
    
    void Unset(uint64_t attr) {
      code_ &= ~(attr << 48);
    }
    
    // Every attr is the same, then true. Otherwise false.
    bool CheckAll(uint16_t attr) const 
    { return (static_cast<uint16_t>(code_ >> 48) ^ attr) == 0; }
  
    // If one of checking attr is set, then true. 
    bool CheckAny(uint16_t attr) const
    { return (static_cast<uint16_t>(code_ >> 48) & attr) != 0; }
    
    // If one of checking attr is set, then true. 
    bool Check(uint16_t attr) const
    { return (static_cast<uint16_t>(code_ >> 48) & attr) == attr; }

};

#endif

struct BaseEntry {
    uint64_t id_;
    Attr     size_;
    uint64_t offset_;
    BaseEntry(void) : id_(0), offset_(0) {}
    BaseEntry(uint64_t id, uint64_t size) 
      : id_(id), size_(size), offset_(0) {}
    BaseEntry(uint64_t id, uint64_t size, uint64_t offset) 
      : id_(id), size_(size), offset_(offset) {}
    BaseEntry(uint64_t id, uint16_t attr, uint64_t size, uint64_t offset) 
      : id_(id), size_(attr, size), offset_(offset) {}
    BaseEntry(const BaseEntry &that) {
      copy(that);
    }
    BaseEntry &SetOffset(uint64_t offset) { offset_ = offset; return *this; }
    BaseEntry &SetSrc(char *src) { return *this; }
    BaseEntry &operator=(const BaseEntry &that) {
      copy(that);
      return *this;
    }
    void copy(const BaseEntry &that) {
      id_    = that.id_;
      size_   = that.size_;
      offset_ = that.offset_;
    }
    void Print(void) const
    { printf("%12lx %12lx %12lx\n", id_, size(), offset_); }

    inline uint64_t size(void)   const { return size_.attr_.size_; }
    inline uint64_t invalid(void)  const { return size_.attr_.invalid_; }
    inline uint64_t attr(void)   const { return size_.code_ >> 48; }
    inline uint64_t offset(void) const { return offset_; }
};

struct CDEntry {
    uint64_t id_;
    Attr size_;
    uint64_t offset_;
    char *src_;
    CDEntry(void) : id_(0), size_(0), offset_(0), src_(0) {}
    CDEntry(uint64_t id, uint64_t size) 
      : id_(id), size_(size), offset_(0), src_(NULL) {}
    CDEntry(uint64_t id, uint64_t size, uint64_t offset) 
      : id_(id), size_(size), offset_(offset), src_(NULL) {}
    CDEntry(uint64_t id, uint64_t size, uint64_t offset, char *src) 
      : id_(id), size_(size), offset_(offset), src_(src) {}
    CDEntry(uint64_t id, uint64_t attr, uint64_t size, uint64_t offset, char *src) 
      : id_(id), size_(attr,size), offset_(offset), src_(src) {}
    CDEntry(const CDEntry &that) {
      copy(that);
    }
    CDEntry &SetOffset(uint64_t offset) { offset_ = offset; return *this; }
    CDEntry &SetSrc(char *src) { src_ = src; return *this; }
    CDEntry &operator=(const CDEntry &that) {
      copy(that);
      return *this;
    }
    void copy(const CDEntry &that) {
      id_    = that.id_;
      size_   = that.size_;
      offset_ = that.offset_;
      src_    = that.src_;
    }
    void Print(const char *str="") const
    { printf("CDEntry:%s (%16lx %10lx %10lx %p)\n", str, id_, size(), offset(), src()); }
    inline uint64_t size(void)   const { return size_.attr_.size_; }
    inline uint64_t invalid(void)  const { return size_.attr_.invalid_; }
    inline uint64_t attr(void)   const { return size_.code_ >> 48; }
    inline uint64_t offset(void) const { return offset_; }
    inline char    *src(void)    const { return src_; }
};

//extern uint32_t entry_type[ENTRY_TYPE_CNT];

} // namespace packer ends


#endif
