#include <stdio.h>
#include <mpi.h>
#include "cds.h"

//#define PRINTF(...) {printf("%d:",myrank);printf(__VA_ARGS__);}
#define PRINTF(...) {fprintf(fp, __VA_ARGS__);}

FILE *fp;


// test non-blocking p2p communication and barriers
int test_nbp2p(int argc, char** argv)
{
  int partner; 
  double message, message2;
  MPI_Status sstatus, rstatus, status, statuses[2];
  MPI_Request srequest, rrequest, request, requests[2];
  int buf_size;
  int wrong_execution=0;

  MPI_Init(&argc, &argv);

  int nprocs;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  int myrank;
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  char name[20]; 
  sprintf(name, "tmp.out.%d", myrank);
  fp = fopen(name, "w");
  if (fp == NULL)
  {
    printf("cannot open new file (%p)!\n", name);
    return 1;
  }

  if (nprocs %2 != 0){
  {
    if (myrank == 0)
      PRINTF("Error: need even number of MPI ranks...\n");
    }
    MPI_Finalize();
    return 0;
  }

  PRINTF("int=%d, MPI_INT=%d, double=%d, MPI_DOUBLE=%d, float=%d, MPI_FLOAT=%d\n", 
      (int)sizeof(int), (int)sizeof(MPI_INT), (int)sizeof(double), (int)sizeof(MPI_DOUBLE),
      (int)sizeof(float), (int)sizeof(MPI_FLOAT));

  int double_size;
  MPI_Type_size(MPI_DOUBLE, &double_size);
  PRINTF("MPI_DOUBLE's size=%d\n", double_size);

  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(nprocs, myrank);

  PRINTF("Root CD begin...\n");
  CD_Begin(root);

  PRINTF("root CD information...\n");
  root->ptr_cd()->GetCDID().Print();

  int num_reexec=0;
  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child1_0 = root->Create("CD1_0", kRelaxed, 0, 0, NULL);
  //CDHandle * child1_0 = root->Create("CD1_0", kStrict, 0, 0, NULL);

  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);
  child1_0->ptr_cd()->GetCDID().Print();


  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 2 ...\n");
  CDHandle * child2_0 = child1_0->Create("CD2_0", kRelaxed, 0, 0, NULL);

  PRINTF("Begin child cd of level 2 ...\n");
  CD_Begin(child2_0);
  child2_0->ptr_cd()->GetCDID().Print();

  
  PRINTF("Complete child CD of level 2 ...\n");
  CD_Complete(child2_0);

  PRINTF("\n---------------------------------\n");

  PRINTF("Begin child cd of level 2 second time...\n");
  CD_Begin(child2_0);
  child2_0->ptr_cd()->GetCDID().Print();
  

  // insert error
  if (num_reexec < 3)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    //// FIXME: this error cannot be recovered if message is corrupted...
    //message = myrank;
    child2_0->CDAssert(false);
  }


  PRINTF("Complete child CD of level 2 second time...\n");
  CD_Complete(child2_0);

  PRINTF("Destroy child CD of level 2 ...\n");
  child2_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  // insert error
  if (num_reexec < 4)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    // reset message for re-execution testing
    message = myrank;
    child1_0->CDAssert(false);
  }
   

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child1_0);

  PRINTF("Destroy child CD of level 1 ...\n");
  child1_0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Complete root CD...\n");
  CD_Complete(root);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  if (wrong_execution)
  {
    PRINTF("\n\n!!!!! Somewhere Error Happened !!!!!\n\n");
  }
  else{
    PRINTF("\n\n!!!!! NO Error !!!!!\n\n");
  }

  fclose(fp);

  MPI_Finalize();

  return 0;
}

int main(int argc, char **argv)
{
  int ret = test_nbp2p(argc, argv);
  return ret;
}
