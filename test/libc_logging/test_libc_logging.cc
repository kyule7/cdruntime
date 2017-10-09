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
#include <stdlib.h>
#include <iostream>
//#include <sstream>
#include <malloc.h>
#include <string>
//#include <map>
#include "cd.h"
#include "libc_wrapper.h"
#include "cd_features.h"
#define CD_MPI_ENABLED 1
#define _CD 0
#if CD_MPI_ENABLED
#include <mpi.h>
#endif
#define LV1 1
#define LV2 1
#define LV3 1 
using namespace tuned;
using namespace std;

int num_reexecution = 0;
int  numProcs = 0;
int  myRank = 0;
CDErrT err;

// Test basic preservation scheme.
int TestCDHierarchy(void)
{

  //cout << "\n==== TestCDHierarchy Start ====\n" << endl; 
	CDHandle *root = CD_Init(numProcs, myRank, kHDD);
#if _CD
  CD_Begin(root, "Root"); 
#endif
  printf("---start \n"); //getchar();
  int *malloc_p = (int *)malloc(128);
  int *calloc_p = (int *)calloc(16, 32);
  printf("calloc:%p\n", calloc_p);
  int *tmp1 = (int*)calloc(16, 64);
  int * tmp2 = (int*)calloc(16, 128); //getchar();
  printf("calloc %p, tmp1:%p, tmp2:%p\n", &calloc, tmp1, tmp2);
  unsigned long long *cval = (unsigned long long *)(&calloc);
  printf("##################-------------------\n");
  logger::GetLogger()->Print();
  printf("%lx %lx %lx %lx ", *cval, *(cval+1), *(cval+2), *(cval+3));
  printf("%lx %lx %lx %lx\n", *(cval+4), *(cval+5), *(cval+6), *(cval+7));
  printf("---\n"); //getchar();
  //cout << "Root CD Begin...\n" << endl;
  //cout << "CD Preserving..\n" << endl;
#if _CD
  CDHandle* child_lv1=root->Create(LV1, "CD1", kStrict, 0, 0, &err);
#endif
  //cout << "Root Creates Level 1 CD. # of children CDs = " << LV1 << "\n" << endl;

#if _CD
  CD_Begin(child_lv1, "child Lv1");
#endif
  //cout << "\t\tLevel 1 CD Begin...\n" << endl;

    int *valloc_p = (int *)valloc(256);
    int *realloc_p = (int *)realloc(malloc_p, 512);

  // Level 1 Body
  //cout << string(1<<1, '\t').c_str() << "Here is computation body of CD level 1...\n" << endl;

#if _CD
  CDHandle* child_lv2=child_lv1->Create(LV2, "CD2", kStrict, 0, 0, &err);
  //cout << string(1<<1, '\t').c_str() << "CD1 Creates Level 2 CD. # of children CDs = " << LV2 << "\n" << endl;
  CD_Begin(child_lv2, "Child Lv2");
#endif
  //cout << string(2<<1, '\t').c_str() <<"Level 2 CD Begin...\n" << endl;
    int *memalign_p = NULL;
    void *ptr = NULL;
      int *malloc_p2 = (int *)malloc(128);
      int *calloc_p2 = (int *)calloc(16, 32);
      int *valloc_p2 = (int *)valloc(256);
      int *realloc_p2 = (int *)realloc(malloc_p2, 512);
      free(calloc_p2);
      free(valloc_p2);

  // Level 2 Body
  //cout << string(2<<1, '\t').c_str() << "Here is computation body of CD level 2...\n" << endl;

#if _CD
  CDHandle* child_lv3=child_lv2->Create(LV3, "CD3", kDRAM|kStrict, 0, 0, &err);
  //cout << string(2<<1, '\t').c_str() << "CD2 Creates Level 3 CD. # of children CDs = " << LV3 << "\n" << endl;
  CD_Begin(child_lv3, "Child Lv3");
#endif
  //cout << string(3<<1, '\t').c_str() << "Level 3 CD Begin...\n" << endl;
      // Body Level 3
      memalign_p = (int *)memalign(512, 32);
      int ret = posix_memalign(&ptr, 512, 64); 
  
      free(realloc_p2);

  // Level 3 Body
  //cout << string(3<<1, '\t').c_str() << "Here is computation body of CD level 3...\n" << endl;

#if _CD
  child_lv3->Detect();
  //cout << string(3<<1, '\t').c_str() << "Level 3 CD Complete...\n" << endl;
  CD_Complete(child_lv3);
  child_lv3->Destroy();
  //cout << string(3<<1, '\t').c_str() << "Level 3 CD Destroyed...\n" << endl;
#endif
    free(realloc_p);
    free(calloc_p);
    free(valloc_p);
#if _CD
  // Detect Error here
  child_lv2->Detect();

  CD_Complete(child_lv2);
  //cout << string(2<<1, '\t').c_str() << "Level 2 CD Complete...\n" << endl;
  child_lv2->Destroy();
  //cout << string(2<<1, '\t').c_str() << "Level 2 CD Destroyed...\n" << endl;
#endif
    free(memalign_p);
    free(ptr);
#if _CD
  // Detect Error here
  child_lv1->Detect();
  CD_Complete(child_lv1);
  //cout << string(1<<1, '\t').c_str() << "Level 1 CD Complete...\n" << endl;
  child_lv1->Destroy();
  //cout << string(1<<1, '\t').c_str() << "Level 1 CD Destroyed...\n" << endl;
  printf("num execution : %d at #%d\n", num_reexecution, myRank);
  // Detect Error here
  root->Detect();

  if(num_reexecution == 0) {
    root->CDAssert(0);
    num_reexecution++;
  }

  CD_Complete(root);
  //cout << "Root CD Complete...\n" << endl;
  //cout << "Root CD Destroyed (Finalized) ...\n" << endl;
  //cout << "\n==== TestCDHierarchy Done ====\n" << endl; 
  //cout.flush();
#endif

  CD_Finalize();


  return kOK; 
}




int main(int argc, char* argv[])
{

#if CD_MPI_ENABLED
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,  &numProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
#else
  numProcs = 1;
  myRank = 0;
#endif

  int ret=0;

//  //cout.open((string("./output/output_app_")+to_string((unsigned long long)myRank)).c_str());
  //cout << "\n==== TestSimpleHierarchy ====\n" << endl;

  ret = TestCDHierarchy();
  
  //if( ret == kError ) 
    //cout << "Test CD Hierarchy FAILED\n" << endl;
  //else 
    //cout << "Test CD Hierarchy PASSED\n" << endl;

//  //cout.close();

#if CD_MPI_ENABLED
  MPI_Finalize();
#endif

  return 0;
} 
