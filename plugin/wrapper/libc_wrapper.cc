#include "logging.h"
#include "libc_wrapper.h"
#include "singleton.h"
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>
#include <pthread.h>
//#include <unistd.h>

using namespace logger;
LogPacker *LogPacker::libc_logger = NULL;
uint64_t logger::libc_id = 0;
uint32_t logger::taskID = 0;
bool logger::replaying = false;
bool logger::disabled  = true;
LogPacker *logger::GetLogger(void)
{
  if(LogPacker::libc_logger == NULL) {
    bool orig_disabled = logger::disabled; 
    logger::disabled = true; 
//    LOGGER_PRINT("before logger\n");
    LogPacker::libc_logger = new LogPacker;
//    LOGGER_PRINT("after logger\n");
    logger::disabled = orig_disabled;
  }
  return LogPacker::libc_logger;
}

void logger::Init(void)
{
  disabled = false;
}

void logger::Fini(void)
{
  replaying = false;
  disabled = true;
}
bool logger::initialized = false;
bool logger::init_calloc = false;
void *logger::libc_handle = NULL;
//logger::Singleton logger::singleton;
//packer::Packer<LogEntry> serdes;
namespace logger {
struct LocalAllocator {
  static char local_buf[66536] __attribute__ ((aligned (4096)));
  static uint32_t local_buf_offset;
  static pthread_mutex_t mutex;
  LocalAllocator(void) { memset(local_buf, 0, 66536); }
  void *Alloc(size_t size) {
//      LOGGER_PRINT("Alloc my memory\n");
      void *ret = NULL;
      pthread_mutex_lock(&mutex);
      ret = local_buf + local_buf_offset;
      local_buf_offset += size;
      pthread_mutex_unlock(&mutex);
      return ret;
  }
};
}

char logger::LocalAllocator::local_buf[66536] __attribute__ ((aligned (4096)));
uint32_t logger::LocalAllocator::local_buf_offset = 0;
pthread_mutex_t logger::LocalAllocator::mutex = PTHREAD_MUTEX_INITIALIZER;

LocalAllocator local_allocator;

//void logger::Init(void)
//{
//  logger::disabled = false;
//}
//
//void logger::Fini(void)
//{
//  logger::replaying = false;
//  logger::disabled = true;
//}

/**************************************************/
DEFINE_FUNCPTR(malloc);
DEFINE_FUNCPTR(calloc);
DEFINE_FUNCPTR(valloc);
DEFINE_FUNCPTR(realloc);
DEFINE_FUNCPTR(memalign);
DEFINE_FUNCPTR(posix_memalign);
DEFINE_FUNCPTR(free);

char logger::ft2str[FTIDNums][64] = { 
                              "invalid"
                            , "malloc"
                            , "calloc"
                            , "valloc"
                            , "realloc"
                            , "memalign"
                            , "posix_memalign"
                            , "free"
                            };
    

