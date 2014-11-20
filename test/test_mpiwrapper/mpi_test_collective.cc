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

  PRINTF("\n\n...............Test MPI_Gather...............\n\n");
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

  PRINTF("\n\n...............Test MPI_Gatherv...............\n\n");
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

  PRINTF("\n\n...............Test MPI_Bcast...............\n\n");
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

// test MPI_Allgather
#define MAX_PROCESSES 10
int test_allgather( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;
  int i,j;
  int table[MAX_PROCESSES][MAX_PROCESSES];
  int errors=0;
  int participants;

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

  PRINTF("\n\n...............Test MPI_Allgather...............\n\n");
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

  PRINTF("-----------------------------\n");
  // use following computation to represent preservation
  PRINTF("Application initialization...\n");
  for (i=0; i<MAX_PROCESSES;i++)
    for (j=0; j<MAX_PROCESSES; j++)
      table[i][j] = i+j;

  // real computation starts...
  PRINTF("Application computation starts...\n");

  /* Exactly MAX_PROCESSES processes can participate */
  if ( size >= MAX_PROCESSES ) participants = MAX_PROCESSES;
  else
  {
    PRINTF("Number of processors must be at least %d\n", MAX_PROCESSES);
    MPI_Abort( MPI_COMM_WORLD, 1 );
  }
  if ( (rank < participants) ) {
    /* Determine what rows are my responsibility */
    int block_size = MAX_PROCESSES / participants;
    int begin_row = rank * block_size;
    int end_row = (rank+1) * block_size;
    int send_count = block_size * MAX_PROCESSES;
    int recv_count = send_count;
    /* Paint my rows my color */
    for (i=begin_row; i<end_row ;i++)
      for (j=0; j<MAX_PROCESSES; j++)
        table[i][j] = rank + 10;
    /* Everybody gets the gathered table */
    MPI_Allgather(&table[begin_row][0], send_count, MPI_INT, 
                  &table[0][0], recv_count, MPI_INT, MPI_COMM_WORLD);
    /* Everybody should have the same table now, */
    for (i=0; i<MAX_PROCESSES;i++) {
      if ( (table[i][0] - table[i][MAX_PROCESSES-1] !=0) ) 
        errors++;
    }
  } 
  PRINTF("-----------------------------\n");

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

  PRINTF("-----------------------------\n");
  PRINTF("Print out error information, if any...\n");
  if (errors)
  {
    PRINTF("[%d] done with ERRORS(%d)!\n", rank, errors);
  }
  else
  {
    PRINTF("\n!!! No Error !!!\n");

    PRINTF("\nTable information (table[%d][%d]):\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%d\t",table[i][j]);
      PRINTF("\n");
    }
  }
  PRINTF("-----------------------------\n");

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

// test MPI_Allgatherv
int test_allgatherv( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int i,j;
  int table[MAX_PROCESSES][MAX_PROCESSES];
  int errors=0;
  int participants;
  int displs[MAX_PROCESSES];
  int recv_counts[MAX_PROCESSES];

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

  PRINTF("\n\n...............Test MPI_Allgatherv...............\n\n");
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

  PRINTF("-----------------------------\n");
  // use following computation to represent preservation
  PRINTF("Application initialization...\n");
  for (i=0; i<MAX_PROCESSES;i++)
    for (j=0; j<MAX_PROCESSES; j++)
      table[i][j] = i+j;
  PRINTF("\nTable information (table[%d][%d]) after initialization:\n", MAX_PROCESSES, MAX_PROCESSES);
  for (i=0; i<MAX_PROCESSES;i++){
    for (j=0; j<MAX_PROCESSES; j++)
      PRINTF("%3d\t",table[i][j]);
    PRINTF("\n");
  }

  // real computation starts...
  PRINTF("Application computation starts...\n");

  /* A maximum of MAX_PROCESSES processes can participate */
  if ( MAX_PROCESSES % size || size > MAX_PROCESSES )
  {
    PRINTF( "Number of processors must divide into %d\n", MAX_PROCESSES );
    MPI_Abort( MPI_COMM_WORLD, 1 );
  }
  participants = size; 
  if ( (rank < participants) ) 
  {
    /* Determine what rows are my responsibility */
    int block_size = MAX_PROCESSES / participants;
    int begin_row = rank * block_size;
    int end_row = (rank+1) * block_size;
    int send_count = block_size * MAX_PROCESSES;

    /* Fill in the displacements and recv_counts */
    for (i=0; i<participants; i++) {
      displs[i] = i * block_size * MAX_PROCESSES;
      recv_counts[i] = send_count;
    }
    /* Paint my rows my color */
    for (i=begin_row; i<end_row ;i++)
      for (j=0; j<MAX_PROCESSES; j++)
        table[i][j] = rank + 10;

    /* Everybody gets the gathered data */
    MPI_Allgatherv(&table[begin_row][0], send_count, MPI_INT, 
                  &table[0][0], recv_counts, displs, MPI_INT, MPI_COMM_WORLD);
    /* Everybody should have the same table now.
       The entries are:
       Table[i][j] = (i/block_size) + 10;
    */

    for (i=0; i<MAX_PROCESSES;i++) 
      if ( (table[i][0] - table[i][MAX_PROCESSES-1] !=0) ) 
        errors++;
    for (i=0; i<MAX_PROCESSES;i++) {
      for (j=0; j<MAX_PROCESSES;j++) {
        if (table[i][j] != (i/block_size) + 10) errors++;
      }
    }
    if (errors) {
      /* Print out table if there are any errors */
      for (i=0; i<MAX_PROCESSES;i++) {
        PRINTF("\n");
        for (j=0; j<MAX_PROCESSES; j++)
          PRINTF("%3d\t",table[i][j]);
      }
      PRINTF("\n");
    }
  } 

  PRINTF("-----------------------------\n");

  // insert error
  if (rank%3==0 && num_reexec < 2)
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

  PRINTF("-----------------------------\n");
  PRINTF("Print out error information, if any...\n");
  if (errors)
  {
    PRINTF("[%d] done with ERRORS(%d)!\n", rank, errors);
    PRINTF("\nTable information (table[%d][%d]):\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%3d\t",table[i][j]);
      PRINTF("\n");
    }
  }
  else
  {
    PRINTF("\n!!! No Error !!!\n");

    PRINTF("\nTable information (table[%d][%d]):\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%d\t",table[i][j]);
      PRINTF("\n");
    }
  }
  PRINTF("-----------------------------\n");

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

