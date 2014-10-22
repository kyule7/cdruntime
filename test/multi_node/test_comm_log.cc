/*
Copyright 2014, The University of Texas at Austin 
All rights reserved.

THIS FILE IS PART OF THE CONTAINMENT DOMAINS RUNTIME LIBRARY

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <iostream>
#include <mpi.h>
#include "cds.h"

#define LV1 8 

using namespace cd;
using namespace std;
ucontext_t context;

#define SIZE_C 1025
#define PRINTF(...) {printf("%d:",myrank);printf(__VA_ARGS__);}
//#define PRINTF(...) {fprintf(fp, __VA_ARGS__);}

FILE *fp;

int test_comm_log_relaxed(CDHandle * root, int myrank)
{

  int num_reexec = 0;
  int a = 257, d=0;
  char b = 127, e=0;
  int c[SIZE_C], f[SIZE_C];
  CDErrT err;

  //Init c array
  for (int i=0;i<SIZE_C;i++)
    c[i]=i+2;

  //PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  //CDHandle * root = CD_Init(nprocs, myrank);

  //PRINTF("Root CD begin...\n");
  //CD_Begin(root);
  //
  //PRINTF("root CD information...\n");
  //root->ptr_cd()->GetCDID().Print();

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 1 ...\n");
  CDHandle * child0 = root->Create("CD1_0", kRelaxed, 0, 0, &err);

  PRINTF("Begin child cd of level 1 ...\n");
  CD_Begin(child0);
  child0->ptr_cd()->GetCDID().Print();

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 2...\n");
  CDHandle * child = child0->Create("CD1_0", kRelaxed, 0, 0, &err);

  PRINTF("Begin child cd of level 2...\n");
  CD_Begin(child);
  child->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 2 child CD comm_log_ptr info...\n");
  if (child->ptr_cd()->cd_type_ == kRelaxed)
    child->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 3 seq 0...\n");
  CDHandle * child2 = child->Create("CD2_0", kRelaxed, 0, 0, &err);

  PRINTF("Begin child cd of level 3 seq 0...\n");
  CD_Begin(child2);
  child2->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 3 seq 0 child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  if (child2->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    PRINTF("Forward execution, and log data...\n");
    //log data
    child2->ptr_cd()->LogData(&a, sizeof(a));
    child2->ptr_cd()->LogData(&b, sizeof(b));
    child2->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child2->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    PRINTF("Reexecution #%d, and replay logs...\n", num_reexec);

    PRINTF("Reset all output variables...\n");
    d=0;
    e=0;
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child2->ptr_cd()->ReadData(&d, sizeof(a));
    if (d!=a) 
    {
      PRINTF("Error: not correct data: d=%d, a=%d\n", d, a);
      return kError;
    }

    child2->ptr_cd()->ReadData(&e, sizeof(b));
    if (e!=b) 
    {
      PRINTF("Error: not correct data: e=%d, b=%d\n", e, b);
      return kError;
    }

    child2->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        PRINTF("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
  }

  //insert error
  if (num_reexec < 2)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child2->CDAssert(false);
  }

  PRINTF("Complete child CD of level 3 seq 0...\n");
  CD_Complete(child2);

  PRINTF("\n------------------------------\n");

  PRINTF("Print level 3 seq 0 child CD comm_log_ptr info after CD_Complete...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("\n------------------------------\n");

  PRINTF("Begin child cd of level 3 seq 0 the second time...\n");
  CD_Begin(child2);
  child2->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 3 seq 0 second time child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("\n------------------------------\n");

  PRINTF("Create child cd of level 4 ...\n");
  CDHandle * child4 = child2->Create("CD4_1", kRelaxed, 0, 0, &err);

  PRINTF("Begin child cd of level 4 ...\n");
  CD_Begin(child4);
  child4->ptr_cd()->GetCDID().Print();

  if (child4->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    PRINTF("Forward execution, and log data...\n");
    //log data
    child4->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child4->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    PRINTF("Reexecution, and read data...\n");

    PRINTF("Reset f...\n");
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child4->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        PRINTF("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
  }

  PRINTF("Complete child CD of level 4...\n");
  CD_Complete(child4);

  PRINTF("Destroy child CD of level 4...\n");
  child4->Destroy();

  PRINTF("\n------------------------------\n");

  PRINTF("Complete child CD of level 3 seq 0 the second time...\n");
  CD_Complete(child2);

  PRINTF("Destroy child CD of level 3 seq 0...\n");
  child2->Destroy();

  PRINTF("\n------------------------------\n");

  PRINTF("Create child cd of level 3 seq 1...\n");
  CDHandle * child3 = child->Create("CD2_1", kRelaxed, 0, 0, &err);

  PRINTF("Begin child cd of level 3 seq 1...\n");
  CD_Begin(child3);
  child3->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 3 seq 1 child CD comm_log_ptr info...\n");
  if (child3->ptr_cd()->cd_type_ == kRelaxed)
    child3->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  // to test the case that forward execution does not log anything, 
  // and if reexecution it will allocate the space
  if (child3->ptr_cd()->GetCommLogMode() == kReplayLog)
  {
    e=0;
    CommLogErrT ret=child3->ptr_cd()->ReadData(&e, sizeof(b));
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINTF("Reached end of logs, and begin to generate logs...\n");
      child3->ptr_cd()->LogData(&b, sizeof(b));
    }
    else
    {
      if (e!=b)
      {
        PRINTF("Error: not correct data: e=%d, b=%d\n", e, b);
        return kError;
      }
    }
  }

  PRINTF("Complete child CD of level 3 seq 1...\n");
  CD_Complete(child3);

  PRINTF("\n------------------------------\n");

  PRINTF("Begin child cd of level 3 seq 1 second time...\n");
  CD_Begin(child3);
  child3->ptr_cd()->GetCDID().Print();

  PRINTF("Complete child CD of level 3 seq 1 second time...\n");
  CD_Complete(child3);

  PRINTF("Destroy child CD of level 3 seq 1...\n");
  child3->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  if (child->ptr_cd()->GetCommLogMode()==kGenerateLog && num_reexec<3)
  {
    PRINTF("Forward execution, and log data...\n");
    //log data
    child->ptr_cd()->LogData(&a, sizeof(a));
    child->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child->ptr_cd()->GetCommLogMode()==kReplayLog || num_reexec>=3)
  {
    PRINTF("Reexecution, and read data...\n");

    PRINTF("Reset d and f...\n");
    d=0;
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child->ptr_cd()->ReadData(&d, sizeof(a));
    if (d!=a) 
    {
      PRINTF("Error: not correct data: d=%d, a=%d\n", d, a);
      return kError;
    }

    child->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        PRINTF("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }

    d=0;
    CommLogErrT ret=child->ptr_cd()->ReadData(&d, sizeof(a));
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINTF("Reached end of logs, and begin to generate logs...\n");
      child->ptr_cd()->LogData(&a, sizeof(a));
    }
    else
    {
      if (d!=a)
      {
        PRINTF("Error: not correct data: d=%d, a=%d\n", d, a);
        return kError;
      }
    }
  }

  //insert error
  if (num_reexec < 4)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child->CDAssert(false);
  }

  PRINTF("Complete child CD of level 2...\n");
  CD_Complete(child);

  PRINTF("Destroy child CD of level 2...\n");
  child->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  //insert error
  if (num_reexec < 5)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child0->CDAssert(false);
  }

  PRINTF("Complete child CD of level 1 ...\n");
  CD_Complete(child0);

  PRINTF("Destroy child CD of level 1 ...\n");
  child0->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  //PRINTF("Complete root CD...\n");
  //CD_Complete(root);

  //PRINTF("Destroy root CD and finalize the runtime...\n");
  //CD_Finalize();

  return kOK;
}


int test_comm_log_combined(CDHandle * root, int myrank)
{

  int num_reexec = 0;
  int a = 257, d=0;
  char b = 127, e=0;
  int c[SIZE_C], f[SIZE_C];
  CDErrT err;

  //Init c array
  for (int i=0;i<SIZE_C;i++)
    c[i]=i+2;

  //PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  //CDHandle * root = CD_Init(nprocs, myrank);

  //PRINTF("Root CD begin...\n");
  //CD_Begin(root);

  //PRINTF("root CD information...\n");
  //root->ptr_cd()->GetCDID().Print();

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 1...\n");
  CDHandle * child = root->Create("CD1_0", kStrict, 0, 0, &err);

  PRINTF("Begin child cd of level 1...\n");
  CD_Begin(child);
  child->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 1 child CD comm_log_ptr info...\n");
  if (child->ptr_cd()->cd_type_ == kRelaxed)
    child->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("\n--------------------------------------------------------------\n");

  PRINTF("Create child cd of level 2 seq 0...\n");
  CDHandle * child2 = child->Create("CD2_0", kRelaxed, 0, 0, &err);

  PRINTF("Begin child cd of level 2 seq 0...\n");
  CD_Begin(child2);
  child2->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 2 seq 0 child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  if (child2->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    PRINTF("Forward execution, and log data...\n");
    //log data
    child2->ptr_cd()->LogData(&a, sizeof(a));
    child2->ptr_cd()->LogData(&b, sizeof(b));
    child2->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child2->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    PRINTF("Reexecution #%d, and replay logs...\n", num_reexec);

    PRINTF("Reset all output variables...\n");
    d=0;
    e=0;
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child2->ptr_cd()->ReadData(&d, sizeof(a));
    if (d!=a) 
    {
      PRINTF("Error: not correct data: d=%d, a=%d\n", d, a);
      return kError;
    }

    child2->ptr_cd()->ReadData(&e, sizeof(b));
    if (e!=b) 
    {
      PRINTF("Error: not correct data: e=%d, b=%d\n", e, b);
      return kError;
    }

    child2->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        PRINTF("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
  }

  //insert error
  if (num_reexec < 2)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child2->CDAssert(false);
  }

  PRINTF("Complete child CD of level 2 seq 0...\n");
  CD_Complete(child2);

  PRINTF("\n------------------------------\n");

  PRINTF("Print level 2 seq 0 child CD comm_log_ptr info after CD_Complete...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("\n------------------------------\n");

  PRINTF("Begin child cd of level 2 seq 0 the second time...\n");
  CD_Begin(child2);
  child2->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 2 seq 0 second time child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  if (child2->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    PRINTF("Forward execution, and log data...\n");
    //log data
    child2->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child2->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    PRINTF("Reexecution, and read data...\n");

    PRINTF("Reset f...\n");
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child2->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        PRINTF("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
  }

  PRINTF("Complete child CD of level 2 seq 0 the second time...\n");
  CD_Complete(child2);

  PRINTF("Destroy child CD of level 2 seq 0...\n");
  child2->Destroy();

  PRINTF("\n------------------------------\n");

  PRINTF("Create child cd of level 2 seq 1...\n");
  CDHandle * child3 = child->Create("CD2_1", kStrict, 0, 0, &err);

  PRINTF("Begin child cd of level 2 seq 1...\n");
  CD_Begin(child3);
  child3->ptr_cd()->GetCDID().Print();

  PRINTF("Print level 2 seq 1 child CD comm_log_ptr info...\n");
  if (child3->ptr_cd()->cd_type_ == kRelaxed)
    child3->ptr_cd()->comm_log_ptr_->Print();
  else
    PRINTF("NULL pointer for strict CD!\n");

  PRINTF("Complete child CD of level 2 seq 1...\n");
  CD_Complete(child3);

  PRINTF("Destroy child CD of level 2 seq 1...\n");
  child3->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  //insert error
  if (num_reexec < 4)
  {
    PRINTF("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child->CDAssert(false);
  }

  PRINTF("Complete child CD of level 1...\n");
  CD_Complete(child);

  PRINTF("Destroy child CD of level 1...\n");
  child->Destroy();

  PRINTF("\n--------------------------------------------------------------\n");

  //PRINTF("Complete root CD...\n");
  //CD_Complete(root);

  //PRINTF("Destroy root CD and finalize the runtime...\n");
  //CD_Finalize();

  return kOK;
}

int main(int argc, char* argv[])
{
  int nprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,  &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  char name[20]; 
  sprintf(name, "tmp.out.%d", myrank);
  fp = fopen(name, "w");
  if (fp == NULL)
  {
    printf("cannot open new file (%p)!\n", name);
    return 1;
  }
 
  PRINTF("test: sizeof(unsigned int) = %ld\n", sizeof(unsigned int));
  PRINTF("test: sizeof(unsigned long) = %ld\n", sizeof(unsigned long));

  PRINTF("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(nprocs, myrank);

  int num_reexec = 0;

  PRINTF("Root CD begin...\n");
  CD_Begin(root);

  PRINTF("root CD information...\n");
  root->ptr_cd()->GetCDID().Print();

  int flag = 1;

  PRINTF("\n");
  PRINTF("------------------------------------------------------------------------------------\n");
  PRINTF("------------------------------------------------------------------------------------\n");

  int ret=0;
  PRINTF("\nTest relaxed comm logging relaxed functionality...\n");
  ret = test_comm_log_relaxed(root, myrank);

  PRINTF("\n\nResults:\n");
  if (ret == kError) 
  {
    PRINTF("relaxed comm log test failed!\n");
  }
  else
  {
    PRINTF("relaxed comm log test passed!\n");
  }
 
  PRINTF("\n");
  PRINTF("------------------------------------------------------------------------------------\n");
  PRINTF("------------------------------------------------------------------------------------\n");

  ret = 0;
  PRINTF("\nTest combined comm logging relaxed functionality...\n");
  ret = test_comm_log_combined(root, myrank);

  PRINTF("\n\nResults:\n");
  if (ret == kError) 
  {
    PRINTF("combined comm log test failed!\n");
  }
  else
  {
    PRINTF("combined comm log test passed!\n");
  }
 
  PRINTF("\n");
  PRINTF("------------------------------------------------------------------------------------\n");
  PRINTF("------------------------------------------------------------------------------------\n");

  ret = 0;
  if (myrank%2 == 0)
  {
    PRINTF("\n%d:Test relaxed comm logging relaxed functionality...\n", myrank);
    ret = test_comm_log_relaxed(root, myrank);
  }
  else if (myrank%2 != 0)
  {
    PRINTF("\n%d:Test combined comm logging relaxed functionality...\n", myrank);
    ret = test_comm_log_combined(root, myrank);
  }

  PRINTF("\n\nResults:\n");
  if (ret == kError) 
  {
    PRINTF("final comm log test failed!\n");
  }
  else
  {
    PRINTF("final comm log test passed!\n");
  }

  PRINTF("\n");
  PRINTF("------------------------------------------------------------------------------------\n");
  PRINTF("------------------------------------------------------------------------------------\n");
  
  //if (num_reexec == 0)
  //{
  //  PRINTF("Insert error in root CD...\n");
  //  num_reexec++;
  //  root->CDAssert(false);
  //}
  
  PRINTF("Complete root CD...\n");
  CD_Complete(root);

  PRINTF("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  fclose(fp);

  MPI_Finalize(); 
  return 0;
}
