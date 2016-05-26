#include <omp.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"

static inline
void OMP_MatMatMul(FLOAT *A, FLOAT *B, FLOAT *C, int coldim, int rowdim, int tmpdim) {
  int i=0, j=0, k=0, tid=-1;
  FLOAT tot_tstart = omp_get_wtime();
  FLOAT max_elapsed = 0.0;
  FLOAT min_elapsed = 99999999999;
  int tot_threads = 0;
  FLOAT avg_elapsed = 0.0;
  #pragma omp parallel private(i,j,k,tid) reduction(+:avg_elapsed) //reduction(max:max_elapsed) reduction(min:min_elapsed)
  {
    tid = omp_get_thread_num();
    tot_threads = omp_get_num_threads();
    FLOAT tstart = omp_get_wtime();
    #pragma omp for schedule(static) nowait
    for(j=0; j<coldim; j++) {
      for(i=0; i<rowdim; i++) {
        FLOAT temp = 0.0;
        int tmpdim_x_j = tmpdim * j;
        for(k=0; k<tmpdim; k++) {
          temp += A[tmpdim_x_j + k] * B[rowdim*k + i];   // Summate (j,k) x (k,i)
//          PRINT("[id:%d] (%d,%d) = (%d,%d)x(%d,%d) %f\n", tid, j, i, j, k, k, i, A[tmpdim_x_j + k] * B[rowdim*k + i]);
        }
        C[rowdim*j + i] = temp; // (j,i) = temp;
//        PRINT("[id:%d] (%d,%d) = %f\n", tid, j, i, temp);
      }
    }
    FLOAT elapsed = omp_get_wtime() - tstart;
    avg_elapsed = elapsed; // it is the same is +=
    min_elapsed = elapsed; 
    max_elapsed = elapsed; 
    PRINT("[tid:%d\t] Elapsed time : %lf\n", tid, elapsed);
  }
  FLOAT tot_elapsed = omp_get_wtime() - tot_tstart;
  PRINT("[OMP tasks %d] time = %lf (avg:%lf %lf~%lf)\n", 
         tot_threads, tot_elapsed, avg_elapsed/tot_threads, min_elapsed, max_elapsed);
}

//int main(int argc, char *argv[]) {
//  int matsize = DEFAULT_SIZE;
//  int nthreads = DEFAULT_THREADS;
//  if(argc == 2) {
//    nthreads = atoi(argv[1]);
//  }
//  else if(argc >= 3) { // (NxN) x (NxM)
//    nthreads = atoi(argv[1]);
//    matsize  = atoi(argv[2]);
//  }
//
//  int totsize = matsize * matsize;
//  FLOAT *result = (FLOAT *)malloc(totsize*sizeof(FLOAT));
//  FLOAT *readonly = (FLOAT *)malloc(totsize*sizeof(FLOAT));
//  FLOAT *test = (FLOAT *)malloc(totsize*sizeof(FLOAT));
//  omp_set_num_threads(nthreads);
//  srand(time(NULL));
//
//  // Initialize Arrays
//  InitArray(result, matsize, matsize, RANDOM);
//  InitArray(readonly, matsize, matsize, RANDOM);
//  PRINT_ARRAY(result, matsize, matsize, "\n-- matrix A ------------------\n");
//  PRINT_ARRAY(readonly, matsize, matsize, "\n-- matrix B ------------------\n");
//
//  memcpy(test, result, sizeof(FLOAT)*totsize);
//  PRINT_ARRAY(test, matsize, matsize, "\n-- matrix A ------------------\n");
//  OMP_MatMatMul(result, readonly, result, matsize, matsize, matsize); 
//  SEQ_MatMatMul(test, readonly, test, matsize, matsize, matsize); 
//
//  TEST_ARRAY(test, result, matsize, matsize);
//  PRINT_ARRAY(result, matsize, matsize, "\n-- Result ------------------\n");
//  PRINT_ARRAY(test, matsize, matsize,  "\n-- Test --------------------\n");
//  free(result);
//  free(readonly);
//  free(test);
//  return 0;
//}

