#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "cds.h"

//#define PRINTF(...) {printf("%d:",rank);printf(__VA_ARGS__);}
#define PRINTF(...) {fprintf(fp, __VA_ARGS__);}

FILE *fp;

// test basic
int test_basic( int argc, char **argv )
{
  MPI_Comm comm;
  int rank, size;
  int num_reexec = 0;

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  PRINTF("\n\n...............Test Basics...............\n\n");
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

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}

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

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

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

  //fclose(fp);

  //MPI_Finalize();
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

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

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
    //MPI_Finalize();
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

  //fclose(fp);

  //MPI_Finalize();
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

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

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

  //fclose(fp);

  //MPI_Finalize();
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
  int recv_table[MAX_PROCESSES][MAX_PROCESSES];
  int errors=0;
  int participants;

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

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
    {
      table[i][j] = i+j;
      recv_table[i][j] = i+j;
    }

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

    PRINTF("\nRecv Table information (recv_table[%d][%d]) before MPI_Allgather:\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%3d\t",recv_table[i][j]);
      PRINTF("\n");
    }
    /* Everybody gets the gathered table */
    MPI_Allgather(&table[begin_row][0], send_count, MPI_INT, 
                  &recv_table[0][0], recv_count, MPI_INT, MPI_COMM_WORLD);

    PRINTF("\nRecv Table information (recv_table[%d][%d]) after MPI_Allgather:\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%3d\t",recv_table[i][j]);
      PRINTF("\n");
    }

    /* Everybody should have the same table now, */
    for (i=0; i<MAX_PROCESSES;i++) {
      if ( (recv_table[i][0] - recv_table[i][MAX_PROCESSES-1] !=0) ) 
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
        PRINTF("%3d\t",table[i][j]);
      PRINTF("\n");
    }

    PRINTF("\nRecv Table information (recv_table[%d][%d]):\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%3d\t",recv_table[i][j]);
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

  //fclose(fp);

  //MPI_Finalize();
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
  int recv_table[MAX_PROCESSES][MAX_PROCESSES];
  int errors=0;
  int participants;
  int displs[MAX_PROCESSES];
  int recv_counts[MAX_PROCESSES];

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

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
    {
      table[i][j] = i+j;
      recv_table[i][j] = i+j;
    }

  PRINTF("\nTable information (recv_table[%d][%d]) after initialization:\n", MAX_PROCESSES, MAX_PROCESSES);
  for (i=0; i<MAX_PROCESSES;i++){
    for (j=0; j<MAX_PROCESSES; j++)
      PRINTF("%3d\t",recv_table[i][j]);
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
                  &recv_table[0][0], recv_counts, displs, MPI_INT, MPI_COMM_WORLD);
    /* Everybody should have the same table now.
       The entries are:
       Table[i][j] = (i/block_size) + 10;
    */

    for (i=0; i<MAX_PROCESSES;i++) 
      if ( (recv_table[i][0] - recv_table[i][MAX_PROCESSES-1] !=0) ) 
        errors++;
    for (i=0; i<MAX_PROCESSES;i++) {
      for (j=0; j<MAX_PROCESSES;j++) {
        if (recv_table[i][j] != (i/block_size) + 10) errors++;
      }
    }
    if (errors) {
      /* Print out table if there are any errors */
      for (i=0; i<MAX_PROCESSES;i++) {
        PRINTF("\n");
        for (j=0; j<MAX_PROCESSES; j++)
          PRINTF("%3d\t",recv_table[i][j]);
      }
      PRINTF("\n");
    }
    else{
      PRINTF("\n!!! NO Error!!!\n");
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
    PRINTF("\nTable information (recv_table[%d][%d]):\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%3d\t",recv_table[i][j]);
      PRINTF("\n");
    }
  }
  else
  {
    PRINTF("\n!!! No Error !!!\n");

    PRINTF("\nTable information (recv_table[%d][%d]):\n", MAX_PROCESSES, MAX_PROCESSES);
    for (i=0; i<MAX_PROCESSES;i++){
      for (j=0; j<MAX_PROCESSES; j++)
        PRINTF("%d\t",recv_table[i][j]);
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

  //fclose(fp);

  //MPI_Finalize();
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

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("\n\n...............Test MPI_Reduce...............\n\n");
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

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


// test MPI_Allreduce
int test_allreduce( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int count = 1000;
  int *in, *out, *sol;
  int i, fnderr=0;

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("\n\n...............Test MPI_Allreduce...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  // application data allocation
  PRINTF("-----------------------------\n");
  PRINTF("Application data allocation...\n");
  in = (int *)malloc( count * sizeof(MPI_INT) );
  out = (int *)malloc( count * sizeof(MPI_INT) );
  sol = (int *)malloc( count * sizeof(MPI_INT) );

  // use following computation to represent preservation
  PRINTF("Application initialization...\n");
  for (i=0; i<count; i++)
  {
    *(in + i) = i;
    *(sol + i) = i*size;
    *(out + i) = 0;
  }

  // real computation starts...
  PRINTF("Application computation starts...\n");
  MPI_Allreduce( in, out, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
  for (i=0; i<count; i++)
  {
    if (*(out + i) != *(sol + i))
    {
      fnderr++;
    }
  }
  if (fnderr)
  {
    PRINTF( "(%d) Error for type MPI_INT and op MPI_SUM\n", rank );
  }
  else
  {
    PRINTF("\n...NO Error!!!...\n");
  }

  free( in );
  free( out );
  free( sol );

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

  PRINTF("-----------------------------\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif
// test MPI_Alltoall
int test_alltoall( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int chunk = 32;
  int i;
  int *sb;
  int *rb;
  int status, gstatus;

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("\n\n...............Test MPI_Alltoall...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  // application data allocation
  PRINTF("-----------------------------\n");
  PRINTF("Application data allocation...\n");
  sb = (int *)malloc(size*chunk*sizeof(MPI_INT));
  if ( !sb ) {
    perror( "can't allocate send buffer" );fflush(stderr);
    MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
  }
  rb = (int *)malloc(size*chunk*sizeof(MPI_INT));
  if ( !rb ) {
    perror( "can't allocate recv buffer");fflush(stderr);
    free(sb);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  // use following computation to represent preservation
  PRINTF("Application initialization...\n");
  for ( i=0 ; i < size*chunk ; ++i ) {
    sb[i] = rank + 1;
    rb[i] = 0;
  }

  // real computation starts...
  PRINTF("Application computation starts...\n");
  status = MPI_Alltoall(sb, chunk, MPI_INT, rb, chunk, MPI_INT, MPI_COMM_WORLD);
  MPI_Allreduce( &status, &gstatus, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
  if (rank == 0) {
    if (gstatus != 0) {
      PRINTF("all_to_all returned %d\n",gstatus);
    }
  }

  PRINTF("\nsendbuf information:");
  for (i=0; i < size*chunk; i++)
  {
    if (i%chunk==0) PRINTF("\n");
    PRINTF("%2d\t", sb[i]);
  }
  PRINTF("\n\nrecvbuf information:");
  for (i=0; i < size*chunk; i++)
  {
    if (i%chunk==0) PRINTF("\n");
    PRINTF("%2d\t", rb[i]);
  }
  PRINTF("\n")
  free(sb);
  free(rb);

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

  PRINTF("-----------------------------\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


// test MPI_Alltoallv
int test_alltoallv( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int *sbuf, *rbuf;
  int *sendcounts, *recvcounts, *rdispls, *sdispls;
  int i, j, *p, err=0;

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("\n\n...............Test MPI_Alltoallv...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  // application data allocation
  PRINTF("-----------------------------\n");
  PRINTF("Application data allocation...\n");
  sbuf = (int *)malloc( size * size * sizeof(MPI_INT) );
  rbuf = (int *)malloc( size * size * sizeof(MPI_INT) );
  if (!sbuf || !rbuf) {
    fprintf( stderr, "Could not allocated buffers!\n" );
    MPI_Abort( comm, 1 );
  }
  /* Create and load the arguments to alltoallv */
  sendcounts = (int *)malloc( size * sizeof(MPI_INT) );
  recvcounts = (int *)malloc( size * sizeof(MPI_INT) );
  rdispls = (int *)malloc( size * sizeof(MPI_INT) );
  sdispls = (int *)malloc( size * sizeof(MPI_INT) );
  if (!sendcounts || !recvcounts || !rdispls || !sdispls) {
    fprintf( stderr, "Could not allocate arg items!\n" );fflush(stderr);
    MPI_Abort( comm, 1 );
  }

  // use the following initialization to represent preservations
  PRINTF("-----------------------------\n");
  PRINTF("Application initialization...\n");
  /* Load up the buffers */
  for (i=0; i<size*size; i++) {
    sbuf[i] = i + 100*rank;
    rbuf[i] = -i;
  }
  PRINTF("\nbefore MPI_Alltoall communication:");
  PRINTF("\nsbuf information:");
  for (i=0; i<size*size; i++) {
    if (i%size==0) PRINTF("\n");
    PRINTF("%2d\t", sbuf[i]);
  }
  PRINTF("\n");
  PRINTF("\nrbuf information:");
  for (i=0; i<size*size; i++) {
    if (i%size==0) PRINTF("\n");
    PRINTF("%2d\t", rbuf[i]);
  }
  PRINTF("\n");
  for (i=0; i<size; i++) {
    sendcounts[i] = i;
    recvcounts[i] = rank;
    rdispls[i] = i * rank;
    sdispls[i] = (i * (i+1))/2;
  }
  for (i=0; i<size; i++) {
    PRINTF("sendcounts[%d]=%d\t",i,sendcounts[i]);
  }
  PRINTF("\n");
  for (i=0; i<size; i++) {
    PRINTF("sdispls[%d]=%d\t",i,sdispls[i]);
  }
  PRINTF("\n");
  for (i=0; i<size; i++) {
    PRINTF("recvcounts[%d]=%d\t",i,recvcounts[i]);
  }
  PRINTF("\n");
  for (i=0; i<size; i++) {
    PRINTF("rdispls[%d]=%d\t",i,rdispls[i]);
  }
  PRINTF("\n");

  // real computation starts...
  PRINTF("Application computation starts...\n");
  MPI_Alltoallv( sbuf, sendcounts, sdispls, MPI_INT,
                 rbuf, recvcounts, rdispls, MPI_INT, comm );

  PRINTF("\nafter MPI_Alltoall communication:");
  PRINTF("\nsbuf information:");
  for (i=0; i<size*size; i++) {
    if (i%size==0) PRINTF("\n");
    PRINTF("%2d\t", sbuf[i]);
  }
  PRINTF("\nrbuf information:");
  for (i=0; i<size*size; i++) {
    if (i%size==0) PRINTF("\n");
    PRINTF("%2d\t", rbuf[i]);
  }

  /* Check rbuf */
  for (i=0; i<size; i++) {
    p = rbuf + rdispls[i];
    for (j=0; j<rank; j++) {
      if (p[j] != i * 100 + (rank*(rank+1))/2 + j) {
        PRINTF( "[%d] got %d expected %d for %dth\n",
                  rank, p[j],(i*(i+1))/2 + j, j );
        err++;
      }
    }
  }
  if (err) {
    PRINTF("Error happens!!\n");
  }
  else {
    PRINTF("\n...No Error!!!...\n");
  }

  free( sdispls );
  free( rdispls );
  free( recvcounts );
  free( sendcounts );
  free( rbuf );
  free( sbuf );

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

  PRINTF("-----------------------------\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


// test MPI_Scatter
int test_scatter( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int i,j;
  int table[MAX_PROCESSES][MAX_PROCESSES];
  int row[MAX_PROCESSES];
  int errors=0;
  int participants;

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("sizeof(int)=%ld, and sizeof(MPI_INT)=%ld\n", sizeof(int), sizeof(MPI_INT));
  PRINTF("\n\n...............Test MPI_Scatter...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  // application data allocation
  PRINTF("-----------------------------\n");
  PRINTF("Application data allocation...\n");

  // use the following initialization to represent preservations
  PRINTF("-----------------------------\n");
  PRINTF("Application initialization...\n");

  // real computation starts...
  PRINTF("Application computation starts...\n");
  /* A maximum of MAX_PROCESSES processes can participate */
  if ( size > MAX_PROCESSES ) participants = MAX_PROCESSES;
  else participants = size;
  if ( (rank < participants) ) {
    int send_count = MAX_PROCESSES;
    int recv_count = MAX_PROCESSES;
                                 
    /* If I'm the root (process 0), then fill out the big table */
    if (rank == 0) 
      for ( i=0; i<participants; i++) 
        for ( j=0; j<MAX_PROCESSES; j++ ) 
          table[i][j] = i+j;

    /* Scatter the big table to everybody's little table */
    MPI_Scatter(&table[0][0], send_count, MPI_INT, 
                &row[0] , recv_count, MPI_INT, 0, MPI_COMM_WORLD);
                                                 
    /* Now see if our row looks right */
    for (i=0; i<MAX_PROCESSES; i++) 
      if ( row[i] != i+rank ) errors++;

    if (errors)
    {
      PRINTF("Error Happens!!\n");
    }
    else
    {
      PRINTF("No Error!!\n");
    }
  } 

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

  PRINTF("-----------------------------\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


// test MPI_Scatterv
int test_scatterv( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int i,j;
  int table[MAX_PROCESSES][MAX_PROCESSES];
  int row[MAX_PROCESSES];
  int errors=0;
  int participants;
  int displs[MAX_PROCESSES];
  int send_counts[MAX_PROCESSES];

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("sizeof(int)=%ld, and sizeof(MPI_INT)=%ld\n", sizeof(int), sizeof(MPI_INT));
  PRINTF("\n\n...............Test MPI_Scatterv...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  // application data allocation
  PRINTF("-----------------------------\n");
  PRINTF("Application data allocation...\n");

  // use the following initialization to represent preservations
  PRINTF("-----------------------------\n");
  PRINTF("Application initialization...\n");

  // real computation starts...
  PRINTF("Application computation starts...\n");
  /* A maximum of MAX_PROCESSES processes can participate */
  if ( size > MAX_PROCESSES ) participants = MAX_PROCESSES;
  else participants = size;
  if ( (rank < participants) ) {
    int recv_count = MAX_PROCESSES;

    /* If I'm the root (process 0), then fill out the big table and setup send_counts and displs arrays */
    if (rank == 0) 
      for ( i=0; i<participants; i++) {
        send_counts[i] = recv_count;
        displs[i] = i * MAX_PROCESSES;
        for ( j=0; j<MAX_PROCESSES; j++ ) 
          table[i][j] = i+j;
      }
                                 
      /* Scatter the big table to everybody's little table */
      MPI_Scatterv(&table[0][0], send_counts, displs, MPI_INT, 
                   &row[0] , recv_count, MPI_INT, 0, MPI_COMM_WORLD);
                                         
      /* Now see if our row looks right */
      for (i=0; i<MAX_PROCESSES; i++) 
        if ( row[i] != i+rank ) errors++;
  } 

  if (errors)
  {
    PRINTF("Error Happens!!\n");
  }
  else
  {
    PRINTF("No Error!!\n");
  }

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

  PRINTF("-----------------------------\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


// test MPI_Reduce_scatter
int test_reduce_scatter( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions
  int err = 0;
  int *sendbuf, recvbuf, *recvcounts;
  int i, sumval;

  //MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("sizeof(int)=%ld, and sizeof(MPI_INT)=%ld\n", sizeof(int), sizeof(MPI_INT));
  PRINTF("\n\n...............Test MPI_Reduce_scatter...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  // application data allocation
  PRINTF("-----------------------------\n");
  PRINTF("Application data allocation...\n");

  // use the following initialization to represent preservations
  PRINTF("-----------------------------\n");
  PRINTF("Application initialization...\n");

  // real computation starts...
  PRINTF("Application computation starts...\n");
  sendbuf = (int *) malloc( size * sizeof(MPI_INT) );
  for (i=0; i<size; i++) 
    sendbuf[i] = rank + i + 1;
  recvcounts = (int *)malloc( size * sizeof(MPI_INT) );
  for (i=0; i<size; i++) 
    recvcounts[i] = 1;
                
  MPI_Reduce_scatter( sendbuf, &recvbuf, recvcounts, MPI_INT, MPI_SUM, comm );
  for (i=0; i<size; i++) 
    PRINTF("sendbuf[%d]=%d\t", i, sendbuf[i]);
  PRINTF("\n");
                    
  sumval = size * rank + ((size - 1) * size)/2 + size;
  /* recvbuf should be size * (rank + i) */
  if (recvbuf != sumval) {
    err++;
    PRINTF( "Did not get expected value for reduce scatter\n" );
    PRINTF( "[%d] Got %d expected %d\n", rank, recvbuf, sumval );fflush(stdout);
  }
  PRINTF("recvbuf=%d, sumval=%d\n", recvbuf, sumval);

  if (err)
  {
    PRINTF("Error Happens!!\n");
  }
  else
  {
    PRINTF("No Error!!\n");
  }

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

  PRINTF("-----------------------------\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


// test MPI_Scan
void addem ( int *, int *, int *, MPI_Datatype * );
void assoc ( int *, int *, int *, MPI_Datatype * );
 
void addem( int *invec, int *inoutvec, int *len, MPI_Datatype *dtype)
{
  int i;
  for ( i=0; i<*len; i++ ) 
    inoutvec[i] += invec[i];
}
#define BAD_ANSWER 100000
/*
 The operation is inoutvec[i] = invec[i] op inoutvec[i] 
 (see 4.9.4). The order is important.
 Note that the computation is in process rank (in the communicator) order, independant of the root.
 */
void assoc( int *invec, int *inoutvec, int *len, MPI_Datatype *dtype)
{
  int i;
  for ( i=0; i<*len; i++ ) {
    if (inoutvec[i] <= invec[i] ) {
      int rank;
      MPI_Comm_rank( MPI_COMM_WORLD, &rank );
      PRINTF( "[%d] inout[0] = %d, in[0] = %d\n", rank, inoutvec[0], invec[0] );fflush(stderr);
      inoutvec[i] = BAD_ANSWER;
    }
    else 
      inoutvec[i] = invec[i];
  }
}

int test_scan( int argc, char **argv )
{
  int num_reexec = 0;
  MPI_Comm comm;
  int rank, size;

  // application definitions start
  int i;
  int data;
  int errors=0;
  int result = -100;
  int correct_result;
  MPI_Op op_assoc, op_addem;
  // application definitions end

  //MPI_Init( &argc, &argv );

  // application extra starts
  MPI_Op_create( (MPI_User_function *)assoc, 0, &op_assoc );
  MPI_Op_create( (MPI_User_function *)addem, 1, &op_addem );
  // application extra ends
  
  comm = MPI_COMM_WORLD;
  /* Determine the sender and receiver */
  MPI_Comm_rank( comm, &rank );
  MPI_Comm_size( comm, &size );

  //char name[20]; 
  //sprintf(name, "tmp.out.%d", rank);
  //fp = fopen(name, "w");
  //if (fp == NULL)
  //{
  //  printf("cannot open new file (%p)!\n", name);
  //  return 1;
  //}

  PRINTF("sizeof(int)=%ld, and sizeof(MPI_INT)=%ld\n", sizeof(int), sizeof(MPI_INT));
  PRINTF("\n\n...............Test MPI_Scan...............\n\n");
  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * rootcd = CD_Init(size, rank);

  PRINTF("Root CD begin...\n");
  CD_Begin(rootcd);

  PRINTF("\n--------------------------------------------------------------\n");
  
  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = rootcd->Create("CD1_0", kRelaxed, 0, 0, NULL);

  PRINTF("-----------------------------\n");
  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);

  // application data allocation
  PRINTF("-----------------------------\n");
  PRINTF("Application data allocation...\n");

  // use the following initialization to represent preservations
  PRINTF("-----------------------------\n");
  PRINTF("Application initialization...\n");

  // real computation starts...
  PRINTF("Application computation starts...\n");
  data = rank;

  correct_result = 0;
  for (i=0;i<=rank;i++)
    correct_result += i;

  MPI_Scan ( &data, &result, 1, MPI_INT, MPI_SUM, comm );
  if (result != correct_result) {
    PRINTF( "[%d] Error suming ints with scan\n", rank );fflush(stderr);
    errors++;
  }
  MPI_Scan ( &data, &result, 1, MPI_INT, MPI_SUM, comm );
  if (result != correct_result) {
    PRINTF( "[%d] Error summing ints with scan (2)\n", rank );fflush(stderr);
    errors++;
  }

  data = rank;
  result = -100;
  MPI_Scan ( &data, &result, 1, MPI_INT, op_addem, comm );
  if (result != correct_result) {
    PRINTF( "[%d] Error summing ints with scan (userop)\n", rank );fflush(stderr);
    errors++;
  }

  MPI_Scan ( &data, &result, 1, MPI_INT, op_addem, comm );
  if (result != correct_result) {
    PRINTF( "[%d] Error summing ints with scan (userop2)\n", rank );fflush(stderr);
    errors++;
  }
  result = -100;
  data = rank;
  MPI_Scan ( &data, &result, 1, MPI_INT, op_assoc, comm );
  if (result == BAD_ANSWER) {
    PRINTF( "[%d] Error scanning with non-commutative op\n", rank );fflush(stderr);
    errors++;
  }
                                                                   
  if (errors)
  {
    PRINTF("Error Happens!!\n");
  }
  else
  {
    PRINTF("No Error!!\n");
  }

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

  PRINTF("-----------------------------\n");

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  // application extra starts
  MPI_Op_free( &op_assoc );
  MPI_Op_free( &op_addem );
  // application extra ends

  PRINTF("-----------------------------\n");

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  MPI_Barrier(comm);

  PRINTF("Complete root CD...\n");
  CD_Complete(rootcd);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  //fclose(fp);

  //MPI_Finalize();
  return 0;
}


int main(int argc, char** argv)
{
  int ret = 0;
  int rank;
  MPI_Init(&argc, &argv);

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  // open tmp.out.* file
  char name[20]; 
  sprintf(name, "tmp.out.%d", rank);
  fp = fopen(name, "w");
  if (fp == NULL)
  {
    printf("cannot open new file (%p)!\n", name);
    return 1;
  }

  //ret = test_basic(argc, argv);
  
  //ret = test_gather(argc, argv);
  
  ////// test_gatherv requires 4 processes
  ////ret = test_gatherv(argc, argv);

  //ret = test_bcast(argc, argv);

  ////// test_allgather requires at least 10 processes
  ////ret = test_allgather(argc, argv);

  // test_allgatherv requires 10 processes
  ret = test_allgatherv(argc, argv);

  //ret = test_reduce(argc, argv);
  //
  //ret = test_allreduce(argc, argv);
  //
  //ret = test_alltoall(argc, argv);
  //
  //ret = test_alltoallv(argc, argv);
  //
  //// at most MAX_PROCESSES (default is 10) ranks
  //ret = test_scatter(argc, argv);
  //
  //ret = test_scatterv(argc, argv);
  //
  //ret = test_reduce_scatter(argc, argv);
  //
  //ret = test_scan(argc, argv);

  MPI_Finalize();
  
  // close tmp.out.* file
  fclose(fp);

  return ret;
}

