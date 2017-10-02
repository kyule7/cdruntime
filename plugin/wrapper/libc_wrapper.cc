#include "logging.h"
#include "libc_wrapper.h"
#include "singleton.h"
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>

using namespace logger;
LogPacker *LogPacker::libc_logger = NULL;
uint64_t LogEntry::gen_ftid = 0;
bool logger::replaying = false;
bool logger::disabled  = true;
bool logger::initialized = false;
bool logger::init_calloc = false;

logger::Singleton logger::singleton;
//packer::Packer<LogEntry> serdes;

static char local_buf[4096] __attribute__ ((aligned (4096)));
static uint32_t local_buf_offset = 0;

void logger::Init(void)
{
  logger::disabled = false;
}

void logger::Fini(void)
{
  logger::replaying = false;
  logger::disabled = true;
}

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
bool LogPacker::IsLogFound(void) 
{
  bool orig_disabled = logger::disabled;
  logger::disabled = true;
  uint64_t current_log_id = LogEntry::gen_ftid;
  LogEntry *entry = table_->FindReverse(current_log_id, current_log_id);
  printf("### Is Pushed Log? %s\n", (entry != NULL)? "True" : "False"); getchar();
  logger::disabled = orig_disabled;;
  return entry != NULL;
}

// Find an entry with kNeedPushed from the offset, 
// and copy the entry at the dst offset to the offset.
// return the dst offset for the next search.
uint64_t LogPacker::PushLogs(uint64_t offset_from) 
{
  bool orig_disabled = logger::disabled;
  logger::disabled = true;
  uint64_t orig_tail = table_->tail();
  uint64_t new_tail = table_->FindWithAttr(kNeedPushed, offset_from);
  printf("Pushed %lu entries (orig: %lu entries)\n", new_tail - offset_from, orig_tail);
  logger::disabled = orig_disabled;;
  return new_tail;
}

uint64_t LogPacker::FreeMemory(uint64_t offset_from) 
{
  bool orig_disabled = logger::disabled;
  logger::disabled = true;
  uint64_t freed_cnt = 0;
  {
  //packer::Packer<LogEntry> free_list;
  LogPacker free_list;
  freed_cnt = table_->FindWithAttr(kNeedFreed, offset_from, free_list.table_);
  Print();
  printf("FreeMemory %ld == %lu entries from offset %lu\n", free_list.table_->used(), freed_cnt, offset_from);
  free_list.Print();
  getchar();
  for(uint32_t i=0; i<freed_cnt; i++) {
    printf("i:%u\n", i);
    LogEntry *entry = free_list.table_->GetAt(i);
    entry->Print();
    FT_free((void *)(entry->offset()));
    entry->size_.Unset(kNeedFreed);
  }

  }
  logger::disabled = orig_disabled;;
  return freed_cnt;
}

