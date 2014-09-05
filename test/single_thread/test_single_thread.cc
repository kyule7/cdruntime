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
#include "../../src/cd.h"

#define SIZE 655360 //10M?

using namespace cd;

ucontext_t context;


#define NUM_TEST_BLOCKS 1000
#define SIZE_BLOCK (25*1024)

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
  int iteration = 0; 
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


  CD_BEGIN(root); 
  accumulate_cnt = 0;
  begin_cnt = getCounter();
  for( i= 0 ; i < NUM_TEST_BLOCKS; i++ )
  {
//    printf("%d\n",i);
    root->Preserve((char *)a[i],SIZE_BLOCK * sizeof(int));
  }
  end_cnt = getCounter();
  printf("Time elapsed:%lld\n",end_cnt - begin_cnt);


  if( iteration== 0) 
  {
    iteration =1;
    root->CDAssert(false);
  }

  //END
  CD_COMPLETE(root);
  CD_Finalize();



  //epilogue
  for( i = 0 ; i < NUM_TEST_BLOCKS; i++ )
  {
    delete [] a[i];
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
//  int iteration = 0;
//int iteration_ = 0;
//  printf("\n\ntest1 begins\n");
//  printf("CD Create\n");
//  CDHandle handle_cd=CD::Create(cd::kStrict, no_parent);
//  CD *root= handle_cd.ptr_cd();// for now let's just get the pointer and use it directly
//
//  printf("CD Begin\n");
////  root->Begin();  
////  getcontext(&root->context_);
//  CD_BEGIN(handle_cd); 
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
//  CD_BEGIN(handle_child); 
//  printf("child CD Preserving..\n");
//  handle_child.Preserve((char *)&c,2* sizeof(int), kReference, "b", "a");
//  handle_child.Preserve((char *)&e,2* sizeof(int), kReference, "a1", "b");
//
//  printf("Before c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], iteration_);
//  if(iteration_ == 0){
//    iteration_ = 1;
//    printf("After c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], iteration_);
//    handle_child.CDAssert(false);
//  }
//  	
//      CDHandle handle_child1 = CD::Create(cd::kStrict, handle_child);
//      CD *child1 = handle_child1.ptr_cd();
//      printf("Child CD Begin\n");
//      CD_BEGIN(handle_child1); 
//  printf("child CD Preserving..\n");
//  handle_child1.Preserve((char *)&d, 2* sizeof(int), kReference, "a2", "a1");
//
//	CDHandle handle_child2 = CD::Create(cd::kStrict, handle_child1);
//	CD *child2 = handle_child2.ptr_cd();
//  printf("Child CD Begin\n");
//  CD_BEGIN(handle_child2); 
//  printf("child CD Preserving..\n");
//  handle_child2.Preserve((char *)&e, 2* sizeof(int), kReference, "", "a2");
//
//printf("Before e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], iteration_);
//if(iteration_ == 0){
//iteration_ = 1;
//printf("After e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], iteration_);
//	handle_child2.CDAssert(false);
//}
//
//  printf("child CD Complete\n");
//	CD_COMPLETE(handle_child2);
//  printf("child CD Destroy\n");
//	CD::Destroy(child2);
//  printf("child CD Complete\n");
//	CD_COMPLETE(handle_child1);
//  printf("child CD Destroy\n");
//	CD::Destroy(child1);
//  printf("child CD Complete\n");
//
//	CD_COMPLETE(handle_child);
//  printf("child CD Destroy\n");
//CD::Destroy(child);
//
//  printf("Before Modify Current value of a[0]=%d a[1]=%d\n", a[0], a[1]);
//  printf("Before Modify Current value of b[0]=%d b[1]=%d\n", b[0], b[1]);
// 
//  if( iteration == 1) 
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
//  if( iteration== 0) 
//  {
//  	printf("is now First error..\n");
//	  iteration= 1;
//	  handle_cd.CDAssert(false);
//  	//setcontext(&root->context_);
//	  
//  }
//
//  // this point is to test whether execution mode becomes kExecution from this point, because before this preservation is called it should be in kReexecution mode
//  handle_cd.Preserve((char *)&c,8* sizeof(int));
//  if( iteration == 2)
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
//  if(iteration ==1)
//  {
//	  printf("is now Second error..\n");
//	  iteration= 2;
//	  handle_cd.CDAssert(false);
//
//  }
//
//  printf("CD Complete\n");
////  root->Complete();
//  CD_COMPLETE(handle_cd);
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
int test1()
{

  int a[4]= {3,0,};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int test_results[8] = {0,};
  int test_result = 0;
  int iteration = 0;
  printf("\n\ntest1 begins\n");
  printf("CD Create\n");
//  CD *root= handle_cd.ptr_cd();// for now let's just get the pointer and use it directly
	CDHandle* root = CD_Init();
  printf("CD Begin.........lol\n");
  //  root->Begin();  
  //  getcontext(&root->context_);
  CD_BEGIN(root); 

  printf("CD Preserving..\n");
  root->Preserve((char *)&a,4* sizeof(int));
  //root->Preserve((char *)&a,4* sizeof(int));
  root->Preserve((char *)&b,8* sizeof(int));
  //root->Preserve((char *)&b,8* sizeof(int));

  printf("Before Modify Current value of a[0]=%d a[1]=%d\n", a[0], a[1]);
  printf("Before Modify Current value of b[0]=%d b[1]=%d\n", b[0], b[1]);

  if( iteration == 1) 
  {
    if( a[0] == 3 ) 
    {
      test_results[0] = 1;
    }
    if( a[1] == 0 ) 
    {
      test_results[1] = 1;
    }

    if( b[0] == 1 ) 
    {
      test_results[2] = 1;
    }
    if( b[1] == 0 ) 
    {
      test_results[3] = 1;
    }
  }

  a[0] =2;
  b[0] =5;
  printf("After Modify Current value of a[0]=%d\n", a[0]);
  printf("After Modify Current value of b[0]=%d\n", b[0]);

  if( iteration== 0) 
  {
    printf("is now First error..\n");
    iteration= 1;
    root->CDAssert(false);
    //setcontext(&root->context_);

  }

  // this point is to test whether execution mode becomes kExecution from this point, because before this preservation is called it should be in kReexecution mode
  root->Preserve((char *)&c,8* sizeof(int));
  if( iteration == 2)
  {
    if( c[0] == 5 )
    {
      test_results[4] =1;
    }

  }
  printf("Before modifying current value of c[0] %d\n", c[0]);
  c[0] =77;
  printf("After modifying current value of c[0] %d\n", c[0]);
  if(iteration ==1)
  {
    printf("is now Second error..\n");
    iteration= 2;
    root->CDAssert(false);

  }
  printf("CD Complete\n");
  //  root->Complete();
  CD_COMPLETE(root);

  printf("CD Destroy\n");
  CD_Finalize();


  // check the test result   
  for( int i = 0; i < 5; i++ )
  {
    printf("test_result[%d] = %d\n", i, test_results[i]);
    if( test_results[i] != 1 )
    {
      test_result = -1;
    }
  }
  if( test_result == -1 ) return kError;
  return kOK; //
}

//// Test basic preservation scheme.
//int test1()
//{
//   
//  CDHandle no_parent;  // not initialized and this means no parent
//  int a[4]= {3,0,};
//  int b[8]= {1,0,};
////  int c[8] = {1,0,};
//  int64_t* c11 = new int64_t[SIZE];
//  int64_t* c12 = new int64_t[SIZE];
//  int64_t* c13 = new int64_t[SIZE];
//  int64_t* c14 = new int64_t[SIZE];
//  int64_t* c15 = new int64_t[SIZE];
//  int64_t* c16 = new int64_t[SIZE];
//  int64_t* c17 = new int64_t[SIZE];
//  int64_t* c18 = new int64_t[SIZE];
//  int64_t* c19 = new int64_t[SIZE];
//  int64_t* c10 = new int64_t[SIZE];
//	
//  int64_t* c1 = new int64_t[SIZE];
//  int64_t* c2 = new int64_t[SIZE];
//  int64_t* c3 = new int64_t[SIZE];
//  int64_t* c4 = new int64_t[SIZE];
//  int64_t* c5 = new int64_t[SIZE];
//  int64_t* c6 = new int64_t[SIZE];
//  int64_t* c7 = new int64_t[SIZE];
//  int64_t* c8 = new int64_t[SIZE];
//  int64_t* c9 = new int64_t[SIZE];
//  int64_t* c = new int64_t[SIZE];
//  int d[8]= {1,2,};
//  int e[8]= {0,3,};
//  int test_results[8] = {0,};
//  int test_result = 0;
//  int iteration = 0;
//int iteration_ = 0;
//  printf("\n\ntest1 begins\n");
//  printf("CD Create\n");
//  CDHandle handle_cd=CD::Create(cd::kStrict, no_parent);
//  CD *root= handle_cd.ptr_cd();// for now let's just get the pointer and use it directly
//
//  printf("CD Begin\n");
////  root->Begin();  
////  getcontext(&root->context_);
//  CD_BEGIN(handle_cd); 
//  
//  printf("CD Preserving..\n");
//  handle_cd.Preserve((char *)&a,4* sizeof(int), kCopy, "a");
//  //root->Preserve((char *)&a,4* sizeof(int));
//  handle_cd.Preserve((char *)&b,8* sizeof(int), kCopy, "b");
//  //root->Preserve((char *)&b,8* sizeof(int));
////	handle_cd.Body(); 
//	
////testing whether it can search all ancestors' entries
////Original copy must have ref_name==0 by default.
//	
//	CDHandle handle_child = CD::Create(cd::kStrict, handle_cd);
//	CD *child = handle_child.ptr_cd();
//  printf("Child CD Begin\n");
//  CD_BEGIN(handle_child); 
//  printf("child CD Preserving..\n");
//	auto preserve_start = chrono::high_resolution_clock::now();
////  handle_child.Preserve((char *)&c,2* sizeof(int), kReference, "b", "a");
//  handle_child.Preserve((char *)c1, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c2, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c3, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c4, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c5, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c6, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c7, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c8, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c9, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c11, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c12, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c13, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c14, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c15, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c16, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c17, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c18, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c19, SIZE*sizeof(int64_t));
//  handle_child.Preserve((char *)c10, SIZE*sizeof(int64_t));
//	auto preserve_end = chrono::high_resolution_clock::now();
//	chrono::duration<double> elapsed_seconds = preserve_end - preserve_start;
//	cout<<endl<<"Elapsed preservation time: "<<elapsed_seconds.count()<<"s"<<endl<<endl;
////  handle_child.Preserve((char *)&e,2* sizeof(int), kReference, "a1", "b");
//
//	printf("Before c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], iteration_);
//if(iteration_ == 0){
//iteration_ = 1;
//printf("After c[0]=%d c[1]=%d iter:%d\n", e[0], e[1], iteration_);
//	handle_child.CDAssert(false);
//}
///*	
//	CDHandle handle_child1 = CD::Create(cd::kStrict, handle_child);
//	CD *child1 = handle_child1.ptr_cd();
//  printf("Child CD Begin\n");
//  CD_BEGIN(handle_child1); 
//  printf("child CD Preserving..\n");
//  handle_child1.Preserve((char *)&d, 2* sizeof(int), kReference, "a2", "a1");
//
//	CDHandle handle_child2 = CD::Create(cd::kStrict, handle_child1);
//	CD *child2 = handle_child2.ptr_cd();
//  printf("Child CD Begin\n");
//  CD_BEGIN(handle_child2); 
//  printf("child CD Preserving..\n");
//  handle_child2.Preserve((char *)&e, 2* sizeof(int), kReference, "", "a2");
//
//printf("Before e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], iteration_);
//if(iteration_ == 0){
//iteration_ = 1;
//printf("After e[0]=%d e[1]=%d iter:%d\n", e[0], e[1], iteration_);
//	handle_child2.CDAssert(false);
//}
//
//  printf("child CD Complete\n");
//	CD_COMPLETE(handle_child2);
//  printf("child CD Destroy\n");
//	CD::Destroy(child2);
//  printf("child CD Complete\n");
//	CD_COMPLETE(handle_child1);
//  printf("child CD Destroy\n");
//	CD::Destroy(child1);
//  printf("child CD Complete\n");
//*/
//	CD_COMPLETE(handle_child);
//  printf("child CD Destroy\n");
//CD::Destroy(child);
//
//  printf("Before Modify Current value of a[0]=%d a[1]=%d\n", a[0], a[1]);
//  printf("Before Modify Current value of b[0]=%d b[1]=%d\n", b[0], b[1]);
// 
//  if( iteration == 1) 
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
//  if( iteration== 0) 
//  {
//  	printf("is now First error..\n");
//	  iteration= 1;
//	  handle_cd.CDAssert(false);
//  	//setcontext(&root->context_);
//	  
//  }
//
//  // this point is to test whether execution mode becomes kExecution from this point, because before this preservation is called it should be in kReexecution mode
//  handle_cd.Preserve((char *)&c,8* sizeof(int));
//  if( iteration == 2)
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
//  if(iteration ==1)
//  {
//	  printf("is now Second error..\n");
//	  iteration= 2;
//	  handle_cd.CDAssert(false);
//
//  }
//
//  printf("CD Complete\n");
////  root->Complete();
//  CD_COMPLETE(handle_cd);
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

// Test basic via reference scheme.
int test2()
{
  int a[4]= {3,0,};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int iteration = 0;
  int test_result = 0;
  
  //CDHandle no_parent;  // not initialized and this means no parent
  printf("\n\ntest2 begins\n");

	CDHandle* root = CD_Init();
  CD_BEGIN(root); 
  root->Preserve((char *)&a,4* sizeof(int), kCopy, "test2viareference");

  printf("Before modifying current value of a[0] %d a[1] %d\n", a[0], a[1]);
  a[0] = 99;  // now when child recovers it should see 3 instead of 99
  CDHandle* child=root->Create();
//  CD *child= handle_cd_child.ptr_cd();
  CD_BEGIN(child); 
  child->Preserve((char *)&a,4* sizeof(int), kCopy, "nonamejusttest", "test2viareference",0);
  printf("Child CD begins a[0] %d a[1] %d\n", a[0], a[1]);
  if( iteration == 0 )
  {
    iteration = 1;
	  child->CDAssert(false);

  }
  if( iteration == 1 )
  {
    if(a[0] == 3 )
      test_result=1;
  }
  CD_COMPLETE(child);
  child->Destroy();

 
  CD_COMPLETE(root);
  CD_Finalize();

  if( test_result == 1 ) return kOK;
  return kError;
}

// Test basic via reference with partial update scheme.
int test3()
{
  int a[4]= {3,7,9,10};
  int b[8]= {1,0,};
  int c[8]= {5,};
  int iteration = 0;
  int test_result = 1;
  
//  CDHandle no_parent;  // not initialized and this means no parent
  printf("\n\ntest3 begins\n");
//  CDHandle handle_cd=CD::Create(cd::kStrict, no_parent);
//  CD *root= handle_cd.ptr_cd();
	CDHandle* root = CD_Init();
  CD_BEGIN(root); 
  root->Preserve((char *)&a,4* sizeof(int), kCopy, "test3viareference");

  printf("Before modifying current value of a[0] %d a[1] %d  a[2] %d a[3] %d \n", a[0], a[1], a[2], a[3]);
  a[0] = 99;  // now when child recovers it should see 99 after recovery of child
  a[1] = 111;  // now when child recovers it should see 7 instead of 111 
  a[2] = 112;  
  a[3] = 113;  

  CDHandle* child=root->Create();
//  CD *child= handle_cd_child.ptr_cd();
  CD_BEGIN(child); 
  child->Preserve((char *)&(a[1]),3* sizeof(int), kCopy, "nonamejusttest", "test3viareference",sizeof(int)*1);
  printf("Child CD begins a[0] %d a[1] %d a[2] %d a[3] %d  \n", a[0], a[1], a[2], a[3]);
  if( iteration == 0 )
  {
    iteration = 1;
	  child->CDAssert(false);

  }
  if( iteration == 1 )
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
  CD_COMPLETE(child);
  child->Destroy();

 
  CD_COMPLETE(root);
  root->Destroy();
  if( test_result == 1 ) return kOK;
  return kError;
}

int main()
{
  int ret=0;
  
 ret = test1();
 if( ret == kError ) printf("Error test1() failed\n");


 printf("test1() "); 
 ret = test1();
 if( ret == kError ) printf("FAILED\n");
 else printf("PASSED\n");



 printf("test2() "); 
 ret = test2();
 if( ret == kError ) printf("FAILED\n");
 else printf("PASSED\n");

  printf("test3() "); 
 ret = test3();
 if( ret == kError ) printf("FAILED\n");
 else printf("PASSED\n");

  performance_test1(); 
  return 0;
}
