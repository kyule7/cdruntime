#pragma once

#include "packer.hpp"
#include "logging.h"
/**
 *  Our approach to log libc library is using dlsym(RTLD_NEXT, symbol)
 *  FT_symbol is our format of function pointer for the real symbol, 
 *  and the address is calculated by dlsym in Initialize() phase.
 *  Currently, Initialize() is invoked before main as constructor of static
 *  object. 
 *
 *  disabled flag is important to play a role in on/off switch for logging.
 *  Any libc call before main() must not be recorded, but just be invoked as it
 *  is. Therefore, disabled is 1 very initially, and once app starts, it changes
 *  to 0. For this, it provides Initialize() interface for user to control it.
 *  When this wrapper is integrated into CD runtime, Initialize() should be
 *  called inside CD runtime's initialization phase. (In CD_Init())
 *
 *  This Initialize() should be invoked for the first place, because the other
 *  things may call libc library before that, may call our wrapper which did not
 *  yet calculate function pointer. 
 *
 *
 *  One way to avoid it is to do dlsym in the wrapper. It lazily calculate
 *  function pointer when the libc function is called by application. 
 *
 *  The following items are issues regarding our approach for libc logging.
 *
 *  1) Recursive calloc/dlsym: In order to wrap calloc, it calls dlsym. dlsym
 *     calls calloc in it, and calloc call invokes our wrapper which calls dlsym
 *     in it, never actually returning calloc function pointer from dlsym. We can
 *     use other memory allocation calls, but dlsym must return the function pointer
 *     of the real function beforehand. Unless we do not support a wrapper for a
 *     certain memory allocation call, it still remains.
 * 
 *     Resolution: dlsym allocates 32 Byte using calloc, and static array is
 *     returned for dlsym, instead of calloc. By test, this 32 Byte memory is not
 *     freed, it is safe to use static array. 
 * 
 *  2) malloc_hooks(https://www.gnu.org/software/libc/manual/html_node/Hooks-for-Malloc.html):
 *     libopen-pal.so which is a part of Open MPI actually uses this.
 *     
 */



namespace logger {
struct LogEntry : public packer::BaseEntry {
    static uint64_t gen_ftid;
    FTID ftype_;
    LogEntry(void) : ftype_(FTID_invalid) {}
    LogEntry(FTID ftype) 
      : packer::BaseEntry(gen_ftid, 0), ftype_(ftype) {}
    LogEntry(uint64_t id, uint64_t size, uint64_t offset) 
      : packer::BaseEntry(id, size, offset), ftype_(FTID_invalid) {}
    LogEntry(uint64_t id, FTID ftype, uint64_t attr, uint64_t offset) 
      : packer::BaseEntry(id, attr, 0, offset), ftype_(ftype) {}
    LogEntry(uint64_t id, FTID ftype, uint16_t attr, uint64_t size, uint64_t offset) 
      : packer::BaseEntry(id, attr, size, offset), ftype_(ftype) {}
//    LogEntry(FTID ftype, uint64_t id, uint64_t size, uint64_t offset) 
//      : packer::BaseEntry(id, size, offset), ftype_(ftype) {}
    LogEntry(const LogEntry &that) {
      ftype_ = that.ftype_;
      copy(that);
    }
    void Print(void) const
    { printf("Entry [%16s] %5lx %4lx %4lx %lx\n", ft2str[ftype_], id_, attr(), size(), offset_); getchar(); }
};
#if 0
  static CDPath *uniquePath_;
public:
  CDPath(void) {
  
#if CD_MPI_ENABLED == 0 && CD_AUTOMATED == 1
//    CDHandle *root = CD_Init(1, 0);
//    CD_Begin(root);
#endif
  }
  ~CDPath(void) {
#if CD_MPI_ENABLED == 0 && CD_AUTOMATED == 1
//    CD_Complete(back());
//    CD_Finalize();
#endif
  }

public:
 /** @brief Get CDHandle of Root CD 
  *
  * \return Pointer to CDHandle of root
  */
  static CDPath* GetCDPath(void) 
  {
    if(uniquePath_ == NULL) 
      uniquePath_ = new CDPath();
    return uniquePath_;
  }
#endif
class LogPacker;
extern bool disabled;
extern LogPacker *GetLogger(void);
class LogPacker : public packer::Packer< LogEntry > {
  friend LogPacker *GetLogger(void);
    static LogPacker *libc_logger;
  public:
    inline uint64_t GetNextID(void) { return LogEntry::gen_ftid; }
    inline void SetNextID(uint64_t orig_ftid) { LogEntry::gen_ftid = orig_ftid; }
    inline uint64_t Set(uint64_t &offset) {
      logger::replaying = false;
      offset = table_->tail();
      return LogEntry::gen_ftid;
    }

