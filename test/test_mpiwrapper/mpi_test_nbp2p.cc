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
  MPI_Status sstatus, rstatus;
  MPI_Request srequest, rrequest;
  int buf_size;
  bool wrong_execution=false;

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

  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child1_0);
  child1_0->ptr_cd()->GetCDID().Print();

  if (myrank < nprocs/2)
  {
    // test MPI_Send and MPI_Recv
    partner = nprocs/2 + myrank;
    message = myrank;
    MPI_Isend(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &srequest);
    PRINTF("Print level 1 child CD comm_log_ptr info after Isend\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    MPI_Wait(&srequest, &sstatus);
    PRINTF("Print level 1 child CD comm_log_ptr info after Isend-Wait\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    PRINTF("Sent message=%f\n",message);

    // insert error
    if (num_reexec < 1)
    {
      PRINTF("Insert error #%d...\n", num_reexec);
      num_reexec++;
      child1_0->CDAssert(false);
    }
     
    MPI_Irecv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &rrequest);
    PRINTF("Print level 1 child CD comm_log_ptr info after Irecv\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    PRINTF("Received message(%p)=%f before MPI_Wait\n", &message, message);
    MPI_Wait(&rrequest, &rstatus);
    PRINTF("Print level 1 child CD comm_log_ptr info after Irecv-Wait\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    PRINTF("Received message(%p)=%f after MPI_Wait\n", &message, message);

    if (message != (myrank+1))
    {
      PRINTF("Wrong execution with message(%f), but expected(%f)!!\n", message, (float)myrank+1);
      wrong_execution = true;
    }
    
    // insert error
    if (num_reexec < 2)
    {
      PRINTF("Insert error #%d...\n", num_reexec);
      num_reexec++;
      child1_0->CDAssert(false);
    }
     
  }
  else if (myrank >= nprocs/2)
  {
    // real communication...
    partner = myrank - nprocs/2;
    // pair for MPI_Isend
    // should receive message equaling partner's rank number
    MPI_Irecv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &rrequest);
    PRINTF("Print level 1 child CD comm_log_ptr info after Irecv\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    PRINTF("Received message(%p)=%f before MPI_Wait\n", &message, message);
    MPI_Wait(&rrequest, &rstatus);
    PRINTF("Print level 1 child CD comm_log_ptr info after Irecv-Wait\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    PRINTF("Received message(%p)=%f after MPI_Wait\n", &message, message);

    message += 1;
    MPI_Isend(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &srequest);
    PRINTF("Print level 1 child CD comm_log_ptr info after Isend\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    MPI_Wait(&srequest, &sstatus);
    PRINTF("Print level 1 child CD comm_log_ptr info after Isend-Wait\n");
    child1_0->ptr_cd()->comm_log_ptr_->Print();
    PRINTF("Sent message=%f\n",message);
  }

  //// insert error
  //if (num_reexec < 1)
  //{
  //  PRINTF("Insert error #%d...\n", num_reexec);
  //  num_reexec++;
  //  child1_0->CDAssert(false);
  //}
     
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