//void LogEntry::Print(void) const
//{ LOGGER_PRINT("Entry [%16s] %5lx %4lx %4lx %lx\n", ft2str[ftype_], id_, attr(), size(), offset_); STOPHERE; }
//
bool LogPacker::IsLogFound(void) 
{
  bool orig_disabled = logger::disabled;
  logger::disabled = true;
  uint64_t current_log_id = logger::libc_id;
  LogEntry *entry = table_->FindReverse(current_log_id, logger::libc_id);
  LOGGER_PRINT("###[logID:%lu, %lu] Is Pushed Log? %s\n", logger::libc_id, current_log_id, (entry != NULL)? "True" : "False"); //STOPHERE;
  logger::disabled = orig_disabled;;
  return entry != NULL;
}
//{
//  bool orig_disabled = logger::disabled;
//  logger::disabled = true;
//  uint64_t current_log_id = logger::libc_id;
//  LogEntry *entry = table_->FindReverse(current_log_id, current_log_id);
//  LOGGER_PRINT("### Is Pushed Log? %s\n", (entry != NULL)? "True" : "False"); //STOPHERE;
//  logger::disabled = orig_disabled;;
//  return entry != NULL;
//}
//
// Find an entry with kNeedPushed from the offset, 
// and copy the entry at the dst offset to the offset.
// return the dst offset for the next search.
uint64_t LogPacker::PushLogs(uint64_t offset_from)
{
  bool orig_disabled = logger::disabled;
  logger::disabled = true;
  uint64_t orig_tail = table_->tail();
  uint64_t new_tail = table_->FindWithAttr(kNeedPushed, offset_from);
  LOGGER_PRINT("Pushed %lu entries (orig: %lu entries)\n", new_tail - offset_from, orig_tail);
  logger::disabled = orig_disabled;;
  return new_tail;
}
//uint64_t LogPacker::PushLogs(uint64_t offset_from) 
//{
//  bool orig_disabled = logger::disabled;
//  logger::disabled = true;
//  uint64_t orig_tail = table_->tail();
//  uint64_t new_tail = table_->FindWithAttr(kNeedPushed, offset_from);
//  LOGGER_PRINT("Pushed %lu entries (orig: %lu entries)\n", new_tail - offset_from, orig_tail);
//  logger::disabled = orig_disabled;;
//  return new_tail;
//}
//
//uint64_t LogPacker::FreeMemory(uint64_t offset_from) 
//{
//  bool orig_disabled = logger::disabled;
//  logger::disabled = true;
//  uint64_t freed_cnt = 0;
//  {
//  //packer::Packer<LogEntry> free_list;
//  LogPacker free_list;
//  freed_cnt = table_->FindWithAttr(kNeedFreed, offset_from, free_list.table_);
//  Print();
//  LOGGER_PRINT("FreeMemory %ld == %lu entries from offset %lu\n", free_list.table_->used(), freed_cnt, offset_from);
//  free_list.Print();
//  //STOPHERE;
//  for(uint32_t i=0; i<freed_cnt; i++) {
//    LOGGER_PRINT("i:%u\n", i);
//    LogEntry *entry = free_list.table_->GetAt(i);
//    entry->Print();
//    FT_free((void *)(entry->offset()));
//    entry->size_.Unset(kNeedFreed);
//  }
//
//  }
//  logger::disabled = orig_disabled;;
//  return freed_cnt;
//}
uint64_t LogPacker::FreeMemory(uint64_t offset_from) 
{
  LOGGER_PRINT("%s\n", __func__);
  bool orig_disabled = logger::disabled;
  logger::disabled = true;
  uint64_t freed_cnt = 0;
  {
    LOGGER_PRINT("%s reach1\n", __func__);
    //packer::Packer<LogEntry> free_list;
    LogPacker free_list;
    LOGGER_PRINT("%s reach2\n", __func__);
    //Print();
    freed_cnt = table_->FindWithAttr(kNeedFreed, offset_from, free_list.table_);
    //Print();
    LOGGER_PRINT("FreeMemory %ld == %lu entries from offset %lu\n", free_list.table_->used(), freed_cnt, offset_from);
    if(taskID == 0) {
//    printf("FreeMemory %ld == %lu entries from offset %lu\n", free_list.table_->used(), freed_cnt, offset_from);
//    free_list.Print();
    }
    //STOPHERE;
    for(uint32_t i=0; i<freed_cnt; i++) {
      LOGGER_PRINT("i:%u\n", i);
      LogEntry *entry = free_list.table_->GetAt(i);
//      entry->Print();
      FT_free((void *)(entry->offset()));
      entry->size_.Unset(kNeedFreed);
    }

  }
  logger::disabled = orig_disabled;
  return freed_cnt;
}

//packer::Packer<LogEntry> *GetLogger(void) { return &serdes; }
//LogPacker *logger::GetLogger(void)
//{
//  if(LogPacker::libc_logger == NULL) LogPacker::libc_logger = new LogPacker;
//  return LogPacker::libc_logger;
//}
/**************************************************/

// This must be called initially.
// Some rare libc functions are wrapped lazily,
// but some critical functions must be done beforehand.
// Please see the description about issue regarding 
// calloc/dlsym in libc_wrapper.h file
void logger::InitMallocPtr(void)
{
  if(logger::initialized == false) {
    LOGGER_PRINT("%s\n", __func__);
//    INIT_FUNCPTR(calloc);
    INIT_FUNCPTR(malloc);
    //INIT_FUNCPTR(valloc); // FIXME:10112018
    INIT_FUNCPTR(realloc);
    INIT_FUNCPTR(memalign);
    INIT_FUNCPTR(posix_memalign);
//    LOGGER_PRINT("Initialized1  %p %p %p %p\n", FT_calloc, FT_malloc, FT_valloc, FT_free); 
//    INIT_FUNCPTR(free);
//    LOGGER_PRINT("Initialized %p %p %p %p\n", FT_calloc, FT_malloc, FT_valloc, FT_free); 
    //GetLogger();
    logger::initialized = true;
  }
}


//void *dlsym(void *handle, const char *symbol){
//  LOGGER_PRINT("dlsym %p, %s\n", handle, symbol);
//}


