#include <mpi.h>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cd.h"

#if MYDEBUG == 1
#define PRINT_ARRAY(ARRAY,DIM_COL,DIM_ROW,TITLE) { \
  printf("%s\n", (TITLE)); \
  for(int ii=0; ii<(DIM_COL); ii++) { \
    for(int jj=0; jj<(DIM_ROW); jj++) { \
      printf("%.2f\t", (ARRAY)[ii*(DIM_ROW) + jj]); \
    } \
    printf("\n"); \
  } \
}
#else
#define PRINT_ARRAY(...)
#endif

#define TEST_ARRAY(ARRAY_X,ARRAY_Y,DIM_COL,DIM_ROW) { \
  for(int ii=0; ii<(DIM_COL); ii++) { \
    for(int jj=0; jj<(DIM_ROW); jj++) { \
      double diff = (ARRAY_X)[ii*(DIM_ROW) + jj] - (ARRAY_Y)[ii*(DIM_ROW) + jj]; \
      if(-0.0001>diff || diff>0.0001) { \
        printf("(%d,%d) is wrong. %f != %f", ii, jj, (ARRAY_X)[ii*(DIM_ROW) + jj], (ARRAY_Y)[ii*(DIM_ROW) + jj]); \
      } \
    } \
  } \
}
#define DEFAULT_SIZE    16
//#define SLEEP_INTERVAL 1
//#define SLEEP(X) sleep(X)
#define SLEEP(X) usleep(X)
#define SLEEP_INTERVAL 1000000 // 0.1 sec
enum {
  INDEX=1,
  ONES=2,
  ZEROS=3
};

int task_size = 0;
int rankID = 0;
FILE *fp = NULL;
void InitArray(float *A, int coldim, int rowdim, int ops) {
  switch(ops) {
    case INDEX: {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = stride + j + rankID*coldim*rowdim;
        }
      }
      break;
    }
    case ONES: {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = 1;
        }
      }
      break;
    }
    case ZEROS : {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = 0;
        }
      }
      break;
    }
    default : {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = 0;
        }
      }
      break;
    }
  }
}

//row,col
void MatVecMul(float *A, float *B, float *C, int ydim, int xdim) 
{
  char do_escalate[16];
  char do_reexecute[16];
  struct stat statbuf;
  FILE *filep = fp;
  sprintf(do_escalate,  "escalate.%d", rankID);
  sprintf(do_reexecute, "reexecute.%d", rankID);
  CDHandle *root = GetCurrentCD(); // root->current
  root->Preserve(A, sizeof(float)*ydim*xdim, kCopy, "arrayA");
  root->Preserve(B, sizeof(float)*xdim, kCopy, "arrayB");
  CDHandle *parent = GetCurrentCD()->Create("Parent", kStrict); // parent->cd_lv1
  for(int j=0; j<ydim; j++) {
    CD_Begin(parent, true, "OuterLoop");
    parent->Preserve(C, sizeof(float)*ydim, kCopy, "arrayC"); 
    CDHandle *child = parent->Create(task_size, "Leaf", kStrict);
    float temp = 0.0;
    for(int i=0; i<xdim; i++) {
      CD_Begin(child, true, "InnerLoop");
      child->Preserve(&temp, sizeof(float), kCopy,"temp");
      child->Preserve(&i, sizeof(int), kCopy,"index");
      child->Preserve(A, sizeof(float)*ydim*xdim, kRef, "arrayA_lv2", "arrayA");
      child->Preserve(B, sizeof(float)*xdim, kRef, "arrayB_lv2", "arrayB");
      temp += A[j*xdim + i] * B[i];
      fprintf(filep, "(%d,%2d)\n", j, i); fflush(filep);
      parent->CDAssert(stat(do_escalate, &statbuf) != 0);
      for(int a=1; a<=10; a++) {
        SLEEP(SLEEP_INTERVAL/10);
        child->CDAssert(true);
      }
      child->CDAssert(stat(do_reexecute, &statbuf) != 0);
      child->Complete();
//      fprintf(filep, "Completed child %d\n", i); fflush(filep);
    }
//    fprintf(filep, "\n"); fflush(filep);
    child->Destroy();
    C[j] = temp;
    SLEEP(SLEEP_INTERVAL);
    parent->CDAssert(stat(do_escalate, &statbuf) != 0);
    parent->Complete();
//    fprintf(filep, "Completed parent %d\n", j); fflush(filep);
  }
  parent->Destroy();
}

int main(int argc, char *argv[]) {
  int colsize = DEFAULT_SIZE;
  int rowsize = DEFAULT_SIZE;
  if(argc == 2) {
    colsize = atoi(argv[1]);
    rowsize = colsize;
  }
  else if(argc >= 3) { // (NxN) x (NxM)
    colsize  = atoi(argv[2]);
    rowsize  = colsize;
  }

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rankID);
  MPI_Comm_size(MPI_COMM_WORLD, &task_size);

  char filepath[256]={};
  sprintf(filepath, "progress.%d", rankID);
  fp = fopen(filepath, "w");
  
  CDHandle *root = CD_Init(task_size, rankID);
  CD_Begin(root, true, "Root");

  int loc_colsize = colsize / task_size;

  float *arrayA = (float *)malloc(loc_colsize*rowsize*sizeof(float));
  float *arrayB = (float *)malloc(rowsize*sizeof(float));
  float *result = (float *)malloc(rowsize*sizeof(float));

  // Initialize Arrays
  InitArray(arrayA, loc_colsize, rowsize, INDEX);
  InitArray(arrayB, 1, rowsize, ONES);

  MatVecMul(arrayA, arrayB, result, loc_colsize, rowsize); 

  PRINT_ARRAY(arrayA, loc_colsize, rowsize, "\n-- Array A ------------------\n");
  PRINT_ARRAY(arrayB, rowsize, 1, "\n-- Array B ------------------\n");
  PRINT_ARRAY(result, rowsize, 1, "\n-- Result Array -------------\n");

  root->Complete();
  CD_Finalize();

  fclose(fp);

  MPI_Finalize();
  free(arrayA);
  free(arrayB);
  free(result);
  return 0;
}
