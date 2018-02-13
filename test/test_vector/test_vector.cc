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
#include "cd_containers.hpp"
#include "cd.h"
using namespace cd;
using namespace std;
static int myRankID = 0;
static int numProcs = 1;
int LV0 = 1;
int LV1 = 1;
int LV2 = 1;
#define TEST_PRINT(...) if(myRankID == 0) { printf(__VA_ARGS__); }

static int count = 1;

uint64_t CompareResult(char *ori, char *prv, uint64_t size) 
{
  uint64_t ret = 0;
  for(uint64_t i=0; i<size; i++) {
    if(ori[i] != prv[i]) { ret = i; break; }
  }
  uint64_t elem_cnt = size/sizeof(int);

  if(ret != 0) {
    TEST_PRINT("\n#### Comparison Failure #############\n");
    for(uint64_t i=0; i<elem_cnt/16; i++) {
      for(uint64_t j=0; j<16; j++) {
        TEST_PRINT("%5i ", ((int*)ori)[i*16 + j]);
      }
      TEST_PRINT("\n");
    }
    TEST_PRINT("\n");
  
    TEST_PRINT("\n#### Preserved Data #################\n");
    for(uint64_t i=0; i<elem_cnt/16; i++) {
      for(uint64_t j=0; j<16; j++) {
        TEST_PRINT("%5i ", ((int*)prv)[i*16 + j]);
      }
      TEST_PRINT("\n");
    }
    TEST_PRINT("\n");
  } else {
    TEST_PRINT("\n#### Comparison Success #############\n");
  }
  return ret;
}

template <typename T>
void MutateVector(T &vec) {
  int i=0;
  int last = vec.size();
  for(; i<last; i++) {
    vec[i] += count;
  }
  auto last_elem = vec[last-1];
  last *= 2;
  for(; i<last; i++) {
    vec.push_back(last_elem + count + i);
  }

  count++;
}

void Test1(void)
{
  packer::CDPacker packer;
  uint64_t magic_offset = 0;//packer.data_->PadZeros(sizeof(packer::MagicStore));
  uint64_t orig_size = magic_offset;// + packer.data_->offset();//packer.data_->used(); // return

  

  int arrA[8] = {0, 1, 2, 3, 4, 5, 6, 7}; // reference
  cd::CDVector<int> vecA({0, 1, 2, 3, 4, 5, 6, 7});
  cd::CDVector<float> vecB({0.0, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7});
  for(int i=0; i<8; i++) {
    vecB[i] = 1.1*i;
  }

  TEST_PRINT("Original ptr: %p, vec size:%zu, vec cap:%zu, vec obj size:%zu\n", 
         vecA.data(), vecA.size(), vecA.capacity(), sizeof(vecA));
  vecA.PreserveObject(packer, kCopy, "A");
  vecB.PreserveObject(packer, kCopy, "B");
  if(cd::myTaskID == 0) {
  vecA.Print(std::cout, "[Before]"); 
  vecB.Print(std::cout, "[Before]"); 
  }
  MutateVector(vecA);
  MutateVector(vecB);
  if(cd::myTaskID == 0) {
  vecA.Print(std::cout, "[Mutated]"); 
  vecB.Print(std::cout, "[Mutated]"); 
  }
  vecA.Deserialize(packer, "A");
  vecB.Deserialize(packer, "B");
  if(cd::myTaskID == 0) {
  vecA.Print(std::cout, "[Deserialize]"); 
  vecB.Print(std::cout, "[Deserialize]");
  }




      packer.data_->PadZeros(0); 
      uint64_t table_offset = packer.AppendTable();
      uint64_t total_size_   = packer.data_->used() - orig_size;
      uint64_t table_offset_ = table_offset - (magic_offset);// + packer.data_->offset());
      uint64_t table_type_   = packer::kCDEntry;
      packer.data_->Flush();
      packer::MagicStore &magic = packer.data_->GetMagicStore();
      magic.total_size_   = total_size_, 
      magic.table_offset_ = table_offset_;
      magic.entry_type_   = table_type_;
      magic.reserved2_    = 0x12345678;
      magic.reserved_     = 0x01234567;
//      uint64_t magic_offset_in_buf = magic_offset - packer.data_->offset();
//      packer.data_->Write((char *)&magic, sizeof(packer::MagicStore), 0);//magic_offset_in_buf);
      packer.data_->FlushMagic(&magic);


}

