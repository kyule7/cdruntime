#include "packer.hpp"
#include "cd_packer.hpp"
#include "test_common.h"
#include <string.h>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <sys/time.h>
using namespace packer;
struct timeval ttime;
//uint64_t CompareResult(char *ori, char *prv, uint64_t size) {
//  for(uint64_t i=0; i<size; i++) {
//    if(ori[i] != prv[i]) return i;
//  }
//  return 0;
//}
FILE *dbgfp=stdout;
char *ArrayTest(int elemsize, int chunksize) 
{
  uint64_t totsize = elemsize * chunksize;
  int *dataA = new int[totsize];
  int *testA = new int[totsize];
  for(uint32_t i=0; i<totsize; i++) {
    dataA[i] = lrand48() % 100;
  }

  CDPacker packer;
  ///Packer<CDEntry> packer;

  int *dataAtmp = dataA;
  for(int i=0; i<elemsize; i++) {
    packer.Add((char *)dataAtmp, CDEntry(i+1, chunksize * sizeof(int), 0, (char *)dataAtmp));
    dataAtmp+=chunksize;

//    if(err == 0) { assert(0); }
//    printf("%p + %lu\n", dataAtmp, offset); 
//    if(i % 32 == 0) //getchar();
  }
  PrintArray((char *)dataA, totsize * sizeof(int)); //getchar(); 
  memcpy(testA, dataA, totsize * sizeof(int));

  memset(dataA, 0, totsize * sizeof(int));
  for(int i=0; i<elemsize; i++) {
    packer.Restore(i+1);
  }


  if(CompareResult((char *)dataA, (char *)testA, totsize*sizeof(int)) == 0) {
    printf("Success\n");
  } else {
    printf("Failed\n"); assert(0);
  }


  








  if(CompareResult((char *)dataA, packer.data_->GetPtr(), totsize*sizeof(int)) == 0) {
    printf("Success\n");
  } else {
    printf("Failed\n"); assert(0);
//    int stride = totsize/16;
//    for(int i=0; i<16; i++) {
//      for(int j=0; j<stride; j++) {
//        printf("%3d", dataA[i*stride + j]);
//      }
//      printf("\n");
//    }
//    printf("\n\n");
//    int *testptr = (int*)packer.data_->GetPtr();
//    for(int i=0; i<16; i++) {
//      for(int j=0; j<stride; j++) {
//        printf("%3d", testptr[i*stride + j]);
//      }
//      printf("\n");
//    }
//    printf("\n");
  }
  packer.table_->Print();
  packer.data_->Print();
  packer.table_->PrintEntry();

  uint64_t ret_size = 0;
  char *serialized = packer.GetTotalData(ret_size);
  
  printf("%lu != %lu\n", ret_size, packer.data_->tail() + packer.table_->usedsize() + sizeof(MagicStore));
  if(ret_size != packer.data_->tail() + packer.table_->usedsize() + sizeof(MagicStore)) {
    printf("%lu != %lu\n", ret_size, packer.data_->used() + packer.table_->usedsize() + sizeof(MagicStore));
    assert(0);
  } else {

  }


//  char *ret_ptr = (char *)malloc(ret_size);
//  memcpy(ret_ptr, serialized, ret_size);
  uint32_t *test_ptr = (uint32_t *)serialized;
  printf("[%s] ret:%u %u %u\n", __func__, *test_ptr, *(test_ptr+1), *(test_ptr+2)); //getchar();
  delete dataA;
  delete testA;
  return serialized;
}

void TestUnpacker(char *chunk) 
{
  printf("#######################################################\n"
         "## %s \n"
         "#######################################################\n", __func__);
  printf("chunk:%p\n", chunk); 
  Unpacker unpacker(chunk);

  uint64_t ret_id, ret_size, ret_offset;
  printf("before loop:%p\n", chunk); 
  //while(unpacker.GetNext(chunk, ret_id, ret_size, ret_offset) != NULL) {
  for(uint64_t i=0; i<10; i++){
    printf("loop:%lu\n", i); 
    
    unpacker.GetNext(chunk, ret_id, ret_size, ret_offset);
    printf("%lu %lu %lu\n", ret_id, ret_size, ret_offset);
  }
}
  
void TestUnpackerFile(FILE *fp)
{
  printf("#######################################################\n"
         "## %s \n"
         "#######################################################\n", __func__);
  FileUnpacker unpacker(fp);

  uint64_t ret_id, ret_size, ret_offset;
  for(uint64_t i=0; i<10; i++) {
    printf("loop:%lu\n", i); 
    
    unpacker.GetNext(ret_id, ret_size, ret_offset);
    printf("%lu %lu %lu\n", ret_id, ret_size, ret_offset);
  }
}

int main() {
  gettimeofday(&ttime,NULL);
  srand48(ttime.tv_usec);
  int elemsize = 1030;
  int chunksize = 32;
  char *chunk = ArrayTest(elemsize, chunksize);
//    Packer packer;;
//  UserObj1 obj1;
//  Obj1Test(elemsize);
//  for(int i=1; i<32; i<<=1) {
//    for(int j=1; j<256; j<<=1) {
//      ArrayTest(elemsize*i, chunksize*j);
//    }
//  }
//


//    FILE *fp = fopen("./test.txt", "w+");
//  
//    fwrite(chunk, sizeof(char), chunksize, fp);
//    fflush(fp); 
//  //  TestUnpacker(chunk);
//  
//    TestUnpackerFile(fp);

  return 0;
}
