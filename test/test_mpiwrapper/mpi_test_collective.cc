#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "cds.h"

//#define PRINTF(...) {printf("%d:",rank);printf(__VA_ARGS__);}
#define PRINTF(...) {fprintf(fp, __VA_ARGS__);}

FILE *fp;

/* Gather data from a vector to contiguous */

// test collective communicaitons
int test_gather( int argc, char **argv )
{
  MPI_Datatype vec;
  MPI_Comm comm;
  double *vecin, *vecout;
  int minsize = 2, count;
  int root_mpi, i, n, stride, errs = 0;
  int rank, size;
  int num_reexec = 0;

  MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  char name[20]; 
  sprintf(name, "tmp.out.%d", rank);
  fp = fopen(name, "w");
  if (fp == NULL)
  {
    printf("cannot open new file (%p)!\n", name);
    return 1;
  }

  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(root);

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = root->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  PRINTF("Print level 1 child CD comm_log_ptr info...\n");
  if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
    child1_0->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  // real computation
  for (root_mpi=0; root_mpi<size; root_mpi++) {
    for (count = 1; count < 65000; count = count * 2) {
      n = 12;
      stride = 10;
      vecin = (double *)malloc( n * stride * size * sizeof(double) );
      vecout = (double *)malloc( size * n * sizeof(double) );
      
      MPI_Type_vector( n, 1, stride, MPI_DOUBLE, &vec );
      MPI_Type_commit( &vec );
      
      for (i=0; i<n*stride; i++) vecin[i] =-2;
      for (i=0; i<n; i++) vecin[i*stride] = rank * n + i;
      
      MPI_Gather( vecin, 1, vec, vecout, n, MPI_DOUBLE, root_mpi, comm );
      if (rank == root_mpi) {
        for (i=0; i<n*size; i++) {
          if (vecout[i] != i) {
            errs++;
            if (errs < 10) {
              PRINTF("vecout[%d]=%d\n", i, (int)vecout[i]);
            }
          }
        }
      }
      MPI_Type_free( &vec );
      free( vecin );
      free( vecout );
    }
  }

  // insert error
  if (rank%3==0 && num_reexec < 1)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child1_0->CDAssert(false);
  }

  PRINTF("Print level 1 child CD comm_log_ptr info...\n");
  if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
    child1_0->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Complete root CD...\n");
  CD_Complete(root);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  fclose(fp);

  MPI_Finalize();
  return 0;
}

// test collective communicaitons
int test_gatherv( int argc, char **argv )
{
  int num_reexec = 0;
  int buffer[7];
  int rank, size, i;
  int receive_counts[4] = { 0, 1, 2, 3 };
  int receive_displacements[4] = { 0, 0, 1, 4 };
  MPI_Comm comm;

  MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  char name[20]; 
  sprintf(name, "tmp.out.%d", rank);
  fp = fopen(name, "w");
  if (fp == NULL)
  {
    printf("cannot open new file (%p)!\n", name);
    return 1;
  }

  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(root);

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = root->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  PRINTF("Print level 1 child CD comm_log_ptr info...\n");
  if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
    child1_0->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  // real computation starts...
  if (size != 4)
  {
    if (rank == 0)
    {
      PRINTF("Please run with 4 processes\n");
    }
    MPI_Finalize();
    return 0;
  }

  // use following computation to represent preservation
  for (i=0; i<rank; i++)
  {
    buffer[i] = rank;
  }
  for (i=rank;i<7;i++)
  {
    buffer[i]=-1;
  }
  MPI_Gatherv(buffer, rank, MPI_INT, buffer, receive_counts, 
              receive_displacements, MPI_INT, 0, MPI_COMM_WORLD);
  if (rank == 0)
  {
    for (i=0; i<7; i++)
    {
      PRINTF("[%d]", buffer[i]);
    }
    PRINTF("\n");
  }

  // insert error
  if (rank%3==0 && num_reexec < 1)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child1_0->CDAssert(false);
  }

  PRINTF("Print level 1 child CD comm_log_ptr info...\n");
  if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
    child1_0->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Complete root CD...\n");
  CD_Complete(root);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  fclose(fp);

  MPI_Finalize();
  return 0;
}