void Test2()
{
  packer::CDPacker packer;
  uint64_t magic_offset = 0;//packer.data_->PadZeros(sizeof(packer::MagicStore));
  uint64_t orig_size = magic_offset;// + packer.data_->offset();//packer.data_->used(); // return

  

  cd::CDVector<int> vecA(32);
  cd::CDVector<float> vecB(64);
  for(int i=0; i<64; i++) {
    if(i < 32) {
      vecA[i] = i;
    }
    vecB[i] = 1.1*i;
  }

  TEST_PRINT("Original ptr: %p, vec size:%zu, vec cap:%zu, vec obj size:%zu\n", 
         vecA.data(), vecA.size(), vecA.capacity(), sizeof(vecA));
  vecA.PreserveObject(packer, kCopy, "A");
  vecB.PreserveObject(packer, kCopy, "B");
  if(cd::myTaskID == 0) {
  vecA.Print(std::cout, "[Before]"); 
  vecB.Print(std::cout, "[Before]"); 
  }
  MutateVector(vecA);
  MutateVector(vecB);
  if(cd::myTaskID == 0) {
  vecA.Print(std::cout, "[Mutated]"); 
  vecB.Print(std::cout, "[Mutated]"); 
  }
  vecA.Deserialize(packer, "A");
  vecB.Deserialize(packer, "B");
  if(cd::myTaskID == 0) {
  vecA.Print(std::cout, "[Deserialize]"); 
  vecB.Print(std::cout, "[Deserialize]");
  }




      packer.data_->PadZeros(0); 
      uint64_t table_offset = packer.AppendTable();
      uint64_t total_size_   = packer.data_->used() - orig_size;
      uint64_t table_offset_ = table_offset - (magic_offset);// + packer.data_->offset());
      uint64_t table_type_   = packer::kCDEntry;
      packer.data_->Flush();
      packer::MagicStore &magic = packer.data_->GetMagicStore();
      magic.total_size_   = total_size_, 
      magic.table_offset_ = table_offset_;
      magic.entry_type_   = table_type_;
      magic.reserved2_    = 0x12345678;
      magic.reserved_     = 0x01234567;
//      uint64_t magic_offset_in_buf = magic_offset - packer.data_->offset();
//      packer.data_->Write((char *)&magic, sizeof(packer::MagicStore), 0);//magic_offset_in_buf);
      packer.data_->FlushMagic(&magic);

}

