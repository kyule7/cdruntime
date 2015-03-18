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

void DecomposeData(int *array, int length, int rank)
{
  for(int i=0; i<length; i++) {
    array[i] = (i + rank) % length;
  }
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
int TestPreservationViaRefRemote(void)
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

	CDHandle *root = CD_Init(numProcs, myRank);
  CD_Begin(root); 

  dbgApp << "Root CD Begin...\n" << endl;
  root->Preserve(arrayA, sizeof(arrayA), kCopy, "a_root");
  root->Preserve(arrayB, sizeof(arrayB), kCopy, "b_root");

  dbgApp << "CD Preserving..\n" << endl;
  CDHandle* child_lv1=root->Create(CDPath::GetCurrentCD()->GetNodeID(), LV1, "CD1", kStrict, 0, 0, &err);
  dbgApp << "Root Creates Level 1 CD. # of children CDs = " << LV1 << "\n" << endl;

  CD_Begin(child_lv1);
  dbgApp << "\t\tLevel 1 CD Begin...\n" << endl;

  DecomposeData(arrayA, sizeof(arrayA), myRank);

  int arrayE[ARRAY_E_SIZE] = {1,2,3,4,5,6,7,8};
  DecomposeData(arrayE, sizeof(arrayE), myRank);
  arrayName[arrayE] = "arrayE";

  child_lv1->Preserve(arrayA, sizeof(arrayA), kCopy | kShared, 
                      (string("arrayA-")+to_string(myRank)).c_str()); // arrayA-rankID
  child_lv1->Preserve(arrayB, sizeof(arrayB), kCopy | kShared, 
                      (string("arrayB-")+to_string(myRank)).c_str()); // arrayB-rankID
  child_lv1->Preserve(arrayE, sizeof(arrayE), kCopy | kShared, 
                      (string("arrayE-")+to_string(myRank)).c_str()); // arrayE-rankID

  dbgApp << "\t\tPreserve via copy: arrayA (Share), arrayB (Share), arrayE (Share)\n" << endl;


  // Level 1 Body

  // Corrupt array arrayA and arrayB
//  if(num_reexecution = 0) {
//    CorruptData(arrayA, ARRAY_A_SIZE);
//    num_reexecution++;
//  }
//
//  child_lv1->CDAssert(CheckArray(arrayA, sizeof(arrayA)));

  int arrayF[ARRAY_F_SIZE] = {0,};
  int arrayG[ARRAY_G_SIZE] = {0,};
  arrayName[arrayF] = "arrayF";
  arrayName[arrayG] = "arrayG";

  dbgApp << "\nCheck Array Before Communication =============" << endl;
  PrintData(arrayA, ARRAY_A_SIZE);
  PrintData(arrayE, ARRAY_E_SIZE);
  PrintData(arrayF, ARRAY_F_SIZE);
  PrintData(arrayG, ARRAY_G_SIZE);
  dbgApp << "=============================================" << endl;
  dbgApp.flush();
  MPI_Request arrayF_req;
  MPI_Request arrayG_req;
  if(myRank % 2 == 1) {
    MPI_Send(arrayA, sizeof(arrayA), MPI_BYTE, (myRank+numProcs-1)%numProcs, 1, MPI_COMM_WORLD); // A3->F2
    MPI_Send(arrayE, sizeof(arrayE), MPI_BYTE, (myRank+numProcs-2)%numProcs, 0, MPI_COMM_WORLD); // E3->G1
    MPI_Irecv(arrayF, sizeof(arrayF), MPI_BYTE, (myRank+1)%numProcs, 1, MPI_COMM_WORLD, &arrayF_req); //F3<-A4
    MPI_Irecv(arrayG, sizeof(arrayG), MPI_BYTE, (myRank+2)%numProcs, 0, MPI_COMM_WORLD, &arrayG_req); //G3<-E5
  }
  else {
    MPI_Irecv(arrayF, sizeof(arrayF), MPI_BYTE, (myRank+1)%numProcs, 1, MPI_COMM_WORLD, &arrayF_req);
    MPI_Irecv(arrayG, sizeof(arrayG), MPI_BYTE, (myRank+2)%numProcs, 0, MPI_COMM_WORLD, &arrayG_req);
    MPI_Send(arrayA, sizeof(arrayA), MPI_BYTE, (myRank+numProcs-1)%numProcs, 1, MPI_COMM_WORLD);
    MPI_Send(arrayE, sizeof(arrayE), MPI_BYTE, (myRank+numProcs-2)%numProcs, 0, MPI_COMM_WORLD);
  }
  MPI_Wait(&arrayF_req, MPI_STATUS_IGNORE);
  MPI_Wait(&arrayG_req, MPI_STATUS_IGNORE);

  dbgApp << "\nCheck Array After Communication =============" << endl; dbgApp.flush();
  PrintData(arrayA, ARRAY_A_SIZE);
  PrintData(arrayE, ARRAY_E_SIZE);
  PrintData(arrayF, ARRAY_F_SIZE);
  PrintData(arrayG, ARRAY_G_SIZE);
  dbgApp << "=============================================" << endl;
  dbgApp.flush();

  CDHandle* child_lv2=child_lv1->Create(CDPath::GetCurrentCD()->GetNodeID(), LV2, "CD2", kStrict, 0, 0, &err);
  dbgApp << "\t\tCD1 Creates Level 2 CD. # of children CDs = " << LV2 << "\n" << endl;

  CD_Begin(child_lv2);
  dbgApp << "\t\t\t\tLevel 2 CD Begin...\n" << endl;
  dbgApp.flush();

//  child_lv2->Preserve(arrayA, sizeof(arrayA), kRef, 
//                      "a_lv2", (string("arrayA-")+to_string(myRank)).c_str()); // local
  child_lv2->Preserve(arrayB, sizeof(arrayB), kRef, 
                      "b_lv2", (string("arrayB-")+to_string(myRank)).c_str()); // local
  child_lv2->Preserve(arrayF, sizeof(arrayF), kRef, 
                      "b_remote_lv2", 
                      (string("arrayA-")+to_string((myRank+1) % numProcs)).c_str()); // remote
  child_lv2->Preserve(arrayG, sizeof(arrayG), kRef, 
                      "b_remote_lv2", 
                      (string("arrayE-")+to_string((myRank+1) % numProcs)).c_str()); // remote
  child_lv2->Preserve(arrayC, sizeof(arrayC), kCopy, "arrayC");
  dbgApp << "\t\t\t\tPreserve via ref : arrayA (local), arrayB (local), arrayF (remote), arrayG (remote)" << endl;
  dbgApp << "\t\t\t\tPreserve via copy: arrayC" << endl;
  dbgApp.flush();


  if(num_reexecution = 1) {
    CorruptData(arrayE, ARRAY_E_SIZE);
    num_reexecution++;
  }
  // Level 2 Body

  child_lv2->CDAssert(CheckArray(arrayE, sizeof(arrayE)));

  CDHandle* child_lv3=child_lv2->Create(CDPath::GetCurrentCD()->GetNodeID(), LV3, "CD3", kStrict, 0, 0, &err);
  dbgApp << "\t\t\t\tCD2 Creates Level 3 CD. # of children CDs = " << LV3 << "\n" << endl;

  CD_Begin(child_lv3);
  dbgApp << "\t\t\t\t\t\tLevel 3 CD Begin...\n" << endl;
  dbgApp.flush();

  child_lv3->Preserve(arrayA, sizeof(arrayA), kRef, "child_a", (string("arrayA")+to_string(myRank)).c_str()); // local
  child_lv3->Preserve(arrayB, sizeof(arrayB), kRef, "child_b", (string("arrayB")+to_string(myRank)).c_str()); // local
  child_lv3->Preserve(arrayC, sizeof(arrayC), kRef, "child_c", "arrayC");
  child_lv3->Preserve(arrayD, sizeof(arrayD), kCopy, "arrayD");


  // Level 3 Body


  child_lv3->Detect();

  CD_Complete(child_lv3);
  dbgApp << "\t\t\t\t\t\tLevel 3 CD Complete...\n" << endl;
  child_lv3->Destroy();
  dbgApp << "\t\t\t\t\t\tLevel 3 CD Destroyed...\n" << endl;

  // Detect Error here
  child_lv2->Detect();

  CD_Complete(child_lv2);
  dbgApp << "\t\t\t\tLevel 2 CD Complete...\n" << endl;
  child_lv2->Destroy();
  dbgApp << "\t\t\t\tLevel 2 CD Destroyed...\n" << endl;

  // Detect Error here
  child_lv1->Detect();

  CD_Complete(child_lv1);
  dbgApp << "\t\tLevel 1 CD Complete...\n" << endl;
  child_lv1->Destroy();
  dbgApp << "\t\tLevel 1 CD Destroyed...\n" << endl;

  // Detect Error here
  root->Detect();

  CD_Complete(root);
  dbgApp << "Root CD Complete...\n" << endl;
  CD_Finalize(&dbgApp);
  dbgApp << "\t\tRoot CD Destroyed (Finalized) ...\n" << endl;
  dbgApp << "\n==== TestPreservationViaRefRemote Done ====\n" << endl; 
  
  // check the test result   
  return kOK; //
}





int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,  &numProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  int ret=0;
  
  dbgApp.open((string("./output/output_app_")+to_string(myRank)).c_str());
  dbgApp << "\n==== TestPreservationViaRefRemote ====\n" << endl; 
  ret = TestPreservationViaRefRemote();
  if( ret == kError ) 
    dbgApp << "Test Preservation via Reference (remote) FAILED\n" << endl;
  else 
    dbgApp << "Test Preservation via Reference (remote) PASSED\n" << endl;
  dbgApp.flush();
  dbgApp.close(); 

  MPI_Finalize();
  return 0;
} 
