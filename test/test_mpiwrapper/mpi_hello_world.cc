#include <stdio.h>
#include <mpi.h>
#include "cds.h"

//#define PRINTF(...) {printf("%d:",myrank);printf(__VA_ARGS__);}
#define PRINTF(...) {fprintf(fp, __VA_ARGS__);}

FILE *fp;

int main(int argc, char** argv)
{
  int partner; 
  double message, message2;
  MPI_Status status;
  int buf_size;
  char * tmp_buf;

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

  PRINTF("sizeof(int)=%d, sizeof(MPI_INT)=%d, and sizeof(MPI_CHAR)=%d\n", 
      (int)sizeof(int), (int)sizeof(MPI_INT), (int)sizeof(MPI_CHAR));

  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(nprocs, myrank);

  PRINTF("Root CD begin...\n");
  CD_Begin(root);

  PRINTF("root CD information...\n");
  root->ptr_cd()->GetCDID().Print();

  int num_reexec=0;
  if (myrank < nprocs/2)
  {
    PRINTF("\n--------------------------------------------------------------\n");

    PRINTF("Create child cd of level 1 ...\n");
    CDHandle * child1_0 = root->Create("CD1_0", kRelaxed, 0, 0, NULL);

    PRINTF("Begin child cd of level 1 ...\n");
    CD_Begin(child1_0);
    child1_0->ptr_cd()->GetCDID().Print();

    // test MPI_Send and MPI_Recv
    partner = nprocs/2 + myrank;
    message = myrank;
    MPI_Send(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD);
    PRINTF("Sent message=%f\n",message);
    MPI_Recv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%f\n",message);

    PRINTF("Print level 1 child CD comm_log_ptr info...\n");
    if (child1_0->ptr_cd()->cd_type_ == kRelaxed)
      child1_0->ptr_cd()->comm_log_ptr_->Print();
    else
      PRINTF("NULL pointer for strict CD!\n");

    // insert error
    if (num_reexec < 1)
    {
      PRINTF("Insert error #%d...\n", num_reexec);
      num_reexec++;
      child1_0->CDAssert(false);
    }

    // test the case to reach end of logs
    MPI_Recv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%f\n",message);

    // test MPI_Ssend
    message+=1;
    MPI_Ssend(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD);
    PRINTF("Ssent message=%f\n",message);

    message+=1;
    MPI_Send(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD);
    PRINTF("Sent message=%f\n",message);

    // test MPI_Bsend
    buf_size = MPI_BSEND_OVERHEAD + sizeof(double);
    tmp_buf = (char *) malloc(buf_size);
    MPI_Buffer_attach(tmp_buf, buf_size);
    message+=1;
    MPI_Bsend(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD);
    PRINTF("Bsent message=%f\n",message);

    // test MPI_Rsend
    // FIXME: need some sync to guarantee MPI_Recv has been posted..
    message+=1;
    MPI_Rsend(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD);
    PRINTF("Rsent message=%f\n",message);

    message+=1;
    MPI_Sendrecv(&message, 1, MPI_DOUBLE, partner, 1,
                &message2, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Sent message=%f and receive message=%f\n", message, message2);
    message = message2;

    message+=1;
    PRINTF("Replace sent message=%f\n", message);
    MPI_Sendrecv_replace(&message, 1, MPI_DOUBLE, partner, 1,
                partner, 1, MPI_COMM_WORLD, &status);
    PRINTF(", and receive message=%f\n", message);

    // insert error
    if (num_reexec < 2)
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
    // pair for MPI_Send
    MPI_Recv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%f\n",message);

    message += 1;
    MPI_Send(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD);
    PRINTF("Sent message=%f\n",message);

    message += 1;
    MPI_Ssend(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD);
    PRINTF("Ssent message=%f\n",message);

    // pair for MPI_Ssend
    MPI_Recv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%f\n",message);

    // pair for MPI_Send
    MPI_Recv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%f\n",message);

    // pair for MPI_Bsend
    MPI_Recv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%f\n",message);

    // pair for MPI_Rsend
    MPI_Recv(&message, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Received message=%f\n",message);

    message+=1;
    MPI_Sendrecv(&message, 1, MPI_DOUBLE, partner, 1,
                &message2, 1, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, &status);
    PRINTF("Sent message=%f and receive message=%f\n", message, message2);
    message = message2;

    message+=2;
    PRINTF("Replace sent message=%f", message);
    MPI_Sendrecv_replace(&message, 1, MPI_DOUBLE, partner, 1,
                partner, 1, MPI_COMM_WORLD, &status);
    PRINTF(", and receive message=%f\n", message);

    // insert error
    if (num_reexec < 1)
    {
      PRINTF("Insert error #%d...\n", num_reexec);
      num_reexec++;
      child1_1->CDAssert(false);
    }

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

  MPI_Buffer_detach(&tmp_buf, &buf_size);

  PRINTF("Complete root CD...\n");
  CD_Complete(root);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  fclose(fp);

  MPI_Finalize();

  return 0;
}
