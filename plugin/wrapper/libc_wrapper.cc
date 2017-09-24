#include "logging.h"
#include "libc_wrapper.h"
#include "packer.hpp"
#include "singleton.h"
#include <dlfcn.h>
#include <assert.h>
log::Singleton log::singleton;
packer::Packer<packer::BaseEntry> serdes;
uint64_t log::gen_ftid = 0;
bool log::replaying = false;
bool log::disabled  = true;

using namespace log;

bool log::initialized = false;
bool log::init_calloc = false;

static char local_buf[4096] __attribute__ ((aligned (4096)));
static uint32_t local_buf_offset = 0;

void log::Init(void)
{
  log::disabled = false;
}

void log::Fini(void)
{
  log::replaying = false;
  log::disabled = true;
}

/**************************************************/
DEFINE_FUNCPTR(malloc);
DEFINE_FUNCPTR(calloc);
DEFINE_FUNCPTR(valloc);
DEFINE_FUNCPTR(realloc);
DEFINE_FUNCPTR(memalign);
DEFINE_FUNCPTR(posix_memalign);
DEFINE_FUNCPTR(free);

char log::ft2str[FTIDNums][64] = { "FTID_reserved"
                            , "malloc"
                            , "calloc"
                            , "valloc"
                            , "realloc"
                            , "memalign"
                            , "posix_memalign"
                            , "free"
                            };
/**************************************************/

