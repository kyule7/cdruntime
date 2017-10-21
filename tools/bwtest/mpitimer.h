#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>
#include <unistd.h>
#include <math.h>
/************************************************************************************
 *
 * MPITimer
 *
 * MPITimer is designed to measure the "action" latency of each MPI rank.
 * "action" here means IO access for now, but can be anything.
 * It measures exp, std of actions of each rank,
 * and exp, std of all ranks of each action.
 *
 * To support it, it has an array of each latency measurement. 
 * If there are N actions measured, it has N elements in the array for record.
 * At finalization routine, it communicates the record to get average result.
 *
 * It supports to co-locate MPI ranks sharing the same node to observe any pattern.
 *
 * It also supports to measure the action latency of each phase.
 *
 ************************************************************************************/


/******************************************************************
 *
 * Jenkins Hash Function
 * Copied from https://en.wikipedia.org/wiki/Jenkins_hash_function
 *
 ******************************************************************/

#define INIT_SIZE 1024

uint32_t JHash(const char* key, size_t length) 
{
  size_t i = 0;
  uint32_t hash = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}


typedef struct MPITimerDef {
  double begin_;
  double end_;
  uint32_t idx_;
  uint32_t num_phase_;
  uint32_t size_;
  float **elapsed_;
  int nRanks_;
  int myRank_;
  int nRanks_in_node_;
  int myRank_in_node_;
  int nRanks_in_heads_;
  int myRank_in_heads_;
  MPI_Comm node_comm_;
  MPI_Comm head_comm_;
} MPITimer;

//typedef struct MPITimerDef MPITimer;

void InitializeMPITimer(MPITimer *mpi_timer, int num_phase) {
  mpi_timer->begin_ = 0;
  mpi_timer->end_ = 0;
  mpi_timer->idx_ = 0;
  mpi_timer->num_phase_ = num_phase;
  mpi_timer->size_ = INIT_SIZE;
  if(num_phase > 0) {
    float **elapsed = (float **)calloc(num_phase, sizeof(float *));
    uint32_t size = INIT_SIZE * sizeof(float);
    for(uint32_t i=0; i<num_phase; i++) {
      elapsed[i] = (float *)malloc(size);
      memset(elapsed[i], 0, size);
    }
    mpi_timer->elapsed_ = elapsed;
  } else {
    printf("Invalid number of phases = %d\n", num_phase);
    assert(0);
  }

  // Grouping tasks in a node
  char hostname[64];
  memset(hostname, 0, sizeof(hostname));
  gethostname(hostname, sizeof(hostname));
  uint32_t node_id = JHash(hostname, 64);

  int nRanks = 0;
  int myRank = 0;
  int nRanks_in_node = 0;
  int myRank_in_node = 0;
  int nRanks_in_heads = 0;
  int myRank_in_heads = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &nRanks);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  MPI_Comm_split(MPI_COMM_WORLD, node_id, myRank, &(mpi_timer->node_comm_));
  MPI_Comm_size(mpi_timer->node_comm_, &nRanks_in_node);
  MPI_Comm_rank(mpi_timer->node_comm_, &myRank_in_node);
  mpi_timer->nRanks_         = nRanks;
  mpi_timer->myRank_         = myRank;
  mpi_timer->nRanks_in_node_ = nRanks_in_node;
  mpi_timer->myRank_in_node_ = myRank_in_node; 

  // Grouping heads in each node
  if(myRank_in_node == 0) {
    MPI_Comm_split(MPI_COMM_WORLD, 0, myRank, &(mpi_timer->head_comm_));
  } else {
    MPI_Comm_split(MPI_COMM_WORLD, 1, myRank, &(mpi_timer->head_comm_));
  }
  MPI_Comm_size(mpi_timer->head_comm_, &nRanks_in_heads);
  MPI_Comm_rank(mpi_timer->head_comm_, &myRank_in_heads);
  mpi_timer->nRanks_in_node_ = nRanks_in_heads;
  mpi_timer->myRank_in_node_ = myRank_in_heads;
  printf("Host:%s (%u) ID:%d/%d, ID in Node:%d/%d, ID in head: %d/%d\n",
      hostname, node_id, myRank, nRanks, myRank_in_node, nRanks_in_node, myRank_in_heads, nRanks_in_heads); 
}

