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
FILE *dbgfp=stdout;


uint64_t PreserveObject(DataStore *data, int *datap, int elemsize, int chunksize) {
  CDPacker nested_packer(NULL, data);
  uint64_t orig_size = nested_packer.data_->used();
  uint64_t magic_offset = 0;
  bool opt = true;
  if(opt == true) {
    //assert(0); // FIXME: need to pad zero, then call FakeWrite
    
    magic_offset = nested_packer.data_->PadZeros(sizeof(MagicStore));
//    magic_offset = nested_packer.data_->FakeWrite(sizeof(MagicStore));

  } else {
    nested_packer.data_->SetFileType(kVolatile);
    MagicStore magic;
    magic_offset = nested_packer.data_->WriteCacheMode((char *)&magic, sizeof(MagicStore));
//  uint64_t magic_offset = nested_packer.data_->WriteFlushMode((char *)&magic, sizeof(MagicStore));
  //  printf("after flush magic\n"); getchar();
  }


  for(int i=0; i<elemsize; i++) {
    nested_packer.Add((char *)datap, CDEntry(i+1, chunksize * sizeof(int), 0, (char *)datap));
    datap+=chunksize;
  }
  uint64_t table_offset =  nested_packer.AppendTable();
  uint64_t total_size = nested_packer.data_->used() - orig_size;

  if(opt == true) {
    MagicStore magic(total_size, 
                     table_offset - magic_offset - nested_packer.data_->offset(), 
                     kCDEntry);
    nested_packer.data_->Write((char *)&magic, sizeof(MagicStore), magic_offset);
  } else {
    // update magic
    MagicStore &magic_in_place = nested_packer.data_->GetMagicStore(magic_offset);
    magic_in_place.total_size_   = total_size;
    magic_in_place.table_offset_ = table_offset - magic_offset;
    magic_in_place.entry_type_   = kCDEntry;
    nested_packer.data_->SetFileType(kPosixFile);

    // test //////////////////
    printf("[offset:%lu, %zu, orig:%lx after:%lx] magic: %lu %lu %u, data:%lu, table:%lu\n", 
              magic_offset, sizeof(MagicStore), orig_size, nested_packer.data_->used(),
              magic_in_place.total_size_  ,
              magic_in_place.table_offset_,
              magic_in_place.entry_type_,
              chunksize*elemsize*sizeof(int) + sizeof(MagicStore), elemsize * sizeof(CDEntry) );  
    //getchar();
  }

  
  MagicStore magictest = *reinterpret_cast<MagicStore *>(
      nested_packer.data_->GetPtr() + (magic_offset - nested_packer.data_->offset()) % nested_packer.data_->size());
  printf("[offset test :%lu, %zu] magic: %lu %lu %u\n", magic_offset, sizeof(MagicStore),
            magictest.total_size_  ,
            magictest.table_offset_,
            magictest.entry_type_);
//  long int *pmagic = (long int *)(nested_packer.data_->GetPtr() + magic_offset % nested_packer.data_->size());
//  for(uint32_t i=0; i <64/16; i++) {
//    for(uint32_t j=0; j<16; j++) {
//      printf("%5lu ", *(pmagic + i*16 + j));
//    }
//    printf("\n");
//  } 
//  getchar();
  return table_offset;
}

//uint64_t Deserialize(int *data, char *serialized)
//{
//  MagicStore *magic = reinterpret_cast<MagicStore *>(serialized);
//  //uint64_t *packed_data = reinterpret_cast<uint64_t *>((char *)ret + sizeof(MagicStore));
//  char *packed_data = reinterpret_cast<char *>(serialized);
////  int *packed_data = reinterpret_cast<int *>(ret);
////  printf("#### check read magicstore###\n");
////  for(int i=0; i<128/16; i++) {
////    for(int j=0; j<16; j++) {
////      printf("%4d ", *(packed_data + i*16 + j));
////    }
////    printf("\n");
////  } 
//  printf("#### check read magicstore###\n");
//  printf("Read id:%lu, attr:%lx chunk: %lu at %lu, "
//         "Read MagicStore: size:%lu, tableoffset:%lu, entry_type:%u\n", 
//          entry.id_, entry.attr(), entry.size(), entry.offset(), 
//          magic->total_size_, magic->table_offset_, magic->entry_type_);
//  CDEntry *pEntry = reinterpret_cast<CDEntry *>(serialized + magic->table_offset_);
//  for(int i=0; i<(magic->total_size_ - magic->table_offset_)/sizeof(CDEntry); i++) {
//    memcpy(pEntry->src_, pEntry->size(), pEntry->offset_, 
//  }
//  getchar();
//}