void Test3(int elemsize, int chunksize)
{

  TEST_PRINT("\n=================== %s ====================\n\n", __func__);
  uint64_t totsize = elemsize * chunksize;
  uint32_t *dataA = new uint32_t[totsize]();
  uint32_t *checkA = new uint32_t[totsize]();
  for(uint64_t i=0; i<totsize; i++) {
    dataA[i] = i;//lrand48() % 1000;
  }

  // write input data to file for reference
  {
    char filename[64];
    sprintf(filename, "./test.ref.%d.%d.%d", elemsize, chunksize, myRankID);
    TEST_PRINT("filename:%s\n", filename);
    FILE *fp = fopen(filename, "w");
    uint32_t rowsize = totsize/16; 
    for(uint64_t i=0; i<rowsize; i++) {
      for(uint64_t j=0; j<16; j++) {
        fprintf(fp, "%u ", dataA[i*16 + j]);
//        if(myRankID == 0) fprintf(stdout, "%u ", dataA[i*16 + j]);
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
  }  

  packer::Packer<packer::CDEntry> packer(8, NULL, NULL, 4*1024*1024, kMPIFile);

  // Preservation //////////////////////////

  uint32_t *pDataA = dataA;
  uint32_t *pCheckA = checkA;
  for(int i=0; i<elemsize; i++, pDataA += chunksize, pCheckA += chunksize) {
    packer.Add((char *)pDataA, packer::CDEntry(i, chunksize*sizeof(uint32_t), 0, (char *)pCheckA));
//    if(myRankID == 0) fprintf(stdout, "prv %u, 
  }
  
  packer.data_->Flush();
  // Restoration //////////////////////////

  for(int i=0; i<elemsize; i++) {
    packer::CDEntry *entry = packer.table_->Find(i);
    packer.data_->GetData(entry->src(), entry->size(), entry->offset());
  }

  ////////////////////////////

  if(CompareResult((char *)dataA, (char *)checkA, totsize*sizeof(int)) == 0) {
    TEST_PRINT("\n\n####################### Success ## %lu ######################\n\n", 
        0UL//packer::GetFileHandle()->GetFileSize()
        ); //getchar();
  } else {
    TEST_PRINT("Failed\n"); assert(0);
  }
  //getchar();
//  table.Print();
//  data.Print();
//
//  packer.table_->PrintEntry();

  delete dataA;
  delete checkA;

}
//packer::MagicStore magic __attribute__((aligned(0x1000)));

template <typename T>
void CorruptData(cd::CDVector<T> &vec)
{
  for(int i=0; i<vec.size(); i++) {
    vec[i] -= 1;
  }
}

template <typename T>
bool TestData(cd::CDVector<T> &vec, cd::CDVector<T> &ref)
{
  for(int i=0; i<vec.size(); i++) {
    if(vec[i] != ref[i])  {
      if(myRankID == 0) {
      vec.Print(cout, "Vector Test ");
      ref.Print(cout, "Refer  Test ");
      }
      return false;
    }
  }
  return true;
}
cd::CDVector<int>    testA({3,5,0,6}); // 4 elems
cd::CDVector<int>    testB({1,2,3,4,5,6,7,8}); // 8 elems
cd::CDVector<float>  testC({5.4, 0.1, 1.0, 0.0}); // 4 elems
cd::CDVector<int>    testD({9,8,7,6,5,4,3,2,1}); // 9 elems
cd::CDVector<double> testE({2.4, 3.2, 94.2, 91.55, 0.013}); // 5 elems
cd::CDVector<long>   testF({2,4,6,8,10,12,14,16}); // 8 elems
cd::CDVector<int>    testG({3,6,9,12,15,18,21,24,27,30}); // 10 elems

#define PrintAll(str) { \
    arrayA.Print(cd::cddbg, str); \
    arrayB.Print(cd::cddbg, str); \
    arrayC.Print(cd::cddbg, str); \
    arrayD.Print(cd::cddbg, str); \
    arrayE.Print(cd::cddbg, str); \
    arrayF.Print(cd::cddbg, str); \
    arrayG.Print(cd::cddbg, str); \
}
// Test basic preservation scheme.
int TestCD(void)
{
  cd::CDVector<int>    arrayA({3,5,0,6}); // 4 elems
  cd::CDVector<int>    arrayB({1,2,3,4,5,6,7,8}); // 8 elems
  cd::CDVector<float>  arrayC({5.4, 0.1, 1.0, 0.0}); // 4 elems
  cd::CDVector<int>    arrayD({9,8,7,6,5,4,3,2,1}); // 9 elems
  cd::CDVector<double> arrayE({2.4, 3.2, 94.2, 91.55, 0.013}); // 5 elems
  cd::CDVector<long>   arrayF({2,4,6,8,10,12,14,16}); // 8 elems
  cd::CDVector<int>    arrayG({3,6,9,12,15,18,21,24,27,30}); // 10 elems
  int test_results[8] = {0,};
  int test_result = 0;
  int num_reexecution = 0;
  int num_reexecution_lv1 = 0;
  int num_reexecution_lv2 = 0;


  char arrAstr[32];
  char arrBstr[32];
  char arrCstr[32];
  char arrDstr[32];
  char arrEstr[32];
  char arrFstr[32];
  char arrGstr[32];
  sprintf(arrAstr, "arrayA-%d", myRankID);
  sprintf(arrBstr, "arrayB-%d", myRankID);
  sprintf(arrCstr, "arrayC-%d", myRankID);
  sprintf(arrDstr, "arrayD-%d", myRankID);
  sprintf(arrEstr, "arrayE-%d", myRankID);
  sprintf(arrFstr, "arrayF-%d", myRankID);
  sprintf(arrGstr, "arrayG-%d", myRankID);
	CDHandle *root = CD_Init(numProcs, myRankID, kPFS);
  CD_Begin(root, "Root"); 
  if(myRankID == 0) cout << "Level 0 CD Begin..." << endl;
  PrintAll((num_reexecution == 0)? "Before Preserve ": "Before Restore ");
  root->Preserve(arrayA, kCopy, arrAstr); 
  root->Preserve(arrayB, kCopy, arrBstr); 
  root->Preserve(arrayC, kCopy, arrCstr); 
  root->Preserve(arrayD, kCopy, arrDstr); 
  root->Preserve(arrayE, kCopy, arrEstr);
  root->Preserve(arrayF, kCopy, arrFstr); 
  root->Preserve(arrayG, kCopy, arrGstr);
  if(num_reexecution != 0) {
    if( myRankID == 0) cout << "Test Level 0 .." << endl;
    assert(TestData(arrayA, testA));
    assert(TestData(arrayB, testB));
    assert(TestData(arrayC, testC));
    assert(TestData(arrayD, testD));
    assert(TestData(arrayE, testE));
    assert(TestData(arrayF, testF));
    assert(TestData(arrayG, testG));
  }
  PrintAll((num_reexecution == 0)? "After Preserve ": "After  Restore ");

  if (1) {
    CDHandle* child_lv1=root->Create(LV1, "CD1", kStrict);
    cd::cddbg << "Root Creates Level 1 CD. # of children CDs = " << LV1 << endl;
  
    CD_Begin(child_lv1, "CD LV 1");
    cd::cddbg << "\t\tLevel 1 CD Begin..." << endl;
    if( myRankID == 0) cout << "\tLevel 1 CD Begin..." << endl;
  
    CDPrvType prv_type_1 = kRef;
    PrintAll((num_reexecution_lv1 == 0)? "  LV1 Before Preserve ": "  LV1 Before Restore ");
    child_lv1->Preserve(arrayA, prv_type_1, "refA-Lv1", arrAstr); 
    child_lv1->Preserve(arrayB, prv_type_1, "refB-Lv1", arrBstr); 
    child_lv1->Preserve(arrayC, prv_type_1, "refC-Lv1", arrCstr);
    child_lv1->Preserve(arrayE, prv_type_1, "refE-Lv1", arrEstr);
    child_lv1->Preserve(arrayF, prv_type_1, "refF-Lv1", arrEstr);
    child_lv1->Preserve(arrayG, prv_type_1, "refG-Lv1", arrEstr);
    PrintAll((num_reexecution_lv1 == 0)? "  LV1 After Preserve ": "  LV1 After  Restore ");
    if(num_reexecution_lv1 != 0 || num_reexecution_lv2 != 0) {
      if( myRankID == 0) cout << "\tTest Level 1 .." << endl;
      assert(TestData(arrayA, testA));
      assert(TestData(arrayB, testB));
      assert(TestData(arrayC, testC));
      assert(TestData(arrayD, testD));
      assert(TestData(arrayE, testE));
      assert(TestData(arrayF, testF));
      assert(TestData(arrayG, testG));
    }
    cd::cddbg << "\t\tPreserve via copy: arrayA (Share), arrayB (Share), arrayE (Share)\n" << endl;
  
    cd::cddbg.flush();
    if (1) { // Level 2
      CDHandle* child_lv2=child_lv1->Create(LV2, "CD2");
      cd::cddbg << "\t\tCD1 Creates Level 2 CD. # of children CDs = " << LV2 << "\n" << endl;
      for(int ii=0; ii<6; ii++) { 
        CD_Begin(child_lv2, "CD LV 2");
        if( myRankID == 0) cout << "\t\tLevel 2 CD Begin..." << endl;
        cd::cddbg << "\t\t\t\tLevel 2 CD Begin...\n" << ii << endl;
        cd::cddbg.flush();
      
        PrintAll((num_reexecution_lv2 == 0)? "    Lv2 Before Preserve ": "    Lv2 Before  Restore ");
        child_lv2->Preserve(arrayA, kCopy, "arrA-Lv2", arrAstr); 
        child_lv2->Preserve(arrayB, kCopy, "arrB-Lv2", arrBstr);
        child_lv2->Preserve(arrayF, kRef, "arrF-Lv2", arrFstr);
        child_lv2->Preserve(arrayG, kRef, "arrG-Lv2", arrGstr);
        child_lv2->Preserve(arrayC, kCopy, arrCstr);
        PrintAll((num_reexecution_lv2 == 0)? "    Lv2 After Preserve ": "    Lv2 After  Restore ");
        cd::cddbg << "\t\t\t\tPreserve via ref : arrayA (local), arrayB (local), arrayF (remote), arrayG (remote)" << endl;
        cd::cddbg << "\t\t\t\tPreserve via copy: arrayC" << endl;
        cd::cddbg.flush();
        if(num_reexecution_lv2 != 0) {
          if( myRankID == 0) cout << "\t\tTest Level 2 .." << endl;
          assert(TestData(arrayA, testA));     
          assert(TestData(arrayB, testB));     
          assert(TestData(arrayF, testF));     
          assert(TestData(arrayG, testG));     
          assert(TestData(arrayC, testC));     
        }
      
      
        if(num_reexecution_lv2 == 0 && ii == 4) {
          CorruptData(arrayA);
          CorruptData(arrayF);
          CorruptData(arrayG);
          CorruptData(arrayC);
          num_reexecution_lv2++;
          child_lv2->CDAssert(false);
          //child_lv1->CDAssert(false);
        }
        // Level 2 Body
      
      
        // Detect Error here
        child_lv2->Detect();
      
        CD_Complete(child_lv2);
        cd::cddbg << "\t\t\t\tLevel 2 CD Complete... "<< ii <<  endl;
      }
      child_lv2->Destroy();
      cd::cddbg << "\t\t\t\tLevel 2 CD Destroyed...\n" << endl;
    }
  
    cd::cddbg.flush(); 
    // Detect Error here
    child_lv1->Detect();
    if(num_reexecution_lv1 == 0) {
      CorruptData(arrayA);
      CorruptData(arrayB);
      CorruptData(arrayC);
      num_reexecution_lv1++;
      child_lv1->CDAssert(false);
    }
  
    CD_Complete(child_lv1);
    cd::cddbg << "\t\tLevel 1 CD Complete...\n" << endl;
    child_lv1->Destroy();
    cd::cddbg << "\t\tLevel 1 CD Destroyed...\n" << endl;
  }

  // Corrupt array arrayA and arrayB
  if(num_reexecution == 0) {
    CorruptData(arrayA);
    CorruptData(arrayB);
    CorruptData(arrayC);
    num_reexecution++;
    root->CDAssert(false);
  }



  // Detect Error here
  root->Detect();

  //cout << "Root CD Complete...\n" << endl;
  CD_Complete(root);
  cd::cddbg << "Root CD Complete...\n" << endl;
  cd::cddbg << "\t\tRoot CD Destroyed (Finalized) ...\n" << endl;
  cd::cddbg << "\n==== TestPreservationViaRefRemote Done ====\n" << endl; 
  cd::cddbg.flush(); 
  CD_Finalize();
  // check the test result   
  return kOK; //
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv) ;
  MPI_Comm_rank(MPI_COMM_WORLD, &myRankID);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &cd::myTaskID);
  MPI_Comm_size(MPI_COMM_WORLD, &cd::totalTaskSize);
  bool test_vector_only = false;
  if(test_vector_only) {
    for(int i=0; i<4; i++) {
      printf("Working on %d\n", i);
      Test3(16*i, 32*i);
    }
  } else {
    TestCD();
  }
  MPI_Finalize();
  return 0;
}

