#ifndef _DEFINE_H
#define _DEFINE_H

#include <cassert>
#include <cstdio>
#include <cstdint>

#define _DEBUG_ENABLED


#define MEGABYTE        1048576
#define GIGABYTE     1073741824 
#define TWO_GIGABYTE 2147483648
#define FOUR_GIGABYTE 4294967296
#define DEFAULT_BASE_FILEPATH "./"
#define ERROR_MESSAGE(...) \
  { fprintf(stderr, __VA_ARGS__); fflush(stderr); assert(0); }

#define ASSERT(...) assert(__VA_ARGS__)

#define MYASSERT(COND, ...) { if((COND) == false) { printf(__VA_ARGS__); } assert(COND); }


enum CDErrType {
  kOK = 0,
  /* Packer Error Types */
  kAlreadyAllocated,
  kMallocFailed,
  kAlreadyFree,
  kNotFound,
  kOutOfTable,
  /* FileHandle Error Types */
  kErrorFileWrite,
  kErrorFileSeek,
  CDErrCnt
};

enum {
  kGrowingMode=0x00,
  kConcurrent=0x01,
  kBoundedMode=0x02,
  kEagerWriting=0x04,
  kBufferFreed=0x08
};

enum {
  kVolatile  = 0x000, // 0x0X00
  kPosixFile = 0x100,
  kAIOFile   = 0x200,
  kMPIFile   = 0x400
};

struct MagicStore {
  uint64_t total_size_;
  uint64_t table_offset_;
  uint32_t entry_type_;
  uint32_t reserved_;
  uint64_t reserved2_; // 32 B
  char pad_[480];
  MagicStore(void);
  MagicStore(uint64_t total_size, uint64_t table_offset=0, uint32_t entry_type=0);
  void Print(void);
};
//#define _DEBUG_ENABLED

#ifdef _DEBUG_ENABLED

#include <map>
#include <string>
#include <cstdarg>
extern std::map<pthread_t, unsigned int> tid2str;
extern int indent_cnt; 

static inline 
int cd_debug_trace(FILE *stream, 
                   const char *indent,
                   const char *function, int line_num,
                   const char *fmt, ...)
{
    int bytes;
    va_list argp;
    bytes = fprintf(stream, "%s[%s:%d] ", indent, function, line_num);
    va_start(argp, fmt);
    bytes += vfprintf(stream, fmt, argp);
    va_end(argp);
    fflush(stream);
    return bytes;
}

#define CD_DEBUG_TRACE_INFO(stream, indent, ...) \
  cd_debug_trace(stream, indent, __FUNCTION__, __LINE__, __VA_ARGS__);

#define MYDBG(...) { \
  pthread_t cur_thread = pthread_self(); \
  if(tid2str.find(cur_thread) == tid2str.end()) { \
    tid2str[cur_thread] = indent_cnt; \
    indent_cnt += 4; \
  } \
  std::string indent(tid2str[cur_thread], '\t'); \
  CD_DEBUG_TRACE_INFO(stdout, indent.c_str(), __VA_ARGS__); \
}

#else

#define MYDBG(...) 

#endif

#endif
