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
#include <mpi.h>
#include "cds.h"

#define SIZE 655360 //10M?
#define LV1 1
#define LV2 8
#define LV3 1 

using namespace cd;
using namespace std;
using namespace chrono;

ostringstream dbg2;
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
int test_preemption()
{

  int a[4]= {3,0,};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int d[16]= {9,8,7,6,5,4,3,2,1,};
  int test_results[8] = {0,};
  int test_result = 0;
  int num_reexecution = 0;
  dbg2 << "\n\n--------------------test preemption begins-----------------------\n" << endl;
  dbg2 << "CD Create\n" << endl;
//  CD *root= handle_cd.ptr_cd();// for now let's just get the pointer and use it directly

	CDHandle* root = CD_Init(np, mr);
  dbg2 <<"CD Begin.........lol\n" << endl;
  //  root->Begin();  
  //  getcontext(&root->context_);
  CD_Begin(root); 

  dbg2 << "CD Preserving..\n" << endl;
  CDHandle* child_lv1=root->Create(CDPath::GetCurrentCD()->GetNodeID(), LV1, "CD1", kStrict, 0, 0, &err);
  CD_Begin(child_lv1);
  dbg2 << "child level 1-------------------------------------\n" << endl;

  CDHandle* child_lv2=child_lv1->Create(CDPath::GetCurrentCD()->GetNodeID(), LV2, "CD2", kStrict, 0, 0, &err);
  CD_Begin(child_lv2);
  dbg2 << "child level 2-------------------------------------\n" << endl;
  dbg << "\n\n\nnode id check : "<< CDPath::GetCurrentCD()->node_id() << "\n\n\n" << endl;

  child_lv2->Preserve(a, sizeof(a), kCopy, "a", "a");
  child_lv2->Preserve(b, sizeof(b), kCopy, "b", "b");
  //child->Preserve((char *)&b,8* sizeof(int));
  dbg2 << "sizeof a : \t" << sizeof(a) << endl; //getchar();
  dbg2 << "sizeof b : \t" << sizeof(b) << endl; //getchar();

  dbg2 << "Before Modify Current value of a[0]="<< a[0] << "a[1]=" << a[1] << endl;
  dbg2 << "Before Modify Current value of b[0]="<< b[0] << "b[1]=" << b[1] << endl;

  if( num_reexecution == 1) {
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

  a[0] =2;
  b[0] =5;
  dbg2 << "After Modify Current value of a[0]=" << a[0] <<endl;
  dbg2 << "After Modify Current value of b[0]=" << b[0] <<endl;

//  if( num_reexecution == 0) {
//    dbg2 <<"\nis now First error..\n <<<<<<<<<<< Error is detected >>>>>>>>>>>>>\n" << endl;
//    num_reexecution = 1;
//    child->CDAssert(false);
//  }
  dbg2 << "\n\n--------------Corruption for c begins ----------------------------\n\n" << endl;
  // this point is to test whether execution mode becomes kExecution from this point, 
  // because before this preservation is called it should be in kReexecution mode
  child_lv2->Preserve(c, sizeof(c), kCopy, "c", "c");
  dbg2 << "sizeof c : " << sizeof(c) << endl; //getchar();

  if( num_reexecution == 2)  {
    if( c[0] == 5 ) {
      test_results[4] = 1;
    }
  }

  // corrupt c[0]
  dbg2 << "Before modifying current value of c[0] : " << c[0] << endl;
  c[0] =77;
  dbg2 << "After modifying current value of c[0] : "<< c[0] << endl;

//  if(num_reexecution == 1) {
//    dbg << "\nis now Second error..\n <<<<<<<<<<< Error is detected >>>>>>>>>>>>>\n\n" << endl;
//    num_reexecution = 2;
//    child->CDAssert(false);
//  }

  dbg2 << "CD Complete\n" << endl;
  //  root->Complete();

  CDHandle* child_lv3=child_lv2->Create(CDPath::GetCurrentCD()->GetNodeID(), LV3, "CD3", kStrict, 0, 0, &err);
  CD_Begin(child_lv3);
  dbg2 << "child level 3-------------------------------------\n" << endl;
  child_lv3->Preserve(a, sizeof(a), kRef, "child_a", "a");
  child_lv3->Preserve(b, sizeof(b), kRef, "child_b", "b");
  child_lv3->Preserve(c, sizeof(b), kRef, "child_c", "c");
  child_lv3->Preserve(d, sizeof(d), kCopy, "child_d", "d");

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

  dbg2 <<"CD Destroy\n" << endl;
  CD_Finalize(&dbg2);


  // check the test result   
  for( int i = 0; i < 5; i++ ) {
    dbg2 << "test_result[" << i << "] = " << test_results[i] << endl;
    if( test_results[i] != 1 ) {
      test_result = -1;
    }
  }
  if( test_result == -1 ) return kError;
  return kOK; //
}




// Test basic via reference scheme.
int test_preemption_2()
{
  int a[4]= {3,0,};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int num_reexecution = 0;
  int test_result = 0;
  
  //CDHandle no_parent;  // not initialized and this means no parent
  dbg2 << "\n\n---------------test_preemption 2 begins-----------------------------\n" << endl;
  CDErrT err;
	CDHandle* root = CD_Init(np, mr);
  CD_Begin(root); 
  root->Preserve(a, sizeof(a), kCopy, "a");

  dbg2 << "Before modifying current value of a[0] " << a[0] <<" a[1] "<< a[1] << endl;
  a[0] = 99;  // now when child recovers it should see 3 instead of 99
  dbg2 << "After modifying current value of a[0] " << a[0] <<" a[1] "<< a[1] << endl;

  CDHandle* child=root->Create(CDPath::GetCurrentCD()->GetNodeID(), LV1, "CD1", kStrict, 0, 0, &err);
  CD_Begin(child); 
  dbg2 << "child cd begin\n" << endl; 
  child->Preserve(a, sizeof(a), kRef, "nonamejusttest", "a", 0);
  
  if( num_reexecution == 0 ) {
    num_reexecution = 1;
	  child->CDAssert(false);

  }
  if( num_reexecution == 1 ) {

    dbg2 << "After Reexec :   value of a[0] "<< a[0] << " a[1] "<< a[1] << endl;
    if(a[0] == 3 )
      test_result=1;
  }
  CD_Complete(child);
  child->Destroy();

 
  CD_Complete(root);
  CD_Finalize();

  if( test_result == 1 ) return kOK;
  return kError;
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
 
 
 
  cout << "\ntest_preemption() \n\n" << endl; 
  ret = test_preemption();
  if( ret == kError ) cout << "test preemption FAILED\n" << endl;
  else cout << "test preemption PASSED\n" << endl;
 

  MPI_Finalize();
  return 0;
} 