// test_bcast
#define ROOT 0
#define NUM_REPS 5
#define NUM_SIZES 3
int test_bcast( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;
  int *buf;
  int i, reps, n;
  int bVerify = 1;
  long sizes[NUM_SIZES] = { 100, 64*1024, 128*1024 };
  int num_errors=0, tot_errors;

  MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  char name[20]; 
  sprintf(name, "tmp.out.%d", rank);
  fp = fopen(name, "w");
  if (fp == NULL)
  {
    printf("cannot open new file (%p)!\n", name);
    return 1;
  }

  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(root);

  PRINTF("\n--------------------------------------------------------------\n");
  
  // use following computation to represent preservation
  // move space allocation out of child cd because we do not have malloc wrapper here
  buf = (int *) malloc(sizes[NUM_SIZES-1]*sizeof(MPI_INT));
  PRINTF("buf=%p, size=%ld\n", buf, sizes[NUM_SIZES-1]*sizeof(MPI_INT));

  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = root->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  PRINTF("Print level 1 child CD comm_log_ptr info...\n");
  if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
    child1_0->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  // use following computation to represent preservation
  PRINTF("Application initialization...\n");
  memset(buf, 0, sizes[NUM_SIZES-1]*sizeof(MPI_INT));

  // real computation starts...
  PRINTF("Application computation starts...\n");
  for (n=0; n<NUM_SIZES; n++)
  {
    if (rank == ROOT)
    {
      PRINTF("bcasting %ld MPI_INTs %d times\n", sizes[n], NUM_REPS);
    }
    for (reps=0; reps < NUM_REPS; reps++)
    {
      if (bVerify)
      {
        if (rank == ROOT)
        {
          for (i=0; i<sizes[n]; i++)
          {
            buf[i] = 1000000 * (n * NUM_REPS + reps) + i;
          }
        }
        else
        {
          for (i=0; i<sizes[n]; i++)
          {
            buf[i] = -1 - (n * NUM_REPS + reps);
          }
        }
      }
      MPI_Bcast(buf, sizes[n], MPI_INT, ROOT, MPI_COMM_WORLD);
      if (bVerify)
      {
        num_errors = 0;
        for (i=0; i<sizes[n]; i++)
        {
          if (buf[i] != 1000000 * (n * NUM_REPS + reps) + i)
          {
            num_errors++;
            if (num_errors < 10)
            {
              PRINTF("Error: Rank=%d, n=%d, reps=%d, i=%d, buf[i]=%d expected=%d\n", 
                      rank, n, reps, i, buf[i], 1000000 * (n * NUM_REPS + reps) +i);
            }
          }
        }
        if (num_errors >= 10)
        {
          PRINTF("Error: Rank=%d, num_errors = %d\n", rank, num_errors);
        }
      }
    }
  }

  // insert error
  if (rank%3==0 && num_reexec < 1)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child1_0->CDAssert(false);
  }

  PRINTF("Print level 1 child CD comm_log_ptr info...\n");
  if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
    child1_0->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();


  MPI_Reduce( &num_errors, &tot_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
  if (rank == 0 && tot_errors == 0)
    PRINTF("!!! No Errors !!!\n");
  free(buf);

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Complete root CD...\n");
  CD_Complete(root);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  fclose(fp);

  MPI_Finalize();
  return 0;
}

int main(int argc, char** argv)
{
  int ret;
  //ret = test_gather(argc, argv);
  
  //// test_gatherv requires 4 ranks
  //ret = test_gatherv(argc, argv);

  ret = test_bcast(argc, argv);

  return ret;
}