void PrintTimer(MPITimer *mpi_timer);

void FinalizeMPITimer(MPITimer *mpi_timer) {
//  PrintTimer(mpi_timer); 
  float **elapsed = mpi_timer->elapsed_;
  uint32_t num_phase = mpi_timer->num_phase_;
  for(uint32_t i=0; i<num_phase; i++) {
    free(elapsed[i]);
  }
  free(elapsed);
}

void GatherTimer(MPITimer *mpi_timer, int iter) 
{
  float **elapsed    = mpi_timer->elapsed_;
  uint32_t num_phase = mpi_timer->num_phase_;
  uint32_t idx       = mpi_timer->idx_;
  uint32_t myRank    = mpi_timer->myRank_;
  uint32_t nRanks    = mpi_timer->nRanks_;
  uint32_t max_idx   = 0;

  MPI_Allreduce(&idx, &max_idx, 1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);
  assert(idx == max_idx);
  uint32_t size = max_idx * sizeof(float);

  int numElem = nRanks * max_idx;
  float **reduced = NULL;
  float **gathered = NULL;
  float **elapsed_sq = NULL;
  float **reduced_sq = NULL;
  float **gathered_sq = NULL;
  if(myRank == 0) {
    reduced = (float **)calloc(num_phase, sizeof(float *));
    gathered= (float **)calloc(num_phase, sizeof(float *));
    reduced_sq = (float **)calloc(num_phase, sizeof(float *));
    gathered_sq= (float **)calloc(num_phase, sizeof(float *));
    for(uint32_t i=0; i<num_phase; i++) {
      reduced[i] = (float *)malloc(size);
      gathered[i] = (float *)malloc(size * nRanks);
      memset(reduced[i], 0, size);
      memset(gathered[i], 0, size * nRanks);
      reduced_sq[i] = (float *)malloc(size);
      gathered_sq[i] = (float *)malloc(size * nRanks);
      memset(reduced_sq[i], 0, size);
      memset(gathered_sq[i], 0, size * nRanks);
    }
  }
  elapsed_sq = (float **)calloc(num_phase, sizeof(float *));
  for(uint32_t i=0; i<num_phase; i++) {
    elapsed_sq[i] = (float *)malloc(size);
  }
  for(uint32_t phase=0; phase<num_phase; phase++) {
    // Calculate second momentum at each task
    for(uint32_t i=0; i<max_idx; i++) {
      elapsed_sq[phase][i] = elapsed[phase][i] * elapsed[phase][i];
    }

    // Gather/Reduce first momentum (avg)
    if(myRank != 0) {
      MPI_Gather(elapsed[phase], max_idx, MPI_FLOAT, NULL, max_idx, MPI_FLOAT, 0, MPI_COMM_WORLD);
    } else {
      MPI_Gather(elapsed[phase], max_idx, MPI_FLOAT, gathered[phase], max_idx, MPI_FLOAT, 0, MPI_COMM_WORLD);
    }
    if(myRank != 0) {
      MPI_Reduce(elapsed[phase], NULL, max_idx, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); 
    } else {
      MPI_Reduce(elapsed[phase], reduced[phase], max_idx, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); 
    }

    // Gather/Reduce second momentum
    if(myRank != 0) {
      MPI_Gather(elapsed_sq[phase], max_idx, MPI_FLOAT, NULL, max_idx, MPI_FLOAT, 0, MPI_COMM_WORLD);
    } else {
      MPI_Gather(elapsed_sq[phase], max_idx, MPI_FLOAT, gathered_sq[phase], max_idx, MPI_FLOAT, 0, MPI_COMM_WORLD);
    }
    if(myRank != 0) {
      MPI_Reduce(elapsed_sq[phase], NULL, max_idx, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); 
    } else {
      MPI_Reduce(elapsed_sq[phase], reduced_sq[phase], max_idx, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); 
    }
  }

  if(myRank == 0) {
    double tot_avg_elapsed = 0.0;
    double tot_avg_elapsed_sq = 0.0;
    float **check = (float **)calloc(num_phase, sizeof(float *));
    for(uint32_t phase=0; phase<num_phase; phase++) {
      check[phase] = (float *)malloc(size);
      memset(check[phase], 0, size);
    }
    printf("nRanks = %d\n", nRanks);
    for(uint32_t phase=0; phase<num_phase; phase++) {
      for(uint32_t i=0; i<max_idx; i++) {
        float orig_reduced = reduced[phase][i];
        float avg_elapsed_bw_tasks = reduced[phase][i] / nRanks;
        float avg_elapsed_sq_bw_tasks = reduced_sq[phase][i] / nRanks;
        reduced[phase][i]    = avg_elapsed_bw_tasks;
        reduced_sq[phase][i] = avg_elapsed_sq_bw_tasks;
        tot_avg_elapsed     += avg_elapsed_bw_tasks;
        tot_avg_elapsed_sq  += avg_elapsed_sq_bw_tasks;

        // sanity check
        float avg_check = 0.0;
        for(uint32_t n=0; n<numElem; n+=max_idx) {
          printf("%f ", gathered[phase][i + n]);
          avg_check += gathered[phase][i + n];
        }
        printf("\n");
        printf("%d reduced:%f = %f\n", i, orig_reduced, avg_check);
        check[phase][i] = avg_check / nRanks;

      }
    }

    // sanity check
    tot_avg_elapsed /= iter;
    tot_avg_elapsed_sq /= iter;
    printf("-- Profile ------------------------\n");
    for(uint32_t phase=0; phase<num_phase; phase++) {
      for(uint32_t i=0; i<max_idx; i++) {
        printf("%8d %16.8f == %16.8f\n", i, reduced[phase][i], check[phase][i]);
      }
    }

    printf("----------------------------------\n");
    double dev = tot_avg_elapsed * tot_avg_elapsed - tot_avg_elapsed_sq;
    double std = sqrt(dev);
    printf("Avg Latency:%lf (%lf, %lf)\n", tot_avg_elapsed, dev, std);
    double shift = 0.00;
    double precision = 0.001;
    //double precision = (8 * std) / 1000;
    int max_bin = 1024;
    uint32_t **histogram = (uint32_t **)calloc(num_phase, sizeof(uint32_t *));
    for(uint32_t phase=0; phase<num_phase; phase++) {
      histogram[phase] = (uint32_t *)malloc(max_bin * sizeof(uint32_t));
      memset(histogram[phase], 0, max_bin * sizeof(uint32_t));
    }
    for(uint32_t phase=0; phase<num_phase; phase++) {
      for(uint32_t i=0; i<max_idx; i++) {
        for(uint32_t n=0; n<numElem; n+=max_idx) {

          printf("%f - %f / %lf\n", gathered[phase][i + n], shift, precision);
          int32_t bin_idx = (int32_t)((gathered[phase][i + n] - shift) / precision);
          if(bin_idx < 0) {
            bin_idx = 0;
          } else if(bin_idx >= max_bin) {
            bin_idx = max_bin - 1;
          }
          histogram[phase][bin_idx]++;
        }
      }
    }
    printf("\n-- Histogram --------------------------------\n");
    uint32_t tot_cnt_known = num_phase * max_idx * nRanks;
    for(uint32_t phase=0; phase<num_phase; phase++) {
      uint32_t tot_cnt = 0;
      int non_zero_now = 0;
      for(uint32_t i=0; i<max_bin; i++) {
        uint32_t cnt = histogram[phase][i];
        if(cnt != 0) { 
          non_zero_now = 1;
          printf("%d, precision:%lf, shift:%lf\n", i, precision, shift);
        }
        if(non_zero_now) { 
          printf("%u ", cnt);
          tot_cnt += cnt;
        }
        if(tot_cnt == tot_cnt_known) break;
      }
      printf("\n-------------------------------------------\n");
      printf("%u %u\n", tot_cnt, tot_cnt_known);
    }
    for(uint32_t i=0; i<num_phase; i++) {
      free(check[i]);
      free(reduced[i]);
      free(gathered[i]);
      free(reduced_sq[i]);
      free(gathered_sq[i]);
    }
    free(check);
    free(reduced);
    free(gathered);
    free(reduced_sq);
    free(gathered_sq);
  } 
  for(uint32_t i=0; i<num_phase; i++) {
    free(elapsed_sq[i]);
  }
  free(elapsed_sq);
}

