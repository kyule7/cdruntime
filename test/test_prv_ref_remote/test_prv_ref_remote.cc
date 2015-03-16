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
#include <chrono>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <mpi.h>
#include "cds.h"

#define SIZE 655360 //10M?
#define LV1 1
#define LV2 1
#define LV3 8 

using namespace cd;
using namespace std;
using namespace chrono;

DebugBuf dbgApp;
CDErrT err;
int  np = 0;
int  mr = 0;

#define NUM_TEST_BLOCKS 10
#define SIZE_BLOCK (25*1024)  //100,000kByte

static __inline__ long long getCounter(void)
{
  unsigned a, d; 
  asm volatile("rdtsc" : "=a" (a), "=d" (d)); 
  return ((long long)a) | (((long long)d) << 32); 
}


// Test basic preservation scheme.
int test_preservation_via_ref_remote()
{
  int a[3]= {3,5,0};
  int b[8]= {1,2,3,4,5,6,7,8};
  int c[8]= {5,};
  int d[16]= {9,8,7,6,5,4,3,2,1,};
  int test_results[8] = {0,};
  int test_result = 0;
  int num_reexecution = 0;
  dbgApp << "\nTest Preservation via Reference (remote) -----------------------\n" << endl;
  dbgApp << "CD Create\n" << endl;

	CDHandle* root = CD_Init(np, mr);
  CD_Begin(root); 

  dbgApp <<"CD Begin.........lol\n" << endl;

  dbgApp << "CD Preserving..\n" << endl;
  CDHandle* child_lv1=root->Create(CDPath::GetCurrentCD()->GetNodeID(), LV1, "CD1", kStrict, 0, 0, &err);
  CD_Begin(child_lv1);
  dbgApp << "child level 1-------------------------------------\n" << endl;
  dbgApp << "Preserve via copy for array a and array b\n" << endl;
  switch(mr) {
    case 0 : a[2] = 10;
    case 1 : a[2] = 11;
    case 2 : a[2] = 12;
    case 3 : a[2] = 13;
    case 4 : a[2] = 14;
    case 5 : a[2] = 15;
    case 6 : a[2] = 16;
    case 7 : a[2] = 17;
    default : a[2] = 0;
  }

  child_lv1->Preserve(a, sizeof(a), kCopy | kShared, (string("a-")+to_string(mr)).c_str());
  child_lv1->Preserve(b, sizeof(b), kCopy | kShared, (string("b-")+to_string(mr)).c_str());
//  child_lv1->Preserve(a, sizeof(a), kCopy, "a");
//  child_lv1->Preserve(b, sizeof(b), kCopy, "b");

  // Corrupt array a and b
  a[0] = 123;
  a[1] = 456;
  a[2] = 789;
  b[0] = 9;
  b[1] = 8;
  b[2] = 7;
  b[3] = 6;
  b[4] = 5;
  b[5] = 4;
  b[6] = 3;
  b[7] = 2;

  int a_from_2[8]= {1,2,3,4,5,6,7,8};

  if(mr == 0) a_from_2[2] = 11;
  if(mr == 1) a_from_2[2] = 12;
  if(mr == 2) a_from_2[2] = 13;
  if(mr == 3) a_from_2[2] = 14;
  if(mr == 4) a_from_2[2] = 15;
  if(mr == 5) a_from_2[2] = 16;
  if(mr == 6) a_from_2[2] = 17;
  if(mr == 7) a_from_2[2] = 10;

  CDHandle* child_lv2=child_lv1->Create(CDPath::GetCurrentCD()->GetNodeID(), LV2, "CD2", kStrict, 0, 0, &err);

  CD_Begin(child_lv2);

  dbgApp << "child level 2-------------------------------------\n" << endl;
  dbg << "\n\n\nnode id check : "<< CDPath::GetCurrentCD()->node_id() << "\n\n\n" << endl;

  child_lv2->Preserve(a, sizeof(a), kRef, "a_lv2", (string("a-")+to_string(mr)).c_str()); // local
  child_lv2->Preserve(b, sizeof(b), kRef, "b_lv2", (string("b-")+to_string(mr)).c_str()); // local
  if(mr == 0) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-1"); // remote
  if(mr == 1) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-2"); // remote
  if(mr == 2) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-3"); // remote
  if(mr == 3) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-4"); // remote
  if(mr == 4) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-5"); // remote
  if(mr == 5) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-6"); // remote
  if(mr == 6) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-7"); // remote
  if(mr == 7) child_lv2->Preserve(a_from_2, sizeof(a_from_2), kRef, "b_remote_lv2", "b-0"); // remote

//  getchar();
  //child->Preserve((char *)&b,8* sizeof(int));
  dbgApp << "sizeof a : \t" << sizeof(a) << endl; //getchar();
  dbgApp << "sizeof b : \t" << sizeof(b) << endl; //getchar();

  dbgApp << "Before Modify Current value of a[0]="<< a[0] << "a[1]=" << a[1] << endl;
  dbgApp << "Before Modify Current value of b[0]="<< b[0] << "b[1]=" << b[1] << endl;


  a[0] =2;
  b[0] =5;
  dbgApp << "After Modify Current value of a[0]=" << a[0] <<endl;
  dbgApp << "After Modify Current value of b[0]=" << b[0] <<endl;

  if( num_reexecution == 0 ) {
    cout <<"\nis now First error..\n <<<<<<<<<<< Error is detected >>>>>>>>>>>>>\n" << endl;
    num_reexecution = 1;
//	  child_lv2->CDAssert(false);
  }
  else if( num_reexecution == 1) {
    if( a[0] == 3 ) {
      test_results[0] = 1;
    }
    if( a[1] == 0 ) {
      test_results[1] = 1;
    }

    if( b[0] == 1 ) {
      test_results[2] = 1;
    }
    if( b[1] == 0 ) {
      test_results[3] = 1;
    }
  }

  dbgApp << "\n\n--------------Corruption for c begins ----------------------------\n\n" << endl;
  // this point is to test whether execution mode becomes kExecution from this point, 
  // because before this preservation is called it should be in kReexecution mode
  child_lv2->Preserve(c, sizeof(c), kCopy, "c");
//  dbgApp << "sizeof c : " << sizeof(c) << endl; //getchar();

//  if( num_reexecution == 2)  {
//    if( c[0] == 5 ) {
//      test_results[4] = 1;
//    }
//  }

  // corrupt c[0]
  dbgApp << "Before modifying current value of c[0] : " << c[0] << endl;
  c[0] =77;
  dbgApp << "After modifying current value of c[0] : "<< c[0] << endl;

  if(num_reexecution == 1) {
    dbg << "\nis now Second error..\n <<<<<<<<<<< Error is detected >>>>>>>>>>>>>\n\n" << endl;
    num_reexecution = 2;
    child_lv2->CDAssert(false);
  }

  dbgApp << "CD Complete\n" << endl;
  //  root->Complete();

  CDHandle* child_lv3=child_lv2->Create(CDPath::GetCurrentCD()->GetNodeID(), LV3, "CD3", kStrict, 0, 0, &err);
  CD_Begin(child_lv3);
  dbgApp << "child level 3-------------------------------------\n" << endl;
  child_lv3->Preserve(a, sizeof(a), kRef, "child_a", (string("a")+to_string(mr)).c_str());
  child_lv3->Preserve(b, sizeof(b), kRef, "child_b", (string("b")+to_string(mr)).c_str());
  child_lv3->Preserve(c, sizeof(b), kRef, "child_c", "c");
  child_lv3->Preserve(d, sizeof(d), kCopy, "d");

  child_lv3->Detect();

  CD_Complete(child_lv3);
  child_lv3->Destroy();

  // Detect Error here
  child_lv2->Detect();

  CD_Complete(child_lv2);
  child_lv2->Destroy();

  // Detect Error here
  child_lv1->Detect();

  CD_Complete(child_lv1);
  child_lv1->Destroy();

  // Detect Error here
  root->Detect();

  CD_Complete(root);

  dbgApp <<"CD Destroy\n" << endl;
  CD_Finalize(&dbgApp);


  // check the test result   
  for( int i = 0; i < 5; i++ ) {
    dbgApp << "test_result[" << i << "] = " << test_results[i] << endl;
    if( test_results[i] != 1 ) {
      test_result = -1;
    }
  }
  if( test_result == -1 ) return kError;
  return kOK; //
}





int main(int argc, char* argv[])
{
  int nprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,  &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  np = nprocs;
  mr = myrank;
  int ret=0;
 
 
 
  cout << "\ntest_preservation_via_ref_remote() \n\n" << endl; 
  ret = test_preservation_via_ref_remote();
  if( ret == kError ) cout << "test preservation_via_ref_remote FAILED\n" << endl;
  else cout << "test preservation_via_ref_remote PASSED\n" << endl;
 

  MPI_Finalize();
  return 0;
} 
