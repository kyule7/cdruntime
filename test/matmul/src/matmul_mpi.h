#if _MPI
#include <mpi.h>
#include "define.h"
#if _CUDA
#include "matmul_cuda.h"
#endif
struct Comm_t {
  MPI_Comm communicator;
  int group_id;
  int id_in_group;
  int rank_id;
  int np;
  void Init(int _group_id, int _id_in_group) {
    group_id    = _group_id;
    id_in_group = _id_in_group;
  }
  void SplitComm(void) { 
    MPI_Comm_split(MPI_COMM_WORLD, group_id, id_in_group, &communicator);
    MPI_Comm_rank(communicator, &rank_id);
    MPI_Comm_size(communicator, &np);
  }  
};

void Transpose(FLOAT *out, FLOAT *in, int start, int col_size, int blk_size, int row_size)
{
#if 1
//  PRINT("start : %d, col : %d, blk : %d, row : %d\n", start, col_size, blk_size, row_size);
  int i, j;
  for(i=0; i<blk_size; ++i) {
    for(j=0; j<col_size; ++j) {
//      PRINT("(%d,%d) %d,%d ", i, j, i * blk_size + j, i * row_size + start + j);
      out[i * blk_size + j] = in[i * row_size + start + j];
//      PRINT("\n(%d,%d) = %3.3f\n", i, j, out[i * blk_size + j]);
    }
//    PRINT("\n");
  }
#endif
}