void PrintTimer(MPITimer *mpi_timer) 
{
  float **elapsed = mpi_timer->elapsed_;
  uint32_t num_phase = mpi_timer->num_phase_;
  uint32_t idx = mpi_timer->idx_ - 1;
  for(uint32_t phase=0; phase<num_phase; phase++) {
    for(uint32_t i=0; i<idx; i++) {
      printf("%8d %16.8f\n", i, elapsed[phase][i]);
    }
  }
  free(elapsed);
  
}

void Realloc(MPITimer *mpi_timer) {
  uint32_t ori_size = mpi_timer->size_;
  uint32_t new_size = ori_size * 2;
  uint32_t num_phase = mpi_timer->num_phase_;
  float **elapsed = mpi_timer->elapsed_;
  mpi_timer->size_  = new_size;
  for(uint32_t i=0; i<num_phase; i++) {
    elapsed[i] = (float *)realloc(elapsed[i], new_size);
    memset(elapsed[i], 0, new_size);
  }
}

void BeginTimer(MPITimer *mpi_timer) {
  mpi_timer->begin_ = MPI_Wtime();
}

void EndTimer(MPITimer *mpi_timer, uint32_t phase) {
  double end = MPI_Wtime();
  if(mpi_timer->idx_ >= mpi_timer->size_) {
    Realloc(mpi_timer);
  }
  mpi_timer->elapsed_[phase][mpi_timer->idx_] = (float)(end - mpi_timer->begin_);
}