/*************************************************
 * Basic operation flow is like below
 * 
 * int LibCFunc(args) {
 *    int ret = 0;
 *    if(disabled) {
 *      ret = func(args);
 *    } else {
 *      entry = packer.GetNext();
 *      assert(entry.id_ == FTID);
 *    }
 *    return ret; 
 *  }
 *
 *
 *************************************************/
EXTERNC void free(void *ptr)
{
//  LOGGER_PRINT("free called %s %s\n",
//      (logger::disabled)? "Disabled":"Enabled", (logger::init_calloc)? "Initialized":"Not Init");  //STOPHERE;

  if(ptr == NULL) {  return; }
  if(logger::disabled) {
    if(FT_calloc == NULL) {
    //    LOGGER_PRINT(" 1 calloc:%p, free:%p\n", FT_calloc, FT_free);
        return;
    } else if(FT_free == NULL) {
    //    LOGGER_PRINT(" 2 calloc:%p, free:%p\n", FT_calloc, FT_free); 
        return;
    } else {
//      LOGGER_PRINT("XXX Logging Disabled %s(%p) XXX %p %p\n", ft2str[FTID_free], ptr, FT_free, free); 
      return FT_free(ptr); 
    }

  } else {
    LOGGER_PRINT("free(ptr : %p) %p\n", ptr, FT_free);
    bool orig_disabled = logger::disabled; 
    logger::disabled = true; 
    LOGGER_PRINT("\n>>> Logging Begin %lu %s\n", logger::libc_id, ft2str[FTID_free]); 
    if(logger::replaying == false) {
    //if(logger::replaying == false || GetLogger()->IsLogFound() == false) { 
      LOGGER_PRINT("Executing %s(%p), disabled:%d\n", __func__, ptr, logger::disabled); 
      uint32_t idx = 0;
      LOGGER_PRINT("[free noerror] find(%p) with %p\n", FT_free, ptr);
#if _DEBUG_10252017
      LogEntry *entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
      if(entry != NULL) {
        entry->size_.Unset(kNeedPushed);
        entry->size_.Set(kNeedFreed);
      } else {
        printf("[%s(%p)] Potential memory leak\n", __func__, ptr);
      }
#else
      LogEntry *entry = NULL;
      while(entry == NULL) {
        entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
  //      if(idx == -1U) { assert(0); }
      }
      entry->size_.Unset(kNeedPushed);
      entry->size_.Set(kNeedFreed);
//      entry->Print();
#endif
    } else { // it is now replaying
  //    LOGGER_PRINT("Replaying %s(%p)\n", __func__, ptr); 
      uint32_t idx = 0;
      LogEntry *entry = NULL;
      LOGGER_PRINT("[free replay] find(%p)\n", ptr); 
//      if(logger::taskID == 0) {
//        printf("[free replay] find(%p)\n", ptr); 
////        GetLogger()->Print();
//      }
      while(entry == NULL) {
        entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
        if(idx == -1U) { LOGGER_PRINT("free %p is not founded in malloc list\n", ptr); break; }
      }

      // FIXME /////////////////////////
      assert(entry);
      entry->size_.Unset(kNeedPushed);
      entry->size_.Set(kNeedFreed);
      //////////////////////////////////

      if(entry != NULL) 
      { LOGGER_PRINT("Replaying %s(%p), freed? %lx\n", __func__, ptr, entry->attr()); }
//      entry->Print();
      //STOPHERE;
    }
    LOGGER_PRINT("<<< Logging End   %lu %s\n", logger::libc_id, ft2str[FTID_free]); 
    logger::libc_id++; 
    logger::disabled = orig_disabled; 
  }
//  LOGGING_EPILOG(free);
}
EXTERNC void *calloc(size_t numElem, size_t size)
{ 
  static bool first_calloc = true;
  void *ret = NULL;
//  LOGGER_PRINT("calloc%p(%zu,%zu) %s, %s\n", FT_calloc, numElem, size, 
//      (logger::disabled)? "Disabled":"Enabled", (logger::init_calloc)? "Initialized":"Not Init"); //STOPHERE;
  if(logger::disabled) { 
//    LOGGER_PRINT("calloc(%zu,%zu) wrapped 2\n", numElem, size); //STOPHERE;
    if(logger::init_calloc == true) {
      assert(FT_calloc);
      LOGGER_PRINT("XXX Logging Disabled %s(%zu, %zu) (%p)\n", ft2str[(FTID_calloc)], numElem, size, FT_calloc); 
      ret = FT_calloc(numElem, size); 
    } else {
      // first make sure it is called by dlsym for calloc
      if(first_calloc) {
        first_calloc = false;
        LOGGER_PRINT("\nfirst calloc\n");
//        libc_handle = dlopen("libc.so", RTLD_NOW);
//        assert(libc_handle);
        LOGGER_PRINT("\nafter dlopen\n");
        INIT_FUNCPTR(calloc);
        LOGGER_PRINT("before first calloc:%p, free %p\n\n", FT_calloc, FT_free);
        INIT_FUNCPTR(free);
        LOGGER_PRINT("after first calloc %p, free: %p\n\n", FT_calloc, FT_free); 
        logger::init_calloc = true;
      }
      if(logger::init_calloc == false) {
      ret = local_allocator.Alloc(numElem*size);
//      logger::init_calloc = true; 
      } else {
//        LOGGER_PRINT("we got calloc fptr: %p\n", FT_calloc);
      ret = FT_calloc(numElem, size); 
        
      }
    }
//    uint64_t *cval = (uint64_t *)(&calloc);
//    LOGGER_PRINT("####################33, %p == %p\n", FT_calloc, &calloc);
//    if(cval != NULL) {
//      LOGGER_PRINT("%lx %lx %lx %lx %lx %lx %lx %lx\n", *cval, *(cval+1), *(cval+2), *(cval+3), *(cval+4), *(cval+5), *(cval+6), *(cval+7) );
//    }
    //STOPHERE;
  } 
  else { 
    bool orig_disabled = logger::disabled; 
    logger::disabled = true; 
    assert(FT_calloc);
    LOGGER_PRINT("\n>>> Logging Begin %lu %s\n", logger::libc_id, ft2str[(FTID_calloc)]); 
    if(logger::replaying == false || GetLogger()->IsLogFound() == false) { 
      ret = FT_calloc(numElem, size);
      LOGGER_PRINT("record calloc\n");
      GetLogger()->Add(LogEntry(logger::libc_id, FTID_calloc, kNeedPushed, (uint64_t)ret));
    } else {
      LOGGER_PRINT("Replaying %s\n", __func__); 
      LogEntry *entry = GetLogger()->GetNext();
      //assert(entry->ftype_ == FTID_calloc);
      if(entry->ftype_ == FTID_calloc) {
        ret = (void *)entry->offset_;
      } else {
        ret = FT_calloc(numElem, size);
        LOGGER_PRINT("regen calloc\n");
        GetLogger()->Add(LogEntry(logger::libc_id, FTID_calloc, kNeedPushed, (uint64_t)ret));
      }
//      entry->Print();
    }
    LOGGER_PRINT("<<< Logging End  %lu %s\n", logger::libc_id, ft2str[(FTID_calloc)]); 
    logger::libc_id++;
    logger::disabled = orig_disabled; 
  }
  LOGGER_PRINT("%p = calloc(%zu,%zu) wrapped, %s %s\n", ret, numElem, size, (logger::disabled)? "Disabled":"Enabled", (logger::init_calloc)? "Initialized":"Not Init");
  //LOGGING_EPILOG(calloc);
  return ret;

}
/**
 *  if( replaying )
 *    if( recreated )
 *      need_replay = isEntryLogged();
 *    else
 *      need_replay = true;
 *
 * if it is not pushed, it should be regenerated by calling real func
 * if it is pushed, just replay it.
 * If it is not recreated (in the scope of begin/complete),
 * it should be always replayed.
 * No escalation always return true, because it finds the log entry.
 */
