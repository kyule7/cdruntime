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
  FLOAT *result;
  rank_id = 0;
  np = 1;
#if _MPI
  // Initialize MPI
  MPI_Init(&argc, &argv);

  // Get rank ID and total number of tasks
  MPI_Comm_rank(MPI_COMM_WORLD, &rank_id);
  MPI_Comm_size(MPI_COMM_WORLD, &np);	
#endif
  printf("rankid %d\n", rank_id);
#if _DEBUG == 1
  char fname[64] = "./dbg_";
  sprintf(fname, "./dbg_%d", rank_id); 
  fp = fopen(fname, "w");
#endif
  if(rank_id == 0) {
    result_file = fopen("./result.out", "a");
  }


  // Get input configurations
  if (argc != 7) {
    printf("usage: {# total task} {# omp thread} {matrix dim} {L1 blk} {L2 blk} {L3 blk}\n");
    exit(1);
  } else {
    PRINT("correct\n");
  }
  int tot_threads = atoi(argv[1]);
  int nthreads    = atoi(argv[2]);
  mat_dim         = atoi(argv[3]); // size of original matrix N i N
  int l1_blk = MIN_DIM_L1;
  int l2_blk = MIN_DIM_L2;
  int l3_blk = MIN_DIM_L3;
  l1_blk = atoi(argv[4]);
  l2_blk = atoi(argv[5]);
  l3_blk = atoi(argv[6]);
  omp_set_num_threads(nthreads);
  omp_set_nested(1);
  xdim_max = mat_dim;
  ydim_max = mat_dim;
  int mat_size = mat_dim * mat_dim;

  PRINT("# tot task\t: %d\n", tot_threads); 
  PRINT("# omp task\t: %d\n", nthreads); 
  PRINT("# mat dim\t: %d \n", mat_dim); 
  PRINT("blocking size L1/L2/L3\t: %d/%d/%d\n", l1_blk, l2_blk, l3_blk); 
  PRINT("# check mat\t: %d\n", check_correctness); 
  // Figure out sub matrix size based on the total number of tasks
  sub_mat_size = (mat_dim * mat_dim);
  if((mat_dim*mat_dim) % np != 0) {
    printf("Assumed matrix %d x %d should be divided by %d (# proc)\n", mat_dim, mat_dim, np);
    assert(0);
  }
  // allocate arrays
  for(i=0; i < TOT_INPUT_MAT; i++) {
    input[i] = (FLOAT *)malloc(sizeof(FLOAT) * mat_size);
  }
  // need zero-initialized array
  result = (FLOAT *)calloc(mat_size, sizeof(FLOAT));
  for(j=0; j < sub_mat_size; j++) {
    result[j]  = 0;
  }
  if(rank_id == 0){
    fprintf(result_file, "# tot task\t: %d\n", tot_threads);
    fprintf(result_file, "# omp task\t: %d\n", nthreads); 
    fprintf(result_file, "# mat dim\t: %d \n", mat_dim); 
    fprintf(result_file, "blocking size L1/L2/L3\t: %d/%d/%d\n", l1_blk, l2_blk, l3_blk); 
  }
  // get sub matrices
  int mat_dim_minus_one = mat_dim - 1;
  int ops = 0;
  for(i=0; i<num_matrices; i++) {
    input[i] = (FLOAT *)malloc(sizeof(FLOAT) * mat_size);
    switch(i) {
      case 0:
        ops = FUNC1;
        break;
      case 1:
        ops = INDEX;
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
                       0, mat_dim - 1, mat_dim, 
                       0, mat_dim - 1, mat_dim, 
                       ROW_MAJOR) == NULL)
      {
        ERROR("inconsistency in InitSubMatrix\n");
      }
    }
    else { 
      if (InitSubMatrix(ops, input[i],
                       0, mat_dim - 1, mat_dim, 
                       0, mat_dim - 1, mat_dim, 
                        COL_MAJOR) == NULL) 
      {
        ERROR("inconsistency in InitSubMatrix\n");
      }
    }
    print_matrix_2D(input[i], mat_dim, mat_dim);
  }
  
  double tstart = MPI_Wtime();
  if(rank_id == 0) {
    OrigMatMul(result, input[0], input[1], mat_dim, mat_dim, mat_dim);
    //OMP_OneLvMatMul(result, input[0], input[1], mat_dim, mat_dim, mat_dim, 32);
  } else if(rank_id == 1) {
    OMP_OneLvMatMul(result, input[0], input[1], mat_dim, mat_dim, mat_dim, l1_blk);
  } else if(rank_id == 2) {
    OMP_TwoLvMatMul(result, input[0], input[1], mat_dim, mat_dim, mat_dim, l1_blk, l2_blk);
  } else if(rank_id == 3) {
    OMP_ThreeLvMatMul(result, input[0], input[1], mat_dim, mat_dim, mat_dim, l1_blk, l2_blk, l3_blk);
  }
#if 0
  int dim[DIM] = {sub_col_size, sub_row_size, ROW_ROW};
  if(rank_id == 0) {
    OrigMatMul(result, input[0], input[1], mat_dim, mat_dim, mat_dim);
//    double *result2 = (FLOAT *)calloc(mat_size, sizeof(FLOAT));
//    OMP_ThreeLvMatMul(result2, input[0], input[1], mat_dim, mat_dim, mat_dim, l1_blk, l2_blk, l3_blk);
//    TEST_ARRAY(result, result2, mat_dim, mat_dim);

  } else if(rank_id == 1) {
    OneLvMatMul(result, input[0], input[1], 0, mat_dim, 0, mat_dim, 0, mat_dim, l1_blk, dim);
  } else if(rank_id == 2) {
    TwoLvMatMul(result, input[0], input[1], 0, mat_dim, 0, mat_dim, 0, mat_dim, l1_blk, l2_blk_ONLY, dim);
  } else if(rank_id == 3) {
    ThreeLvMatMul(result, input[0], input[1], 0, mat_dim, 0, mat_dim, 0, mat_dim, l1_blk, l2_blk, l3_blk, dim);
  }
#endif
  double mm_elapsed = MPI_Wtime() - tstart;

  double elapsed[np];
  MPI_Gather(&mm_elapsed, 1, MPI_DOUBLE,
             &elapsed,    1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  if(rank_id == 0) {
    fprintf(result_file, "------------------------\n");
    for(int k=0; k<np; k++) {
      fprintf(result_file, "elapsed\t:%lf\n", elapsed[k]);
    }
    fprintf(result_file, "------------------------\n");
  }
  if(rank_id == 0) {
    fclose(result_file);
  }
#if _DEBUG == 1
  fclose(fp);
#endif
  for(i=0; i < TOT_INPUT_MAT; i++) {
    free(input[i]);
  }
  free(result);

#if _MPI
  MPI_Finalize();
#endif

  return 0;
}


