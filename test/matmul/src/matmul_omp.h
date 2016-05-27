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


void OMP_OneLvMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                   int i_dim, 
                   int j_dim, 
                   int k_dim,
                   int blksize1) {
  #pragma omp parallel for collapse(3) schedule(static)// reduction(+:cnt)
  for (int ii = 0; ii< i_dim; ii += blksize1){ // block i (col for A)
    for (int jj = 0; jj< j_dim; jj += blksize1){ // block j (row for B)
      for (int kk=0; kk < k_dim; kk += blksize1){ // block k (row for A, col for B)
//        PRINT("block (%d,%d) = (%d,%d)x(%d,%d) ================ \n", ii, jj, ii, kk, kk, jj);
        int i_max = min(ii+blksize1,i_dim);
        for (int i=ii; i < i_max; i++){
          int i_stride = i*i_dim;
          int j_max = min(jj+blksize1,j_dim);
          for (int j=jj; j < j_max; j++){
            FLOAT temp = 0.0;
            int k_max = min(kk+blksize1,k_dim);
            int k_stride = j;
            for (int k = kk; k < k_max; k++, k_stride+=k_dim){
//              PRINT("(%d,%d)x(%d,%d) [%d,%d]\t-> %3.3lfx%3.3lf=%3.3lf\n", i, k, k, j, i_stride, k*k_dim,
//                  matA[i_stride + k], matB[k*k_dim + j], matA[i_stride + k] * matB[k*k_dim + j]); 
              temp += matA[i_stride + k] * matB[k_stride]; 
//              cnt++;
            }
            matResult[i_stride + j] += temp;
          }
        }
//        PRINT("\n");

      }
    }
  }
//  printf("[%d] cnt=%llu\n", rank_id, cnt);
}
void OMP_TwoLvMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                   int i_dim, 
                   int j_dim, 
                   int k_dim,
                   int blksize1,
                   int blksize2) {
//  int print = 0;
  #pragma omp parallel for collapse(3) schedule(static)// reduction(+:cnt)
  for (int iii = 0; iii< i_dim; iii += blksize2){ // block i (col for A)
    for (int jjj = 0; jjj< j_dim; jjj += blksize2){ // block j (row for B)
      for (int kkk=0; kkk < k_dim; kkk += blksize2){ // block k (row for A, col for B)
//              PRINT("\t[lv2]block (%d,%d) = (%d,%d)x(%d,%d) ================ \n", 
//                  iii, jjj, iii, kkk, kkk, jjj);
        int ii=0, jj=0, kk=0;          
        #pragma omp parallel for collapse(3) private(ii,jj,kk) schedule(static)// reduction(+:cnt)
        for (ii = iii; ii< iii+blksize2; ii += blksize1){ // block i (col for A)
          for (jj = jjj; jj< jjj+blksize2; jj += blksize1){ // block j (row for B)
            for (kk=kkk; kk < kkk+blksize2; kk += blksize1){ // block k (row for A, col for B)
//                    if(print == 0) {
//                      printf("[%d]# trheads %d\n", rank_id, omp_get_num_threads()); print = 1; 
//                    }
              PRINT("\t\t[lv1]block (%d,%d) = (%d,%d)x(%d,%d) ================ \n", ii, jj, ii, kk, kk, jj);
              int i_max = min(ii+blksize1,i_dim);
              for (int i=ii; i < i_max; i++){
                int i_stride = i*i_dim;
                int j_max = min(jj+blksize1,j_dim);
                for (int j=jj; j < j_max; j++){
                  FLOAT temp = 0.0;
                  int k_max = min(kk+blksize1,k_dim);
                  int k_stride = j;
                  for (int k = kk; k < k_max; k++, k_stride+=k_dim){
                    PRINT("(%d,%d)x(%d,%d) [%d,%d]\t-> %3.3lfx%3.3lf=%3.3lf\n", i, k, k, j, i_stride, k*k_dim,
                        matA[i_stride + k], matB[k*k_dim + j], matA[i_stride + k] * matB[k_stride]); 
                    temp += matA[i_stride + k] * matB[k_stride];
//                   cnt++; 
                  }
                  matResult[i_stride + j] += temp;
                }
              }
//              PRINT("\n");
      
            } // blk level 1 ends
          }
        }
//        PRINT("\n");
      } // blk level 2 ends
    }
  }
//  printf("[%d] cnt=%llu\n", rank_id, cnt);
}
void OMP_ThreeLvMatMul(FLOAT *matResult, FLOAT *matA, FLOAT *matB,
                   int i_dim, 
                   int j_dim, 
                   int k_dim,
                   int blksize1,
                   int blksize2,
                   int blksize3) {
//  int print = 0;
//  #pragma omp parallel for collapse(3) schedule(static)// reduction(+:cnt)
  for (int iiii = 0; iiii< i_dim; iiii += blksize3){ // block i (col for A)
    for (int jjjj = 0; jjjj< j_dim; jjjj += blksize3){ // block j (row for B)
      for (int kkkk=0; kkkk < k_dim; kkkk += blksize3){ // block k (row for A, col for B)
//              PRINT("\t[lv3]block (%d,%d) = (%d,%d)x(%d,%d) ================ \n", 
//                  iiii, jjjj, iiii, kkkk, kkkk, jjjj);
        int iii=0, jjj=0, kkk=0;          
        #pragma omp parallel for collapse(3) private(iii,jjj,kkk) schedule(static)// reduction(+:cnt)
        for (iii = iiii; iii< iiii+blksize3; iii += blksize2){ // block i (col for A)
          for (jjj = jjjj; jjj< jjjj+blksize3; jjj += blksize2){ // block j (row for B)
            for (kkk=kkkk; kkk < kkkk+blksize3; kkk += blksize2){ // block k (row for A, col for B)
//                    PRINT("\t\t[lv2]block (%d,%d) = (%d,%d)x(%d,%d) ================ \n", 
//                        iii, jjj, iii, kkk, kkk, jjj);
              int ii=0, jj=0, kk=0;          
              #pragma omp parallel for collapse(3) private(ii,jj,kk) schedule(static)// reduction(+:cnt)
              for (ii = iii; ii< iii+blksize2; ii += blksize1){ // block i (col for A)
                for (jj = jjj; jj< jjj+blksize2; jj += blksize1){ // block j (row for B)
                  for (kk=kkk; kk < kkk+blksize2; kk += blksize1){ // block k (row for A, col for B)
//                    if(print == 0) {
//                      printf("[%d]# trheads %d\n", rank_id, omp_get_num_threads()); print = 1; 
//                    }
                    PRINT("\t\t\t[lv1]block (%d,%d) = (%d,%d)x(%d,%d) ================ \n", ii, jj, ii, kk, kk, jj);
                    int i_max = min(ii+blksize1,i_dim);
                    for (int i=ii; i < i_max; i++){
                      int i_stride = i*i_dim;
                      int j_max = min(jj+blksize1,j_dim);
                      for (int j=jj; j < j_max; j++){
                        FLOAT temp = 0.0;
                        int k_max = min(kk+blksize1,k_dim);
                        int k_stride = j;
                        for (int k = kk; k < k_max; k++, k_stride+=k_dim){
                          PRINT("(%d,%d)x(%d,%d) [%d,%d]\t-> %3.3lfx%3.3lf=%3.3lf\n", i, k, k, j, i_stride, k*k_dim,
                              matA[i_stride + k], matB[k*k_dim + j], matA[i_stride + k] * matB[k*k_dim + j]);                              
                          temp += matA[i_stride + k] * matB[k_stride];
//                         cnt++; 
                        }
                        matResult[i_stride + j] += temp;
                      }
                    }
//                    PRINT("\n");
            
                  } // blk level 1 ends
                }
              }
 //             PRINT("\n");
            } // blk level 2 ends
          }
        }
      } // blk level 3 ends
    }
  }
//  printf("[%d] cnt=%llu\n", rank_id, cnt);
}

#if 0
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
#endif








































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

