#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#include <cstdio>
#include <cstdint>
#include <cstdlib>
extern FILE *dbgfp;

void PrintArray(char *ori, uint64_t intsize, FILE *out=stdout) 
{
  intsize /= sizeof(int);
  for(uint64_t i=0; i<intsize/16; i++) {
    for(uint64_t j=0; j<16; j++) {
      fprintf(out, "%5i ", ((int*)ori)[i*16 + j]);
    }
    fprintf(out, "\n");
  }
//  fprintf(out, "\n");
}
uint64_t CompareResult(char *ori, char *prv, uint64_t size) 
{
  uint64_t ret = 0;
  for(uint64_t i=0; i<size; i++) {
    if(ori[i] != prv[i]) { ret = i; break; }
  }
  uint64_t intsize = size/sizeof(int);

  fprintf(dbgfp, "#####################################\n");
  if(ret != 0) 
  {
    for(uint64_t i=0; i<intsize/16; i++) {
      for(uint64_t j=0; j<16; j++) {
        fprintf(dbgfp, "%5i ", ((int*)ori)[i*16 + j]);
      }
      fprintf(dbgfp, "\n");
    }
    fprintf(dbgfp, "\n");
  
    fprintf(dbgfp, "#####################################\n");
    for(uint64_t i=0; i<intsize/16; i++) {
      for(uint64_t j=0; j<16; j++) {
        fprintf(dbgfp, "%5i ", ((int*)prv)[i*16 + j]);
      }
      fprintf(dbgfp, "\n");
    }
    fprintf(dbgfp, "\n");
  }
  fflush(dbgfp);
  return ret;
}

#endif
