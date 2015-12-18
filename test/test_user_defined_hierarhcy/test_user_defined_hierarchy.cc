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
#include <map>

#include "cd.h"

#if CD_MPI_ENABLED
#include <mpi.h>
#endif

#define KILO 1024
#define MEGA 1048576
#define GIGA 1073741824

#define LV2 1

using namespace cd;
using namespace std;
using namespace chrono;

DebugBuf dbgApp;
CDErrT err;
int  numProcs = 0;
int  myRank = 0;
std::map<int*, string> arrayName;
#define NUM_TEST_BLOCKS 10
#define SIZE_BLOCK (25*1024)  //100,000kByte
#define ARRAY_A_SIZE 32
#define ARRAY_B_SIZE 8
#define ARRAY_C_SIZE 8
#define ARRAY_D_SIZE 16
#define ARRAY_E_SIZE 16
#define ARRAY_F_SIZE 32
#define ARRAY_G_SIZE 16

static __inline__ long long getCounter(void)
{
  unsigned arrayA, arrayD; 
  asm volatile("rdtsc" : "=arrayA" (arrayA), "=arrayD" (arrayD)); 
  return ((long long)arrayA) | (((long long)arrayD) << 32); 
}



void PrintData(int *array, int length)
{
  if(arrayName.find(array) != arrayName.end())
    dbgApp <<"Print Data "<< arrayName[array] << ", Size: " << length << endl;

  for(int i=0; i<length; i++) {
    dbgApp << array[i] << " ";
  }
  dbgApp <<"\n=========================="<<endl;
}

void CorruptData(int *array, int length)
{
  for(int i=0; i<length; i++) {
    array[i] = (-20 + i);
  }
}

bool CheckArray(int *array, int length)
{
  for(int i=0; i<length; i++) {
    if(array[i] < 0) {
      return false;
    }
  }
  return true;
}