EXTERNC void *malloc(size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(malloc, size);
  bool is_log_found = GetLogger()->IsLogFound();
  if(logger::replaying == false || is_log_found == false) { 
    ret = FT_malloc(size);
    GetLogger()->Add(LogEntry(logger::libc_id, FTID_malloc, kNeedPushed, (uint64_t)ret));
  } else {
    LogEntry *entry = GetLogger()->GetNext();
    //assert(entry->ftype_ == FTID_malloc);
    if(entry->ftype_ == FTID_malloc) {
      ret = (void *)entry->offset_;
//      printf("[Reexec %lu] Replay %s(zu)=%p\n", libc_id, __func__, size, ret); 
    } else {
      ret = FT_malloc(size);
      GetLogger()->Add(LogEntry(logger::libc_id, FTID_malloc, kNeedPushed, (uint64_t)ret));
//      printf("[Reexec %lu] Regen %s(zu)=%p\n", libc_id, __func__, size, ret); 
    }
//    entry->Print();
  }
  LOGGING_EPILOG(malloc);
  LOGGER_PRINT("%p = malloc(%zu)\n", ret, size);
  return ret;
  
}
/*
EXTERNC void *valloc(size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(valloc, size);
  if(logger::replaying == false || GetLogger()->IsLogFound() == false) { 
    ret = FT_valloc(size);
    GetLogger()->Add(LogEntry(logger::libc_id, FTID_valloc, kNeedPushed, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    //assert(entry->ftype_ == FTID_valloc);
    if(entry->ftype_ == FTID_valloc) {
      ret = (void *)entry->offset_;
    } else {
      ret = FT_valloc(size);
      GetLogger()->Add(LogEntry(logger::libc_id, FTID_valloc, kNeedPushed, (uint64_t)ret));
    }
//    entry->Print();
  }
  LOGGING_EPILOG(valloc);
  LOGGER_PRINT("%p = valloc(%zu)\n", ret, size);
  return ret;
}
*/

