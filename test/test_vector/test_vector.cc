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
#define CD_MPI_ENABLED 1
#if CD_MPI_ENABLED
#include <mpi.h>
#endif


#define KILO 1024
#define MEGA 1048576
#define GIGA 1073741824

#define LV1 1
#define LV2 1
#define LV3 1 

#define CD1_ITER 2
#define CD2_ITER 4
#define CD3_ITER 6

using namespace tuned;
//using namespace cd;
using namespace std;
using namespace chrono;

//DebugBuf cout;
CDErrT err;
int  numProcs = 0;
int  myRank = 0;
std::map<uint32_t*, string> arrayName;
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



void PrintData(uint32_t *array, int length)
{
  if(arrayName.find(array) != arrayName.end())
    cout <<"Print Data "<< arrayName[array] << ", Size: " << length << endl;

  for(int i=0; i<length; i++) {
    cout << array[i] << " ";
  }
  cout <<"\n=========================="<<endl;
}

void CorruptData(uint32_t *array, int length)
{
  for(int i=0; i<length; i++) {
    array[i] = (-20 + i);
  }
}

bool CheckArray(uint32_t *array, int length)
{
  for(int i=0; i<length; i++) {
    if(array[i] < 0) {
      return false;
    }
  }
  return true;
}

class UserObject1 {
    uint32_t data0[8];// = {0x89ABCDEF, 0x01234567, 0x6789ABCD, 0xCDAB8967, 0xFEDCBA56, 0x10000000, 0x50055521};
    double data1[8];// = {0.001, 1.394, 391832.29, 392.35, 10000, 32, 32.125, 64.0625};
    bool data2[8];// = {1, 0, 0, 0, 0, 1, 1};
    char data3[8];// = {'a', '3', '5', '8', 'b', '\0', 'c'};
  public:
    Serdes serdes;
  
    UserObject1(void) {
//    data0 = {0x89ABCDEF, 0x01234567, 0x6789ABCD, 0xCDAB8967, 0xFEDCBA56, 0x10000000, 0x50055521};
//    data1 = {0.001, 1.394, 391832.29, 392.35, 10000, 32, 32.125, 64.0625};
//    data2 = {1, 0, 0, 0, 0, 1, 1};
//    data3 = {'a', '3', '5', '8', 'b', '\0', 'c'};
      serdes.Register(0, data0, sizeof(data0));
      printf("data0 is registered. size : %lu\n", sizeof(data0));
      serdes.Register(1, data1, sizeof(data1));
      printf("data1 is registered. size : %lu\n", sizeof(data1));
      serdes.Register(2, data2, sizeof(data2));
      printf("data2 is registered. size : %lu\n", sizeof(data2));
      serdes.Register(3, data3, sizeof(data3));
      printf("data3 is registered. size : %lu\n", sizeof(data3));
    }
};

int num_reexecution = 0;

