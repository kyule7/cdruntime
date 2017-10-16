#ifndef _DEFINE_H
#define _DEFINE_H

#include <cassert>
#include <cstdio>
#include <cstdint>
#include "packer_common.h"
//#define _DEBUG_ENABLED


#define MEGABYTE        1048576
#define GIGABYTE     1073741824 
#define TWO_GIGABYTE 2147483648
#define FOUR_GIGABYTE 4294967296
#define INVALID_NUM     0xFFFFFFFFFFFFFFFF
#define TABLE_ID_OFFSET 0xFFFFFFFF00000000
#define CHUNK_ALIGNMENT 512
#define DEFAULT_BASE_FILEPATH "."
//#ifdef _DEBUG_0402        

#define ERROR_MESSAGE_PACKER(...) \
  { fprintf(stderr, __VA_ARGS__); fflush(stderr); assert(0); }

#define PACKER_ASSERT(...) assert(__VA_ARGS__)

#define PACKER_ASSERT_STR(COND, ...) { if((COND) == false) { printf(__VA_ARGS__); } assert(COND); }

//#define ALIGN_UP(X, Y) (((X) & ((Y)-1) > 0)? ((X) & ~((Y)-1)) + (Y) : ((X) & ~((Y)-1)))

namespace packer {
extern uint64_t table_id; 
extern int packerTaskID;
extern int packerTaskSize;
class BaseTable;

inline bool align_check(char *ptr, uint64_t mask=CHUNK_ALIGNMENT) {
  return (((uint64_t)ptr & (mask-1)) == 0);
}

inline uint64_t align_up(uint64_t num, uint64_t mask=CHUNK_ALIGNMENT) {
  uint64_t aligned_down = num & ~(mask-1);
  uint64_t remain = num & (mask-1);
  if(remain > 0) {
    aligned_down += mask;
  }
  return aligned_down;
}

inline uint64_t align_down(uint64_t num, uint64_t mask=CHUNK_ALIGNMENT) {
  return (num & ~(mask-1));
}

BaseTable *GetTable(uint32_t entry_type, char *ptr_entry, uint32_t len_in_byte);
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
  kGrowingMode    =0x00,
  kConcurrent     =0x01,
  kBoundedMode    =0x02,
  kEagerWriting   =0x04,
  kBufferFreed    =0x08,
  kReadMode       =0x10,
  kBufferReserved1=0x20,
  kBufferReserved2=0x40,
  kBufferReserved3=0x80
};

//struct MagicStoreEntry {
//  uint64_t total_size_;
//  uint64_t table_offset_;
//  uint32_t entry_type_;
//  uint32_t reserved_;
//};

struct MagicStore {
  uint64_t total_size_;
  uint64_t table_offset_;
  uint32_t entry_type_;
  uint32_t reserved_;
  uint64_t reserved2_; // 32 B
  char pad_[480];
  MagicStore(void);
  MagicStore(uint64_t total_size, uint64_t table_offset=0, uint32_t entry_type=1);
  MagicStore(const MagicStore &that);
  void Print(void);
} __attribute__((aligned(CHUNK_ALIGNMENT)));

} // namespace packer ends


extern FILE *packer_stream;
#ifdef _DEBUG_ENABLED
#include <map>
#include <string>
#include <cstdarg>
extern std::map<pthread_t, unsigned int> tid2str;
extern int indent_cnt; 

static inline 
int packer_debug_trace(FILE *stream, 
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

#define PACKER_DEBUG_TRACE_INFO(stream, indent, ...) \
  packer_debug_trace(stream, indent, __FUNCTION__, __LINE__, __VA_ARGS__);

#define MYDBG(...) { \
  pthread_t cur_thread = pthread_self(); \
  if(tid2str.find(cur_thread) == tid2str.end()) { \
    tid2str[cur_thread] = indent_cnt; \
    indent_cnt += 4; \
  } \
  std::string indent(tid2str[cur_thread], '\t'); \
  PACKER_DEBUG_TRACE_INFO(packer_stream, indent.c_str(), __VA_ARGS__); \
}

#else

#define MYDBG(...) 

#endif

#define CHECK_POSIX_FUNC(STR, X) { \
    if( (X) < 0 ) { \
      perror(STR); \
    }}


#endif