    inline void Reset(uint64_t orig_ftid) {
      logger::replaying = true;
      LogEntry::gen_ftid = orig_ftid;
    }

    bool IsLogFound(void);
//     {
//       uint64_t current_log_id = LogEntry::gen_ftid;
//       LogEntry *entry = table_->FindReverse(current_log_id, current_log_id);
// //      GetLogger()->table_->head_ - 1;
// //      Entry *upto  = GetLogger()->GetAt(offset_from);
// //      Entry *entry = GetLogger()->table_->GetLast();
// //      for(; entry != upto; entry--) {
// //        entry->size_.Check(kPushed);
// //      }
// //      LogEntry *entry = GetLogger()->table_->Find(id);
//       printf("### Is Pushed Log? %s\n", (entry != NULL)? "True" : "False"); getchar();
//       return entry != NULL;
//     }

    // Find an entry with kNeedPushed from the offset, 
    // and copy the entry at the dst offset to the offset.
    // return the dst offset for the next search.
    uint64_t PushLogs(uint64_t offset_from); 
//    {
//      uint64_t orig_tail = table_->tail();
//      uint64_t new_tail = table_->FindWithAttr(kNeedPushed, offset_from);
//      printf("Pushed %lu entries (orig: %lu entries)\n", new_tail - offset_from, orig_tail);
//      return new_tail;
//    }

    uint64_t FreeMemory(uint64_t offset_from);
    void Print(void) {
      LogEntry *ptr = table_->GetAt(0);
      for(uint32_t i=0; i<table_->used(); i++) {
        (ptr + i)->Print();
      }
    }
};


//extern packer::Packer<LogEntry> *GetLogger(void);


//inline uint64_t GetNextID(void) { return LogEntry::gen_ftid; }
//inline void SetNextID(uint64_t orig_ftid) { LogEntry::gen_ftid = orig_ftid; }
//bool IsLogFound(void);
//void PushLogs(uint64_t offset_from);

}

#define CHECK_INIT(func) \
  if(FT_##func == NULL) { \
    INIT_FUNCPTR(func); \
    if(logger::initialized == false) logger::InitMallocPtr(); \
    assert(FT_##func); \
    assert(logger::initialized); \
  }

#define LOGGING_PROLOG(func, ...) \
  CHECK_INIT(func) \
  if(logger::disabled) { \
    printf("XXX Logging Disabled %s XXX\n", ft2str[(FTID_##func)]); \
    return FT_##func(__VA_ARGS__); \
  } else { \
    logger::disabled = true; \
    printf("\n>>> Logging Begin %lu %s\n", logger::LogEntry::gen_ftid, ft2str[(FTID_##func)]); 

#define LOGGING_EPILOG(func) \
    printf("<<< Logging End   %lu %s\n", logger::LogEntry::gen_ftid, ft2str[(FTID_##func)]); \
    logger::LogEntry::gen_ftid++; \
    logger::disabled = false; \
  }



//typedef void*(*FType_malloc)(size_t size);
//typedef void*(*FType_calloc)(size_t numElem, size_t size);
//typedef void*(*FType_valloc)(size_t size);
//typedef void*(*FType_realloc)(void *ptr, size_t size);
//typedef void*(*FType_memalign)(size_t boundary, size_t size);
//typedef int(*FType_posix_memalign)(void **memptr, size_t alignment, size_t size);
//typedef void(*FType_free)(void *ptr);

void *calloc2(size_t numElem, size_t size);
void *calloc(size_t numElem, size_t size) __THROW __attribute_malloc__ __wur;
