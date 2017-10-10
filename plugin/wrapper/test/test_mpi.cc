#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "libc_wrapper.h"
#include <mpi.h>
using namespace logger;
uint64_t begin1 = 0;
uint64_t begin2 = 0;
uint64_t begin3 = 0;
uint64_t end1 = 0;
uint64_t end2 = 0;
uint64_t end3 = 0;
uint64_t upto1 = 0;
uint64_t upto2 = 0;
uint64_t upto3 = 0;

/**
 * If LogPacker is integrated into CD runtime,
 * At Begin point, CD runtime records offset of LogPacker and ftid.
 * 
 * @ Begin, records log sequence ID, and LogPacker tail_ on execution.
 *          restores the log ID, and sets replaying flag.
 * @ Complete, FreeMemory(from_log_begin_), 
 *             then PushLogs(from_log_begin_) to parent's from_log_end_
 *
 */

void Test1(bool replay) {
  if(replay == false) {
    upto1 = GetLogger()->Set(begin1);
    printf("begin1 : %lu\n", begin1);
  } else { 
    GetLogger()->Reset(upto1);
  }

  { // Root CD
    int *malloc_p = (int *)malloc(128);
    int *calloc_p = (int *)calloc(16, 32);
    int *valloc_p = (int *)valloc(256);
    int *realloc_p = (int *)realloc(malloc_p, 512);
  
    if(replay == false) {
      GetLogger()->Print();
      end1 = GetLogger()->PushLogs(begin1);
      printf("begin1 : %lu ~ %lu\n", begin1, end1);
      upto2 = GetLogger()->Set(begin2);
    }
    int *memalign_p = NULL;
    void *ptr = NULL;
    { // CD1
      int *malloc_p2 = (int *)malloc(128);
      int *calloc_p2 = (int *)calloc(16, 32);
      int *valloc_p2 = (int *)valloc(256);
      int *realloc_p2 = (int *)realloc(malloc_p2, 512);
      free(calloc_p2);
      free(valloc_p2);
      if(replay == false) {
        GetLogger()->Print();
        end2 = GetLogger()->PushLogs(begin2);
        printf("begin2 : %lu ~ %lu\n", begin2, end2);
        upto3 = GetLogger()->Set(begin3);
      }

      // CD2
      memalign_p = (int *)memalign(512, 32);
      int ret = posix_memalign(&ptr, 512, 64); 
  
      free(realloc_p2);

      if(replay == false) {
        GetLogger()->Print();
        end3 = GetLogger()->PushLogs(begin3); // delete the entry for realloc
        printf("begin3 : %lu ~ %lu\n", begin3, end3);
      }
    }

    free(realloc_p);
    free(calloc_p);
    free(valloc_p);
//    if(replay == false) {
//      begin1 = GetLogger()->PushLogs(begin1); // only memalign_p and ptr remain after it.
//      printf("begin1 : %lu\n", begin1);
//    }
    free(memalign_p);
    free(ptr);
    if(replay == false) {
      GetLogger()->FreeMemory(begin2); // should free realloc_p2, realloc_p, calloc_p, valloc_p
    }
  
  }
}



int main(int argc, char *argv[]) {
  Init();
  MPI_Init(&argc, &argv);
  printf("--- Test Begin ---\n\n");
  printf("starting ftid: %lu\n", GetLogger()->GetNextID());
  Test1(false);
  printf("\n--- Replaying ---\n\n");
  printf("starting ftid: %lu\n", GetLogger()->GetNextID()); getchar();
  Test1(true);
  printf("\n ----- End -----\n\n");

  MPI_Finalize();
  Fini();
//  GetLogger()->FreeMemory(begin2);
  return 0;
}



#if 0
  uint64_t begin1 = 0;
  uint64_t upto1 = GetLogger()->Set(begin1);
  printf("--- Test Begin ---\n\n");
  int *malloc_p = (int *)malloc(128);
  int *calloc_p = (int *)calloc(16, 32);
  int *valloc_p = (int *)valloc(256);
  int *realloc_p = (int *)realloc(malloc_p, 512);

  GetLogger()->PushLogs(begin1);
  printf("--- Test Begin ---\n\n");

  uint64_t begin2 = 0;
  uint64_t upto2 = GetLogger()->Set(begin2);
  int *malloc_p2 = (int *)malloc(128);
  int *calloc_p2 = (int *)calloc(16, 32);
  int *valloc_p2 = (int *)valloc(256);
  int *realloc_p2 = (int *)realloc(malloc_p2, 512);

  GetLogger()->PushLogs(begin2);

  int *memalign_p = (int *)memalign(512, 32);
  void *ptr = NULL;
  int ret = posix_memalign(&ptr, 512, 64); 
  free(realloc_p);
  free(calloc_p);
  free(valloc_p);
  free(memalign_p);
  free(ptr);

  free(realloc_p2);
  free(calloc_p2);
  free(valloc_p2);

  // Reset is called at Begin in kReexecution
  GetLogger()->Reset(upto1);

  printf("\n--- Reply Begin ---\n\n");
  int *malloc_q = (int *)malloc(128);
  int *calloc_q = (int *)calloc(16, 32);
  int *valloc_q = (int *)valloc(256);
  int *realloc_q = (int *)realloc(malloc_q, 512);
  int *memalign_q = (int *)memalign(512, 32);
  void *ptr2 = NULL;
  int ret2 = posix_memalign(&ptr2, 512, 64); 
  free(realloc_q);
  free(calloc_q);
  free(valloc_q);
  free(memalign_q);
  free(ptr2);
  printf("\n ----- End -----\n\n");
#endif
