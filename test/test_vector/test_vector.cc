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
//#include <stdarg.h>
#include <initializer_list>

#define KILO 1024
#define MEGA 1048576
#define GIGA 1073741824

#define LV1 1
#define LV2 1
#define LV3 1 

#define CD1_ITER 2
#define CD2_ITER 4
#define CD3_ITER 6

#define PRINT(...) { if(myRank == 0) { printf(__VA_ARGS__); } }

//using namespace tuned;
using namespace cd;
using namespace std;
using namespace chrono;

//DebugBuf cout;
CDErrT err;
int  numProcs = 0;
int  myRank = 0;
std::map<uint32_t*, string> vectorName;
#define NUM_TEST_BLOCKS 10
#define SIZE_BLOCK (25*1024)  //100,000kByte
#define ARRAY_A_SIZE 32
#define ARRAY_B_SIZE 8
#define ARRAY_C_SIZE 8
#define ARRAY_D_SIZE 16
#define ARRAY_E_SIZE 16
#define ARRAY_F_SIZE 32
#define ARRAY_G_SIZE 16
#if 0
template< vector<uint32_t>, typename... ArgTypes >
static inline void MutateVector(T, ArgTypes... args) {
  MutateVector(args...);
  
  for (auto &vec: il) {
    int i = 0;
    for(; i<vec.size(); i++) {
      vec[i] = i + count;
    }
    printf("Before vec size:%zu %zu\n", vec.size(), vec.capacity());
    vec.push_back(i + count);
    printf("After  vec size:%zu %zu\n", vec.size(), vec.capacity());
  }
  count++;
}

template<typename... ArgTypes>
static inline void DoCompute(int level, ArgTypes... args) 
{ 
  switch(level) {
    case 0:
      MutateVector(args...); 

      break;
    case 1:
      MutateVector(args...); 

      break;
    case 2:

      MutateVector(args...); 

      break;

    case 3:
      MutateVector(args...); 
      break;


    default:
      printf("Invalid level: %d\n", level); assert(0);
      break;
  }


  return;  
//  sleep(1); 

}
#endif
static int count = 0;

static __inline__ long long getCounter(void)
{
  unsigned vectorA, vectorD; 
  asm volatile("rdtsc" : "=vectorA" (vectorA), "=vectorD" (vectorD)); 
  return ((long long)vectorA) | (((long long)vectorD) << 32); 
}

static inline void MutateVector(vector<uint32_t> &vec) {
  int i = 0;
  int last = vec.size();
  for(; i<last; i++) {
    vec[i] = i + count;
  }
  PRINT("Before vec (%p) size:%zu %zu\n", vec.data(), vec.size(), vec.capacity());
  last *= 2;
  for(; i<last; i++) {
    vec.push_back(i + count); // this will realloc vector
  }
  PRINT("Bfter  vec (%p) size:%zu %zu\n", vec.data(), vec.size(), vec.capacity());
  count++;
  sleep(1);
}

//static inline void DoCompute(int level, initializer_list<vector<uint32_t>> il) 
//{ 
//  switch(level) {
//    case 0:
//      //MutateVector(il); 
//      for (auto &vec: il) {
//        int i = 0;
//        for(; i<vec.size(); i++) {
//       //   vec[i] = i + count;
//        }
//        printf("before vec size:%zu %zu\n", vec.size(), vec.capacity());
//       // vec.push_back(i + count);
//        printf("after  vec size:%zu %zu\n", vec.size(), vec.capacity());
//      }
//      count++;
//
//      break;
//    case 1:
////      MutateVector(il); 
//
//      break;
//    case 2:
//
////      MutateVector(il); 
//
//      break;
//
//    case 3:
////      MutateVector(il); 
//      break;
//
//
//    default:
//      printf("Invalid level: %d\n", level); assert(0);
//      break;
//  }
//
//
//  return;  
////  sleep(1); 
//
//}

void PrintData(uint32_t *vector, int length)
{
  if(vectorName.find(vector) != vectorName.end())
    PRINT("Print Data %s, Size: %d\n", vectorName[vector].c_str(), length);

  for(int i=0; i<length; i++) {
    PRINT("%d ", vector[i]);
  }
  PRINT("\n==========================\n");
}

void CorruptData(uint32_t *vector, int length)
{
  for(int i=0; i<length; i++) {
    vector[i] = (-20 + i);
  }
}

