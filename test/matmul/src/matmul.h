#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>
#include <algorithm>
#include "define.h"

#define DEFAULT_CUTOFF 8
using namespace std;
unsigned long long cnt=0;
////////////////////////////////////////////////////////////////////
// row major nxk * kxm Matrix Multiplication
void OrigMatMulAcc(FLOAT *matResult, FLOAT *matA, FLOAT *matB, 
                int i_dim, int j_dim, int k_dim) {
  int i, j, k;
  for (i = 0; i < i_dim; ++i) {
    int i_dim_size = i * i_dim;
    for (j = 0; j < j_dim; ++j) {
      FLOAT sum = 0.0;
      int k_stride = j;
      for (k = 0; k < k_dim; ++k, k_stride+=k_dim) {
	      sum += matA[i_dim_size + k] *  matB[k_stride]; // matA[i,k] x matB[k,j] 
//        PRINT("%.4f = %.4f * %.4f\n", sum, a[i_dim_size + k], b[k * dim_size + j]);
      }
      matResult[i_dim_size + j] += sum;
    }
  }
}
void OrigMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB, 
                int i_dim, int j_dim, int k_dim) {
  int i, j, k;
  for (i = 0; i < i_dim; ++i) {
    int i_dim_size = i * i_dim;
    for (j = 0; j < j_dim; ++j) {
      FLOAT sum = 0.0;
      for (k = 0; k < k_dim; ++k) {
	      sum += matA[i_dim_size + k] *  matB[k*k_dim  + j]; // matA[i,k] x matB[k,j]
//        cnt++; 
//        PRINT("%.4f = %.4f * %.4f (%d,%d)\n", sum, matA[i_dim_size + k], matB[k * k_dim + j], i_dim_size+k, k*k_dim+j);
      }
      matResult[i_dim_size + j] = sum;
    }
  }
//  printf("[%d] cnt=%llu\n", rank_id, cnt);
}

////////////////////////////////////////////////////////////////////
// Three-level blocking cache-aware matrix multiplication
enum {
  M=0,
  K
};
void L1CacheAwareMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB, 
                        int i0, int i1, 
                        int j0, int j1, 
                        int k0, int k1, int dim[]) {
  
  int i=0,j=0,k=0;
//  PRINT("\t\t\tL1 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();

  for (i = i0; i < i1; ++i) {
    int i_dim   = i * dim[K];
    int res_dim = i * dim[M];
    for (j = j0; j < j1; ++j) {
      int j_dim = j * dim[K];
      FLOAT sum = 0.0f;
      for (k = k0; k < k1; ++k) {
////        PRINT("matA[%d] * matb[%d]\n", i_dim + k, k * k_dim + j);
        if(dim[ROW_COL_MAJOR]) {
          sum += matA[i_dim + k] * matB[j_dim + k]; // row major * col major
        } else {
          sum += matA[i_dim + k] * matB[k*dim[K] + j]; // row major * row major
        }
//        PRINT("\t\t\t(%d,%d=%d) sum : %3.3f = %3.3f(%d) * %3.3f(%d)", i, j, res_dim  + j, 
//                sum, matA[i_dim + k], i_dim + k, matB[j_dim + k], j_dim+k);
      }
      //PRINT("\t\t\t(%d,%d=%d) sum : %3.3f = %3.3f(%d) * %3.3f(%d)", i, j, res_dim  + j, sum, matA[i * mat_dim + k], i * mat_dim + k, matB[k*mat_dim], k*mat_dim);
      matResult[res_dim  + j] += sum;
    }
//    PRINT("\n");
  }

}

