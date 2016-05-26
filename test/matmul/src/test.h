#ifndef _TEST_H
#define _TEST_H

#include "define.h"

#if DEBUG == 1
#define PRINT_ARRAY(ARRAY,DIM_COL,DIM_ROW,TITLE) { \
  PRINT("%s\n", (TITLE)); \
  for(int ii=0; ii<(DIM_COL); ii++) { \
    for(int jj=0; jj<(DIM_ROW); jj++) { \
      PRINT("%.2f\t", (ARRAY)[ii*(DIM_ROW) + jj]); \
    } \
    PRINT("\n"); \
  } \
}
#else
#define PRINT_ARRAY(...)
#endif

#define TEST_ARRAY(ARRAY_X,ARRAY_Y,DIM_COL,DIM_ROW) { \
  printf("Test Array\n"); \
  for(int ii=0; ii<(DIM_COL); ii++) { \
    for(int jj=0; jj<(DIM_ROW); jj++) { \
      FLOAT diff = (ARRAY_X)[ii*(DIM_ROW) + jj] - (ARRAY_Y)[ii*(DIM_ROW) + jj]; \
      if(-0.0001>diff || diff>0.0001) { \
        printf("%lf == %lf (%d,%d)\n", (ARRAY_X)[ii*(DIM_ROW) + jj], (ARRAY_Y)[ii*(DIM_ROW) + jj], ii, jj); \
      } \
    } \
  } \
}
void TestMatMul(FLOAT *result, FLOAT *origResult, int dim_j, int dim_i) {
  PRINT("\n\nTest Matrix Multiplication ======================123===\n");
  int j, i;
  for (j = 0; j < dim_j; ++j) {
    for (i = 0; i < dim_i; ++i) {
      PRINT("%3.3f == %3.3f\n", 
          result[j * dim_i + i], 
          origResult[(j + rank_id*mat_dim) * dim_i + i]);
    }
    PRINT("\n");
  }
}

void print_matrix_1D(FLOAT *result, int mat_size) {
  for (int i = 0; i < mat_size; ++i) {
    PRINT("%3.1lf ", result[i]);
  }
  PRINT("\n");
}

void print_matrix_2D(FLOAT *result, int ydim, int xdim) {
  for (int y = 0; y < ydim; ++y) {
    for (int x = 0; x < xdim; ++x) {
      PRINT("%3.1lf ", result[y * xdim + x]);
    }
    PRINT("\n");
  }
  PRINT("\n");
}


void print_submatrix(FLOAT *result, int dim_j, int dim_i, int row_col) {
//  PRINT("%d %d %d\n", dim_j, dim_i, row_col);
  int i, j;
  int stride = (row_col == ROW_MAJOR)? dim_i : dim_j;
  if(row_col) {
    for (j = 0; j < dim_j; ++j) {
      for (i = 0; i < dim_i; ++i) {
        PRINT("%3.3f ", result[j * stride + i]);
      }
      PRINT("\n");
    }
    PRINT("\n");
  }
  else {
    for (j = 0; j < dim_j; ++j) {
      for (i = 0; i < dim_i; ++i) {
        PRINT("%3.3f ", result[j + stride * i]);
      }
      PRINT("\n");
    }
    PRINT("\n");
  
  }
}
#endif
