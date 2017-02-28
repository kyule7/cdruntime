#ifndef _CKPT_HIST_H
#define _CKPT_HIST_H

#include <iostream>
#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>
#include <utility>
#include "hist_def.h"
#define CKPT_CNT_LIMIT 4096
#define HISTO_SIZE_LIMIT 16384
#define HISTO_SIZE 512
#define HISTO_SIZE_DEFAULT 2048
#define HISTO_SMALLSIZE_DEFAULT 1024

#define BIN_PRECISION 0.01
#define BIN_SHIFT 0
#define NUM_PHASE 4
#define MAX_CKPT_CNT 2048
#define MIN_CKPT_TIME 0.1
#define TOTAL_PHASE 0

bool openclose = false;
bool finalized = false;
uint32_t bin_shift = BIN_SHIFT;
uint32_t ckpt_cnt = 0;
float max_latency = 0.0f;

//struct MyVector {
//  std::vector<uint32_t> histogram_;
//  std::vector<float> latency_;
//  MyVector() {
//    histogram_.reserve(HISTO_SMALLSIZE_DEFAULT);
//    latency_.reserve(HISTO_SMALLSIZE_DEFAULT);
//    memset(histogram_.data(), 0, HISTO_SMALLSIZE_DEFAULT*sizeof(uint32_t));
//    memset(latency_.data(), 0, HISTO_SMALLSIZE_DEFAULT*sizeof(uint32_t));
//    printf("MyVector created1\n");
//  }
//  MyVector(uint32_t size, uint32_t latency_size=HISTO_SMALLSIZE_DEFAULT) {
//    histogram_.reserve(size);
//    latency_.reserve(latency_size);
//    memset(histogram_.data(), 0, size * sizeof(uint32_t));
//    memset(latency_.data(), 0, latency_size*sizeof(uint32_t));
//    printf("MyVector created2, size:%u, %zu, latencysize:%u, %zu\n", 
//        size, histogram_.capacity(), latency_size, latency_.capacity());
//  }
//  void Reserve(uint32_t size, uint32_t latency_size=HISTO_SMALLSIZE_DEFAULT) {
//    histogram_.reserve(size);
//    latency_.reserve(latency_size);
//    memset(histogram_.data(), 0, size * sizeof(uint32_t));
//    memset(latency_.data(), 0, latency_size*sizeof(uint32_t));
//    printf("reserve size:%u, %zu, latencysize:%u, %zu\n", 
//        size, histogram_.capacity(), latency_size, latency_.capacity());
//  }
//};

//typedef std::map<uint32_t,MyVector> std::map< uint32_t, MyVector >;
typedef uint32_t PhaseType;

struct Singleton {
  double prv_elapsed;
  double prv_timer;
  double total_timer;
  double prv_time;
  double total_overhead;
  uint32_t prv_cnt;
  struct timeval start;

  float data[NUM_PHASE][MAX_CKPT_CNT];
  char     *histogram_filename;

  // Comm related
  MPI_Comm node_comm;
  MPI_Comm zero_comm;
  int myRank;
  int numRanks;
  int numNodeRanks;
  int myNodeRank;
  int numZeroRanks;
  int myZeroRank;

  bool initialized;
//  int num_phase;

  enum MeasureType {
    kMeasureIO = 0,
    kMeasureNonIO = 1
  };

  Singleton(void) {
    prv_elapsed = 0.0;
    prv_timer = 0.0;
    total_timer = 0.0;
    prv_time = 0.0;
    prv_cnt = 0;
    for(int i=0; i<NUM_PHASE; i++) {
      memset(&(data[i][0]), 0, MAX_CKPT_CNT * sizeof(float));
    }
//    data.insert(std::make_pair(TOTAL_PHASE,MyVector(HISTO_SIZE_DEFAULT)));
//    data[0].Reserve(HISTO_SIZE_DEFAULT);
//    printf("histo:%zu, latency:%zu\n", data[0].histogram_.capacity(), data[0].latency_.capacity());
    histogram_filename = NULL;
    gettimeofday(&start, NULL);
    myRank = -1;
    numRanks = -1;
    numZeroRanks = -1;
    myZeroRank = -1;
    numZeroRanks = -1;
    myZeroRank = -1;
    initialized = false;
  }
  
