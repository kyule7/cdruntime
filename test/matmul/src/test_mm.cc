#include <time.h>
#include "define.h"
#include "test.h"
#include "util.h"
#include "matmul.h"
#include "matmul_seq.h"
#include "matmul_omp.h"
#include "matmul_mpi.h"
FILE *fp = NULL;
FILE *result_file = NULL;
int rank_id=0;
int np=1;
int blk_size=1;
int np_row=1, np_col=1;
int sub_col_size, sub_row_size;
int locX=0, locY=0,
    locX_m=0, locY_m=0;
int sub_mat_size;
int num_matrices;
int mat_dim=0;
int xdim_max=0, ydim_max=0;
int main(int argc, char *argv[]) {
  int i=0, j=0;
  FLOAT *input[TOT_INPUT_MAT];
  FLOAT *result[2];
  rank_id = 0;
  np = 1;
#if _MPI
  // Initialize MPI
  MPI_Init(&argc, &argv);

  // Get rank ID and total number of tasks
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_id);
  MPI_Comm_size(MPI_COMM_WORLD, &np);	
#endif

#if _DEBUG == 1
  char fname[64] = "./dbg_";
  sprintf(fname, "./dbg_%d", rank_id); 
  fp = fopen(fname, "w");
#endif
  if(rank_id == 0) {
    result_file = fopen("./result.out", "a");
  }


  // Get input configurations
  if (argc != 6) {
    printf("usage: {# total task} {# omp thread} {matrix dim} {# matrix} {test}\n");
    exit(1);
  } else {
    PRINT("correct\n");
  }
  int tot_threads = atoi(argv[1]);
  int nthreads    = atoi(argv[2]);
  mat_dim         = atoi(argv[3]); // size of original matrix N i N
  omp_set_num_threads(nthreads);
  omp_set_nested(1);
  xdim_max = mat_dim;
  ydim_max = mat_dim;
  num_matrices    = atoi(argv[4]);
  int check_correctness = atoi(argv[5]);

  PRINT("# tot task\t: %d\n", tot_threads); 
  PRINT("# omp task\t: %d\n", nthreads); 
  PRINT("# mat dim\t: %d \n", mat_dim); 
  PRINT("# matrix\t: %d\n", num_matrices); 
  PRINT("# check mat\t: %d\n", check_correctness); 
  // Figure out sub matrix size based on the total number of tasks
  sub_mat_size = (mat_dim * mat_dim) / np;
  if((mat_dim*mat_dim) % np != 0) {
    printf("Assumed matrix %d x %d should be divided by %d (# proc)\n", mat_dim, mat_dim, np);
    assert(0);
  }
  // allocate arrays
  for(i=0; i < TOT_INPUT_MAT; i++) {
    input[i] = (FLOAT *)malloc(sizeof(FLOAT) * sub_mat_size);
  }
  // need zero-initialized array
  result[0] = (FLOAT *)calloc(sub_mat_size, sizeof(FLOAT));
  result[1] = (FLOAT *)calloc(sub_mat_size, sizeof(FLOAT));
  for(j=0; j < sub_mat_size; j++) {
    result[0][j]  = 0;
    result[1][j]  = 0;
  }
  // Get two factors from # of processors
  int lcm = GetTwoFactors(np, &np_row, &np_col);

  blk_size = mat_dim / lcm;

  sub_row_size = mat_dim / np_row;
  sub_col_size = mat_dim / np_col;
  locX = rank_id % np_row;
  locY = rank_id / np_row;
  locX_m = sub_row_size * locX;
  locY_m = sub_col_size * locY;
  PRINT("# of (col,row)\t: (%d,%d)\n", np_row, np_col);
  PRINT("blk_size\t: %d (= mat_dim/%d)\n", blk_size, lcm);
  PRINT("submat size\t: (tot) %d = %d x %d (sub col)x(sub row)\n", sub_mat_size, sub_col_size, sub_row_size);
//  assert(sub_mat_size == sub_col_size * sub_row_size);
  PRINT("(locY  ,locX)\t: (%d,%d)\n", locX, locY);
  PRINT("(locY_m,locX_m)\t: (%d,%d)\n", locX_m, locY_m);
  if(rank_id == 0){
    fprintf(result_file, "# tot task\t: %d\n", tot_threads);
    fprintf(result_file, "# omp task\t: %d\n", nthreads); 
    fprintf(result_file, "# mat dim\t: %d \n", mat_dim); 
    fprintf(result_file, "# matrix\t: %d\n", num_matrices); 
    fprintf(result_file, "# of (col,row)\t: (%d,%d)\n", np_row, np_col);
    fprintf(result_file, "blk_size\t: %d (= mat_dim/%d)\n", blk_size, lcm);
    fprintf(result_file, "submat size\t: (tot) %d = %d x %d (sub col)x(sub row)\n", 
                         sub_mat_size, sub_col_size, sub_row_size);
    fprintf(result_file, "(locY  ,locX)\t: (%d,%d)\n", locX, locY);
    fprintf(result_file, "(locY_m,locX_m)\t: (%d,%d)\n", locX_m, locY_m);
  }
  // get sub matrices
  int mat_dim_minus_one = mat_dim - 1;
  int location = rank_id * mat_dim;
  int ops = 0;
  for(i=0; i<num_matrices; i++) {
    input[i] = (FLOAT *)malloc(sizeof(FLOAT) * sub_mat_size);
    switch(i) {
      case 0:
        ops = FUNC1;
        break;
      case 1:
        ops = FUNC2;
        break;
      case 2:
        ops = I_MAT;
        break;
      default:
        ops = ZEROS;
        break;
    }
    if(1) {
//    if(i==0) {
      if(InitSubMatrix(ops, input[i], 
                       locX_m, locX_m + sub_row_size - 1, sub_row_size, 
                       locY_m, locY_m + sub_col_size - 1, sub_col_size, 
                       ROW_MAJOR) == NULL)
      {
        ERROR("inconsistency in InitSubMatrix\n");
      }
    }
    else { 
      if (InitSubMatrix(ops, input[i],
                        locX_m, locX_m + sub_row_size - 1, sub_row_size, 
                        locY_m, locY_m + sub_col_size - 1, sub_col_size,
                        COL_MAJOR) == NULL) 
      {
        ERROR("inconsistency in InitSubMatrix\n");
      }
    }
  }
  
  double tstart = MPI_Wtime();
  int dim[DIM] = {sub_col_size, sub_row_size, ROW_ROW};
