#include "table_store.hpp"
#include "data_store.h"
#include "cd_file_handle.h"
#include <string.h>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <sys/time.h>
using namespace packer;
struct timeval mytime;

class MyStore : public TableStore<CDEntry> {
  public:
    virtual void Print(void)
    {
      MYDBG("[MyTable] %lu/%lu, grow:%lu, alloc:%u\n", tail_*sizeof(CDEntry), size_, grow_unit_, allocated_);
      //getchar();
    }
    void PrintEntry(uint64_t print_upto=0)
    {
      printf("[MyTable PrintEntry]\n"); //getchar();
      if(print_upto == 0) print_upto = tail_;
      for(uint64_t i=0; i<print_upto; i++) {
        ptr_[i].Print();
      }
    }
};

uint64_t CompareResult(char *ori, char *prv, uint64_t size) {
  uint64_t ret = 0;
  for(uint64_t i=0; i<size; i++) {
    if(ori[i] != prv[i]) { ret = i; break; }
  }
  uint64_t intsize = size/sizeof(int);

  if(ret != 0) {
  for(uint64_t i=0; i<intsize/16; i++) {
    for(uint64_t j=0; j<16; j++) {
      printf("%5i ", ((int*)ori)[i*16 + j]);
    }
    printf("\n");
  }
  printf("\n");

  printf("\n#####################################\n");
  for(uint64_t i=0; i<intsize/16; i++) {
    for(uint64_t j=0; j<16; j++) {
      printf("%5i ", ((int*)prv)[i*16 + j]);
    }
    printf("\n");
  }
  printf("\n");
  }
  return ret;
}

struct UserObj1 {
  char c[64];
  int  ii[128];
  double d[32];
  float  f[64];
  UserObj1(void) {
    for(uint64_t i=0; i<64; i++) {
      c[i] = (char)lrand48();
    }
    for(uint64_t i=0; i<128; i++) {
      ii[i] = (int)lrand48();
    }
    for(uint64_t i=0; i<32; i++) {
      d[i] = drand48();
    }
    for(uint64_t i=0; i<64; i++) {
      f[i] = drand48();
    }
    printf("%zu %zu %zu %zu\n", sizeof(c), sizeof(ii), sizeof(d), sizeof(f));
  }

};

void Obj1Test(int elemsize) 
{
  uint64_t totsize = elemsize * sizeof(UserObj1);
  UserObj1 *dataA = new UserObj1[elemsize];
  TableStore<CDEntry> table;
  DataStore data;
  UserObj1 *dataAtmp = dataA;
  for(int i=0; i<elemsize; i++) {
    uint64_t err;
    {
    uint64_t chunksize = sizeof(dataAtmp[i].c);
    CDEntry entry(i, chunksize, 0, (char *)(dataAtmp[i].c));
    uint64_t offset = data.Write((char *)(dataAtmp[i].c), chunksize);
    err = table.Insert(entry.SetOffset(offset));
    }
    {
    uint64_t chunksize = sizeof(dataAtmp[i].ii);
    CDEntry entry(i, chunksize, 0, (char *)(dataAtmp[i].ii));
    uint64_t offset = data.Write((char *)(dataAtmp[i].ii), chunksize);
    err = table.Insert(entry.SetOffset(offset));
    }
    {
    uint64_t chunksize = sizeof(dataAtmp[i].d);
    CDEntry entry(i, chunksize, 0, (char *)(dataAtmp[i].d));
    uint64_t offset = data.Write((char *)(dataAtmp[i].d), chunksize);
    err = table.Insert(entry.SetOffset(offset));
    }
    {
    uint64_t chunksize = sizeof(dataAtmp[i].f);
    CDEntry entry(i, chunksize, 0, (char *)(dataAtmp[i].f));
    uint64_t offset = data.Write((char *)(dataAtmp[i].f), chunksize);
    err = table.Insert(entry.SetOffset(offset));
    }
    if(err == 0) { assert(0); }
//    printf("%p + %lu\n", dataAtmp, offset); 
//    if(i % 32 == 0) getchar();
    //dataAtmp+=chunksize;
  }

  if(CompareResult((char *)dataA, data.GetPtr(), totsize) == 0) {
    printf("Success\n");
  } else {
    printf("Failed\n"); assert(0);
  }
  table.Print();
  data.Print();
  table.PrintEntry();
  delete dataA;
}

