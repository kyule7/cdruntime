#include "packer_prof.h"
#include "packer.h"
#include "base_store.h"
#include <stdio.h>
#include <string.h>

using namespace packer;

bool packer::isHead = true;
Time packer::time_write("time_write"); 
Time packer::time_read("time_read"); 
Time packer::time_posix_write("posix_write"); 
Time packer::time_posix_read("posix_read"); 
Time packer::time_posix_seek("posix_seek"); 
Time packer::time_mpiio_write("mpiio_write"); 
Time packer::time_mpiio_read("mpiio_read"); 
Time packer::time_mpiio_seek("mpiio_seek"); 

Time::Time(const char *str) { 
  Init(str); 
}

Time::~Time(void) {
  Print();
}

void Time::Init(const char *str) 
{ 
  elapsed = 0.0; size = 0; count = 0; 
  if(str != NULL) {
    strcpy(name, str); 
  } else {
    strcpy(name, "NoName");
  }
}

void Time::Print(void)
{
  if(isHead && count > 0) { 
    printf("[%16s] elapsed:%8.3lf, BW:%8.3lf, size:%12lu, count:%6lu\n", 
      name, elapsed, GetBW(), size, count); 
  }
}

void packer::SetHead(bool head)
{
  isHead = head;
}