// Test basic preservation scheme.
int TestCDHierarchy(void)
{
  uint32_t arrayA[ARRAY_A_SIZE] = {0xABCDEFCD,0xCE53AE3B,0xABF31FD0,0xFA34FDA4};
  uint32_t arrayB[ARRAY_B_SIZE] = {1,2,3,4,5,6,7,8};
  uint32_t arrayC[ARRAY_C_SIZE] = {5,};
  uint32_t arrayD[ARRAY_D_SIZE] = {9,8,7,6,5,4,3,2,1,};
  arrayName[arrayA] = "arrayA";
  arrayName[arrayB] = "arrayB";
  arrayName[arrayC] = "arrayC";
  arrayName[arrayD] = "arrayD";

  cout << "\n==== TestCDHierarchy Start ====\n" << endl; 
  CDHandle *root = CD_Init(numProcs, myRank, kHDD);

  root->Begin();

  cout << "Root CD Begin...\n" << endl;

  cout << "CD Preserving..\n" << endl;

    CDHandle* child_lv1=root->Create(LV1, "CD1", kStrict, 0, 0, &err);
    cout << "Root Creates Level 1 CD. # of children CDs = " << LV1 << "\n" << endl;
    for(uint32_t ii = 0; ii<CD1_ITER; ii++) {
      child_lv1->Begin();
      cout << "\t\tLevel 1 CD Begin...\n" << endl;
    
      uint32_t arrayE[ARRAY_E_SIZE] = {1,2,3,4,5,6,7,8};
      arrayName[arrayE] = "arrayE";
    
      cout << "\t\tPreserve via copy: arrayA, arrayB, arrayE\n\n" << endl;
    
      // Level 1 Body
      cout << string(1<<1, '\t').c_str() << "Here is computation body of CD level 1...\n" << endl;
    
      uint32_t arrayF[ARRAY_F_SIZE] = {0,};
      uint32_t arrayG[ARRAY_G_SIZE] = {0,};
      arrayName[arrayF] = "arrayF";
      arrayName[arrayG] = "arrayG";
         
        CDHandle* child_lv2=child_lv1->Create(LV2, "CD2", kStrict, 0, 0, &err);
        cout << string(1<<1, '\t').c_str() 
             << "CD1 Creates Level 2 CD. # of children CDs = " 
             << LV2 << "\n" << endl;
        for(uint32_t jj=0; jj<CD2_ITER; jj++) {
          child_lv2->Begin(false, (string("CD2_") + to_string((long long)jj / 2)).c_str());
        
            CDHandle* child_lv3=child_lv2->Create(LV3, "CD3", kDRAM|kStrict, 0, 0, &err);
            for(uint32_t kk=0; kk<CD3_ITER; kk++) { 
              child_lv3->Begin(false, (string("CD3_") + to_string((long long)kk / 2)).c_str());
            
              child_lv3->Detect();
            
              child_lv3->Complete();
              cout << string(3<<1, '\t').c_str() << "Level 3 CD Complete..." << kk << endl <<endl;
            }
            child_lv3->Destroy();
            cout << string(3<<1, '\t').c_str() << "Level 3 CD Destroyed...\n" << endl;
        
          // Detect Error here
          child_lv2->Detect();
        
          child_lv2->Complete();
          cout << string(2<<1, '\t').c_str() << "Level 2 CD Complete...\n" << jj << endl << endl;
        }
        child_lv2->Destroy();
        cout << string(2<<1, '\t').c_str() << "Level 2 CD Destroyed...\n" << endl;
    
      // Detect Error here
      child_lv1->Detect();
    
      child_lv1->Complete();
      cout << string(1<<1, '\t').c_str() << "Level 1 CD Complete...\n" << ii << endl << endl;
    }
    child_lv1->Destroy();
    cout << string(1<<1, '\t').c_str() << "Level 1 CD Destroyed...\n" << endl;

  printf("num execution : %d at #%d\n", num_reexecution, myRank);
  // Detect Error here
  root->Detect();

//  if(num_reexecution == 0) {
//    root->CDAssert(0);
//    num_reexecution++;
//  }

  root->Complete();
  cout << "Root CD Complete...\n" << endl;
  cout << "Root CD Destroyed (Finalized) ...\n" << endl;
  cout << "\n==== TestCDHierarchy Done ====\n" << endl; 
  cout.flush();

  CD_Finalize();


  return common::kOK; 
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

//  cout.open((string("./output/output_app_")+to_string((unsigned long long)myRank)).c_str());
  cout << "\n==== TestSimpleHierarchy ====\n" << endl;

  ret = TestCDHierarchy();
  
  if( ret == kError ) 
    cout << "Test CD Hierarchy FAILED\n" << endl;
  else 
    cout << "Test CD Hierarchy PASSED\n" << endl;

//  cout.close();

#if CD_MPI_ENABLED
  MPI_Finalize();
#endif

  return 0;
} 
