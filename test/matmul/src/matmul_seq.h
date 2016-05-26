#include "util.h"
#include <omp.h>
// (coldim x N) x (N x rowdim)
static inline
void SEQ_MatMatMul(FLOAT *A, FLOAT *B, FLOAT *C, int coldim, int rowdim, int tmpdim) {
  FLOAT tstart = omp_get_wtime();
  for(int i=0; i<rowdim; i++) {
    for(int j=0; j<coldim; j++) {
      FLOAT temp = 0.0;
      int tmpdim_x_j = tmpdim * j;
      for(int k=0; k<tmpdim; k++) {
        temp += A[tmpdim_x_j + k] * B[rowdim*k + i];   // Summate (j,k) x (k,i)
      }
      C[rowdim*j + i] = temp; // (j,i) = temp;
    }
  }
  FLOAT elapsed = omp_get_wtime() - tstart;
  PRINT("[Sequential ] Elapsed time : %lf\n", elapsed);
}
