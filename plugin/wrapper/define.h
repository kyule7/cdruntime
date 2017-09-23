#pragma once
/** LibC Logging List
  malloc
  calloc
  valloc
  realloc
  memalign
  posix_memalign
  free
*/
#include <stdint.h>
namespace log {

enum FTID {
  FTID_malloc = 1,
  FTID_calloc = 2,
  FTID_valloc = 3,
  FTID_realloc = 4,
  FTID_memalign = 5,
  FTID_posix_memalign = 6,
  FTID_free = 7, 
  FTIDNums
};

void InitMallocPtr(void);
void Init(void);
void Fini(void);

extern char ft2str[FTIDNums][64];
extern int disabled;
extern int replaying;
extern uint64_t gen_ftid;
extern bool initialized;
extern bool init_calloc;

inline uint64_t GenID(enum FTID ftid) {
  return ((gen_ftid++ << 16) | ftid);
}

inline uint64_t CheckID(uint64_t id) {
  return (id >> 16);
}
inline uint64_t CheckType(uint64_t id) {
  return (id & 0xFFFF);
}

} // namespace log ends

/*
#define INIT_FUNCPTR(func) \
  FT_##func = (FType_##func)dlsym(RTLD_NEXT, #func)
#define DEFINE_FUNCPTR(func) \
  FType_##func FT_##func = NULL
*/
#define INIT_FUNCPTR(func) \
  FT_##func = (__typeof__(&func))dlsym(RTLD_NEXT, #func)

#define DEFINE_FUNCPTR(func) \
  __typeof__(&func) FT_##func = NULL

#define CHECK_INIT(func) \
  if(FT_##func == NULL) { \
    INIT_FUNCPTR(func); \
    if(log::initialized == false) log::InitMallocPtr(); \
    assert(FT_##func); \
    assert(log::initialized); \
  }

#define LOGGING_PROLOG(func, ...) \
  CHECK_INIT(func) \
  if(log::disabled == 1) { \
    printf("Logging Disabled %s\n", ft2str[(FTID_##func)]); \
    return FT_##func(__VA_ARGS__); \
  } else { \
    log::disabled = 1; \
    printf("Logging Begin %d %s\n", (FTID_##func), ft2str[(FTID_##func)]); 

#define LOGGING_EPILOG(func) \
    printf("Logging End   %d %s\n", (FTID_##func), ft2str[(FTID_##func)]); \
    log::disabled = 0; \
  }
/*
#define LOGGING_PROLOG(FTID) \
  if(log::disabled == 1) { \
    printf("Logging Disabled %s\n", ft2str[(FTID)]); \
    goto END_LIBC; \
  } else { \
    log::disabled = 1; \
    printf("Logging Begin %d %s\n", (FTID), ft2str[(FTID)]); \
  }

#define LOGGING_EPILOG(FTID) \
  printf("Logging End   %d %s\n", (FTID), ft2str[(FTID)]); \
  log::disabled = 0; \
END_LIBC: 
#define LOGGING_PROLOG(ret_type, func, ...) \
  if(log::disabled == 1) { \
    printf("Logging Disabled %s\n", ft2str[(FTID_##func)]); \
    ret = FT_##func(__VA_ARGS__); \
  } else { \
    log::disabled = 1; \
    printf("Logging Begin %d %s\n", (FTID_##func), ft2str[(FTID_##func)]); 

#define LOGGING_PROLOG2(func, ...) \
  if(log::disabled == 1) { \
    printf("Logging Disabled %s\n", ft2str[(FTID_##func)]); \
    FT_##func(__VA_ARGS__); \
  } else { \
    log::disabled = 1; \
    printf("Logging Begin %d %s\n", (FTID_##func), ft2str[(FTID_##func)]); 

#define LOGGING_EPILOG(func) \
    printf("Logging End   %d %s\n", (FTID_##func), ft2str[(FTID_##func)]); \
    log::disabled = 0; \
  }
*/

#define LOGGER_PRINT(...) fprintf(stdout, __VA_ARGS__)
