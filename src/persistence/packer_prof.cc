#include "packer_prof.h"
#include "packer.h"
#include "base_store.h"
#include <string.h>
using namespace packer;

bool packer::gathered_bw = false;
std::vector<packer::Time *> packer::prof_list;
//packer::ProfList packer::prof_list;
Time packer::time_copy("time_copy"); 
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

void Time::UpdateBW(double avg, double std, double min, double max) 
{
  bw_avg = avg;
  bw_std = std;
  bw_min = min;
  bw_max = max;
}

void Time::Init(const char *str) 
{ 
  elapsed = 0.000000000001; size = 0; count = 0;
  UpdateBW(0,0,0,0);
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
  fprintf(buf, "      \"bandwidth avg\" : %lf,\n", bw_avg);
  fprintf(buf, "      \"bandwidth std\" : %lf,\n", bw_std);
  fprintf(buf, "      \"bandwidth min\" : %lf,\n", bw_min);
  fprintf(buf, "      \"bandwidth max\" : %lf,\n", bw_max);
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

//void packer::GatherBW(void) 
//{
//  const int prof_elems = 9;
//  double sendbuf[9] = { 
//                         packer::time_copy.GetBW(), 
//                         packer::time_write.GetBW(), 
//                         packer::time_read.GetBW(), 
//                         packer::time_posix_write.GetBW(), 
//                         packer::time_posix_read.GetBW(), 
//                         packer::time_posix_seek.GetBW(), 
//                         packer::time_mpiio_write.GetBW(), 
//                         packer::time_mpiio_read.GetBW(), 
//                         packer::time_mpiio_seek.GetBW()
//  };
//  double recvbufmax[prof_elems] = { 0, 0, 0, 0 };
//  double recvbufmin[prof_elems] = { 0, 0, 0, 0 };
//  double recvbufavg[prof_elems] = { 0, 0, 0, 0 };
//  double recvbufstd[prof_elems] = { 0, 0, 0, 0 };
//  double sendbufstd[prof_elems];
//  for(int i=0; i<prof_elems; i++) { sendbufstd[i] = sendbuf[i] * sendbuf[i]; }
//  MPI_Reduce(sendbuf, recvbufmax, prof_elems, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
//  MPI_Reduce(sendbuf, recvbufmin, prof_elems, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
//  MPI_Reduce(sendbuf, recvbufavg, prof_elems, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
//  MPI_Reduce(sendbufstd, recvbufstd, prof_elems, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
//  for(int i=0; i<prof_elems; i++) { 
//    recvbufavg[i] /= numRanks; // avg
//    recvbufstd[i] /= numRanks; // avg of 2nd momentum
//    recvbufstd[i] =  sqrt(recvbufstd[i] - recvbufavg[i] * recvbufavg[i]);
//  }
//  packer::time_copy.UpdateBW(       recvbufavg[0], recvbufstd[0], recvbufmin[0], recvbufmax[0]); 
//  packer::time_write.UpdateBW(      recvbufavg[1], recvbufstd[1], recvbufmin[1], recvbufmax[1]); 
//  packer::time_read.UpdateBW(       recvbufavg[2], recvbufstd[2], recvbufmin[2], recvbufmax[2]); 
//  packer::time_posix_write.UpdateBW(recvbufavg[3], recvbufstd[3], recvbufmin[3], recvbufmax[3]); 
//  packer::time_posix_read.UpdateBW( recvbufavg[4], recvbufstd[4], recvbufmin[4], recvbufmax[4]); 
//  packer::time_posix_seek.UpdateBW( recvbufavg[5], recvbufstd[5], recvbufmin[5], recvbufmax[5]); 
//  packer::time_mpiio_write.UpdateBW(recvbufavg[6], recvbufstd[6], recvbufmin[6], recvbufmax[6]); 
//  packer::time_mpiio_read.UpdateBW( recvbufavg[7], recvbufstd[7], recvbufmin[7], recvbufmax[7]); 
//  packer::time_mpiio_seek.UpdateBW( recvbufavg[8], recvbufstd[8], recvbufmin[8], recvbufmax[8]);
//}