  ~Singleton(void) {
    //printf("[Destructor] ");
    CheckTime();
  }

  void BeginTimer(enum MeasureType target=kMeasureIO) {
    if(finalized) return;
    if(target == kMeasureIO) {
      prv_timer = MPI_Wtime();
    } else if(target == kMeasureNonIO) {
      total_timer = MPI_Wtime();
    }
    //printf("[%d] start:%lf\n", myRank, prv_timer);
  }

  void EndTimerPhase(int phase, int ckpt_id) {
    if(finalized) return;
    double newtime= MPI_Wtime();
    double elapsed = newtime - prv_timer;
    double total_elapsed = newtime - total_timer;
    prv_time += elapsed;
    total_overhead += total_elapsed;
//    printf("[%d] elapsed:%lf (newtime %lf - prv_timer %lf)\n", myRank, elapsed, newtime, prv_timer);
//    printf("phase:%d\n", phase);

    if(phase != TOTAL_PHASE) {
      InsertTime(phase, elapsed, ckpt_id);
    }
  }

  void FinishTimer(uint32_t ckpt_id) {
    if(finalized) return;
    InsertTime(TOTAL_PHASE, prv_time, ckpt_id);
    prv_elapsed += prv_time;
    prv_cnt++;
    prv_time = 0;
  }
  
  void CheckTime(void) {
    if(finalized) return;
    struct timeval end;
    gettimeofday(&end, NULL);
    double tv_val = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec))*0.000001;
    if(myRank == 0)
      printf("final elapsed:%lf\n", tv_val);
  }

  //FIXME
  void InsertBin(uint32_t *histogram, float *latency, uint32_t size) {
    for(uint32_t i=0; i<size; i++) {  
      //printf("[%d]prv time : %lf\n", myRank, sample);
  //    double shifted_sample = latency[i];// -  bin_shift;
  //    if(shifted_sample < 0) shifted_sample = 0.0;
  //    uint32_t bin_idx = (uint32_t)(shifted_sample / BIN_PRECISION);
  
  //    printf("(%d) prv time [\t%d] : %lf (phase:%d)\n", interval, bin_idx, sample, phase_bin);
      uint32_t idx = (uint32_t)(latency[i] / BIN_PRECISION);
//      printf("(%d/%u)rank0 idx:%u (%lf) \n", i, size, idx, latency[i]);
      if(latency[i] > MIN_CKPT_TIME) {
        histogram[idx]++;
      }
    }
  }
  void InsertTime(int phase, double sample, uint32_t ckpt_id) {
    if(ckpt_id < MAX_CKPT_CNT) { 
      data[phase][ckpt_id] = sample;
    } 
    max_latency = (sample > max_latency)? sample : max_latency;
    
    // ckpt_id is always less than MAX_CKPT_CNT
    if(ckpt_id < MAX_CKPT_CNT-1) {
      ckpt_cnt++;
    }
    //printf("[%u| Insert #%u] sample:%lf\n", myRank, ckpt_id, sample);

  }