EXTERNC void *realloc(void *ptr, size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(realloc, ptr, size);
  if(logger::replaying == false || GetLogger()->IsLogFound() == false) {
    // it is difficult to hook some internal free() call inside realloc. 
    // FT_realloc() is never called, but malloc is called instead for
    // reallocation. free() is deferred to FreeMemory().
    ret = FT_malloc(size);
    GetLogger()->Add(LogEntry(logger::libc_id, FTID_realloc, kNeedPushed, (uint64_t)ret));
//    GetLogger()->table_->GetLast()->Print();
    uint32_t idx = 0;
#if _DEBUG_10252017
    LogEntry *entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
    if(entry != NULL) {
      entry->size_.Unset(kNeedPushed);
      entry->size_.Set(kNeedFreed);
    } else {
      printf("[%s(%p, %zu)] Potential memory leak\n", __func__, ptr, size);
    }
#else
    LogEntry *entry = NULL;
    while(entry == NULL) {
      entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
      if(idx == -1U) { assert(0); }
    }
    entry->size_.Unset(kNeedPushed);
    entry->size_.Set(kNeedFreed);
//    entry->Print();
#endif


  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    //assert(entry->ftype_ == FTID_realloc);
    if(entry->ftype_ == FTID_realloc) {
      ret = (void *)entry->offset_;
    } else {
      ret = FT_malloc(size);
      GetLogger()->Add(LogEntry(logger::libc_id, FTID_realloc, kNeedPushed, (uint64_t)ret));
      uint32_t idx = 0;
      LogEntry *malloc_entry = NULL;
      while(malloc_entry == NULL) {
        malloc_entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
        if(idx == -1U) { assert(0); }
      }
      malloc_entry->size_.Unset(kNeedPushed);
      malloc_entry->size_.Set(kNeedFreed);

    }
    LOGGER_PRINT("[%lu, %lu, %lu, %lx] %d == %d\n", entry->id_, entry->attr(), entry->size(), entry->offset(), entry->ftype_ , FTID_realloc);
//    entry->Print();
  }
  LOGGING_EPILOG(realloc);
  LOGGER_PRINT("%p = realloc(%p, %zu)\n", ret, ptr, size);
  return ret;
}


EXTERNC void *memalign(size_t boundary, size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(memalign, boundary, size);
  ret = FT_memalign(boundary, size);
  if(logger::replaying == false) { 
    ret = FT_memalign(boundary, size);
    GetLogger()->Add(LogEntry(logger::libc_id, FTID_memalign, kNeedPushed, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    //assert(entry->ftype_ == FTID_memalign);
    if(entry->ftype_ == FTID_memalign) {
      ret = (void *)entry->offset_;
    } else {
      ret = FT_memalign(boundary, size);
      GetLogger()->Add(LogEntry(logger::libc_id, FTID_memalign, kNeedPushed, (uint64_t)ret));
    }
//    entry->Print();
  }
  LOGGING_EPILOG(memalign);
  return ret;
}


EXTERNC int posix_memalign(void **memptr, size_t alignment, size_t size)
{
  int ret = -1;
  LOGGING_PROLOG(posix_memalign, memptr, alignment, size);

  if(logger::replaying == false) { 
    ret = FT_posix_memalign(memptr, alignment, size);
    GetLogger()->Add(LogEntry(logger::libc_id, FTID_posix_memalign, kNeedPushed, ret, (uint64_t)(*memptr)));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    //assert(entry->ftype_ == FTID_posix_memalign);
    if(entry->ftype_ == FTID_posix_memalign) {
      ret = entry->size();
      *memptr = (void *)entry->offset_;
    } else {
      ret = FT_posix_memalign(memptr, alignment, size);
      GetLogger()->Add(LogEntry(logger::libc_id, FTID_posix_memalign, kNeedPushed, ret, (uint64_t)(*memptr)));
    }
//    entry->Print();
  }

  LOGGING_EPILOG(posix_memalign);
  return ret;
}
