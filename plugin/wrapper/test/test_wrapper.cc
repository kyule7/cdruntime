#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "logging.h"
using namespace log;

int main() {
  Init();
  printf("--- Test Begin ---\n\n");
  int *malloc_p = (int *)malloc(128);
  printf("[After] malloc        : %p\n",  malloc_p);
  int *calloc_p = (int *)calloc(16, 32);
  printf("[After] calloc        : %p\n",  calloc_p);
  int *valloc_p = (int *)valloc(256);
  printf("[After] valloc        : %p\n",  valloc_p);
  int *realloc_p = (int *)realloc(malloc_p, 512);
  printf("[After] realloc       : %p\n",  realloc_p);
  int *memalign_p = (int *)memalign(512, 32);
  printf("[After] memalign      : %p\n",  memalign_p);
  void *ptr = NULL;
  int ret = posix_memalign(&ptr, 512, 64); 
  printf("[After] posix_memalign: %p\n",  ptr);
  printf("[Before] free          : %p\n",  realloc_p);
  free(realloc_p);
  printf("[After] free realloc  : %p\n",  realloc_p);
  free(calloc_p);
  printf("[After] free calloc   : %p\n",  calloc_p);
  free(valloc_p);
  printf("[After] free valloc   : %p\n",  valloc_p);

  replaying = 1;

  printf("\n--- Reply Begin ---\n\n");
  int *malloc_q = (int *)malloc(128);
  printf("[After] malloc        : %p\n",  malloc_q);
  int *calloc_q = (int *)calloc(16, 32);
  printf("[After] calloc        : %p\n",  calloc_q);
  int *valloc_q = (int *)valloc(256);
  printf("[After] valloc        : %p\n",  valloc_q);
  int *realloc_q = (int *)realloc(malloc_q, 512);
  printf("[After] realloc       : %p\n",  realloc_q);
  int *memalign_q = (int *)memalign(512, 32);
  printf("[After] memalign      : %p\n",  memalign_q);
  void *ptr2 = NULL;
  int ret2 = posix_memalign(&ptr2, 512, 64); 
  printf("[After] posix_memalign: %p\n",  ptr2);
  printf("[Before] free          : %p\n",  realloc_q);
  free(realloc_p);
  printf("[After] free realloc  : %p\n",  realloc_q);
  free(calloc_p);
  printf("[After] free calloc   : %p\n",  calloc_q);
  free(valloc_p);
  printf("[After] free valloc   : %p\n",  valloc_q);
  printf("\n ----- End -----\n\n");

  Fini();
  return 0;
}