//  void InitMyRank(int _myRank) {
//    myRank = _myRank;
//    if(numRanks != -1) {
//      Initialize();
//      initialized = true;
//    }
//  }
//  void InitNumRanks(int _numRanks) {
//    numRanks = _numRanks;
//    if(myRank != -1) {
//      Initialize();
//      initialized = true;
//    }
//  }
  void Initialize(int _myRank, int _numRanks) {
//    if(initialized) return;
//    if(myRank == -1 || numRanks == -1) assert(0);
    myRank = _myRank;
    numRanks = _numRanks;
    char *set_bin_shift = getenv("SET_BIN_SHIFT");
    if(set_bin_shift != NULL) {
      bin_shift = (uint32_t)atoi(set_bin_shift);
    }
    histogram_filename = new char[64]();
  
    char hostname[32] = {0};
    gethostname(hostname, sizeof(hostname));
    char hostname_cat[9];
    memcpy(hostname_cat, hostname, 8);
    hostname_cat[8] = '\0';
 //   printf("hostname %s.%d\n", hostname,myRank);
    snprintf(histogram_filename, 64, "local.%s.%d", hostname_cat, myRank); 
  
 
//    std::hash<std::string> ptr_hash; 
//    uint64_t long_id = ptr_hash(std::string(hostname_cat));
//    //printf("hostname %s %lu\n", histogram_filename, long_id);
//    uint32_t node_id = ((uint32_t)(long_id >> 32)) ^ ((uint32_t)(long_id));
// //   printf("hostname %s %lu (%u)\n", histogram_filename, long_id, node_id);
// 
//    MPI_Comm_split(MPI_COMM_WORLD, node_id, myRank, &node_comm);
//    MPI_Comm_size(node_comm, &numNodeRanks) ;
//    MPI_Comm_rank(node_comm, &myNodeRank) ;
// 
//    // for rank 0s
//    if(myNodeRank == 0) {
//      MPI_Comm_split(MPI_COMM_WORLD, 0, myRank, &zero_comm);
//    } else {
//      MPI_Comm_split(MPI_COMM_WORLD, 1, myRank, &zero_comm);
//    }
//    MPI_Comm_size(zero_comm, &numZeroRanks);
//    MPI_Comm_rank(zero_comm, &myZeroRank);
  }

  void Finalize(void) {
    finalized = true;
//    printf("[%u] Finalize data:%zu!\n", myRank, data.size());
    float *gathered;
    uint32_t *histogram;
    uint32_t latency_size = NUM_PHASE * ckpt_cnt;
    float global_max;
    PMPI_Allreduce(&max_latency, &global_max, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
    uint32_t histo_size = (uint32_t)(NUM_PHASE * global_max / BIN_PRECISION);
//    printf("check histo_size:%u(%f|%f)\n", histo_size, max_latency, global_max);
    if(myRank == 0) {
      gathered = (float *)calloc(numRanks * latency_size, sizeof(float));
      histogram = (uint32_t *)calloc(histo_size, sizeof(uint32_t));
    }
//    printf("[%u] Gather! check histo_size:%u(%f|%f)\n", myRank, histo_size, max_latency, global_max);
    // Gather among ranks in the same node
    PMPI_Gather(data, latency_size, MPI_FLOAT, 
                gathered, latency_size, MPI_FLOAT, 
                0, MPI_COMM_WORLD);
//    printf("[Final] prv time  : %lf %lf %u %d\n",  prv_elapsed, prv_elapsed/ckpt_idx, prv_cnt, ckpt_idx);
 
    if(myRank == 0) {
      printf("WriteThis!\n");
      InsertBin(histogram, gathered, numRanks * latency_size);

      char latency_filename[64] = {0};
      printf("WriteThis done!\n");
      char unique_hist_filename[64] = {0};
      snprintf(latency_filename, 64, "latency.%s.csv", histogram_filename); 
      snprintf(unique_hist_filename, 64, "histogram.%s.csv", histogram_filename); 
      FILE *histfp = fopen(unique_hist_filename, "w");
      FILE *latencyfp = fopen(latency_filename, "w");

      for(uint32_t i=0; i<histo_size; i++) {  
        fprintf(histfp, "%u,", histogram[i]);
        //if(histogram[i] != 0)
          //printf("histo[%u]=%u\n", i, histogram[i]);
      }
      fprintf(histfp, "\n");
      for(int32_t j=0; j<numRanks; j++) {
        uint32_t stride = latency_size*j;
        for(uint32_t i=0; i<latency_size; i++) {   
           fprintf(latencyfp, "%.3f,", gathered[i + stride]);
           //printf("latency[%u]=%f\n", i+stride, gathered[i+stride]);
        }
        fprintf(latencyfp, "\n");
      }
      fprintf(histfp, "\n");
      fprintf(latencyfp, "\n");
 
      fclose(histfp);
      fclose(latencyfp);
      free(gathered);
      free(histogram);
      printf("WriteThis done!\n");
    } else {
//      printf("[%u] skip Write\n", myRank);
    }
  } // Finalize ends
}; // class Singleton ends



#endif
