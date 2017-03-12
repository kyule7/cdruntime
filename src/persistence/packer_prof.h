#ifndef _PACKER_PROF
#define _PACKER_PROF

#include <sys/time.h>
#include <stdint.h>
#include <stddef.h>

namespace packer {

struct Time { 
  char name[16];
  struct timeval begin;
  struct timeval end;
  double elapsed;
  uint64_t size;
  uint64_t count;
  Time(const char *str=NULL);
  ~Time(void);
  void Init(const char *str=NULL);
  double GetAvgChunk(void) const { return (double)size/count; }
  double GetBW(void)       const { return (double)size/elapsed * 0.000001; }
  inline void Begin(uint64_t len=0) { size += len; gettimeofday(&begin, NULL); }
  inline void End(uint64_t len=0) { 
    gettimeofday(&end, NULL); 
    elapsed += (double)(end.tv_sec - begin.tv_sec) + (double)(end.tv_usec - begin.tv_usec)*0.000001;
    count++;
    size += len;
  }
  void Print(void);
};

extern Time time_write; 
extern Time time_read; 
extern Time time_posix_write; 
extern Time time_posix_read; 
extern Time time_posix_seek; 
extern Time time_mpiio_write; 
extern Time time_mpiio_read; 
extern Time time_mpiio_seek; 

}

/*
//#define PACKER_BEGIN_CLK(X) gettimeofday(&(X), NULL);
//#define PACKER_END_CLK(Z,X,Y) { \
//  gettimeofday(&(Y), NULL); \
//  Z += (double)((Y).tv_sec - (X).tv_sec) + (double)((Y).tv_usec - (X).tv_usec)*0.000001; \
//}
*/


#endif