// test MPI_Reduce
int test_reduce( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int errs = 0;
  int root;
  int *sendbuf, *recvbuf, i;
  int minsize = 2, count; 

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

  PRINTF("\n\n...............Test MPI_Allgatherv...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  // use following computation to represent preservation
  PRINTF("Application initialization...\n");

  // real computation starts...
  PRINTF("Application computation starts...\n");
  int err_happened=0;
  for (count = 1; count < 60000; count = count * 2) {
    PRINTF("-----------------------------\n");
    PRINTF("Begin child cd of level 1 ...\n");
    CD_Begin(child1_0);

    PRINTF("count=%d\n", count);
    sendbuf = (int *)malloc( count * sizeof(MPI_INT) );
    recvbuf = (int *)malloc( count * sizeof(MPI_INT) );
    for (root = 0; root < size; root ++) {
      for (i=0; i<count; i++) sendbuf[i] = i;
      for (i=0; i<count; i++) recvbuf[i] = -1;
      MPI_Reduce( sendbuf, recvbuf, count, MPI_INT, MPI_SUM, root, comm );
      if (rank == root) {
        errs=0;
        for (i=0; i<count; i++) {
          if (recvbuf[i] != i * size) {
            errs++;
          }
        }
        if (errs) {
          err_happened=1;
          PRINTF("sendbuf:\n");
          for (i=0; i<count; i++) 
          {
            PRINTF("%3d\t", sendbuf[i]);
            if (i%20==0) PRINTF("\n");
          }
          PRINTF("recvbuf:\n");
          for (i=0; i<count; i++) 
          {
            PRINTF("%3d\t", recvbuf[i]);
            if (i%20==0) PRINTF("\n");
          }
        }
        else {
          PRINTF("\n...!!! No Error !!!...\n");
        }
      }
    }
    free( sendbuf );
    free( recvbuf );

    PRINTF("-----------------------------\n");
    PRINTF("Print level 1 child CD comm_log_ptr info...\n");
    if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
      child1_0->ptr_cd()->comm_log_ptr_->Print();
    else
      PRINTF("NULL pointer for strict CD!\n");

    // insert error
    if (rank%3==0 && num_reexec < 2)
    {
      PRINTF("Insert error #%d...\n", num_reexec);
      num_reexec++;
      child1_0->CDAssert(false);
    }

    PRINTF("Complete child CD of level 1 ...\n");
    CD_Complete(child1_0);
  }
  if (err_happened) {
    PRINTF("\n...Error happened!!!...\n");
  }
  else {
    PRINTF("\n...All done!!...\n");
  }

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  //MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  fclose(fp);

  MPI_Finalize();
  return 0;
}

int main(int argc, char** argv)
{
  int ret = 0;
  //ret = test_gather(argc, argv);
  
  //// test_gatherv requires 4 processes
  //ret = test_gatherv(argc, argv);

  //ret = test_bcast(argc, argv);

  //// test_allgather requires at least 10 processes
  //ret = test_allgather(argc, argv);

  //// test_allgatherv requires 10 processes
  //ret = test_allgatherv(argc, argv);

  ret = test_reduce(argc, argv);
  
  return ret;
}