/*
   ---> X
 | +------+------+------+------+------+
 | |0,0 0 |0,1 1 |0,2 2 |0,3 3 |0,4 4 |
 Y +------+------+------+------+------+
   |1,0 5 |1,1 6 |1,2 7 |1,3 8 |1,4 9 |
   +------+------+------+------+------+
   |2,0 10|2,1 11|2,2 12|2,3 13|2,4 14|
   +------+------+------+------+------+
   |3,0 15|3,1 16|3,2 17|3,3 18|3,4 19|
   +------+------+------+------+------+

  1. Create subcommunicator for row and column
*/
bool initialized = false;
void MPI_MatMatMul(FLOAT *result, FLOAT *matA, FLOAT *matB, FLOAT **tempBuff, 
                   int n_dim, int m_dim, int k_dim, Comm_t *comm, MPI_Win *win) 
{
  if(initialized == false) {
    initialized = true;
    // FIXME : Possible optimization in terms of process mapping
    int row_group_id = rank_id/np_row;
    int row_rank_id  = rank_id%np_row;
    comm[ROW].Init(row_group_id, row_rank_id);
    comm[COL].Init(row_rank_id,  row_group_id);
    comm[ROW].SplitComm(); 
    comm[COL].SplitComm();
    
    PRINT("\n[OLD] ID: %d, rank_id: %d, size: %d\n", 0, rank_id, np);
    PRINT("[ROW] ID: %d, rank_id: %d, size: %d\n",   
          comm[ROW].group_id, comm[ROW].rank_id, comm[ROW].np);
    PRINT("[COL] ID: %d, rank_id: %d, size: %d\n\n", 
          comm[COL].group_id, comm[COL].rank_id, comm[COL].np);
  }
  int num_blk_at_row = sub_row_size/blk_size;
  int num_blk_at_col = sub_col_size/blk_size;

  // Be careful!!
  int rowBuff_size = blk_size * blk_size;
  int colBuff_size = blk_size * blk_size;

  FLOAT *vizBuff = (FLOAT  *)malloc(sizeof(FLOAT) * sub_mat_size);
  FLOAT *rowBuff = (FLOAT  *)malloc(sizeof(FLOAT) * rowBuff_size);
  FLOAT *colBuff = (FLOAT  *)malloc(sizeof(FLOAT) * colBuff_size);
  FLOAT *rowBuff_tmp = rowBuff; 
  FLOAT *colBuff_tmp = colBuff;
  FLOAT *rowBuff_to_erase = rowBuff;
  FLOAT *colBuff_to_erase = colBuff;

  PRINT("# blk at row : %d\trowBuff size : %d\n# blk at col : %d\tcolBuff size : %d\n\n", 
        num_blk_at_row, rowBuff_size, num_blk_at_col, colBuff_size);

  //  MPI_Win win[np_col];
  int i=0;
//  for(i=0; i<np_col; i++) {
//    if(comm[ROW].rank_id == 0) {
//      MPI_Win_create(vizBuff, sizeof(FLOAT) * sub_mat_size, sizeof(FLOAT), MPI_INFO_NULL, 
//                     comm[ROW].communicator, &(win[i]));
//    }
//    else {
//      MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, comm[ROW].communicator, &(win[i]));
//    }
//  }
#if 1

  MPI_Datatype colBuffType; // row major
  MPI_Datatype rowBuffType; // col major

  // blk_size * sub_col_size = col buff size
  // row major
  PRINT("[colbuf] MPI_Type_vector(%d, %d, %d)\n", blk_size, sub_col_size, sub_row_size);
  MPI_Type_vector(blk_size, blk_size, sub_row_size, COMM_MAT_DATATYPE, &colBuffType);

  // blk_size * sub_row_size = row buff size
  // col major
  PRINT("[rowbuf] MPI_Type_vector(%d, %d, %d)\n", blk_size, sub_row_size, sub_col_size);
  MPI_Type_vector(blk_size, blk_size, sub_col_size, COMM_MAT_DATATYPE, &rowBuffType);

  MPI_Type_commit(&colBuffType);
  MPI_Type_commit(&rowBuffType);
  int idx=1;
  int total_num_blk_at_row_or_col = mat_dim / blk_size;

  PRINT("\n\n");
  PRINT("Total # of blk: %d\n", total_num_blk_at_row_or_col);
  PRINT("\nRank #%d == Sub Mat Check ====================\n", rank_id);
  print_matrix_1D(matA, sub_mat_size);
  print_matrix_1D(matB, sub_mat_size);
  PRINT("==============================================\n\n\n\n");
  FLOAT *matAA = matA;
//  double comm_start = MPI_Wtime();
//  double mm_overhead = 0.0;
//  double bcast_overhead = 0.0;
  for(idx=0; idx < total_num_blk_at_row_or_col; ++idx) {

    PRINT("=== [CHECK %d/%d] [row : %d~%d, col : %d~%d] ===\n", idx, total_num_blk_at_row_or_col, 
            (comm[ROW].rank_id) * num_blk_at_row, (comm[ROW].rank_id + 1) * num_blk_at_row, 
            (comm[COL].rank_id) * num_blk_at_col, (comm[COL].rank_id + 1) * num_blk_at_col);
    PRINT("matA[%d] = %lf\n", idx, matA[idx]);
    int root_row = idx/num_blk_at_row;
    int root_col = idx/num_blk_at_col;
    PRINT("check %d=<%d<%d\n", comm[ROW].rank_id, root_row, comm[ROW].rank_id + 1);
    // Be careful of idx/num_blk_at_row
     // int blk_idx = blk_size * (idx % num_blk_at_row);
     // PRINT("matA : %p, idx:%d (%p %3.3lf)\n", matA, blk_idx, matA+2, *(matA+2));
//    double bcast_start = MPI_Wtime();
    if((comm[ROW].rank_id <= root_row) && (root_row < (comm[ROW].rank_id + 1))) {

      PRINT("[ROW] rank #%d (%d,%d) Doit Root #%d!!\n", rank_id, comm[ROW].rank_id, comm[COL].rank_id, root_row);

      int blk_idx = blk_size * (idx % num_blk_at_row);
      colBuff = matA + blk_idx;
      PRINT("matA : %p, idx:%d (%p %3.3lf)\n", matA, blk_idx, matA, matA[2]);
      matA = matAA;
      MPI_Bcast(colBuff, 
                1, colBuffType, root_row, comm[ROW].communicator);

      //Transpose(colBuff, matA, blk_size*(idx%num_blk_at_row), sub_col_size, blk_size, sub_row_size);
      Transpose(colBuff, matA, blk_size*(idx%num_blk_at_row), blk_size, blk_size, sub_row_size);
    } 
    else {
//      PRINT("[ROW] rank #%d (%d,%d) Doit Root #%d!!\n", rank_id, comm[ROW].rank_id, comm[COL].rank_id, root_row);

      MPI_Bcast(colBuff, colBuff_size, COMM_MAT_DATATYPE, root_row, comm[ROW].communicator);

    }


    PRINT("%d=<%d<%d\n" ,comm[COL].rank_id, root_col, comm[COL].rank_id + 1);
    // Be careful of idx/num_blk_at_col
    if((comm[COL].rank_id <= root_col) && (root_col < (comm[COL].rank_id + 1))) {

      PRINT("[COL] rank #%d (%d,%d) Doit Root #%d!!\n", rank_id, comm[ROW].rank_id, comm[COL].rank_id, root_col);
      rowBuff = matB + blk_size * (idx % num_blk_at_col);
      MPI_Bcast(rowBuff, 
                1, rowBuffType, root_col, comm[COL].communicator);
    }
    else {
//      PRINT("[COL] rank #%d (%d,%d) Doit Root #%d!!\n", rank_id, comm[ROW].rank_id, comm[COL].rank_id, root_col);

      MPI_Bcast(rowBuff, rowBuff_size, COMM_MAT_DATATYPE, root_col, comm[COL].communicator);

    }

    PRINT("\nRank #%d =====================================\n", rank_id);
    print_matrix_1D(rowBuff, rowBuff_size);
    print_matrix_1D(colBuff, colBuff_size);
    PRINT("==============================================\n\n");
    
    // colBuff : row major
    // rowBuff : col major
//    int i,j;
    PRINT("col buf\n");
    print_matrix_2D(colBuff, blk_size, blk_size);
//    for(i=0; i<sub_col_size; ++i) {
//      for(j=0; j<blk_size; ++j) {
//        PRINT("%3.3f ", colBuff[i * sub_col_size + j]);
//      }
//      PRINT("\n");
//    }
//
//    PRINT("\n");
    PRINT("row buf\n");
    print_matrix_2D(rowBuff, blk_size, blk_size);
//    for(i=0; i<blk_size; ++i) {
//      for(j=0; j<sub_row_size; ++j) {
//        PRINT("%3.3f ", rowBuff[j * blk_size + i]);
//      }
//      PRINT("\n");
//    }
//    PRINT("\n");
#if 1
#if _CUDA
    //CUDA_MatMul(result, colBuff, rowBuff, blk_size, blk_size, blk_size);
#endif

//    double mm_start = MPI_Wtime();
    OMP_OneLvMatMul(result, colBuff, rowBuff, blk_size, blk_size, blk_size, MIN_DIM_L1);
//    OMP_ThreeLvMatMul(result, colBuff, rowBuff, blk_size, blk_size, blk_size, MIN_DIM_L1, MIN_DIM_L2, MIN_DIM_L3);
    //OMP_ThreeLvMatMul(result, input[0], input[1], mat_dim, mat_dim, mat_dim, l1_blk, l2_blk, l3_blk);
//    mm_overhead += MPI_Wtime() - mm_start;
//    int dim[DIM] = {sub_col_size, sub_row_size, ROW_ROW};
//    OneLvMatMul(result, colBuff, rowBuff, 0, blk_size, 0, blk_size, 0, blk_size, MIN_DIM_L1, dim);
//    if(idx == 0) { 
//      OrigMatMul(result, colBuff, rowBuff, sub_col_size, sub_row_size, blk_size);
//    }
//    else {
//      OrigMatMulAcc(result, colBuff, rowBuff, sub_col_size, sub_row_size, blk_size);
//    }
#else
    int blockSizeForL1 = MIN_DIM_L1;
    int blockSizeForL2 = MIN_DIM_L2;
    int blockSizeForL3 = MIN_DIM_L3;
    int dim[2] = {sub_row_size, blk_size};
    ThreeLvMatMul(result, colBuff, rowBuff, 0, sub_col_size, 0, sub_row_size, 0, blk_size, 
                  blockSizeForL1, blockSizeForL2, blockSizeForL3, dim);
#endif
  }
#endif
  PRINT("\nRank #%d == Result ===========================\n", rank_id);
  //print_matrix_1D(result, sub_mat_size);
  int j;
  for(i=0; i<sub_col_size; ++i) {
    for(j=0; j<sub_row_size; ++j) {

      PRINT("%3.3f ", result[i * sub_row_size + j]);

    }
    PRINT("\n");
  }
  PRINT("==============================================\n\n");

//  double comm_elapsed = MPI_Wtime() - comm_start;
//  printf("total : %lf, comm:%lf, mm:%lf\n", comm_elapsed, comm_elapsed - mm_overhead, mm_overhead);
//  for() {
//    MPI_Bcast(colBuffer, colBuff_size, COMM_MAT_DATATYPE, rowIdx, comm[ROW].communicator);
//    for() {
//      MPI_Bcast(rowBuffer, rowBuff_size, COMM_MAT_DATATYPE, rowIdx, comm[ROW].communicator);
//    }
//  }
  free(rowBuff_to_erase);
  free(colBuff_to_erase);
  *tempBuff = vizBuff;
}

#endif