//packer::Packer<LogEntry> *GetLogger(void) { return &serdes; }
LogPacker *logger::GetLogger(void)
{
  if(LogPacker::libc_logger == NULL) LogPacker::libc_logger = new LogPacker;
  return LogPacker::libc_logger;
}
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
    INIT_FUNCPTR(calloc);
    INIT_FUNCPTR(malloc);
    INIT_FUNCPTR(valloc);
    INIT_FUNCPTR(realloc);
    INIT_FUNCPTR(memalign);
    INIT_FUNCPTR(posix_memalign);
    INIT_FUNCPTR(free);
    LOGGER_PRINT("Initialized\n");
    GetLogger();
    logger::initialized = true;
  }
}



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
EXTERNC void *calloc2(size_t numElem, size_t size){ return NULL; }
EXTERNC void *calloc(size_t numElem, size_t size)
{ 
  void *ret = NULL;
  //LOGGING_PROLOG(calloc, numElem, size);
  LOGGER_PRINT("calloc(%zu,%zu) wrapped\n", numElem, size); getchar();
  
  if(logger::disabled) { 
    if(logger::init_calloc == true) {
      LOGGER_PRINT("XXX Logging Disabled %s\n", ft2str[(FTID_calloc)]); 
      ret = FT_calloc(numElem, size); 
    } else {
      LOGGER_PRINT("XXX Not yet initialize calloc(%zu) %s\n", numElem * size, ft2str[(FTID_calloc)]); 
      ret = local_buf + local_buf_offset;
      local_buf_offset += numElem * size;
      logger::init_calloc = true; 
    }
  } 
  else { 
    logger::disabled = true; 
    LOGGER_PRINT("\n>>> Logging Begin %lu %s\n", logger::LogEntry::gen_ftid, ft2str[(FTID_calloc)]); 
    //if(logger::replaying == 0) { 
    if(logger::replaying == false || GetLogger()->IsLogFound() == false) { 
      ret = FT_calloc(numElem, size);
      GetLogger()->Add(LogEntry(logger::LogEntry::gen_ftid, FTID_calloc, kNeedPushed, (uint64_t)ret));
    } else {
      LOGGER_PRINT("Replaying %s\n", __func__); 
      LogEntry *entry = GetLogger()->GetNext();
      ret = (void *)entry->offset_;
      assert(entry->ftype_ == FTID_calloc);
      entry->Print();
    }
    LOGGER_PRINT("<<< Logging End  %lu %s\n", logger::LogEntry::gen_ftid, ft2str[(FTID_calloc)]); 
    logger::LogEntry::gen_ftid++;
    logger::disabled = false; 
  }
  LOGGER_PRINT("%p = calloc(%zu,%zu) wrapped\n", ret, numElem, size);
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
 */
EXTERNC void *malloc(size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(malloc, size);
  //if(logger::replaying == 0) { 
  if(logger::replaying == false || GetLogger()->IsLogFound() == false) { 
    ret = FT_malloc(size);
    GetLogger()->Add(LogEntry(logger::LogEntry::gen_ftid, FTID_malloc, kNeedPushed, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
//    if(logger::recreated) {
//            
//    } else {
//
//    }
      LogEntry *entry = GetLogger()->GetNext();
      ret = (void *)entry->offset_;
      assert(entry->ftype_ == FTID_malloc);
      entry->Print();
  }
  LOGGING_EPILOG(malloc);
  LOGGER_PRINT("%p = malloc(%zu)\n", ret, size);
  return ret;
  
}

EXTERNC void *valloc(size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(valloc, size);
//  bool need_replay = false;
//  if(logger::replaying) {
//    // if it is not pushed, it should be regenerated by calling real func
//    // if it is pushed, just replay it.
//    // If it is not recreated (in the scope of begin/complete),
//    // it should be always replayed.
//    // No escalation always return true, because it finds the log entry.
//    need_replay = GetLogger()->IsLogFound();
////    if(logger::recreated = false) {
////      need_replay = true;
////    } else {
////      need_replay = IsLogFound();
////    }
////    printf("### need_replay = %d, replaying: %d\n", need_replay, replaying);
//  }
//  //printf("### need_replay = %d, replaying: %d\n", need_replay, replaying);
  if(logger::replaying == false || GetLogger()->IsLogFound() == false) { 
  //if(logger::replaying == 0) { 
    ret = FT_valloc(size);
    GetLogger()->Add(LogEntry(logger::LogEntry::gen_ftid, FTID_valloc, kNeedPushed, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    ret = (void *)entry->offset_;
    assert(entry->ftype_ == FTID_valloc);
    entry->Print();
  }
  LOGGING_EPILOG(valloc);
  LOGGER_PRINT("%p = valloc(%zu)\n", ret, size);
  return ret;
}


EXTERNC void *realloc(void *ptr, size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(realloc, ptr, size);
  //if(logger::replaying == 0) { 
  if(logger::replaying == false || GetLogger()->IsLogFound() == false) {
    // it is difficult to hook some internal free() call inside realloc. 
    // FT_realloc() is never called, but malloc is called instead for
    // reallocation. free() is deferred to FreeMemory().
    ret = FT_malloc(size);
    GetLogger()->Add(LogEntry(logger::LogEntry::gen_ftid, FTID_realloc, kNeedPushed, (uint64_t)ret));
    GetLogger()->table_->GetLast()->Print();
    uint32_t idx = 0;
    LogEntry *entry = NULL;
    while(entry == NULL) {
      entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
      if(idx == -1U) { assert(0); }
    }
    entry->size_.Unset(kNeedPushed);
    entry->size_.Set(kNeedFreed);
    entry->Print();
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    ret = (void *)entry->offset_;
    printf("[%lu, %lu, %lu, %lx] %d == %d\n", entry->id_, entry->attr(), entry->size(), entry->offset(), entry->ftype_ , FTID_realloc);
    assert(entry->ftype_ == FTID_realloc);
    entry->Print();
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
  if(logger::replaying == 0) { 
    ret = FT_memalign(boundary, size);
    GetLogger()->Add(LogEntry(logger::LogEntry::gen_ftid, FTID_memalign, kNeedPushed, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    ret = (void *)entry->offset_;
    assert(entry->ftype_ == FTID_memalign);
    entry->Print();
  }
  LOGGING_EPILOG(memalign);
  return ret;
}


EXTERNC int posix_memalign(void **memptr, size_t alignment, size_t size)
{
  int ret = -1;
  LOGGING_PROLOG(posix_memalign, memptr, alignment, size);

  if(logger::replaying == 0) { 
    ret = FT_posix_memalign(memptr, alignment, size);
    GetLogger()->Add(LogEntry(logger::LogEntry::gen_ftid, FTID_posix_memalign, kNeedPushed, ret, (uint64_t)(*memptr)));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    LogEntry *entry = GetLogger()->GetNext();
    *memptr = (void *)entry->offset_;
    ret = entry->size();
    assert(entry->ftype_ == FTID_posix_memalign);
    entry->Print();
  }

  LOGGING_EPILOG(posix_memalign);
  return ret;
}


EXTERNC void free(void *ptr)
{
  LOGGING_PROLOG(free, ptr);
  if(((uint64_t)ptr >> 12) == ((uint64_t)local_buf >> 12)) {LOGGER_PRINT("skip this free\n"); getchar(); }
  if(logger::replaying == 0) {
    LOGGER_PRINT("Executing %s(%p), disabled:%d\n", __func__, ptr, logger::disabled); 
    uint32_t idx = 0;
    LogEntry *entry = NULL;
    while(entry == NULL) {
      entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
//      if(idx == -1U) { assert(0); }
    }
    entry->size_.Unset(kNeedPushed);
    entry->size_.Set(kNeedFreed);
    entry->Print();
  } else {
//    LOGGER_PRINT("Replaying %s(%p)\n", __func__, ptr); 
    uint32_t idx = 0;
    LogEntry *entry = NULL;
    while(entry == NULL) {
      entry = GetLogger()->table_->FindWithOffset((uint64_t)ptr, idx);
      if(idx == -1U) { LOGGER_PRINT("free %p is not founded in malloc list\n", ptr); break; }
    }
    if(entry != NULL) 
    { LOGGER_PRINT("Replaying %s(%p), freed? %lx\n", __func__, ptr, entry->attr()); }
    entry->Print();
    //getchar();
  }
  LOGGING_EPILOG(free);
}

