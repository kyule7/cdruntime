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
#include <mpi.h>
#include "cds.h"
#include <mcheck.h>
//#include "cd_global.h"
#define SIZE 655360 //10M?
#define LV1 8 

using namespace cd;
using namespace std;
using namespace chrono;
ucontext_t context;

FILE *fp;

int  np = 0;
int  mr = 0;

//#define NUM_TEST_BLOCKS 10
#define NUM_TEST_BLOCKS 10
#define SIZE_BLOCK (25*1024)  //100,000kByte
//#define SIZE_BLOCK (16)

static __inline__ long long getCounter(void)
{
  unsigned a, d; 
  asm volatile("rdtsc" : "=a" (a), "=d" (d)); 
  return ((long long)a) | (((long long)d) << 32); 
}
int performance_test1()
{
  CDHandle* root = CD_Init();
//  CD *root= handle_cd.ptr_cd();// for now let's just get the pointer and use it directly
  int num_reexecution = 0; 
  int i;
  long long begin_cnt;
  long long end_cnt;
  long long accumulate_cnt;

  //prologue 
  int *a[NUM_TEST_BLOCKS]={NULL,};
  for(  i = 0 ; i < NUM_TEST_BLOCKS; i++ )
  {
    a[i] = new int[SIZE_BLOCK];
  }

  // START


  CD_Begin(root);
 
  accumulate_cnt = 0;
  begin_cnt = getCounter();
  for( i= 0 ; i < NUM_TEST_BLOCKS; i++ )
  {
//    printf("%d\n",i);
    root->Preserve(a[i], SIZE_BLOCK*sizeof(int));
  }
  end_cnt = getCounter();
  printf("Time elapsed:%lld\n",end_cnt - begin_cnt);


  if( num_reexecution== 0) 
  {
    num_reexecution =1;
    root->CDAssert(false);
  }

  //END
  CD_Complete(root);
  CD_Finalize();



  //epilogue
  for( i = 0 ; i < NUM_TEST_BLOCKS; i++ )
  {
    delete[] a[i];
  }

}


