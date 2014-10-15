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

int test_comm_log_relaxed(int myrank, int nprocs)
{

  int num_reexec = 0;
  int a = 257, d=0;
  char b = 127, e=0;
  int c[SIZE_C], f[SIZE_C];
  CDErrT err;

  //Init c array
  for (int i=0;i<SIZE_C;i++)
    c[i]=i+2;

  printf("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(nprocs, myrank);

  printf("Root CD begin...\n");
  CD_Begin(root);

  printf("\n--------------------------------------------------------------\n");

  printf("Create child cd of level 1...\n");
  CDHandle * child = root->Create("CD1_0", kRelaxed, 0, 0, &err);

  printf("Begin child cd of level 1...\n");
  CD_Begin(child);

  printf("Print level 1 child CD comm_log_ptr info...\n");
  if (child->ptr_cd()->cd_type_ == kRelaxed)
    child->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  printf("\n--------------------------------------------------------------\n");

  printf("Create child cd of level 2 seq 0...\n");
  CDHandle * child2 = child->Create("CD2_0", kRelaxed, 0, 0, &err);

  printf("Begin child cd of level 2 seq 0...\n");
  CD_Begin(child2);

  printf("Print level 2 seq 0 child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  if (child2->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    printf("Forward execution, and log data...\n");
    //log data
    child2->ptr_cd()->LogData(&a, sizeof(a));
    child2->ptr_cd()->LogData(&b, sizeof(b));
    child2->ptr_cd()->LogData(c, sizeof(c));

    //printf("a=%d\n", a);
    //printf("b=%d\n", b);
    //for (int i=0;i<SIZE_C;i++)
    //  printf("c[%d]=%d\t", i, c[i]);
    //printf("\n");
  }
  else if (child2->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    printf("Reexecution #%d, and replay logs...\n", num_reexec);

    printf("Reset all output variables...\n");
    d=0;
    e=0;
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child2->ptr_cd()->ReadData(&d, sizeof(a));
    if (d!=a) 
    {
      printf("Error: not correct data: d=%d, a=%d\n", d, a);
      return kError;
    }

    child2->ptr_cd()->ReadData(&e, sizeof(b));
    if (e!=b) 
    {
      printf("Error: not correct data: e=%d, b=%d\n", e, b);
      return kError;
    }

    child2->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        printf("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
    //printf("d=%d\n", d);
    //printf("e=%d\n", e);
    //for (int i=0;i<SIZE_C;i++)
    //  printf("f[%d]=%d\t", i, f[i]);
    //printf("\n");
  }
  else
  {
    printf("num_reexec=%d\n", num_reexec);
  }

  //insert error
  if (num_reexec < 2)
  {
    printf("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child2->CDAssert(false);
  }

  printf("Complete child CD of level 2 seq 0...\n");
  CD_Complete(child2);

  printf("\n------------------------------\n");

  printf("Print level 2 seq 0 child CD comm_log_ptr info after CD_Complete...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  printf("\n------------------------------\n");

  printf("Begin child cd of level 2 seq 0 the second time...\n");
  CD_Begin(child2);

  printf("Print level 2 seq 0 second time child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  if (child2->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    printf("Forward execution, and log data...\n");
    //log data
    child2->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child2->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    printf("Reexecution, and read data...\n");

    printf("Reset f...\n");
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child2->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        printf("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
  }

  printf("Complete child CD of level 2 seq 0 the second time...\n");
  CD_Complete(child2);

  printf("Destroy child CD of level 2 seq 0...\n");
  child2->Destroy();

  printf("\n------------------------------\n");

  printf("Create child cd of level 2 seq 1...\n");
  CDHandle * child3 = child->Create("CD2_1", kRelaxed, 0, 0, &err);

  printf("Begin child cd of level 2 seq 1...\n");
  CD_Begin(child3);

  printf("Print level 2 seq 1 child CD comm_log_ptr info...\n");
  if (child3->ptr_cd()->cd_type_ == kRelaxed)
    child3->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  printf("Complete child CD of level 2 seq 1...\n");
  CD_Complete(child3);

  printf("Destroy child CD of level 2 seq 1...\n");
  child3->Destroy();

  printf("\n--------------------------------------------------------------\n");

  if (child->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    printf("Forward execution, and log data...\n");
    //log data
    child->ptr_cd()->LogData(&a, sizeof(a));
    child->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    printf("Reexecution, and read data...\n");

    printf("Reset d and f...\n");
    d=0;
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child->ptr_cd()->ReadData(&d, sizeof(a));
    if (d!=a) 
    {
      printf("Error: not correct data: d=%d, a=%d\n", d, a);
      return kError;
    }

    child->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        printf("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }

    CommLogErrT ret=child->ptr_cd()->ReadData(&a, sizeof(a));
    if (ret == kCommLogCommLogModeFlip)
    {
      printf("Reached end of logs, and begin to generate logs...\n");
      child->ptr_cd()->LogData(&a, sizeof(a));
    }
  }

  //insert error
  if (num_reexec < 4)
  {
    printf("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child->CDAssert(false);
  }

  printf("Complete child CD of level 1...\n");
  CD_Complete(child);

  printf("Destroy child CD of level 1...\n");
  child->Destroy();

  printf("\n--------------------------------------------------------------\n");

  printf("Complete root CD...\n");
  CD_Complete(root);

  printf("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  return kOK;
}


int test_comm_log_combined(int myrank, int nprocs)
{

  int num_reexec = 0;
  int a = 257, d=0;
  char b = 127, e=0;
  int c[SIZE_C], f[SIZE_C];
  CDErrT err;

  //Init c array
  for (int i=0;i<SIZE_C;i++)
    c[i]=i+2;

  printf("\n\nInit cd runtime and create root CD.\n\n");
  CDHandle * root = CD_Init(nprocs, myrank);

  printf("Root CD begin...\n");
  CD_Begin(root);

  printf("\n--------------------------------------------------------------\n");

  printf("Create child cd of level 1...\n");
  CDHandle * child = root->Create("CD1_0", kStrict, 0, 0, &err);

  printf("Begin child cd of level 1...\n");
  CD_Begin(child);

  printf("Print level 1 child CD comm_log_ptr info...\n");
  if (child->ptr_cd()->cd_type_ == kRelaxed)
    child->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  printf("\n--------------------------------------------------------------\n");

  printf("Create child cd of level 2 seq 0...\n");
  CDHandle * child2 = child->Create("CD2_0", kRelaxed, 0, 0, &err);

  printf("Begin child cd of level 2 seq 0...\n");
  CD_Begin(child2);

  printf("Print level 2 seq 0 child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  if (child2->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    printf("Forward execution, and log data...\n");
    //log data
    child2->ptr_cd()->LogData(&a, sizeof(a));
    child2->ptr_cd()->LogData(&b, sizeof(b));
    child2->ptr_cd()->LogData(c, sizeof(c));

    //printf("a=%d\n", a);
    //printf("b=%d\n", b);
    //for (int i=0;i<SIZE_C;i++)
    //  printf("c[%d]=%d\t", i, c[i]);
    //printf("\n");
  }
  else if (child2->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    printf("Reexecution #%d, and replay logs...\n", num_reexec);

    printf("Reset all output variables...\n");
    d=0;
    e=0;
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child2->ptr_cd()->ReadData(&d, sizeof(a));
    if (d!=a) 
    {
      printf("Error: not correct data: d=%d, a=%d\n", d, a);
      return kError;
    }

    child2->ptr_cd()->ReadData(&e, sizeof(b));
    if (e!=b) 
    {
      printf("Error: not correct data: e=%d, b=%d\n", e, b);
      return kError;
    }

    child2->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        printf("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
    //printf("d=%d\n", d);
    //printf("e=%d\n", e);
    //for (int i=0;i<SIZE_C;i++)
    //  printf("f[%d]=%d\t", i, f[i]);
    //printf("\n");
  }
  else
  {
    printf("num_reexec=%d\n", num_reexec);
  }

  //insert error
  if (num_reexec < 2)
  {
    printf("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child2->CDAssert(false);
  }

  printf("Complete child CD of level 2 seq 0...\n");
  CD_Complete(child2);

  printf("\n------------------------------\n");

  printf("Print level 2 seq 0 child CD comm_log_ptr info after CD_Complete...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  printf("\n------------------------------\n");

  printf("Begin child cd of level 2 seq 0 the second time...\n");
  CD_Begin(child2);

  printf("Print level 2 seq 0 second time child CD comm_log_ptr info...\n");
  if (child2->ptr_cd()->cd_type_ == kRelaxed)
    child2->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  if (child2->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    printf("Forward execution, and log data...\n");
    //log data
    child2->ptr_cd()->LogData(c, sizeof(c));
  }
  else if (child2->ptr_cd()->GetCommLogMode()==kReplayLog)
  {
    printf("Reexecution, and read data...\n");

    printf("Reset f...\n");
    for (int i=0;i<SIZE_C;i++)
      f[i]=0;

    //read data
    child2->ptr_cd()->ReadData(f, sizeof(c));
    for (int i=0;i<SIZE_C;i++)
    {
      if (c[i]!=f[i]) 
      {
        printf("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
        return kError;
      }
    }
  }

  printf("Complete child CD of level 2 seq 0 the second time...\n");
  CD_Complete(child2);

  printf("Destroy child CD of level 2 seq 0...\n");
  child2->Destroy();

  printf("\n------------------------------\n");

  printf("Create child cd of level 2 seq 1...\n");
  CDHandle * child3 = child->Create("CD2_1", kStrict, 0, 0, &err);

  printf("Begin child cd of level 2 seq 1...\n");
  CD_Begin(child3);

  printf("Print level 2 seq 1 child CD comm_log_ptr info...\n");
  if (child3->ptr_cd()->cd_type_ == kRelaxed)
    child3->ptr_cd()->comm_log_ptr_->Print();
  else
    printf("NULL pointer for strict CD!\n");

  printf("Complete child CD of level 2 seq 1...\n");
  CD_Complete(child3);

  printf("Destroy child CD of level 2 seq 1...\n");
  child3->Destroy();

  printf("\n--------------------------------------------------------------\n");

  //if (child->ptr_cd()->GetCommLogMode()==kGenerateLog)
  //{
  //  printf("Forward execution, and log data...\n");
  //  //log data
  //  child->ptr_cd()->LogData(&a, sizeof(a));
  //  child->ptr_cd()->LogData(c, sizeof(c));
  //}
  //else if (child->ptr_cd()->GetCommLogMode()==kReplayLog)
  //{
  //  printf("Reexecution, and read data...\n");

  //  printf("Reset d and f...\n");
  //  d=0;
  //  for (int i=0;i<SIZE_C;i++)
  //    f[i]=0;

  //  //read data
  //  child->ptr_cd()->ReadData(&d, sizeof(a));
  //  if (d!=a) 
  //  {
  //    printf("Error: not correct data: d=%d, a=%d\n", d, a);
  //    return kError;
  //  }

  //  child->ptr_cd()->ReadData(f, sizeof(c));
  //  for (int i=0;i<SIZE_C;i++)
  //  {
  //    if (c[i]!=f[i]) 
  //    {
  //      printf("Error: not correct data: c[%d]=%d, f[%d]=%d\n", i, c[i], i, f[i]);
  //      return kError;
  //    }
  //  }

  //  CommLogErrT ret=child->ptr_cd()->ReadData(&a, sizeof(a));
  //  if (ret == kCommLogCommLogModeFlip)
  //  {
  //    printf("Reached end of logs, and begin to generate logs...\n");
  //    child->ptr_cd()->LogData(&a, sizeof(a));
  //  }
  //}

  //insert error
  if (num_reexec < 4)
  {
    printf("Insert error #%d...\n", num_reexec);
    num_reexec++;
    child->CDAssert(false);
  }

  printf("Complete child CD of level 1...\n");
  CD_Complete(child);

  printf("Destroy child CD of level 1...\n");
  child->Destroy();

  printf("\n--------------------------------------------------------------\n");

  printf("Complete root CD...\n");
  CD_Complete(root);

  printf("Destroy root CD and finalize the runtime...\n");
  CD_Finalize();

  return kOK;
}


int main(int argc, char* argv[])
{
  int nprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,  &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
 
  printf("test: sizeof(unsigned int) = %ld\n", sizeof(unsigned int));
  printf("test: sizeof(unsigned long) = %ld\n", sizeof(unsigned long));

  int ret=0, ret1=0;
  printf("\nTest relaxed comm logging relaxed functionality...\n");
  ret = test_comm_log_relaxed(myrank, nprocs);

  printf("\n");
  printf("------------------------------------------------------------------------------------\n");
  printf("------------------------------------------------------------------------------------\n");
  printf("\nTest combined comm logging relaxed functionality...\n");
  ret1 = test_comm_log_combined(myrank, nprocs);

  printf("\n\nResults:\n");
  if (ret == kError) 
    printf("relaxed comm log test failed!\n");
  else
    printf("relaxed comm log test passed!\n");
 
  if (ret1 == kError) 
    printf("combined comm log test failed!\n");
  else
    printf("combined comm log test passed!\n");
 
  MPI_Finalize(); 
  return 0;
}