//  OrigMatMul(result[1], input[0], input[1], sub_col_size, sub_row_size, blk_size);
//  ThreeLvMatMul(result[1], input[0], input[1], 0, sub_col_size, 0, sub_row_size, 0, blk_size, MIN_DIM_L1, MIN_DIM_L2, MIN_DIM_L3, dim);
//  TwoLvMatMul(result[1], input[0], input[1], 0, mat_dim, 0, mat_dim, 0, mat_dim, MIN_DIM_L1, MIN_DIM_L2, dim);
//  OneLvMatMul(result[1], input[0], input[1], 0, mat_dim, 0, mat_dim, 0, mat_dim, MIN_DIM_L1, dim);

  FLOAT *vizResult = NULL;
  MPI_Win win[np_col];
  Comm_t comm[DIM];
  MPI_MatMatMul(result[1], input[0], input[1], &vizResult, mat_dim, mat_dim, mat_dim, comm, win);

  double mm_elapsed = MPI_Wtime() - tstart;

  double avg_elapsed = 0.0;
  MPI_Reduce(&mm_elapsed, &avg_elapsed, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  double max_elapsed = 0.0;
  MPI_Reduce(&mm_elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  if(rank_id == 0) {
    fprintf(result_file, "elapsed\t:%lf | max\t:%lf\n", avg_elapsed/np, max_elapsed);
  }
//  TEST_ARRAY(result[0], result[1], mat_dim, mat_dim);
#if 0
#if _MPI

  FLOAT this_point = 0.0, elapsed_time = 0.0;
  if(rank_id == 0) this_point = MPI_Wtime();

  FLOAT *vizResult = NULL;
  int n=0;
  MPI_MatMatMul(result[n], input[0], input[1], &vizResult, mat_dim, mat_dim, mat_dim, comm, win);
  for (i = 2; i < num_matrices; i++, n^=1) {
    MPI_MatMatMul(result[n ^ 0x1], result[n], input[i], &vizResult, mat_dim, mat_dim, mat_dim, comm, win);
    for(j=0; j<sub_mat_size; j++) {
      result[n][j] = 0;
    }
    n = n ^ 0x1;
  }
  if(rank_id == 0) {
    FLOAT elapsed_time = MPI_Wtime() - this_point;
    printf("elapsed time : %lf\n", elapsed_time);
  }

  if (check_correctness == true) {
    PRINT("\n\nargument matrix %d, %d, %d\n", i, mat_dim, mat_dim);
    PRINT("result matrix\n");

    // Print Result Matrix
    MPI_Barrier(MPI_COMM_WORLD);
    for(int k=0; k<np_col; k++) {
      PRINT("%d %d %d\n", k, comm[ROW].group_id, rank_id);
  
      if(comm[ROW].group_id == k) {
        MPI_Win_fence(0, win[comm[ROW].group_id]);
        PRINT("=== Rank #%d ==============================\n", rank_id);
        for(i=0;i<sub_col_size; i++) {
          MPI_Put(result[n] + i*sub_row_size, sub_row_size, MPI_DOUBLE, 0, comm[ROW].rank_id * sub_row_size,
                                 sub_row_size, MPI_DOUBLE, win[comm[ROW].group_id]);
          MPI_Win_fence(0, win[comm[ROW].group_id]);
      
          if(comm[ROW].rank_id == 0) {
            for(j=0; j<mat_dim; ++j) {
              printf("%3.3f ", vizResult[j]);
            }
            printf("\n");
          }
        }
        PRINT("\n===========================================\n");
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }
  } 
  else { // performance mode
    FLOAT sum = 0.0;
    for (i = 0; i < sub_mat_size; ++i) {
      sum += result[n][i];
    }
    FLOAT recvbuf = 0.0f;

    MPI_Reduce(&sum, &recvbuf, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if(rank_id == 0)
      printf("%f\n", recvbuf);
    
  }

  for(i=0; i<np_col; i++) {
    MPI_Win_free(&(win[i]));
  }
#endif
#endif


//  printf("orig mm elapsed\t:%lf\n", orig_mm_elapsed);
//  printf("omp  mm elapsed\t:%lf\n", omp_mm_elapsed);
  if(rank_id == 0) {
    fclose(result_file);
  }
#if _DEBUG == 1
  fclose(fp);
#endif
  for(i=0; i < TOT_INPUT_MAT; i++) {
    free(input[i]);
  }
  free(result[0]);
  free(result[1]);

#if _MPI
  MPI_Finalize();
#endif

  return 0;
}