void ArrayTest(int elemsize, int chunksize) {
  uint64_t totsize = elemsize * chunksize;
  int *dataA = new int[totsize];
  for(uint64_t i=0; i<totsize; i++) {
    dataA[i] = lrand48();
  }

 // TableStore<CDEntry> table;
  //MyStore table;
  TableStore<BaseEntry> table;
  DataStore data;
  int *dataAtmp = dataA;
  for(int i=0; i<elemsize; i++) {
    //CDEntry entry(i, chunksize * sizeof(int), (char *)dataAtmp);
    BaseEntry entry(i, chunksize * sizeof(int), 0);
    uint64_t offset = data.Write((char *)dataAtmp, chunksize*sizeof(int));
   // printf("Test#####333\n");
    uint64_t err = table.Insert(entry.SetOffset(offset));

    if(err == 0) { assert(0); }
//    printf("%p + %lu\n", dataAtmp, offset); 
//    if(i % 32 == 0) getchar();
    dataAtmp+=chunksize;
  }

  if(CompareResult((char *)dataA, data.GetPtr(), totsize*sizeof(int)) == 0) {
    printf("Success\n");
  } else {
    printf("Failed\n"); assert(0);
  }
  table.Print();
  data.Print();
  table.PrintEntry();
  delete dataA;
}

void BoundedBufferTest(int elemsize, int chunksize) {
  printf("\n\n\n=================== %s ====================\n\n", __func__);
  uint64_t totsize = elemsize * chunksize;
  uint32_t *dataA = new uint32_t[totsize];
  for(uint64_t i=0; i<totsize; i++) {
    dataA[i] = lrand48() % 1000;
  }

  {
    char filename[64];
    sprintf(filename, "test.ref.%d.%d", elemsize, chunksize);
    FILE *fp = fopen(filename, "w");
    uint32_t rowsize = totsize/16; 
    for(uint64_t i=0; i<rowsize; i++) {
      for(uint64_t j=0; j<16; j++) {
        fprintf(fp, "%u ", dataA[i*16 + j]);
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
  }  

 // TableStore<CDEntry> table;
  //MyStore table;
  TableStore<BaseEntry> table;
  DataStore data;
//  data.SetMode(kBoundedMode);
//  data.SetFileType(kPosixFile);
//  FileHandle fh;
//  char filepath[64];
 // sprintf(filepath, "./bounded.%d.%d", elemsize, chunksize);
//  GetFileHandle()->SetFilepath(filepath);
//  printf("filepath:%s\n", filepath); //getchar(); 
  uint32_t *dataAtmp = dataA;
  for(int i=0; i<elemsize; i++) {
    //CDEntry entry(i, chunksize * sizeof(int), (char *)dataAtmp);
    BaseEntry entry(i, chunksize * sizeof(int), 0);
    uint64_t offset = data.Write((char *)dataAtmp, chunksize*sizeof(int));
    uint64_t err = table.Insert(entry.SetOffset(offset));

    if(err == 0) { assert(0); }
    printf("%p + %lu\n", dataAtmp, offset); 
//    if(i % 32 == 0) //getchar();
    dataAtmp+=chunksize;
  }

  FileHandle *fh = GetFileHandle();
  assert(fh != NULL);
  uint64_t read_size=0;
  //char *preserved = (char *)malloc(totsize * sizeof(int));
  //data.ReadAll(preserved);
  char *preserved = data.ReadAll(read_size);
  data.Flush();
  if(totsize*sizeof(int) != read_size) {
    printf("%lu != %lu (read)\n", totsize*sizeof(int), read_size);
  }
  //if(CompareResult((char *)dataA, data.GetPtr(), totsize*sizeof(int)) == 0) {
  if(CompareResult((char *)dataA, preserved, totsize*sizeof(int)) == 0) {
    printf("\n\n####################### Success ## %lu ######################\n\n", GetFileHandle()->GetFileSize()); //getchar();
  } else {
    printf("Failed\n"); assert(0);
  }
  //getchar();
  table.Print();
  data.Print();
  table.PrintEntry();

  delete dataA;
  free(preserved);
}



int main() {
  gettimeofday(&mytime,NULL);
  srand48(mytime.tv_usec);
  int elemsize = 16;
  int chunksize = 32;
//  UserObj1 obj1;
//  Obj1Test(elemsize);
  for(int i=1; i<32; i<<=1) {
    for(int j=1; j<256; j<<=1) {
      //ArrayTest(elemsize*i, chunksize*j);
      BoundedBufferTest(elemsize*i, chunksize*j);
    }
  }
  return 0;
}
