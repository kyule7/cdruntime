#include "packer_prof.h"
#include "packer.h"
#include "base_store.h"
#include <string.h>

using namespace packer;

std::vector<packer::Time *> packer::prof_list;
//packer::ProfList packer::prof_list;
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
  prof_list.push_back(this);
}

Time::~Time(void) {
  Print();
}

void Time::Init(const char *str) 
{ 
  elapsed = 0.000000000001; size = 0; count = 0; 
  if(str != NULL) {
    strcpy(name, str); 
  } else {
    strcpy(name, "NoName");
  }
}

void Time::Print(FILE *buf)
{
  if(isHead && count > 0) { 
    fprintf(buf, "[%16s] elapsed:%8.3lf, BW:%8.3lf, size:%12lu, count:%6lu\n", 
      name, elapsed, GetBW(), size, count); 
  }
}

void Time::PrintJSON(FILE *buf)
{
  fprintf(buf, "    \"%s\" : {\n", name);
  fprintf(buf, "      \"volume\" : %lu,\n", size);
  fprintf(buf, "      \"count\"  : %lu,\n", count);
  fprintf(buf, "      \"bandwidth\" : %lf,\n", GetBW());
  fprintf(buf, "      \"elapsed\"   : %lf,\n", elapsed);
  fprintf(buf, "    },\n");
}

void packer::SetHead(bool head)
{
  isHead = head;
}

void PrintPackerProf(FILE *buf)
{
  for(auto it=prof_list.begin(); it!=prof_list.end(); ++it) { 
    (*it)->PrintJSON(buf); } 
}