// Test basic preservation scheme.
int TestCDHierarchy(void)
{
  int arrayA[ARRAY_A_SIZE] = {3,5,0,6};
  int arrayB[ARRAY_B_SIZE] = {1,2,3,4,5,6,7,8};
  int arrayC[ARRAY_C_SIZE] = {5,};
  int arrayD[ARRAY_D_SIZE] = {9,8,7,6,5,4,3,2,1,};
  arrayName[arrayA] = "arrayA";
  arrayName[arrayB] = "arrayB";
  arrayName[arrayC] = "arrayC";
  arrayName[arrayD] = "arrayD";
  int test_results[8] = {0,};
  int test_result = 0;
  int num_reexecution = 0;

  dbgApp << "\n==== TestCDHierarchy Start ====\n" << endl; 
	CDHandle *root = CD_Init(numProcs, myRank);
  CD_Begin(root); 

  dbgApp << "Root CD Begin...\n" << endl;
  root->Preserve(arrayA, sizeof(arrayA), kCopy, "a_root");
  root->Preserve(arrayB, sizeof(arrayB), kCopy, "b_root");

  dbgApp << "CD Preserving..\n" << endl;
  CDHandle* child_lv1=root->Create("CD1", kStrict | kDRAM, 0, 0, &err);
  dbgApp << "Root Creates Level 1 CD. # of children CDs = " << 1 << "\n" << endl;

  CD_Begin(child_lv1);
  dbgApp << "\t\tLevel 1 CD Begin...\n" << endl;

  int arrayE[ARRAY_E_SIZE] = {1,2,3,4,5,6,7,8};
  arrayName[arrayE] = "arrayE";

  child_lv1->Preserve(arrayA, sizeof(arrayA), kCopy, 
                      (string("arrayA-")+to_strong((long long int)myRank)).c_str()); // arrayA-rankID
  child_lv1->Preserve(arrayB, sizeof(arrayB), kCopy, 
                      (string("arrayB-")+to_strong((long long int)myRank)).c_str()); // arrayB-rankID
  child_lv1->Preserve(arrayE, sizeof(arrayE), kCopy, 
                      (string("arrayE-")+to_strong((long long int)myRank)).c_str()); // arrayE-rankID

  dbgApp << "\t\tPreserve via copy: arrayA, arrayB, arrayE\n\n" << endl;


  // Level 1 Body
  dbgApp << string(1<<1, '\t').c_str() << "Here is computation body of CD level 1...\n" << endl;


  int arrayF[ARRAY_F_SIZE] = {0,};
  int arrayG[ARRAY_G_SIZE] = {0,};
  arrayName[arrayF] = "arrayF";
  arrayName[arrayG] = "arrayG";


  // User-defined Create() example.
  int color = 0;
  int task_in_color = 0;
  int color_num = 8;
  switch(myRank) {
    case 0:
      color = 0;
      task_in_color = 0;
      break;
    case 1:
      color = 0;
      task_in_color = 1;
      break;
    case 2:
      color = 0;
      task_in_color = 2;
      break;
    case 3:
      color = 0;
      task_in_color = 3;
      break;
    case 4:
      color = 1;
      task_in_color = 0;
      break;
    case 5:
      color = 1;
      task_in_color = 1;
      break;
    case 6:
      color = 1;
      task_in_color = 2;
      break;
    case 7:
      color = 1;
      task_in_color = 3;
      break;
    default:
      cerr << "Assumed the # of ranks are 8 in this example code." << endl;
      assert(0);
  }
  CDHandle* child_lv2=child_lv1->Create(color, task_in_color, color_num, "CD2", kStrict, 0, 0, &err);
  dbgApp << string(1<<1, '\t').c_str() << "CD1 Creates Level 2 CD. # of children CDs = " << LV2 << "\n" << endl;

  CD_Begin(child_lv2);
  dbgApp << string(2<<1, '\t').c_str() <<"Level 2 CD Begin...\n" << endl;

  child_lv2->Preserve(arrayA, sizeof(arrayA), kRef, 
                      "a_lv2", (string("arrayA-")+to_strong((long long int)myRank)).c_str()); // local
  child_lv2->Preserve(arrayB, sizeof(arrayB), kRef, 
                      "b_lv2", (string("arrayB-")+to_strong((long long int)myRank)).c_str());
  child_lv2->Preserve(arrayE, sizeof(arrayE), kRef, 
                      "e_lv2", (string("arrayE-")+to_strong((long long int)myRank)).c_str());
  child_lv2->Preserve(arrayC, sizeof(arrayC), kCopy, "arrayC");
  dbgApp << string(2<<1, '\t').c_str() <<"Preserve via ref : arrayA (local), arrayB (local), arrayE (local)" << endl;
  dbgApp << string(2<<1, '\t').c_str() <<"Preserve via copy: arrayC\n" << endl;

  // Level 2 Body
  dbgApp << string(2<<1, '\t').c_str()  << "Here is computation body of CD level 2...\n" << endl;

  child_lv2->CDAssert(CheckArray(arrayE, sizeof(arrayE)));

  CDHandle* child_lv3=child_lv2->Create("CD3", kStrict, 0, 0, &err);
  dbgApp << string(2<<1, '\t').c_str() << "CD2 Creates Level 3 CD. # of children CDs = " << 1 << "\n" << endl;

  CD_Begin(child_lv3);
  dbgApp << string(3<<1, '\t').c_str() << "Level 3 CD Begin...\n" << endl;

  child_lv3->Preserve(arrayA, sizeof(arrayA), kRef, "child_a", (string("arrayA")+to_strong((long long int)myRank)).c_str()); // local
  child_lv3->Preserve(arrayB, sizeof(arrayB), kRef, "child_b", (string("arrayB")+to_strong((long long int)myRank)).c_str()); // local
  child_lv3->Preserve(arrayC, sizeof(arrayC), kRef, "child_c", "arrayC");
  child_lv3->Preserve(arrayD, sizeof(arrayD), kCopy, "arrayD");
  dbgApp << string(3<<1, '\t').c_str() << "Preserve via ref : arrayA (local), arrayB (local), arrayC (local)" << endl;
  dbgApp << string(3<<1, '\t').c_str() << "Preserve via copy: arrayD\n" << endl;

  // Level 3 Body
  dbgApp << string(3<<1, '\t').c_str() << "Here is computation body of CD level 3...\n" << endl;


  child_lv3->Detect();

  CD_Complete(child_lv3);
  dbgApp << string(3<<1, '\t').c_str() << "Level 3 CD Complete...\n" << endl;
  child_lv3->Destroy();
  dbgApp << string(3<<1, '\t').c_str() << "Level 3 CD Destroyed...\n" << endl;

  // Detect Error here
  child_lv2->Detect();

  CD_Complete(child_lv2);
  dbgApp << string(2<<1, '\t').c_str() << "Level 2 CD Complete...\n" << endl;
  child_lv2->Destroy();
  dbgApp << string(2<<1, '\t').c_str() << "Level 2 CD Destroyed...\n" << endl;

  // Detect Error here
  child_lv1->Detect();

  CD_Complete(child_lv1);
  dbgApp << string(1<<1, '\t').c_str() << "Level 1 CD Complete...\n" << endl;
  child_lv1->Destroy();
  dbgApp << string(1<<1, '\t').c_str() << "Level 1 CD Destroyed...\n" << endl;

  // Detect Error here
  root->Detect();

  CD_Complete(root);
  dbgApp << "Root CD Complete...\n" << endl;
  dbgApp << "Root CD Destroyed (Finalized) ...\n" << endl;
  dbgApp << "\n==== TestCDHierarchy Done ====\n" << endl; 
  dbgApp.flush();
  CD_Finalize();


  return kOK; 
}




int main(int argc, char* argv[])
{

#if CD_MPI_ENABLED
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,  &numProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
#endif

  int ret=0;

  dbgApp.open((string("./dbg_logs/output_app_")+to_strong((long long int)myRank)).c_str());
  dbgApp << "\n==== TestUserDefinedHierarchy ====\n" << endl;
 
  ret = TestCDHierarchy();
  
  if( ret == kError ) 
    cout << "Test CD Hierarchy FAILED\n" << endl;
  else 
    cout << "Test CD Hierarchy PASSED\n" << endl;


  dbgApp.close();

#if CD_MPI_ENABLED
  MPI_Finalize();
#endif

  return 0;
} 