void AdvanceTimer(MPITimer *mpi_timer) { 
  mpi_timer->idx_++;
}
//
//MPITimer {
//  double begin_;
//  double end_;
//  uint32_t idx_;
//  uint32_t num_phase_;
//  uint32_t size_;
//  float **elapsed_;
//  MPITimer(uint32_t num_phase=1) {
//    begin_ = 0;
//    end_ = 0;
//    idx_ = 0;
//    num_phase_ = num_phase;
//    size_ = INIT_SIZE;
//
//    elapsed_ = (float **)calloc(num_phase, sizeof(float *));
//    for(uint32_t i=0; i<num_phase; i++) {
//      elapsed_[i] = (float *)calloc(size_, sizeof(float));
//      memset(elapsed_[i], 0, sizeof(float) * size_);
//    }
//  }
//
//  ~MPITimer(void) { 
//    for(uint32_t i=0; i<num_phase; i++) {
//      free(elapsed_[i]);
//    }
//    free(elapsed_);
//
//  }
//
//  void Realloc(void) {
//    size_ *= 2;
//    uint32_t new_size = size_;
//    for(uint32_t i=0; i<num_phase; i++) {
//      elapsed_[i] = (float *)realloc(elapsed_[i], new_size);
//      memset(elapsed_[i], 0, new_size);
//    }
//  }
//
//  void BeginTimer(void) {
//    begin_ = MPI_Wtime();
//  }
//
//  void EndTimer(uint32_t phase) {
//    end_ = MPI_Wtime();
//    elapsed_[phase][idx_] = (float)(end_ - begin_);
//  }
//};
