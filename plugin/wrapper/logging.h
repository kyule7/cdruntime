#ifndef _LOGGING_H
#define _LOGGING_H
/**********************
 *
 * logging.h file is for declaration for internal libc logging.
 *
 * LibC Logging List
  malloc
  calloc
  valloc
  realloc
  memalign
  posix_memalign
  free
*/
#include <stdint.h>
//#include "libc_wrapper.h"
#define EXTERNC extern 
#ifdef _DEBUG
# define STOPHERE getchar()
#else
# define STOPHERE
#endif
namespace logger {

//enum FTID {
//  FTID_invalid = 0,
//  FTID_malloc = 1,
//  FTID_calloc = 2,
//  FTID_valloc = 3,
//  FTID_realloc = 4,
//  FTID_memalign = 5,
//  FTID_posix_memalign = 6,
//  FTID_free = 7, 
//  FTIDNums
//};
//
//enum {
//  kNeedFreed = 0x100,
//  kNeedPushed = 0x200,
//};

void InitMallocPtr(void);
//extern void Init(void);
//extern void Fini(void);

//extern char ft2str[FTIDNums][64];
//extern bool disabled;
//extern bool replaying;
//extern uint64_t gen_ftid;
extern bool initialized;
extern bool init_calloc;

//inline uint64_t GenID(enum FTID ftid) {
////  return ((ftid << 48) | (gen_ftid++));
//  return (gen_ftid++);
//}

//inline uint64_t CheckID(uint64_t id) {
//  return (id >> 16);
//}
//inline uint64_t CheckType(uint64_t id) {
//  return (id & 0xFFFF);
//}

} // namespace logger ends

/*
#define INIT_FUNCPTR(func) \
  FT_##func = (FType_##func)dlsym(RTLD_NEXT, #func)
#define DEFINE_FUNCPTR(func) \
  FType_##func FT_##func = NULL
*/


#define DEFINE_FUNCPTR(func) \
  __typeof__(&func) FT_##func = NULL

#define CHECK_INIT(func) \
  if(FT_##func == NULL) { \
    INIT_FUNCPTR(func); \
    if(logger::initialized == false) logger::InitMallocPtr(); \
    assert(FT_##func); \
    assert(logger::initialized); \
  }
/*
#define LOGGING_PROLOG(func, ...) \
  CHECK_INIT(func) \
  if(logger::disabled) { \
    printf("Logging Disabled %s\n", ft2str[(FTID_##func)]); \
    return FT_##func(__VA_ARGS__); \
  } else { \
    logger::disabled = true; \
    printf("Logging Begin %d %s\n", (FTID_##func), ft2str[(FTID_##func)]); 

#define LOGGING_EPILOG(func) \
    printf("Logging End   %d %s\n", (FTID_##func), ft2str[(FTID_##func)]); \
    logger::gen_ftid++; \
    logger::disabled = false; \
  }
*/
//#define LOGGER_PRINT(...) printf(__VA_ARGS__)
//#define LOGGER_PRINT(...)


#endif