// Test basic preservation scheme.
//int test1()
//{
//   
//  CDHandle no_parent;  // not initialized and this means no parent
//  int a[4]= {3,0,};
//  int b[8]= {1,0,};
//  int c[8] = {1,0,};
//   int d[8]= {1,2,};
//  int e[8]= {0,3,};
//  int test_results[8] = {0,};
//  int test_result = 0;
//  int num_reexecution = 0;
//int num_reexecution_ = 0;
//  printf("\n\ntest1 begins\n");
//  printf("CD Create\n");
//  CDHandle handle_cd=CD::Create(cd::kStrict, no_parent);
//  CD *root= handle_cd.ptr_cd();// for now let's just get the pointer and use it directly
//
//  printf("CD Begin\n");
////  root->Begin();  
////  getcontext(&root->context_);
//  CD_Begin(handle_cd); 
//  
//  printf("CD Preserving..\n");
//  handle_cd.Preserve((char *)&a,4* sizeof(int), kCopy, "a");
//  handle_cd.Preserve((char *)&b,8* sizeof(int), kCopy, "b");
////	handle_cd.Body(); 
//	
////testing whether it can search all ancestors' entries
////Original copy must have ref_name==0 by default.
//	
//	CDHandle handle_child = CD::Create(cd::kStrict, handle_cd);
//	CD *child = handle_child.ptr_cd();
//  printf("Child CD Begin\n");
//  CD_Begin(handle_child); 
//  printf("child CD Preserving..\n");
//  handle_child.Preserve((char *)&c,2* sizeof(int), kReference, "b", "a");
//  handle_child.Preserve((char *)&e,2* sizeof(int), kReference, "a1", "b");
//
//  printf("Before c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
//  if(num_reexecution_ == 0){
//    num_reexecution_ = 1;
//    printf("After c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
//    handle_child.CDAssert(false);
//  }
//  	
//      CDHandle handle_child1 = CD::Create(cd::kStrict, handle_child);
//      CD *child1 = handle_child1.ptr_cd();
//      printf("Child CD Begin\n");
//      CD_Begin(handle_child1); 
//  printf("child CD Preserving..\n");
//  handle_child1.Preserve((char *)&d, 2* sizeof(int), kReference, "a2", "a1");
//
//	CDHandle handle_child2 = CD::Create(cd::kStrict, handle_child1);
//	CD *child2 = handle_child2.ptr_cd();
//  printf("Child CD Begin\n");
//  CD_Begin(handle_child2); 
//  printf("child CD Preserving..\n");
//  handle_child2.Preserve((char *)&e, 2* sizeof(int), kReference, "", "a2");
//
//printf("Before e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
//if(num_reexecution_ == 0){
//num_reexecution_ = 1;
//printf("After e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
//	handle_child2.CDAssert(false);
//}
//
//  printf("child CD Complete\n");
//	CD_Complete(handle_child2);
//  printf("child CD Destroy\n");
//	CD::Destroy(child2);
//  printf("child CD Complete\n");
//	CD_Complete(handle_child1);
//  printf("child CD Destroy\n");
//	CD::Destroy(child1);
//  printf("child CD Complete\n");
//
//	CD_Complete(handle_child);
//  printf("child CD Destroy\n");
//CD::Destroy(child);
//
//  printf("Before Modify Current value of a[0]=%d a[1]=%d\n", a[0], a[1]);
//  printf("Before Modify Current value of b[0]=%d b[1]=%d\n", b[0], b[1]);
// 
//  if( num_reexecution == 1) 
//  {
//    if( a[0] == 3 ) 
//     {
//       test_results[0] = 1;
//     }
//     if( a[1] == 0 ) 
//     {
//       test_results[1] = 1;
//     }
//
//   if( b[0] == 1 ) 
//     {
//       test_results[2] = 1;
//     }
//    if( b[1] == 0 ) 
//     {
//       test_results[3] = 1;
//     }
//  }
//
//
//
//
//  a[0] =2;
//  b[0] =5;
//  printf("After Modify Current value of a[0]=%d\n", a[0]);
//  printf("After Modify Current value of b[0]=%d\n", b[0]);
//
//  if( num_reexecution== 0) 
//  {
//  	printf("is now First error..\n");
//	  num_reexecution= 1;
//	  handle_cd.CDAssert(false);
//  	//setcontext(&root->context_);
//	  
//  }
//
//  // this point is to test whether execution mode becomes kExecution from this point, because before this preservation is called it should be in kReexecution mode
//  handle_cd.Preserve((char *)&c,8* sizeof(int));
//  if( num_reexecution == 2)
//  {
//    if( c[0] == 5 )
//    {
//      test_results[4] =1;
//    }
//
//  }
//  printf("Before modifying current value of c[0] %d\n", c[0]);
//  c[0] =77;
//  printf("After modifying current value of c[0] %d\n", c[0]);
//  if(num_reexecution ==1)
//  {
//	  printf("is now Second error..\n");
//	  num_reexecution= 2;
//	  handle_cd.CDAssert(false);
//
//  }
//
//  printf("CD Complete\n");
////  root->Complete();
//  CD_Complete(handle_cd);
//
//  printf("CD Destroy\n");
//  CD::Destroy(root);
//printf("test?\n"); 
// // check the test result   
//  for( int i = 0; i < 5; i++ )
//  {
//    printf("test_result[%d] = %d\n", i, test_results[i]);
//    if( test_results[i] != 1 )
//    {
//      test_result = -1;
//    }
//  }
//  if( test_result == -1 ) return kError;
//  return kOK; //
//}

