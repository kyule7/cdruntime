#include <stdio.h>
#include <mpi.h>
#include "cds.h"

//#define PRINTF(...) {printf("%d:",myrank);printf(__VA_ARGS__);}
#define PRINTF(...) {fprintf(fp, __VA_ARGS__);}

FILE *fp;

int main(int argc, char** argv)
{
  int partner, message;
  MPI_Status status;

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

  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(nprocs, myrank);

  PRINTF("Root CD begin...\n");
  CD_Begin(root);

  PRINTF("root CD information...\n");
  root->ptr_cd()->GetCDID().Print();

  if (myrank < nprocs/2)
  {
    int num_reexec=0;

    PRINTF("\n--------------------------------------------------------------\n");

    PRINTF("Create child cd of level 1 ...\n");
    CDHandle * child1_0 = root->Create("CD1_0", kRelaxed, 0, 0, NULL);

    PRINTF("Begin child cd of level 1 ...\n");
    CD_Begin(child1_0);
    child1_0->ptr_cd()->GetCDID().Print();

    // real communication...
    partner = nprocs/2 + myrank;
    MPI_Send(&myrank, 1, MPI_INT, partner, 1, MPI_COMM_WORLD);
    MPI_Recv(&message, 1, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%d\n",message);

    PRINTF("Print level 1 child CD comm_log_ptr info...\n");
    if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
      child1_0->ptr_cd()->comm_log_ptr_->Print();
    else
      PRINTF("NULL pointer for strict CD!\n");

    //insert error
    if (num_reexec < 2)
    {
      PRINTF("Insert error #%d...\n", num_reexec);
      num_reexec++;
      child1_0->CDAssert(false);
    }

    PRINTF("Complete child CD of level 1 ...\n");
    CD_Complete(child1_0);

    PRINTF("Destroy child CD of level 1 ...\n");
    child1_0->Destroy();

    PRINTF("\n--------------------------------------------------------------\n");
  }
  else if (myrank >= nprocs/2)
  {
    PRINTF("\n--------------------------------------------------------------\n");

    PRINTF("Create child cd of level 1 ...\n");
    CDHandle * child1_1 = root->Create("CD1_0", kRelaxed, 0, 0, NULL);

    PRINTF("Begin child cd of level 1 ...\n");
    CD_Begin(child1_1);
    child1_1->ptr_cd()->GetCDID().Print();

    // real communication...
    partner = myrank - nprocs/2;
    MPI_Recv(&message, 1, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%d\n",message);
    MPI_Send(&myrank, 1, MPI_INT, partner, 1, MPI_COMM_WORLD);

    PRINTF("Print level 1 child CD comm_log_ptr info...\n");
    if (child1_1->ptr_cd()->cd_type_ == kRelaxed)
      child1_1->ptr_cd()->comm_log_ptr_->Print();
    else
      PRINTF("NULL pointer for strict CD!\n");

    PRINTF("Complete child CD of level 1 ...\n");
    CD_Complete(child1_1);

    PRINTF("Destroy child CD of level 1 ...\n");
    child1_1->Destroy();

    PRINTF("\n--------------------------------------------------------------\n");
  }

  PRINTF("Rank %d is partner with %d\n", myrank, message);

  PRINTF("Complete root CD...\n");
  CD_Complete(root);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  fclose(fp);

  MPI_Finalize();

  return 0;
}