char *TestNestedPacker(int elemsize, int chunksize) 
{
  uint64_t totsize = elemsize * chunksize;
  int *dataA = new int[totsize];
  int *testA = new int[totsize];
  for(uint32_t i=0; i<totsize; i++) {
    dataA[i] = i;//lrand48() % 100;
  }
  memcpy(testA, dataA, totsize * sizeof(int));
//////////////////////////////////////////////////////
  int elemsizeB = 24;
  int chunksizeB = 1280;
  uint64_t totsizeB = elemsizeB * chunksizeB;
  int *dataB = new int[totsizeB];
  int *testB = new int[totsizeB];
  for(uint32_t i=0; i<totsizeB; i++) {
    dataB[i] = i + 1000;//lrand48() % 100;
  }
  for(uint32_t i=0; i <totsizeB/16; i++) {
    for(uint32_t j=0; j<16; j++) {
      printf("%4d ", dataB[i*16 + j]);
    }
    printf("\n");
  } 
  memcpy(testB, dataB, totsizeB * sizeof(int));
//////////////////////////////////////////////////////
  printf("totsize:%lu, %lu\n", totsize * sizeof(int), totsizeB * sizeof(int));

  CDPacker packer;

  int *dataAtmp = dataA;
  int first = elemsize / 2;
  for(int i=0; i<first; i++) {
//    if(i == first - 1) { printf("first i:%d\n", i); getchar(); }
//    if(i == first) { printf("first i:%d\n", i); getchar(); }
    packer.Add((char *)dataAtmp, CDEntry(i+1, chunksize * sizeof(int), 0, (char *)dataAtmp));
    dataAtmp+=chunksize;

//    if(err == 0) { assert(0); }
//    printf("%p + %lu\n", dataAtmp, offset); 
//    if(i % 32 == 0) //getchar();
  }

//  int second = elemsize - first;
//  printf("================PrintArray====================\n");
//  PrintArray((char *)dataA, first *chunksize* sizeof(int)); //getchar(); 
//  PrintArray((char *)(dataA + first*chunksize), second *chunksize* sizeof(int)); //getchar(); 
//  printf("==============================================\n");

  {
    uint64_t packed_offset = packer.data_->used();
    uint64_t table_offset = PreserveObject(packer.data_, dataB, elemsizeB, chunksizeB);
    uint64_t packed_size = packer.data_->used() - packed_offset;
//    uint64_t table_offset_in_packed = packer.data_->used() - table_offset;
    uint64_t attr = Attr::knested | Attr::ktable;
    uint64_t id = first+1;
    CDEntry *entry = packer.table_->InsertEntry(CDEntry(id, attr, packed_size, packed_offset, (char *)table_offset));
    printf("[nested ID:%lu] attr:%lx, size:%lu==%lu, "
           "packed offset:%lx table_offset:%lx\n", 
        id, entry->size_.code_ >> 48, entry->size(), packed_size, packed_offset, table_offset); //getchar();

  }

  packer.data_->Flush();
//  printf("after pack offset (sync):%lx~%lx\n", 
//      packer.data_->offset() + packer.data_->head(), packer.data_->used()); getchar();
  for(int i=first+1; i<elemsize+1; i++) {
//    if(i == first+1) { printf("second i:%d\n", i); getchar(); }
    //uint64_t offset = 0;
    packer.Add((char *)dataAtmp, CDEntry(i+1, chunksize * sizeof(int), 0, (char *)dataAtmp));
//    printf("packer offset after nested, [%d]: %d %lu %lx\n", i, *dataAtmp, chunksize*sizeof(int), offset); getchar();
    dataAtmp+=chunksize;
//    if(i == 700)
//      packer.data_->Flush();
  }

  memset(dataA, 0, totsize * sizeof(int));
  memset(dataB, 0, totsizeB * sizeof(int));

  packer.data_->Flush();
  // +1 is for packer
  for(int i=0; i<elemsize+1; i++) {
    packer.Restore(i+1);
//    char *ret = packer.Restore(i+1);
//    if(ret != NULL) {
//      Deserialize(dataB, ret);
//    }

  }

  printf("================PrintArrayB====================\n");
  PrintArray((char *)dataB, totsizeB* sizeof(int)); //getchar(); 
  printf("==============================================\n");


  if(CompareResult((char *)dataA, (char *)testA, totsize*sizeof(int)) == 0) {
    printf("Success\n");
  } else {
    printf("Failed\n"); assert(0);
  }

  if(CompareResult((char *)dataB, (char *)testB, totsizeB*sizeof(int)) == 0) {
    printf("Nested Success\n");
  } else {
    printf("Nested Failed\n"); assert(0);
  }
  printf("#####################################\n");
//  packer.table_->Print();
//  packer.data_->Print();
//  packer.table_->PrintEntry();

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
/*
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
*/
int main() {
  gettimeofday(&ttime,NULL);
  srand48(ttime.tv_usec);
  int elemsize = 10000;
  int chunksize = 640;
  //char *chunk = ArrayTest(elemsize, chunksize);
  TestNestedPacker(elemsize, chunksize);
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