bool CheckArray(uint32_t *vector, int length)
{
  for(int i=0; i<length; i++) {
    if(vector[i] < 0) {
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
int TestVector(void)
{
  vector<uint32_t> vectorA(8, 1);
  vector<uint32_t> vectorB(16, 1);
  vector<uint32_t> vectorC(32, 1);
  vector<uint32_t> vectorD(64, 1);

  PRINT("\n==== TestVector Start ====\n"); 
  CDHandle *root = CD_Init(numProcs, myRank, kHDD);

  CD_Begin(root);

  PRINT("Root CD Begin...\n");

  PRINT("CD Preserving..\n");
  root->Preserve(vectorA.data(), sizeof(uint32_t) * vectorA.size(), kCopy, "vectorA");
  root->Preserve(vectorB.data(), sizeof(uint32_t) * vectorB.size(), kCopy, "vectorB");
  root->Preserve(vectorC.data(), sizeof(uint32_t) * vectorC.size(), kCopy, "vectorC");
  root->Preserve(vectorD.data(), sizeof(uint32_t) * vectorD.size(), kCopy, "vectorD");
    CDHandle* child_lv1=root->Create(LV1, "CD1", kStrict, 0, 0, &err);
    PRINT("Root Creates Level 1 CD. # of children CDs = %d\n", LV1);
    for(uint32_t ii = 0; ii<CD1_ITER; ii++) {
      CD_Begin(child_lv1, "Level 1"); /* Level 1 Begin ************************************************/
      PRINT("%sBegin at Level 1 (%d)\n", string(1<<1, '\t').c_str(), ii);
      PRINT("%sPreserve via copy: vectorB, vectorC, vectorD\n\n", string(1<<1, '\t').c_str());
      child_lv1->Preserve(vectorB.data(), sizeof(uint32_t) * vectorB.size(), kCopy, "vectorB");
      child_lv1->Preserve(vectorC.data(), sizeof(uint32_t) * vectorC.size(), kCopy, "vectorC");
      child_lv1->Preserve(vectorD.data(), sizeof(uint32_t) * vectorD.size(), kCopy, "vectorD");
    
      // Level 1 Body
      PRINT("%sHere is computation body of CD level 1...\n", string(1<<1, '\t').c_str());
    
        CDHandle* child_lv2=child_lv1->Create(LV2, "CD2", kStrict, 0, 0, &err);
        PRINT("%sCD1 Creates Level 2 CD. # of children CDs = %d\n", string(1<<1, '\t').c_str(), LV2);
        for(uint32_t jj=0; jj<CD2_ITER; jj++) {
          /* Level 2 Begin **********************/
          CD_Begin(child_lv2, (string("CD2_") + to_string((long long)jj / 2)).c_str());
          PRINT("%sBegin at Level 2 (%d)\n", string(2<<1, '\t').c_str(), jj);
          child_lv2->Preserve(vectorC.data(), sizeof(uint32_t) * vectorC.size(), kCopy, "vectorC");
          child_lv2->Preserve(vectorD.data(), sizeof(uint32_t) * vectorD.size(), kCopy, "vectorD");
        
            CDHandle* child_lv3=child_lv2->Create(LV3, "CD3", kDRAM|kStrict, 0, 0, &err);
            for(uint32_t kk=0; kk<CD3_ITER; kk++) { 
              CD_Begin(child_lv3, (string("CD3_") + to_string((long long)kk / 2)).c_str());
              PRINT("%sBegin at Level 3 (%d)\n", string(3<<1, '\t').c_str(), kk);
              child_lv3->Preserve(vectorD.data(), sizeof(uint32_t) * vectorD.size(), kCopy, "vectorD");
              
              // Update vector vectorD

              MutateVector(vectorD);
              child_lv3->Detect();
            
              child_lv3->Complete();
              PRINT("%sComplete at Level 3 (%d)\n", string(3<<1, '\t').c_str(), kk);
            }
            child_lv3->Destroy();
            PRINT("%sDestroyed at Level 3 (%d)\n", string(3<<1, '\t').c_str(), jj);
        
          MutateVector(vectorC);
          MutateVector(vectorD);
          // Detect Error here
          child_lv2->Detect();
        
          child_lv2->Complete(); /* Level 2 End *****************************************/
          PRINT("%sComplete at Level 2 (%d)\n", string(2<<1, '\t').c_str(), jj);
        }
        child_lv2->Destroy(); 
        PRINT("%sDestroyed at Level 2 (%d)\n", string(2<<1, '\t').c_str(), ii);
    
      MutateVector(vectorB);
      MutateVector(vectorC);
      MutateVector(vectorD);
      // Detect Error here
      child_lv1->Detect();
    
      child_lv1->Complete(); /* Level 1 End *******************************************/
      PRINT("%sComplete at Level 1 (%d)\n", string(1<<1, '\t').c_str(), ii);
    }
    child_lv1->Destroy();
    PRINT("%sDestroyed at Level 1 (Root)\n", string(1<<1, '\t').c_str());

  PRINT("num execution : %d at #%d\n", num_reexecution, myRank);

  MutateVector(vectorA);
  MutateVector(vectorB);
  MutateVector(vectorC);
  MutateVector(vectorD);
  // Detect Error here
  root->Detect();

//  if(num_reexecution == 0) {
//    root->CDAssert(0);
//    num_reexecution++;
//  }

  root->Complete();
  PRINT("Root CD Complete...\n");
  PRINT("Root CD Destroyed (Finalized) ...\n");
  PRINT("\n==== TestVector Done ====\n"); 
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
  PRINT("\n==== TestVector ====\n");

  ret = TestVector();
  
  if( ret == kError ) { 
    PRINT("Test CD Hierarchy FAILED\n");
  } else {
    PRINT("Test CD Hierarchy PASSED\n");
  }
//  cout.close();

#if CD_MPI_ENABLED
  MPI_Finalize();
#endif

  return 0;
} 