// Test basic preservation scheme.
int test_preservation_via_copy()
{

  CDErrT err;
  int a[4]= {3,0,};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int test_results[8] = {0,};
  int test_result = 0;
  int num_reexecution = 0;
  printf("\n\n--------------------test1 begins-----------------------\n");
//  CD *root= handle_cd.ptr_cd();// for now let's just get the pointer and use it directly
  printf("CD Create\n");



	CDHandle* root = CD_Init(np, mr);
  printf("CD Begin.........lol\n");
  //  root->Begin();  
  //  getcontext(&root->context_);
  CD_Begin(root); 
//GONG
//  printf("CD Preserving..\n");
  
  
  printf("test fopen() 1\n");
//  FILE * test_fopen_parent = fopen("test_fopen_parent.txt", "w");
//  FILE * test_fopen_ = fopen("test_fopen_.txt", "w");

  printf("create child CD\n");

  root->Preserve(a, sizeof(a), kCopy, "a", "a");

  CDHandle* child=root->Create("CD1", kStrict | kPFS, 0, 0, &err);
  CD_Begin(child); 
  printf("Child CD Begin\n");
  printf("child CD Preserving..\n");

  child->Preserve(a, sizeof(a), kCopy, "a", "a");
  child->Preserve(b, sizeof(b), kCopy, "b", "b");
  
  //test fopen
  printf("test fopen() 2\n");
//  FILE * test_fopen = fopen("test_fopen.txt", "w");
//  fprintf(test_fopen,"number of re-execution: %i\n", num_reexecution);
//  fprintf(test_fopen_,"number of re-execution: %i\n", num_reexecution);
//  printf("test_malloc here\n");
//  int *test_malloc = new int[10];
//  test_malloc[0] = rand();
//  printf("test_calloc here\n");
//  int *test_calloc = (int*) calloc(10,sizeof(int));
//  test_calloc[0] = rand();
//  
//  printf("child CD Complete..\n");
//  free(test_malloc);
///*  printf("complete child 1\n");
//  CD_Complete(child);
//  
//  CD_Begin(child);*/
//  free(test_calloc);
//
//  printf("test_valloc here\n");
//  int *test_valloc = (int*) valloc(10*sizeof(int));
//  test_valloc[0] = rand();
//  printf("test_realloc here\n");
//  int *test_realloc = (int*) malloc(10*sizeof(int));//= new int[10];
//  test_realloc = (int*) realloc(test_realloc, 10*sizeof(int));
//  test_realloc[0] = rand();
//  printf("rand value check : %i\t%i\t%i\t%i\t%i\n", test_malloc[0], test_calloc[0], test_valloc[0], test_realloc[0], test_realloc[9]);
//  free(test_valloc);
//  free(test_realloc);
  
  printf("Before Modify Current value of a[0]=%d a[1]=%d\n", a[0], a[1]);
  printf("Before Modify Current value of b[0]=%d b[1]=%d\n", b[0], b[1]);

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
  printf("After Modify Current value of a[0]=%d\n", a[0]);
//  printf("After Modify Current value of b[0]=%d==app_side?  %i\n\n", b[0], app_side);
  
//  printf("close test_fopen\n");
//  fclose(test_fopen);

  if( num_reexecution == 0 ){
    printf("\nis now First error..\n <<<<<<<<<<< Error is detected >>>>>>>>>>>>>\n\n");
    num_reexecution = 1;
//    child->CDAssert(false);
  }


  printf("Complete child CD\n");
  CD_Complete(child);
  printf("Destroy child CD\n");
  child->Destroy();

  printf("\n\n--------------Corruption for c begins ----------------------------\n\n");
  // this point is to test whether execution mode becomes kExecution from this point, 
  // because before this preservation is called it should be in kReexecution mode
  root->Preserve(c, sizeof(c), kCopy, "c", "c");
  printf("sizeof c : %d\n", (int)sizeof(c)); //getchar();
  MPI_Barrier(MPI_COMM_WORLD);

  if( num_reexecution == 2)  {
    if( c[0] == 5 ) {
      test_results[4] = 1;
    }
  }

  // corrupt c[0]
  printf("Before modifying current value of c[0] %d\n", c[0]);
  c[0] =77;
  printf("After modifying current value of c[0] %d\n", c[0]);

//  printf("close test_fopen_parent\n");
//  fclose(test_fopen_parent);
  if(num_reexecution == 1) {
    printf("\nis now Second error..\n <<<<<<<<<<< Error is detected >>>>>>>>>>>>>\n\n");
    num_reexecution = 2;
//    root->CDAssert(false);
  }

//  free(test_valloc);
//  free(test_realloc);
  printf("root CD Complete\n");
  //  root->Complete();
  CD_Complete(root);

  printf("root CD Destroy\n");
  CD_Finalize();


  // check the test result   
  for( int i = 0; i < 5; i++ ) {
    printf("test_result[%d] = %d\n", i, test_results[i]);
    if( test_results[i] != 1 ) {
      test_result = -1;
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(mr == 0) getchar();
  if( test_result == -1 ) return kError;
  return kOK; //
}



// Test basic preservation scheme.
int test_preservation_via_copy_2()
{
   
  int a[4]= {3,0,};
  int b[8]= {1,0,};
	int c[8]= {0,};
//  int64_t* c  = new int64_t[SIZE];
  int64_t* c1 = new int64_t[SIZE];
  int64_t* c2 = new int64_t[SIZE];
  int64_t* c3 = new int64_t[SIZE];
  int64_t* c4 = new int64_t[SIZE];
  int64_t* c5 = new int64_t[SIZE];
  int64_t* c6 = new int64_t[SIZE];
  int64_t* c7 = new int64_t[SIZE];
  int64_t* c8 = new int64_t[SIZE];
  int64_t* c9 = new int64_t[SIZE];
  int64_t* c10 = new int64_t[SIZE];
  int64_t* c11 = new int64_t[SIZE];
  int64_t* c12 = new int64_t[SIZE];
  int64_t* c13 = new int64_t[SIZE];
  int64_t* c14 = new int64_t[SIZE];
  int64_t* c15 = new int64_t[SIZE];
  int64_t* c16 = new int64_t[SIZE];
  int64_t* c17 = new int64_t[SIZE];
  int64_t* c18 = new int64_t[SIZE];
  int64_t* c19 = new int64_t[SIZE];
  int64_t* c20 = new int64_t[SIZE];

  int d[8]= {1,2,};
  int e[8]= {0,3,};

  int test_results[8] = {0,};
  int test_result = 0;
  int num_reexecution = 0;
  int num_reexecution_ = 0;


  CDErrT err;
  printf("\n\n--test_preservation_via_copy_2 begins-----------------------\n");
  printf("CD Create\n");
	CDHandle* root = CD_Init(np, mr);
  printf("CD Begin.........lol\n");
  CD_Begin(root);
  
  printf("CD Preserving..\n");
  root->Preserve((char *)&a,4* sizeof(int), kCopy, "a");
  root->Preserve((char *)&b,8* sizeof(int), kCopy, "b");
	
//testing whether it can search all ancestors' entries
//Original copy must have ref_name==0 by default.
	
  CDHandle* child=root->Create("CD1", kStrict, 0, 0, &err);
  printf("Child CD Begin\n");
  CD_Begin(child); 
  printf("child CD Preserving..\n");
	auto preserve_start = chrono::high_resolution_clock::now();
//  handle_child.Preserve((char *)&c,2* sizeof(int), kReference, "b", "a");
  child->Preserve((char *)c1, SIZE*sizeof(int64_t));
  child->Preserve((char *)c2, SIZE*sizeof(int64_t));
  child->Preserve((char *)c3, SIZE*sizeof(int64_t));
  child->Preserve((char *)c4, SIZE*sizeof(int64_t));
  child->Preserve((char *)c5, SIZE*sizeof(int64_t));
  child->Preserve((char *)c6, SIZE*sizeof(int64_t));
  child->Preserve((char *)c7, SIZE*sizeof(int64_t));
  child->Preserve((char *)c8, SIZE*sizeof(int64_t));
  child->Preserve((char *)c9, SIZE*sizeof(int64_t));
  child->Preserve((char *)c, SIZE*sizeof(int64_t));
  child->Preserve((char *)c11, SIZE*sizeof(int64_t));
  child->Preserve((char *)c12, SIZE*sizeof(int64_t));
  child->Preserve((char *)c13, SIZE*sizeof(int64_t));
  child->Preserve((char *)c14, SIZE*sizeof(int64_t));
  child->Preserve((char *)c15, SIZE*sizeof(int64_t));
  child->Preserve((char *)c16, SIZE*sizeof(int64_t));
  child->Preserve((char *)c17, SIZE*sizeof(int64_t));
  child->Preserve((char *)c18, SIZE*sizeof(int64_t));
  child->Preserve((char *)c19, SIZE*sizeof(int64_t));
  child->Preserve((char *)c10, SIZE*sizeof(int64_t));
	auto preserve_end = chrono::high_resolution_clock::now();
	chrono::duration<double> elapsed_seconds = preserve_end - preserve_start;
	cout<<endl<<"Elapsed preservation time: "<<elapsed_seconds.count()<<"s"<<endl<<endl; //getchar();

	printf("Before c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
  if(num_reexecution_ == 0){
    num_reexecution_ = 1;
    printf("After c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
	  child->CDAssert(false);
  }

//1 more children
/*	
	CDHandle handle_child1 = CD::Create(cd::kStrict, handle_child);
	CD *child1 = handle_child1.ptr_cd();
  printf("Child CD Begin\n");
  CD_Begin(handle_child1); 
  printf("child CD Preserving..\n");
  handle_child1.Preserve((char *)&d, 2* sizeof(int), kReference, "a2", "a1");

	CDHandle handle_child2 = CD::Create(cd::kStrict, handle_child1);
	CD *child2 = handle_child2.ptr_cd();
  printf("Child CD Begin\n");
  CD_Begin(handle_child2); 
  printf("child CD Preserving..\n");
  handle_child2.Preserve((char *)&e, 2* sizeof(int), kReference, "", "a2");

printf("Before e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
if(num_reexecution_ == 0){
num_reexecution_ = 1;
printf("After e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], num_reexecution_);
	handle_child2.CDAssert(false);
}

  printf("child CD Complete\n");
	CD_Complete(handle_child2);
  printf("child CD Destroy\n");
	CD::Destroy(child2);
  printf("child CD Complete\n");
	CD_Complete(handle_child1);
  printf("child CD Destroy\n");
	CD::Destroy(child1);
  printf("child CD Complete\n");
*/

	CD_Complete(child);
  printf("child CD Destroy\n");
  child->Destroy();

  printf("Before Modify Current value of a[0]=%d a[1]=%d\n", a[0], a[1]);
  printf("Before Modify Current value of b[0]=%d b[1]=%d\n", b[0], b[1]);
 
  if( num_reexecution == 1)  {
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
  printf("After Modify Current value of a[0]=%d\n", a[0]);
  printf("After Modify Current value of b[0]=%d\n", b[0]);

  if( num_reexecution== 0) {
  	printf("is now First error..\n");
	  num_reexecution = 1;
	  root->CDAssert(false);
  }

  // this point is to test whether execution mode becomes kExecution from this point, 
  // because before this preservation is called it should be in kReexecution mode
  root->Preserve((char *)&c,8* sizeof(int));

  if( num_reexecution == 2) {
    if( c[0] == 5 ) {
      test_results[4] = 1;
    }
  }

  printf("Before modifying current value of c[0] %d\n", c[0]);
  c[0] =77;
  printf("After modifying current value of c[0] %d\n", c[0]);

  if(num_reexecution ==1)
  {
	  printf("is now Second error..\n");
	  num_reexecution= 2;
	  root->CDAssert(false);

  }

  printf("CD Complete\n");
  CD_Complete(root);

  printf("CD Destroy\n");
  CD_Finalize();

  printf("\n-- Test Result Check! ----------------------------\n"); //getchar();
  // check the test result   
  for( int i = 0; i < 5; i++ ) {
    printf("test_result[%d] = %d\n", i, test_results[i]);
    if( test_results[i] != 1 ){
      test_result = -1;
    }
  }

  printf("\n-- Test Result : %d ----------------------------\n", test_result); //getchar();
  if( test_result == -1 ) 
    return kError;
  else
    return kOK; 
}

// Test basic via reference scheme.
int test_preservation_via_ref()
{
  int a[4]= {3,0,};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int num_reexecution = 0;
  int test_result = 0;
  
  //CDHandle no_parent;  // not initialized and this means no parent
  printf("\n\n---------------test_preservation_via_ref begins-----------------------------\n");
  CDErrT err;
	CDHandle* root = CD_Init(np, mr);
  CD_Begin(root); 
  root->Preserve(a, sizeof(a), kCopy, "a");

  printf("Before modifying current value of a[0] %d a[1] %d\n", a[0], a[1]);
  a[0] = 99;  // now when child recovers it should see 3 instead of 99
  printf("After modifying  current value of a[0] %d a[1] %d\n", a[0], a[1]);

  CDHandle* child=root->Create(LV1, "CD1", kStrict, 0, 0, &err);
  CD_Begin(child); 
  printf("child cd begin\n"); 
  child->Preserve(a, sizeof(a), kRef, "nonamejusttest", "a", 0);
  
  if( num_reexecution == 0 ) {
    num_reexecution = 1;
	  child->CDAssert(false);

  }
  if( num_reexecution == 1 ) {

    printf("After Reexec :   value of a[0] %d a[1] %d\n", a[0], a[1]);
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

// Test basic via reference with partial update scheme.
int test_preservation_via_referenence_partial_update()
{
  int a[4]= {3,7,9,10};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int num_reexecution = 0;
  int test_result = 1;
  
  printf("\n\n---------------test_preservation_via_referenence_partial_update begins-----------------------------\n");
//  CDHandle no_parent;  // not initialized and this means no parent
//  CDHandle handle_cd=CD::Create(cd::kStrict, no_parent);
//  CD *root= handle_cd.ptr_cd();
	CDHandle* root = CD_Init(np, mr);
  CD_Begin(root); 
  root->Preserve(a, sizeof(a), kCopy, "a");

  printf("Before modifying current value of a[0] %d a[1] %d  a[2] %d a[3] %d \n", a[0], a[1], a[2], a[3]);
  a[0] = 99;  // now when child recovers it should see 99 after recovery of child
  a[1] = 111;  // now when child recovers it should see 7 instead of 111 
  a[2] = 112;  
  a[3] = 113;  
  CDErrT err;
  CDHandle* child=root->Create(LV1, "CD1", kStrict, 0, 0, &err);
  CD_Begin(child); 
  child->Preserve(&(a[1]), 3*sizeof(int), kRef, "a", "a", sizeof(int));
  printf("Child CD begins a[0] %d a[1] %d a[2] %d a[3] %d  \n", a[0], a[1], a[2], a[3]);

  if( num_reexecution == 0 ) {
    num_reexecution = 1;
	  child->CDAssert(false);
  }

  if( num_reexecution == 1 )
  {
    if(a[0] != 99 )
      test_result=0;
    if(a[1] != 7 )
      test_result=0;
    if(a[2] != 9 )
      test_result=0;
    if(a[3] != 10 )
      test_result=0;
  }
  CD_Complete(child);
  child->Destroy();

 
  CD_Complete(root);
  root->Destroy();
  if( test_result == 1 ) return kOK;
  return kError;
}

/*
// Test basic via reference scheme.
int test_preservation_via_ref()
{
  int a[4]= {3,0,};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int num_reexecution = 0;
  int test_result = 0;
  
  //CDHandle no_parent;  // not initialized and this means no parent
  printf("\n\n---------------test_preemption begins-----------------------------\n");
  CDErrT err;
	CDHandle* root = CD_Init(np, mr);
  CD_Begin(root); 
  root->Preserve(a, sizeof(a), kCopy, "a");

  printf("Before modifying current value of a[0] %d a[1] %d\n", a[0], a[1]);
  a[0] = 99;  // now when child recovers it should see 3 instead of 99
  printf("After modifying  current value of a[0] %d a[1] %d\n", a[0], a[1]);

  CDHandle* child=root->Create(LV1, "CD1", kStrict, 0, 0, &err);
  CD_Begin(child); 
  printf("child cd begin\n"); 
  child->Preserve(a, sizeof(a), kRef, "nonamejusttest", "a", 0);
  
  if( num_reexecution == 0 ) {
    num_reexecution = 1;
	  child->CDAssert(false);

  }
  if( num_reexecution == 1 ) {

    printf("After Reexec :   value of a[0] %d a[1] %d\n", a[0], a[1]);
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

// Test basic via reference with partial update scheme.
int test_preservation_via_referenence_partial_update()
{
  int a[4]= {3,7,9,10};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int num_reexecution = 0;
  int test_result = 1;
  
  printf("\n\n---------------test_preservation_via_referenence_partial_update begins-----------------------------\n");
//  CDHandle no_parent;  // not initialized and this means no parent
//  CDHandle handle_cd=CD::Create(cd::kStrict, no_parent);
//  CD *root= handle_cd.ptr_cd();
	CDHandle* root = CD_Init(np, mr);
  CD_Begin(root); 
  root->Preserve(a, sizeof(a), kCopy, "a");

  printf("Before modifying current value of a[0] %d a[1] %d  a[2] %d a[3] %d \n", a[0], a[1], a[2], a[3]);
  a[0] = 99;  // now when child recovers it should see 99 after recovery of child
  a[1] = 111;  // now when child recovers it should see 7 instead of 111 
  a[2] = 112;  
  a[3] = 113;  
  CDErrT err;
  CDHandle* child=root->Create(LV1, "CD1", kStrict, 0, 0, &err);
  CD_Begin(child); 
  child->Preserve(&(a[1]), 3*sizeof(int), kRef, "a", "a", sizeof(int));
  printf("Child CD begins a[0] %d a[1] %d a[2] %d a[3] %d  \n", a[0], a[1], a[2], a[3]);

  if( num_reexecution == 0 ) {
    num_reexecution = 1;
	  child->CDAssert(false);
  }

  if( num_reexecution == 1 )
  {
    if(a[0] != 99 )
      test_result=0;
    if(a[1] != 7 )
      test_result=0;
    if(a[2] != 9 )
      test_result=0;
    if(a[3] != 10 )
      test_result=0;
  }
  CD_Complete(child);
  child->Destroy();

 
  CD_Complete(root);
  root->Destroy();
  if( test_result == 1 ) return kOK;
  return kError;
}

*/

int main(int argc, char* argv[])
{
  int nprocs, myrank;

  app_side=false;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,  &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  app_side=true;
  np = nprocs;
  mr = myrank;
  int ret=0;
 
 
 
  printf("\ntest_preservation_via_copy() \n\n"); 
  ret = test_preservation_via_copy();
  if( ret == kError ) printf("test via copy FAILED\n");
  else printf("test via copy PASSED\n");
 
 
/* 
  printf("\ntest_preservation_via_referenence \n\n"); 
  ret = test_preservation_via_ref();
  if( ret == kError ) printf("test_preservation_via_reference FAILED\n");
  else printf("test_preservation_via_reference PASSED\n");
 
  printf("\ntest_preservation_via_referenence_partial_update() \n\n"); 
  ret = test_preservation_via_referenence_partial_update();
  if( ret == kError ) printf("test_preservation_via_referenence_partial_update FAILED\n");
  else printf("test_preservation_via_referenence_partial_update PASSED\n");
*/

//  printf("\n\n------------ performance test 1 ---------------\n\n");  //getchar();
//  performance_test1();
//  if(mr==0) getchar();
  MPI_Barrier(MPI_COMM_WORLD);


  MPI_Finalize();
  return 0;
}
