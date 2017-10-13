#ifndef _LIBC_WRAPPER_H 
#define _LIBC_WRAPPER_H 

#include <dlfcn.h>
#include <assert.h>
#include "packer.hpp"
//#include "logging.h"
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
enum {
  kNeedFreed = 0x100,
  kNeedPushed = 0x200,
};

enum FTID {
  FTID_invalid = 0,
  FTID_malloc = 1,
  FTID_calloc = 2,
  FTID_valloc = 3,
  FTID_realloc = 4,
  FTID_memalign = 5,
  FTID_posix_memalign = 6,
  FTID_free = 7, 
  FTIDNums
};

//bool replaying;
//bool disabled = true;
extern bool disabled;
extern bool replaying;
extern void *libc_handle;
//#define LOGGER_PRINT(...) printf(__VA_ARGS__)
#define LOGGER_PRINT(...)
#define INIT_FUNCPTR(func) \
  FT_##func = (__typeof__(&func))dlsym(RTLD_NEXT, #func); \
  assert(FT_##func); LOGGER_PRINT("Init %s:%p\n", ft2str[(FTID_##func)], FT_##func);
//extern void Init(void);
//extern void Fini(void);
void Init(void);
//{
//  disabled = false;
//}

void Fini(void);
//{
//  replaying = false;
//  disabled = true;
//}
extern char ft2str[FTIDNums][64];
//char ft2str[FTIDNums][64] = { 
//                              "invalid"
//                            , "malloc"
//                            , "calloc"
//                            , "valloc"
//                            , "realloc"
//                            , "memalign"
//                            , "posix_memalign"
//                            , "free"
//                            };
//    

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
    { LOGGER_PRINT("Entry [%16s] %5lx %4lx %4lx %lx\n", ft2str[ftype_], id_, attr(), size(), offset_); }
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
//extern void (*FT_free)(void);
//__typeof__(&free) FT_free;

class LogPacker;
extern LogPacker *GetLogger(void);

class LogPacker : public packer::Packer< logger::LogEntry > {
  friend LogPacker *GetLogger(void);
    static LogPacker *libc_logger;
  public:
    LogPacker() { LOGGER_PRINT("LogPacker created\n"); }
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
//    {
//      bool orig_disabled = logger::disabled;
//      logger::disabled = true;
//      uint64_t current_log_id = LogEntry::gen_ftid;
//      LogEntry *entry = table_->FindReverse(current_log_id, current_log_id);
//      LOGGER_PRINT("### Is Pushed Log? %s\n", (entry != NULL)? "True" : "False"); //STOPHERE;
//      logger::disabled = orig_disabled;;
//      return entry != NULL;
//    }
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
//       LOGGER_PRINT("### Is Pushed Log? %s\n", (entry != NULL)? "True" : "False"); STOPHERE;
//       return entry != NULL;
//     }

    // Find an entry with kNeedPushed from the offset, 
    // and copy the entry at the dst offset to the offset.
    // return the dst offset for the next search.
    uint64_t PushLogs(uint64_t offset_from);
//    {
//      bool orig_disabled = logger::disabled;
//      logger::disabled = true;
//      uint64_t orig_tail = table_->tail();
//      uint64_t new_tail = table_->FindWithAttr(kNeedPushed, offset_from);
//      LOGGER_PRINT("Pushed %lu entries (orig: %lu entries)\n", new_tail - offset_from, orig_tail);
//      logger::disabled = orig_disabled;;
//      return new_tail;
//    }
//    {
//      uint64_t orig_tail = table_->tail();
//      uint64_t new_tail = table_->FindWithAttr(kNeedPushed, offset_from);
//      LOGGER_PRINT("Pushed %lu entries (orig: %lu entries)\n", new_tail - offset_from, orig_tail);
//      return new_tail;
//    }

    uint64_t FreeMemory(uint64_t offset_from);
//    {
//      LOGGER_PRINT("%s\n", __func__);
//      bool orig_disabled = logger::disabled;
//      logger::disabled = true;
//      uint64_t freed_cnt = 0;
//      {
//      LOGGER_PRINT("%s reach1\n", __func__);
//      //packer::Packer<LogEntry> free_list;
//      LogPacker free_list;
//      LOGGER_PRINT("%s reach2\n", __func__);
//      Print();
//      freed_cnt = table_->FindWithAttr(kNeedFreed, offset_from, free_list.table_);
//      Print();
//      LOGGER_PRINT("FreeMemory %ld == %lu entries from offset %lu\n", free_list.table_->used(), freed_cnt, offset_from);
//      free_list.Print();
//      //STOPHERE;
//      for(uint32_t i=0; i<freed_cnt; i++) {
//        LOGGER_PRINT("i:%u\n", i);
//        LogEntry *entry = free_list.table_->GetAt(i);
//        entry->Print();
//        FT_free((void *)(entry->offset()));
//        entry->size_.Unset(kNeedFreed);
//      }
//    
//      }
//      logger::disabled = orig_disabled;
//      return freed_cnt;
//    }

    void Print(void) {
      LogEntry *ptr = table_->GetAt(0);
      for(uint32_t i=0; i<table_->used(); i++) {
        (ptr + i)->Print();
      }
    }
};

//LogPacker *LogPacker::libc_logger;
//uint64_t LogEntry::gen_ftid;
//LogPacker *GetLogger(void)
//{
//  if(LogPacker::libc_logger == NULL) {
//    bool orig_disabled = logger::disabled; 
//    logger::disabled = true; 
//    LOGGER_PRINT("before logger\n");
//    LogPacker::libc_logger = new LogPacker;
//    LOGGER_PRINT("after logger\n");
//    logger::disabled = orig_disabled;
//  }
//  return LogPacker::libc_logger;
//}


//extern packer::Packer<LogEntry> *GetLogger(void);


//inline uint64_t GetNextID(void) { return LogEntry::gen_ftid; }
//inline void SetNextID(uint64_t orig_ftid) { LogEntry::gen_ftid = orig_ftid; }
//bool IsLogFound(void);
//void PushLogs(uint64_t offset_from);

}
/*
#define CHECK_INIT(func) \
  if(FT_##func == NULL) { \
    LOGGER_PRINT("logging: %d\n", logger::disabled); \
    INIT_FUNCPTR(func); \
    if(logger::initialized == false) logger::InitMallocPtr(); \
    assert(FT_##func); \
    assert(logger::initialized); \
  }
*/
#define LOGGING_PROLOG(func, ...) \
  CHECK_INIT(func) \
  if(logger::disabled) { \
    return FT_##func(__VA_ARGS__); \
  } else { \
    bool orig_disabled = logger::disabled; \
    logger::disabled = true; \
    LOGGER_PRINT("\n>>> Logging Begin %lu %s\n", logger::LogEntry::gen_ftid, ft2str[(FTID_##func)]); 

#define LOGGING_EPILOG(func) \
    LOGGER_PRINT("<<< Logging End   %lu %s\n", logger::LogEntry::gen_ftid, ft2str[(FTID_##func)]); \
    logger::LogEntry::gen_ftid++; \
    logger::disabled = orig_disabled; \
  }



//typedef void*(*FType_malloc)(size_t size);
//typedef void*(*FType_calloc)(size_t numElem, size_t size);
//typedef void*(*FType_valloc)(size_t size);
//typedef void*(*FType_realloc)(void *ptr, size_t size);
//typedef void*(*FType_memalign)(size_t boundary, size_t size);
//typedef int(*FType_posix_memalign)(void **memptr, size_t alignment, size_t size);
//typedef void(*FType_free)(void *ptr);

//void *calloc2(size_t numElem, size_t size);
//void *calloc(size_t numElem, size_t size) __THROW __attribute_malloc__ __wur;

#endif