void L2CacheAwareMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                        int i0, int i1, 
                        int j0, int j1, 
                        int k0, int k1, 
                        int blockSizeForL1, int dim[]){
  int i=0, j=0, k=0, 
      di = i1 - i0, 
      dj = j1 - j0, 
      dk = k1 - k0;
//  PRINT("\t\tL2 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();
  if (di >= dj && di >= dk && di > blockSizeForL1) {
    for(i=0; i<di/blockSizeForL1; i++) {
      L2CacheAwareMatMul(matResult, matA, matB, 
                         i0 + i*blockSizeForL1, i0 + (i+1)*blockSizeForL1, 
                         j0, j1, 
                         k0, k1,
                         blockSizeForL1, dim);
    }
  } else if (dj >= dk && dj > blockSizeForL1) {

    for(j=0; j<dj/blockSizeForL1; j++) {
      L2CacheAwareMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0 + j*blockSizeForL1, j0 + (j+1)*blockSizeForL1, 
                         k0, k1, 
                         blockSizeForL1, dim);
    }

  } else if (dk > blockSizeForL1) {
    for(k=0; k<dk/blockSizeForL1; k++) {
      L2CacheAwareMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0, j1,
                         k0 + k*blockSizeForL1, k0 + (k+1)*blockSizeForL1, 
                         blockSizeForL1, dim);
    }
  } else {
//    PRINT("\t\t\tL1 i (%d-%d,%d-%d) * j (%d-%d,%d-%d)\n", i0, i1-1, k0, k1-1, k0, k1-1, j0, j1-1); //getchar();
//    PRINT("\n\t\t\tL1 i(%d,%d) * j(%d,%d)\n", i0, k0, k0, j0); //getchar();
    L1CacheAwareMatMul(matResult, matA, matB, 
                       i0, i1, 
                       j0, j1,
                       k0, k1, dim);

  }

}
void L3CacheAwareMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                         int i0, int i1, 
                         int j0, int j1, 
                         int k0, int k1, 
                         int blockSizeForL1, int blockSizeForL2, int dim[]){
//  PRINT("\tL3 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();
  int i=0, j=0, k=0, 
      di = i1 - i0, 
      dj = j1 - j0, 
      dk = k1 - k0;


  if (di >= dj && di >= dk && di > blockSizeForL2) {
    for(i=0; i<di/blockSizeForL2; i++) {
      L3CacheAwareMatMul(matResult, matA, matB, 
                         i0 + i*blockSizeForL2, i0 + (i+1)*blockSizeForL2, 
                         j0, j1, 
                         k0, k1,
                         blockSizeForL1, blockSizeForL2, dim);
    }
  } else if (dj >= dk && dj > blockSizeForL2) {

    for(j=0; j<dj/blockSizeForL2; j++) {
      L3CacheAwareMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0 + j*blockSizeForL2, j0 + (j+1)*blockSizeForL2, 
                         k0, k1, 
                         blockSizeForL1, blockSizeForL2, dim);
    }

  } else if (dk > blockSizeForL2) {
    for(k=0; k<dk/blockSizeForL2; k++) {
      L3CacheAwareMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0, j1,
                         k0 + k*blockSizeForL2, k0 + (k+1)*blockSizeForL2, 
                         blockSizeForL1, blockSizeForL2, dim);
    }
  } else {

//    PRINT("\t\tL2 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();
    L2CacheAwareMatMul(matResult, matA, matB, 
                       i0, i1, 
                       j0, j1,
                       k0, k1, 
                       blockSizeForL1, dim);

  }
}

void ThreeLvMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                   int i0, int i1, 
                   int j0, int j1, 
                   int k0, int k1,
                   int blockSizeForL1, int blockSizeForL2, int blockSizeForL3, int dim[]) {
//  printf(":%d %d %d[%d]\n", di_end, , dj_end, dk_end, omp_get_thread_num());
  int i=0, j=0, k=0, 
      di = i1 - i0, 
      dj = j1 - j0, 
      dk = k1 - k0;
  int di_end = di/blockSizeForL3;
  int dj_end = dj/blockSizeForL3;
  int dk_end = dk/blockSizeForL3;
//  PRINT("\nThreeLvMatMul di %d(%d) dj %d(%d) dk %d(%d)\n", di, di/blockSizeForL3, dj, dj/blockSizeForL3, dk, dk/blockSizeForL3); //getchar();

 // #pragma omp parallel  //reduction(+:avg_elapsed) //reduction(max:max_elapsed) reduction(min:min_elapsed)
  {
    //#pragma omp single
    {
  if (di >= dj && di >= dk && di > blockSizeForL3) {
    //printf("... di_end:%d [%d]\n", di_end, omp_get_thread_num());
    #pragma omp parallel for private(i) firstprivate(di_end) schedule(static)
    for(i=0; i<di_end; i++) {
//      PRINT("\nTop i %d - %d\n", i*blockSizeForL3, (i+1)*blockSizeForL3);
//    printf("... di_end:%d [%d]\n", di_end, omp_get_thread_num());
      ThreeLvMatMul(matResult, matA, matB, 
                         i0 + i*blockSizeForL3, i0 + (i+1)*blockSizeForL3, 
                         j0, j1, 
                         k0, k1,
                         blockSizeForL1, blockSizeForL2, blockSizeForL3, dim);
    }
  } else if (dj >= dk && dj > blockSizeForL3) {

   // printf("... dj_end:%d\n", dj_end);
//        #pragma omp parallel for firstprivate(j, dj_end) schedule(static)
    for(j=0; j<dj_end; j++) {
//      PRINT("\nTop j %d - %d\n", j*blockSizeForL3, (j+1)*blockSizeForL3);
      ThreeLvMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0 + j*blockSizeForL3, j0 + (j+1)*blockSizeForL3, 
                         k0, k1, 
                         blockSizeForL1, blockSizeForL2, blockSizeForL3, dim);
    }

  } else if (dk > blockSizeForL3) {
    //printf("... dk_end:%d\n", dk_end);
    for(k=0; k<dk_end; k++) {
//      PRINT("\nTop k %d - %d\n", k*blockSizeForL3, (k+1)*blockSizeForL3); 
      ThreeLvMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0, j1,
                         k0 + k*blockSizeForL3, k0 + (k+1)*blockSizeForL3, 
                         blockSizeForL1, blockSizeForL2, blockSizeForL3, dim);
    }
  } else {
//    printf("di_end:%d\n", di_end);
//    printf("dj_end:%d\n", dj_end);
//    printf("dk_end:%d\n", dk_end);
//    #pragma omp task
    {
//    PRINT("Call L3 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();
    L3CacheAwareMatMul(matResult, matA, matB, 
                       i0, i1, 
                       j0, j1,
                       k0, k1, 
                       blockSizeForL1, blockSizeForL2, dim);
    }
  }

    } // single ends
  } // omp parallel ends

}

void TwoLvMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                   int i0, int i1, 
                   int j0, int j1, 
                   int k0, int k1,
                   int blockSizeForL1, int blockSizeForL2, int dim[]) {
  int i=0, j=0, k=0, 
      di = i1 - i0, 
      dj = j1 - j0, 
      dk = k1 - k0;
  int di_end = di/blockSizeForL2;
  int dj_end = dj/blockSizeForL2;
  int dk_end = dk/blockSizeForL2;
//  PRINT("\nTwoLvMatMul di %d(%d) dj %d(%d) dk %d(%d)\n", di, di/blockSizeForL2, dj, dj/blockSizeForL2, dk, dk/blockSizeForL2); //getchar();

  //#pragma omp parallel  //reduction(+:avg_elapsed) //reduction(max:max_elapsed) reduction(min:min_elapsed)
  {
   // #pragma omp single
    {
  if (di >= dj && di >= dk && di > blockSizeForL2) {
        #pragma omp parallel for private(i) firstprivate(di_end) schedule(static)
    for(i=0; i<di_end; i++) {
//      PRINT("\nTop i %d - %d\n", i*blockSizeForL2, (i+1)*blockSizeForL2);
      TwoLvMatMul(matResult, matA, matB, 
                         i0 + i*blockSizeForL2, i0 + (i+1)*blockSizeForL2, 
                         j0, j1, 
                         k0, k1,
                         blockSizeForL1, blockSizeForL2, dim);
    }
  } else if (dj >= dk && dj > blockSizeForL2) {

        #pragma omp parallel for private(j) firstprivate(dj_end) schedule(static)
    for(j=0; j<dj_end; j++) {
//      PRINT("\nTop j %d - %d\n", j*blockSizeForL2, (j+1)*blockSizeForL2);
      TwoLvMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0 + j*blockSizeForL2, j0 + (j+1)*blockSizeForL2, 
                         k0, k1, 
                         blockSizeForL1, blockSizeForL2, dim);
    }

  } else if (dk > blockSizeForL2) {
    for(k=0; k<dk_end; k++) {
//      PRINT("\nTop k %d - %d\n", k*blockSizeForL2, (k+1)*blockSizeForL2); 
      TwoLvMatMul(matResult, matA, matB, 
                         i0, i1, 
                         j0, j1,
                         k0 + k*blockSizeForL2, k0 + (k+1)*blockSizeForL2, 
                         blockSizeForL1, blockSizeForL2, dim);
    }
  } else {
//    #pragma omp task
    {
//    PRINT("Call L2 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();
    L2CacheAwareMatMul(matResult, matA, matB, 
                       i0, i1, 
                       j0, j1,
                       k0, k1, 
                       blockSizeForL1, dim);
    }
  }

    } // single ends
  } // omp parallel ends

}


void OneLvMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                   int i0, int i1, 
                   int j0, int j1, 
                   int k0, int k1,
                   int blockSizeForL1, int dim[]) {
  int i=0, j=0, k=0, 
      di = i1 - i0, 
      dj = j1 - j0, 
      dk = k1 - k0;
  int di_end = di/blockSizeForL1;
  int dj_end = dj/blockSizeForL1;
  int dk_end = dk/blockSizeForL1;
//  PRINT("\nOneLvMatMul di %d(%d) dj %d(%d) dk %d(%d)\n", di, di/blockSizeForL1, dj, dj/blockSizeForL1, dk, dk/blockSizeForL1); //getchar();
//  #pragma omp parallel  //reduction(+:avg_elapsed) //reduction(max:max_elapsed) reduction(min:min_elapsed)
  {
//    #pragma omp single
    {
//      PRINT("di:%d dj:%d dk:%d\n", di, dj, dk);
      if (di >= dj && di >= dk && di > blockSizeForL1) {
//        PRINT("di:%d\n", di);
        #pragma omp parallel for private(i) firstprivate(di_end) schedule(static)
        for(i=0; i<di_end; i++) {
          int tid = omp_get_thread_num();
          PRINT("[%d] OneLvMatMul %d %d %d\n", tid, di, dj, dk);
    //      PRINT("\nTop i %d - %d\n", i*blockSizeForL1, (i+1)*blockSizeForL1);
          OneLvMatMul(matResult, matA, matB, 
                             i0 + i*blockSizeForL1, i0 + (i+1)*blockSizeForL1, 
                             j0, j1, 
                             k0, k1,
                             blockSizeForL1, dim);
        }
      } else if (dj >= dk && dj > blockSizeForL1) {
    
//        PRINT("dj:%d\n", dj);
//        #pragma omp for private(j)
        #pragma omp parallel for private(j) firstprivate(dj_end) schedule(static)
        for(j=0; j<dj_end; j++) {
          int tid = omp_get_thread_num();
          PRINT("[%d] OneLvMatMul %d %d %d\n", tid, di, dj, dk);
    //      PRINT("\nTop j %d - %d\n", j*blockSizeForL1, (j+1)*blockSizeForL1);
          OneLvMatMul(matResult, matA, matB, 
                             i0, i1, 
                             j0 + j*blockSizeForL1, j0 + (j+1)*blockSizeForL1, 
                             k0, k1, 
                             blockSizeForL1, dim);
        }
    
      } else if (dk > blockSizeForL1) {
//        PRINT("dk:%d\n", dk);
//        #pragma omp for private(k)
        for(k=0; k<dk_end; k++) {
          int tid = omp_get_thread_num();
          PRINT("[%d] OneLvMatMul %d %d %d\n", tid, di, dj, dk);
    //      PRINT("\nTop k %d - %d\n", k*blockSizeForL1, (k+1)*blockSizeForL1); 
          OneLvMatMul(matResult, matA, matB, 
                             i0, i1, 
                             j0, j1,
                             k0 + k*blockSizeForL1, k0 + (k+1)*blockSizeForL1, 
                             blockSizeForL1, dim);
        }
      } else {
    //    PRINT("Call L1 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();
//        #pragma omp task firstprivate(di,dj,dk)
        {
          int tid = omp_get_thread_num();
          PRINT("[%d] OneLvMatMul %d %d %d\n", tid, di, dj, dk);
        PRINT("Call L1 i %d - %d j %d - %d k %d - %d\n", i0, i1, j0, j1, k0, k1); //getchar();
        L1CacheAwareMatMul(matResult, matA, matB, 
                           i0, i1, 
                           j0, j1,
                           k0, k1, 
                           dim);
        }
//        #pragma omp barrier
      }
    } // single ends
  } // omp parallel ends
}


// Cache-Oblivious Matrix Multiplication
void RecursiveMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB, 
                     int i0, int i1, 
                     int j0, int j1, 
                     int k0, int k1) {
  int i, j, k, 
      di = i1 - i0, 
      dj = j1 - j0, 
      dk = k1 - k0;
  const int CUTOFF = DEFAULT_CUTOFF;
  if (di >= dj && di >= dk && di > CUTOFF) {
    int im = (i0 + i1) / 2;
    RecursiveMatMul(matResult, matA, matB, i0, im, j0, j1, k0, k1);                 
    RecursiveMatMul(matResult, matA, matB, im, i1, j0, j1, k0, k1);
  } else if (dj >= dk && dj > CUTOFF) {
    int jm = (j0 + j1) / 2;
    RecursiveMatMul(matResult, matA, matB, i0, i1, j0, jm, k0, k1);   
    RecursiveMatMul(matResult, matA, matB, i0, i1, jm, j1, k0, k1);
  } else if (dk > CUTOFF) {
    int km = (k0 + k1) / 2;
    RecursiveMatMul(matResult, matA, matB, i0, i1, j0, j1, k0, km);
    RecursiveMatMul(matResult, matA, matB, i0, i1, j0, j1, km, k1);
  } else {

    for (i = i0; i < i1; ++i) {
      int i_dim = i * dk;
      for (j = j0; j < j1; ++j) {
        int j_dim = j * dk;
        FLOAT sum = 0.0f;
        for (k = k0; k < k1; ++k) {
//          PRINT("matA[%d] * matb[%d]\n", i_dim + k, k * dk + j);
//          sum += matA[i_dim + k] * matB[k * dk + j]; // row major * row major
          sum += matA[i_dim + k] * matB[j_dim + k]; // row major * col major
        }
//        PRINT("sum : %3.3f\n", sum);
        matResult[i_dim + j] += sum;
      }
    }
  }
}