// This must be called initially.
// Some rare libc functions are wrapped lazily,
// but some critical functions must be done beforehand.
// Please see the description about issue regarding 
// calloc/dlsym in libc_wrapper.h file
void log::InitMallocPtr(void)
{
  if(log::initialized == false) {
    LOGGER_PRINT("%s\n", __func__);
    INIT_FUNCPTR(calloc);
    INIT_FUNCPTR(malloc);
    INIT_FUNCPTR(valloc);
    INIT_FUNCPTR(realloc);
    INIT_FUNCPTR(memalign);
    INIT_FUNCPTR(posix_memalign);
    INIT_FUNCPTR(free);
    LOGGER_PRINT("Initialized\n");
    log::initialized = true;
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
void *calloc(size_t numElem, size_t size)
{ 
  void *ret = NULL;
  //LOGGING_PROLOG(calloc, numElem, size);
  LOGGER_PRINT("calloc(%zu,%zu) wrapped\n", numElem, size);
  
  if(log::disabled) { 
    if(log::init_calloc == true) {
      LOGGER_PRINT("Logging Disabled %s\n", ft2str[(FTID_calloc)]); 
      ret = FT_calloc(numElem, size); 
    } else {
      LOGGER_PRINT("Not yet initialize calloc(%zu) %s\n", numElem * size, ft2str[(FTID_calloc)]); 
      ret = local_buf + local_buf_offset;
      local_buf_offset += numElem * size;
      log::init_calloc = true; 
    }
  } 
  else { 
    log::disabled = true; 
    LOGGER_PRINT("Logging Begin %d %s\n", (FTID_calloc), ft2str[(FTID_calloc)]); 
    if(log::replaying == 0) { 
      ret = FT_calloc(numElem, size);
      serdes.Add(packer::BaseEntry(GenID(FTID_calloc), 0, (uint64_t)ret));
    } else {
      LOGGER_PRINT("Replaying %s\n", __func__); 
      packer::BaseEntry *entry = serdes.GetNext();
      ret = (void *)entry->offset_;
      assert(CheckType(entry->id_) == FTID_calloc);
    }
    LOGGER_PRINT("Logging End   %d %s\n", (FTID_calloc), ft2str[(FTID_calloc)]); 
    log::disabled = false; 
  }
  LOGGER_PRINT("%p = calloc(%zu,%zu) wrapped\n", ret, numElem, size);
  //LOGGING_EPILOG(calloc);
  return ret;

}

void *malloc(size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(malloc, size);
  if(log::replaying == 0) { 
    ret = FT_malloc(size);
    serdes.Add(packer::BaseEntry(GenID(FTID_malloc), 0, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    packer::BaseEntry *entry = serdes.GetNext();
    ret = (void *)entry->offset_;
    assert(CheckType(entry->id_) == FTID_malloc);
  }
  LOGGING_EPILOG(malloc);
  LOGGER_PRINT("%p = malloc(%zu)\n", ret, size);
  return ret;
  
}

void *valloc(size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(valloc, size);
  if(log::replaying == 0) { 
    ret = FT_valloc(size);
    serdes.Add(packer::BaseEntry(GenID(FTID_valloc), 0, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    packer::BaseEntry *entry = serdes.GetNext();
    ret = (void *)entry->offset_;
    assert(CheckType(entry->id_) == FTID_valloc);
  }
  LOGGING_EPILOG(valloc);
  LOGGER_PRINT("%p = valloc(%zu)\n", ret, size);
  return ret;
}


void *realloc(void *ptr, size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(realloc, ptr, size);
  if(log::replaying == 0) { 
    ret = FT_realloc(ptr, size);
    serdes.Add(packer::BaseEntry(GenID(FTID_realloc), 0, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    packer::BaseEntry *entry = serdes.GetNext();
    ret = (void *)entry->offset_;
    assert(CheckType(entry->id_) == FTID_realloc);
  }
  LOGGING_EPILOG(realloc);
  LOGGER_PRINT("%p = realloc(%p, %zu)\n", ret, ptr, size);
  return ret;
}


void *memalign(size_t boundary, size_t size)
{
  void *ret = NULL;
  LOGGING_PROLOG(memalign, boundary, size);
  ret = FT_memalign(boundary, size);
  if(log::replaying == 0) { 
    ret = FT_memalign(boundary, size);
    serdes.Add(packer::BaseEntry(GenID(FTID_memalign), 0, (uint64_t)ret));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    packer::BaseEntry *entry = serdes.GetNext();
    ret = (void *)entry->offset_;
    assert(CheckType(entry->id_) == FTID_memalign);
  }
  LOGGING_EPILOG(memalign);
  return ret;
}


int posix_memalign(void **memptr, size_t alignment, size_t size)
{
  int ret = -1;
  LOGGING_PROLOG(posix_memalign, memptr, alignment, size);

  if(log::replaying == 0) { 
    ret = FT_posix_memalign(memptr, alignment, size);
    serdes.Add(packer::BaseEntry(GenID(FTID_posix_memalign), ret, (uint64_t)(*memptr)));
  } else {
    LOGGER_PRINT("Replaying %s\n", __func__); 
    packer::BaseEntry *entry = serdes.GetNext();
    *memptr = (void *)entry->offset_;
    ret = entry->size();
    assert(CheckType(entry->id_) == FTID_posix_memalign);
  }

  LOGGING_EPILOG(posix_memalign);
  return ret;
}


void free(void *ptr)
{
  LOGGING_PROLOG(free, ptr);
  if(((uint64_t)ptr >> 12) == ((uint64_t)local_buf >> 12)) {LOGGER_PRINT("skip this free\n"); getchar(); }
  if(log::replaying == 0) {
    LOGGER_PRINT("Executing %s(%p)\n", __func__, ptr); 
    uint32_t idx = 0;
    packer::BaseEntry *entry = NULL;
    while(entry == NULL) {
      entry = serdes.table_->FindWithOffset((uint64_t)ptr, idx);
      if(idx == -1U) { assert(0); }
    }
    entry->size_.code_ |= kFreed;
  } else {
    LOGGER_PRINT("Replaying %s(%p)\n", __func__, ptr); 
    uint32_t idx = 0;
    packer::BaseEntry *entry = NULL;
    while(entry == NULL) {
      entry = serdes.table_->FindWithOffset((uint64_t)ptr, idx);
      if(idx == -1U) { LOGGER_PRINT("free %p is not founded in malloc list\n", ptr); break; }
    }
    if(entry != NULL) 
    { LOGGER_PRINT("Replaying %s(%p), freed? %lu\n", __func__, ptr, entry->size_.code_); }
    getchar();
  }
  LOGGING_EPILOG(free);
}

