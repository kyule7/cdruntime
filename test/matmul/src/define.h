#ifndef _DEFINE_H
#define _DEFINE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef float FLOAT;
#if _MPI == 1
#include <mpi.h>
#define COMM_MAT_DATATYPE MPI_FLOAT
#endif
#define TOT_INPUT_MAT 5
#define RAND_MAX_VAL 100
#define ROW_ROW 0
#define ROW_COL 1
#define MIN_DIM_L1 32// For 32KB L1 cache
#define MIN_DIM_L2_ONLY 256  // For 256KB private cache
#define MIN_DIM_L2 512  // For 256KB private cache
#define MIN_DIM_L3 1024 // Bigger thatn 4~5MB
// Debugging-related
extern FILE *fp;
extern FILE *result_file;
#if _DEBUG == 1

#define PRINT(...) \
  fprintf(fp, __VA_ARGS__); fflush(fp);

#elif _DEBUG == 2

#define PRINT(...) \
  printf(__VA_ARGS__);
 
#else

#define PRINT(...) 

#endif

#define ERROR(...) {printf(__VA_ARGS__); assert(0);}

#define BLOCK_SIZE 32

enum MAT_DIR_T {
  COL_MAJOR=0,
  ROW_MAJOR=1
};
enum {
  ROW=0,
  COL,
  ROW_COL_MAJOR=2,
  DIM
};

extern int mat_dim, xdim_max, ydim_max;
extern int blk_size;
extern int rank_id;
extern int np;
extern int np_row, np_col;
extern int sub_col_size, sub_row_size, sub_mat_size;
extern int locX, locY, locX_m, locY_m;
#endif
