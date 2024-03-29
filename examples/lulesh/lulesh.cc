/*
  This is a Version 2.0 MPI + OpenMP Beta implementation of LULESH

                 Copyright (c) 2010-2013.
      Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
                  LLNL-CODE-461231
                All rights reserved.

This file is part of LULESH, Version 2.0.
Please also read this link -- http://www.opensource.org/licenses/index.php

//////////////
DIFFERENCES BETWEEN THIS VERSION (2.x) AND EARLIER VERSIONS:
* Addition of regions to make work more representative of multi-material codes
* Default size of each domain is 30^3 (27000 elem) instead of 45^3. This is
  more representative of our actual working set sizes
* Single source distribution supports pure serial, pure OpenMP, MPI-only, 
  and MPI+OpenMP
* Addition of ability to visualize the mesh using VisIt 
  https://wci.llnl.gov/codes/visit/download.html
* Various command line options (see ./lulesh2.0 -h)
 -q              : quiet mode - suppress stdout
 -i <iterations> : number of cycles to run
 -s <size>       : length of cube mesh along side
 -r <numregions> : Number of distinct regions (def: 11)
 -b <balance>    : Load balance between regions of a domain (def: 1)
 -c <cost>       : Extra cost of more expensive regions (def: 1)
 -f <filepieces> : Number of file parts for viz output (def: np/9)
 -p              : Print out progress
 -v              : Output viz file (requires compiling with -DVIZ_MESH
 -h              : This message

 printf("Usage: %s [opts]\n", execname);
      printf(" where [opts] is one or more of:\n");
      printf(" -q              : quiet mode - suppress all stdout\n");
      printf(" -i <iterations> : number of cycles to run\n");
      printf(" -s <size>       : length of cube mesh along side\n");
      printf(" -r <numregions> : Number of distinct regions (def: 11)\n");
      printf(" -b <balance>    : Load balance between regions of a domain (def: 1)\n");
      printf(" -c <cost>       : Extra cost of more expensive regions (def: 1)\n");
      printf(" -f <numfiles>   : Number of files to split viz dump into (def: (np+10)/9)\n");
      printf(" -p              : Print out progress\n");
      printf(" -v              : Output viz file (requires compiling with -DVIZ_MESH\n");
      printf(" -h              : This message\n");
      printf("\n\n");

*Notable changes in LULESH 2.0

* Split functionality into different files
lulesh.cc - where most (all?) of the timed functionality lies
lulesh-comm.cc - MPI functionality
lulesh-init.cc - Setup code
lulesh-viz.cc  - Support for visualization option
lulesh-util.cc - Non-timed functions
*
* The concept of "regions" was added, although every region is the same ideal
*    gas material, and the same sedov blast wave problem is still the only
*    problem its hardcoded to solve.
* Regions allow two things important to making this proxy app more representative:
*   Four of the LULESH routines are now performed on a region-by-region basis,
*     making the memory access patterns non-unit stride
*   Artificial load imbalances can be easily introduced that could impact
*     parallelization strategies.  
* The load balance flag changes region assignment.  Region number is raised to
*   the power entered for assignment probability.  Most likely regions changes
*   with MPI process id.
* The cost flag raises the cost of ~45% of the regions to evaluate EOS by the
*   entered multiple. The cost of 5% is 10x the entered multiple.
* MPI and OpenMP were added, and coalesced into a single version of the source
*   that can support serial builds, MPI-only, OpenMP-only, and MPI+OpenMP
* Added support to write plot files using "poor mans parallel I/O" when linked
*   with the silo library, which in turn can be read by VisIt.
* Enabled variable timestep calculation by default (courant condition), which
*   results in an additional reduction.
* Default domain (mesh) size reduced from 45^3 to 30^3
* Command line options to allow numerous test cases without needing to recompile
* Performance optimizations and code cleanup beyond LULESH 1.0
* Added a "Figure of Merit" calculation (elements solved per microsecond) and
*   output in support of using LULESH 2.0 for the 2017 CORAL procurement
*
* Possible Differences in Final Release (other changes possible)
*
* High Level mesh structure to allow data structure transformations
* Different default parameters
* Minor code performance changes and cleanup

TODO in future versions
* Add reader for (truly) unstructured meshes, probably serial only
* CMake based build system

//////////////

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the disclaimer below.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the disclaimer (as noted below)
     in the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the LLNS/LLNL nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY, LLC,
THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Additional BSD Notice

1. This notice is required to be provided under our contract with the U.S.
   Department of Energy (DOE). This work was produced at Lawrence Livermore
   National Laboratory under Contract No. DE-AC52-07NA27344 with the DOE.

2. Neither the United States Government nor Lawrence Livermore National
   Security, LLC nor any of their employees, makes any warranty, express
   or implied, or assumes any liability or responsibility for the accuracy,
   completeness, or usefulness of any information, apparatus, product, or
   process disclosed, or represents that its use would not infringe
   privately-owned rights.

3. Also, reference herein to any specific commercial products, process, or
   services by trade name, trademark, manufacturer or otherwise does not
   necessarily constitute or imply its endorsement, recommendation, or
   favoring by the United States Government or Lawrence Livermore National
   Security, LLC. The views and opinions of authors expressed herein do not
   necessarily state or reflect those of the United States Government or
   Lawrence Livermore National Security, LLC, and shall not be used for
   advertising or product endorsement purposes.

*/

#include <climits>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <string>
#include <functional>
#if _OPENMP
# include <omp.h>
#endif

#include "lulesh.h"
#include <cassert>

//#define SERDES_ENABLED 1
#define ENABLE_HISTOGRAM 0
#define OPTIMIZE 1
#if OPTIMIZE == 0
#undef _ANALYZE_PHASE
#endif

#if _CD
//using namespace cd;
using namespace tuned;
//class DomainSerdes;
int counter=0;
Domain::DomainSerdes::SerdesInfo *Domain::DomainSerdes::serdesRegElem = NULL;
Domain::DomainSerdes::SerdesInfo Domain::DomainSerdes::serdes_table[TOTAL_ELEMENT_COUNT];
bool Domain::DomainSerdes::serdes_table_init = false; 
std::map<std::string,int> func_cnt;

uint64_t preserve_vec_all = ( M__X  | M__Y  | M__Z  | 
                              M__XD | M__YD | M__ZD |
                              M__XDD| M__YDD| M__ZDD|
                              M__DXX| M__DYY| M__DZZ|
                              M__FX | M__FY | M__FZ |
                              M__QQ | M__QL | M__SS |
                              M__Q  | M__E  | M__VOLO | 
                              M__V | M__P | //M__VNEW |
                              M__DELV | M__VDOV | M__AREALG |
                              M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA |
                              M__SYMMX | M__SYMMY | M__SYMMZ |
                              M__NODALMASS | M__MATELEMLIST | M__NODELIST | 
                              M__LXIM | M__LXIP | M__LETAM | 
                              M__LETAP | M__LZETAM | M__LZETAP | 
                              M__ELEMBC | M__ELEMMASS | M__REGELEMSIZE |
                              M__REGELEMLIST | M__REGELEMLIST_INNER | M__REG_NUMLIST );
uint64_t preserve_vec_ref = ( M__SYMMX | M__SYMMY | M__SYMMZ |
                              M__NODALMASS | M__MATELEMLIST | M__NODELIST | 
                              M__LXIM | M__LXIP | M__LETAM | 
                              M__LETAP | M__LZETAM | M__LZETAP | 
                              M__ELEMBC | M__ELEMMASS | M__REGELEMSIZE |
                              M__REGELEMLIST | M__REGELEMLIST_INNER | M__REG_NUMLIST );
// CalcForceForNodes
uint64_t preserve_vec_0   = ( M__FX | M__FY | M__FZ );
// CalcAccelerationForNodes ~ CalcPositionForNodes
uint64_t preserve_vec_1   = ( M__X  | M__Y  | M__Z  | 
                              M__XD | M__YD | M__ZD |
                              M__XDD| M__YDD| M__ZDD);
// CalcLagrangeElements ~ CalcQForElems
uint64_t preserve_vec_2   = ( M__DELV | M__VDOV | M__AREALG |
                              M__DXX| M__DYY| M__DZZ|
                              M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
                              M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA );

// ApplyMaterialPropertiesForElems ~ UpdateVolumesForElems
uint64_t preserve_vec_3   = ( M__QQ | M__QL | M__SS |
                              M__Q  | M__E  | M__VOLO | M__V | M__P );
int ckpt_idx=0;
uint32_t interval = 1;
#endif
//std::map<std::string,int> func_cnt;

#define HISTO_SIZE 1024
#define BIN_PRECISION 0.001
#define BIN_SHIFT 0
bool openclose = false; 
struct Singleton {
  double prv_elapsed;
  double prv_timer;
  double prv_time;
  uint32_t prv_cnt;
  uint32_t bin_cnt;
  uint32_t history_size;
  struct timeval start;

  uint32_t *histogram;
  uint32_t *latency_history;
#ifdef _ANALYZE_PHASE
  uint32_t *histogram_phase;
#endif
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

  int num_phase;
  Singleton(void) {
    prv_elapsed = 0.0;
    prv_timer = 0.0;
    prv_time = 0.0;
    prv_cnt = 0;
    bin_cnt = 0;
    history_size = 0;
    histogram = NULL;
    latency_history = NULL;
#ifdef _ANALYZE_PHASE
    histogram_phase = NULL;
#endif
    histogram_filename = NULL;
    gettimeofday(&start, NULL);
    myRank = -1;
    numRanks = -1;
    numZeroRanks = -1;
    myZeroRank = -1;
    numZeroRanks = -1;
    myZeroRank = -1;
    num_phase = -1;
  }
  
  ~Singleton(void) {
    //printf("[Destructor] ");
    CheckTime();
  }

  void BeginTimer(void) {
    prv_timer = MPI_Wtime();
  }

#ifdef _ANALYZE_PHASE
  void EndTimer(int phase) {
    double elapsed = MPI_Wtime() - prv_timer;
    prv_time += elapsed;
//    printf("phase:%d\n", phase);
    if(phase != -1)
      InsertBin(histogram_phase+phase*HISTO_SIZE, elapsed, true);
  }
#else
  void EndTimer(int phase) {
    prv_time += MPI_Wtime() - prv_timer;
  }
#endif

  void FinishTimer(void) {
      InsertBin(histogram, prv_time, false);
      prv_elapsed += prv_time;
      prv_cnt++;
      prv_time = 0;
  }
  
  void CheckTime(void) {
    struct timeval end;
    gettimeofday(&end, NULL);
    double tv_val = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec))*0.000001;
    if(myRank == 0)
      printf("final elapsed:%lf\n", tv_val);
  }

  void InsertBin(uint32_t *histogram, double sample, bool phase_bin) {
     //   printf("prv time [\t%d] : %lf\n", idx, prv_time);
     double shifted_sample = sample - BIN_SHIFT;
     if(shifted_sample < 0) shifted_sample = 0.0;
     uint32_t bin_idx = (uint32_t)(shifted_sample / BIN_PRECISION);
//     printf("(%d) prv time [\t%d] : %lf (phase:%d)\n", interval, bin_idx, sample, phase_bin);
     if(bin_idx >= HISTO_SIZE) {
       bin_idx = HISTO_SIZE-1;
     }
     histogram[bin_idx]++;
     if(phase_bin == false) {
       latency_history[bin_cnt++] = bin_idx;
     }
  }
#ifdef _ANALYZE_PHASE
  void AllocateHistogramForPhase(int _num_phase) {
    assert(_num_phase != -1);
    num_phase = _num_phase;
    histogram_phase = new uint32_t[num_phase * HISTO_SIZE]();
//    histogram_phase = new uint32_t*[num_phase];
//    for(int i=0; i<num_phase; i++) {
//      histogram_phase[i] = new uint32_t[HISTO_SIZE]();
//    }
  }
#endif
  void Initialize(int _myRank, int _numRanks, uint32_t _total_its, uint32_t &_interval) {
    myRank = _myRank;
    numRanks = _numRanks;
    if(_interval == 0) { return; }

    char *ckpt_var = getenv("CKPT_INTERVAL");
    if(ckpt_var != NULL) {
      _interval = (uint32_t)atoi(ckpt_var);
    }
    if(myRank == 0) 
      printf("interval:%u\n", _interval);
    history_size = (_total_its/_interval);
    latency_history = new unsigned int[history_size](); // zero-initialized
    histogram = new unsigned int[HISTO_SIZE]();
    histogram_filename = new char[64]();
  
    char hostname[32] = {0};
    gethostname(hostname, sizeof(hostname));
    char hostname_cat[9];
    memcpy(hostname_cat, hostname, 8);
    hostname_cat[8] = '\0';
    //printf("hostname %s\n", hostname);
    snprintf(histogram_filename, 64, "local.%s.%d", hostname_cat, myRank); 
  
 
    std::hash<std::string> ptr_hash; 
    uint64_t long_id = ptr_hash(std::string(hostname_cat));
    //printf("hostname %s %lu\n", histogram_filename, long_id);
    uint32_t node_id = ((uint32_t)(long_id >> 32)) ^ ((uint32_t)(long_id));
 //   printf("hostname %s %lu (%u)\n", histogram_filename, long_id, node_id);
 
    MPI_Comm_split(MPI_COMM_WORLD, node_id, myRank, &node_comm);
    MPI_Comm_size(node_comm, &numNodeRanks) ;
    MPI_Comm_rank(node_comm, &myNodeRank) ;
 
    // for rank 0s
    if(myNodeRank == 0) {
      MPI_Comm_split(MPI_COMM_WORLD, 0, myRank, &zero_comm);
    } else {
      MPI_Comm_split(MPI_COMM_WORLD, 1, myRank, &zero_comm);
    }
    MPI_Comm_size(zero_comm, &numZeroRanks);
    MPI_Comm_rank(zero_comm, &myZeroRank);
 
#ifdef _ANALYZE_PHASE
    AllocateHistogramForPhase(4);
#endif
 
//    printf("hostname %s %lu (%u): %d %d/%d %d/%d\n", 
//        histogram_filename, long_id, node_id, 
//        myRank, myNodeRank, numNodeRanks, myZeroRank, numZeroRanks);
  } 

  void Finalize(void) {
    unsigned int *latency_history_global = NULL;
    unsigned int *latency_history_semiglobal = NULL;
    unsigned int *histogram_global = NULL; //[HISTO_SIZE]
    unsigned int *histogram_semiglobal = NULL; //[HISTO_SIZE]
    unsigned int *histogram_phase_global = NULL;
    unsigned int *histogram_phase_semiglobal = NULL;
    if(myRank == 0) {
      latency_history_global = new unsigned int[history_size * numRanks](); // zero-initialized
      histogram_global = new unsigned int[HISTO_SIZE * numRanks]();
#ifdef _ANALYZE_PHASE
      if(num_phase != -1) {
        histogram_phase_global = new unsigned int[HISTO_SIZE * numRanks * num_phase]();
      }
#endif
    }
    if(myNodeRank == 0) {
      // numZeroRanks = 4
      // numRanks/numZeroRanks = 16
      latency_history_semiglobal = new unsigned int[history_size * numNodeRanks](); // zero-initialized
      histogram_semiglobal = new unsigned int[HISTO_SIZE * numNodeRanks]();
#ifdef _ANALYZE_PHASE
      if(num_phase != -1) {
        histogram_phase_semiglobal = new unsigned int[HISTO_SIZE * numNodeRanks * num_phase]();
      }
#endif
    }
 
    // Gather among ranks in the same node
    PMPI_Gather(latency_history, history_size, MPI_UNSIGNED, 
               latency_history_semiglobal, history_size, MPI_UNSIGNED, 
               0, node_comm);
 
    PMPI_Gather(histogram, HISTO_SIZE, MPI_UNSIGNED, 
               histogram_semiglobal, HISTO_SIZE, MPI_UNSIGNED, 
               0, node_comm);
#ifdef _ANALYZE_PHASE
    if(num_phase != -1) {
      PMPI_Gather(histogram_phase, HISTO_SIZE * num_phase, MPI_UNSIGNED, 
                 histogram_phase_semiglobal, HISTO_SIZE * num_phase, MPI_UNSIGNED, 
                 0, node_comm);
    }
#endif
    // Gather among zero ranks
    if(myNodeRank == 0) {
       PMPI_Gather(latency_history_semiglobal, history_size * numNodeRanks, MPI_UNSIGNED, 
                  latency_history_global, history_size * numNodeRanks, MPI_UNSIGNED, 
                  0, zero_comm);
       PMPI_Gather(histogram_semiglobal, HISTO_SIZE * numNodeRanks, MPI_UNSIGNED,
                  histogram_global, HISTO_SIZE * numNodeRanks, MPI_UNSIGNED, 
                  0, zero_comm);
#ifdef _ANALYZE_PHASE
       if(num_phase != -1) {
          PMPI_Gather(histogram_phase_semiglobal, HISTO_SIZE * numNodeRanks * num_phase, MPI_UNSIGNED,
                     histogram_phase_global, HISTO_SIZE * numNodeRanks * num_phase, MPI_UNSIGNED, 
                     0, zero_comm);
       }
#endif
    }

//    printf("[Final] prv time  : %lf %lf %u %d\n",  prv_elapsed, prv_elapsed/ckpt_idx, prv_cnt, ckpt_idx);
 
    if(myRank == 0) {
      char latency_history_filename[64] = {0};
      char unique_hist_filename[64] = {0};
      snprintf(latency_history_filename, 64, "latency.%s.csv", histogram_filename); 
      snprintf(unique_hist_filename, 64, "histogram.%s.csv", histogram_filename); 
      FILE *histfp = fopen(unique_hist_filename, "w");
      FILE *latencyfp = fopen(latency_history_filename, "w");

      for(uint32_t j=0; j<numRanks; j++) {
         uint32_t stride = HISTO_SIZE*j;
         for(uint32_t i=0; i<HISTO_SIZE; i++) {  
            fprintf(histfp, "%u,", histogram_global[i + stride]);
         }
         fprintf(histfp, "\n");
 
         stride = history_size*j;
         for(uint32_t i=0; i<history_size; i++) {   
            fprintf(latencyfp, "%u,", latency_history_global[i + stride]);
         }
         fprintf(latencyfp, "\n");
      }
      fprintf(histfp, "\n");
      fprintf(latencyfp, "\n");
 
      fclose(histfp);
      fclose(latencyfp);
#ifdef _ANALYZE_PHASE
      char history_phase_filename[64] = {0};
      snprintf(history_phase_filename, 64, "histogram_phase.%s.csv", histogram_filename); 
      FILE *histphasefp = fopen(history_phase_filename, "w");
      //for(uint32_t k=0; k<num_phase; k++) {
//      for(uint32_t j=0; j<numRanks; j++) {
      for(uint32_t ii=0; ii<num_phase; ii++) {
         uint32_t phase_stride = HISTO_SIZE*ii;
//         uint32_t stride = HISTO_SIZE*num_phase*j;
//         for(uint32_t ii=0; ii<num_phase; ii++) {
         for(uint32_t j=0; j<numRanks; j++) {
            uint32_t stride = phase_stride + HISTO_SIZE*num_phase*j;
//            uint32_t stride_phase = stride + HISTO_SIZE*ii;
            for(uint32_t i=0; i<HISTO_SIZE; i++) {  
               fprintf(histphasefp, "%u,", histogram_phase_global[i + stride]);
            }
            fprintf(histphasefp, "\n");
         }
         fprintf(histphasefp, "\n\n\n\n");
      }
      fclose(histphasefp);
#endif
    }

#if 0
    FILE *histfp = fopen(histogram_filename, "w");
    for(uint32_t i=0; i<HISTO_SIZE; i++) {   
       fprintf(histfp, "%u ", histogram[i]);
    }
    fprintf(histfp, "\n");
    fprintf(histfp, "\n================================================================\n");
    for(uint32_t i=0; i<history_size; i++) {   
       fprintf(histfp, "%u,", latency_history[i]);
    }
    fprintf(histfp, "\n");
    fclose(histfp);
#endif

#if 0
    if(myRank == 0) {
      delete latency_history_global;
      delete histogram_global;
#ifdef _ANALYZE_PHASE
      if(num_phase != -1) {
        delete histogram_phase_global;
      }
#endif
    }
    if(myNodeRank == 0) {
      delete latency_history_semiglobal;
      delete histogram_semiglobal;
#ifdef _ANALYZE_PHASE
      if(num_phase != -1) {
        delete histogram_phase_semiglobal;
      }
#endif
    }
#endif

    if(myRank == 0) {
      delete [] latency_history_global;
      delete [] histogram_global;
#ifdef _ANALYZE_PHASE
      if(num_phase != -1) {
        delete [] histogram_phase_global;
      }
#endif
    }
    if(myNodeRank == 0) {
      // numZeroRanks = 4
      // numRanks/numZeroRanks = 16
      delete [] latency_history_semiglobal;
      delete [] histogram_semiglobal;
#ifdef _ANALYZE_PHASE
      if(num_phase != -1) {
        delete [] histogram_phase_semiglobal;
      }
#endif
    }
  } // Finalize ends
}; // class Singleton ends

//double Singleton::start_clk = 0.0;
Singleton st;
/*********************************/
/* Data structure implementation */
/*********************************/

/* might want to add access methods so that memory can be */
/* better managed, as in luleshFT */

template <typename T>
T *Allocate(size_t size)
{
   return static_cast<T *>(malloc(sizeof(T)*size)) ;
}

template <typename T>
void Release(T **ptr)
{
   if (*ptr != NULL) {
      free(*ptr) ;
      *ptr = NULL ;
   }
}


/******************************************/

/* Work Routines */

static inline
void TimeIncrement(Domain& domain)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
   Real_t targetdt = domain.stoptime() - domain.time() ;

   if ((domain.dtfixed() <= Real_t(0.0)) && (domain.cycle() != Int_t(0))) {
      Real_t ratio ;
      Real_t olddt = domain.deltatime() ;

      /* This will require a reduction in parallel */
      Real_t gnewdt = Real_t(1.0e+20) ;
      Real_t newdt ;
      if (domain.dtcourant() < gnewdt) {
         gnewdt = domain.dtcourant() / Real_t(2.0) ;
      }
      if (domain.dthydro() < gnewdt) {
         gnewdt = domain.dthydro() * Real_t(2.0) / Real_t(3.0) ;
      }

#if USE_MPI      
      MPI_Allreduce(&gnewdt, &newdt, 1,
                    ((sizeof(Real_t) == 4) ? MPI_FLOAT : MPI_DOUBLE),
                    MPI_MIN, MPI_COMM_WORLD) ;
#else
      newdt = gnewdt;
#endif
      
      ratio = newdt / olddt ;
      if (ratio >= Real_t(1.0)) {
         if (ratio < domain.deltatimemultlb()) {
            newdt = olddt ;
         }
         else if (ratio > domain.deltatimemultub()) {
            newdt = olddt*domain.deltatimemultub() ;
         }
      }

      if (newdt > domain.dtmax()) {
         newdt = domain.dtmax() ;
      }
      domain.deltatime() = newdt ;
   }

   /* TRY TO PREVENT VERY SMALL SCALING ON THE NEXT CYCLE */
   if ((targetdt > domain.deltatime()) &&
       (targetdt < (Real_t(4.0) * domain.deltatime() / Real_t(3.0))) ) {
      targetdt = Real_t(2.0) * domain.deltatime() / Real_t(3.0) ;
   }

   if (targetdt < domain.deltatime()) {
      domain.deltatime() = targetdt ;
   }

   domain.time() += domain.deltatime() ;

   ++domain.cycle() ;


}

/******************************************/

static inline
void CollectDomainNodesToElemNodes(Domain &domain,
                                   const Index_t* elemToNode,
                                   Real_t elemX[8],
                                   Real_t elemY[8],
                                   Real_t elemZ[8])
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   Index_t nd0i = elemToNode[0] ;
   Index_t nd1i = elemToNode[1] ;
   Index_t nd2i = elemToNode[2] ;
   Index_t nd3i = elemToNode[3] ;
   Index_t nd4i = elemToNode[4] ;
   Index_t nd5i = elemToNode[5] ;
   Index_t nd6i = elemToNode[6] ;
   Index_t nd7i = elemToNode[7] ;

   elemX[0] = domain.x(nd0i);
   elemX[1] = domain.x(nd1i);
   elemX[2] = domain.x(nd2i);
   elemX[3] = domain.x(nd3i);
   elemX[4] = domain.x(nd4i);
   elemX[5] = domain.x(nd5i);
   elemX[6] = domain.x(nd6i);
   elemX[7] = domain.x(nd7i);

   elemY[0] = domain.y(nd0i);
   elemY[1] = domain.y(nd1i);
   elemY[2] = domain.y(nd2i);
   elemY[3] = domain.y(nd3i);
   elemY[4] = domain.y(nd4i);
   elemY[5] = domain.y(nd5i);
   elemY[6] = domain.y(nd6i);
   elemY[7] = domain.y(nd7i);

   elemZ[0] = domain.z(nd0i);
   elemZ[1] = domain.z(nd1i);
   elemZ[2] = domain.z(nd2i);
   elemZ[3] = domain.z(nd3i);
   elemZ[4] = domain.z(nd4i);
   elemZ[5] = domain.z(nd5i);
   elemZ[6] = domain.z(nd6i);
   elemZ[7] = domain.z(nd7i);

}

/******************************************/

static inline
void InitStressTermsForElems(Domain &domain,
                             Real_t *sigxx, Real_t *sigyy, Real_t *sigzz,
                             Index_t numElem)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
   //
   // pull in the stresses appropriate to the hydro integration
   //


//#pragma omp parallel for firstprivate(numElem)
   for (Index_t i = 0 ; i < numElem ; ++i){
      sigxx[i] = sigyy[i] = sigzz[i] =  - domain.p(i) - domain.q(i) ;
   }

}

/******************************************/


static inline
void CalcElemShapeFunctionDerivatives( Real_t const x[],
                                       Real_t const y[],
                                       Real_t const z[],
                                       Real_t b[][8],
                                       Real_t* const volume )
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);


  const Real_t x0 = x[0] ;   const Real_t x1 = x[1] ;
  const Real_t x2 = x[2] ;   const Real_t x3 = x[3] ;
  const Real_t x4 = x[4] ;   const Real_t x5 = x[5] ;
  const Real_t x6 = x[6] ;   const Real_t x7 = x[7] ;

  const Real_t y0 = y[0] ;   const Real_t y1 = y[1] ;
  const Real_t y2 = y[2] ;   const Real_t y3 = y[3] ;
  const Real_t y4 = y[4] ;   const Real_t y5 = y[5] ;
  const Real_t y6 = y[6] ;   const Real_t y7 = y[7] ;

  const Real_t z0 = z[0] ;   const Real_t z1 = z[1] ;
  const Real_t z2 = z[2] ;   const Real_t z3 = z[3] ;
  const Real_t z4 = z[4] ;   const Real_t z5 = z[5] ;
  const Real_t z6 = z[6] ;   const Real_t z7 = z[7] ;

  Real_t fjxxi, fjxet, fjxze;
  Real_t fjyxi, fjyet, fjyze;
  Real_t fjzxi, fjzet, fjzze;
  Real_t cjxxi, cjxet, cjxze;
  Real_t cjyxi, cjyet, cjyze;
  Real_t cjzxi, cjzet, cjzze;

  fjxxi = Real_t(.125) * ( (x6-x0) + (x5-x3) - (x7-x1) - (x4-x2) );
  fjxet = Real_t(.125) * ( (x6-x0) - (x5-x3) + (x7-x1) - (x4-x2) );
  fjxze = Real_t(.125) * ( (x6-x0) + (x5-x3) + (x7-x1) + (x4-x2) );

  fjyxi = Real_t(.125) * ( (y6-y0) + (y5-y3) - (y7-y1) - (y4-y2) );
  fjyet = Real_t(.125) * ( (y6-y0) - (y5-y3) + (y7-y1) - (y4-y2) );
  fjyze = Real_t(.125) * ( (y6-y0) + (y5-y3) + (y7-y1) + (y4-y2) );

  fjzxi = Real_t(.125) * ( (z6-z0) + (z5-z3) - (z7-z1) - (z4-z2) );
  fjzet = Real_t(.125) * ( (z6-z0) - (z5-z3) + (z7-z1) - (z4-z2) );
  fjzze = Real_t(.125) * ( (z6-z0) + (z5-z3) + (z7-z1) + (z4-z2) );

  /* compute cofactors */
  cjxxi =    (fjyet * fjzze) - (fjzet * fjyze);
  cjxet =  - (fjyxi * fjzze) + (fjzxi * fjyze);
  cjxze =    (fjyxi * fjzet) - (fjzxi * fjyet);

  cjyxi =  - (fjxet * fjzze) + (fjzet * fjxze);
  cjyet =    (fjxxi * fjzze) - (fjzxi * fjxze);
  cjyze =  - (fjxxi * fjzet) + (fjzxi * fjxet);

  cjzxi =    (fjxet * fjyze) - (fjyet * fjxze);
  cjzet =  - (fjxxi * fjyze) + (fjyxi * fjxze);
  cjzze =    (fjxxi * fjyet) - (fjyxi * fjxet);

  /* calculate partials :
     this need only be done for l = 0,1,2,3   since , by symmetry ,
     (6,7,4,5) = - (0,1,2,3) .
  */
  b[0][0] =   -  cjxxi  -  cjxet  -  cjxze;
  b[0][1] =      cjxxi  -  cjxet  -  cjxze;
  b[0][2] =      cjxxi  +  cjxet  -  cjxze;
  b[0][3] =   -  cjxxi  +  cjxet  -  cjxze;
  b[0][4] = -b[0][2];
  b[0][5] = -b[0][3];
  b[0][6] = -b[0][0];
  b[0][7] = -b[0][1];

  b[1][0] =   -  cjyxi  -  cjyet  -  cjyze;
  b[1][1] =      cjyxi  -  cjyet  -  cjyze;
  b[1][2] =      cjyxi  +  cjyet  -  cjyze;
  b[1][3] =   -  cjyxi  +  cjyet  -  cjyze;
  b[1][4] = -b[1][2];
  b[1][5] = -b[1][3];
  b[1][6] = -b[1][0];
  b[1][7] = -b[1][1];

  b[2][0] =   -  cjzxi  -  cjzet  -  cjzze;
  b[2][1] =      cjzxi  -  cjzet  -  cjzze;
  b[2][2] =      cjzxi  +  cjzet  -  cjzze;
  b[2][3] =   -  cjzxi  +  cjzet  -  cjzze;
  b[2][4] = -b[2][2];
  b[2][5] = -b[2][3];
  b[2][6] = -b[2][0];
  b[2][7] = -b[2][1];

  /* calculate jacobian determinant (volume) */
  *volume = Real_t(8.) * ( fjxet * cjxet + fjyet * cjyet + fjzet * cjzet);

}

/******************************************/

static inline
void SumElemFaceNormal(Real_t *normalX0, Real_t *normalY0, Real_t *normalZ0,
                       Real_t *normalX1, Real_t *normalY1, Real_t *normalZ1,
                       Real_t *normalX2, Real_t *normalY2, Real_t *normalZ2,
                       Real_t *normalX3, Real_t *normalY3, Real_t *normalZ3,
                       const Real_t x0, const Real_t y0, const Real_t z0,
                       const Real_t x1, const Real_t y1, const Real_t z1,
                       const Real_t x2, const Real_t y2, const Real_t z2,
                       const Real_t x3, const Real_t y3, const Real_t z3)
{
   Real_t bisectX0 = Real_t(0.5) * (x3 + x2 - x1 - x0);
   Real_t bisectY0 = Real_t(0.5) * (y3 + y2 - y1 - y0);
   Real_t bisectZ0 = Real_t(0.5) * (z3 + z2 - z1 - z0);
   Real_t bisectX1 = Real_t(0.5) * (x2 + x1 - x3 - x0);
   Real_t bisectY1 = Real_t(0.5) * (y2 + y1 - y3 - y0);
   Real_t bisectZ1 = Real_t(0.5) * (z2 + z1 - z3 - z0);
   Real_t areaX = Real_t(0.25) * (bisectY0 * bisectZ1 - bisectZ0 * bisectY1);
   Real_t areaY = Real_t(0.25) * (bisectZ0 * bisectX1 - bisectX0 * bisectZ1);
   Real_t areaZ = Real_t(0.25) * (bisectX0 * bisectY1 - bisectY0 * bisectX1);

   *normalX0 += areaX;
   *normalX1 += areaX;
   *normalX2 += areaX;
   *normalX3 += areaX;

   *normalY0 += areaY;
   *normalY1 += areaY;
   *normalY2 += areaY;
   *normalY3 += areaY;

   *normalZ0 += areaZ;
   *normalZ1 += areaZ;
   *normalZ2 += areaZ;
   *normalZ3 += areaZ;
}

/******************************************/

static inline
void CalcElemNodeNormals(Real_t pfx[8],
                         Real_t pfy[8],
                         Real_t pfz[8],
                         const Real_t x[8],
                         const Real_t y[8],
                         const Real_t z[8])
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   for (Index_t i = 0 ; i < 8 ; ++i) {
      pfx[i] = Real_t(0.0);
      pfy[i] = Real_t(0.0);
      pfz[i] = Real_t(0.0);
   }
   /* evaluate face one: nodes 0, 1, 2, 3 */
   SumElemFaceNormal(&pfx[0], &pfy[0], &pfz[0],
                  &pfx[1], &pfy[1], &pfz[1],
                  &pfx[2], &pfy[2], &pfz[2],
                  &pfx[3], &pfy[3], &pfz[3],
                  x[0], y[0], z[0], x[1], y[1], z[1],
                  x[2], y[2], z[2], x[3], y[3], z[3]);
   /* evaluate face two: nodes 0, 4, 5, 1 */
   SumElemFaceNormal(&pfx[0], &pfy[0], &pfz[0],
                  &pfx[4], &pfy[4], &pfz[4],
                  &pfx[5], &pfy[5], &pfz[5],
                  &pfx[1], &pfy[1], &pfz[1],
                  x[0], y[0], z[0], x[4], y[4], z[4],
                  x[5], y[5], z[5], x[1], y[1], z[1]);
   /* evaluate face three: nodes 1, 5, 6, 2 */
   SumElemFaceNormal(&pfx[1], &pfy[1], &pfz[1],
                  &pfx[5], &pfy[5], &pfz[5],
                  &pfx[6], &pfy[6], &pfz[6],
                  &pfx[2], &pfy[2], &pfz[2],
                  x[1], y[1], z[1], x[5], y[5], z[5],
                  x[6], y[6], z[6], x[2], y[2], z[2]);
   /* evaluate face four: nodes 2, 6, 7, 3 */
   SumElemFaceNormal(&pfx[2], &pfy[2], &pfz[2],
                  &pfx[6], &pfy[6], &pfz[6],
                  &pfx[7], &pfy[7], &pfz[7],
                  &pfx[3], &pfy[3], &pfz[3],
                  x[2], y[2], z[2], x[6], y[6], z[6],
                  x[7], y[7], z[7], x[3], y[3], z[3]);
   /* evaluate face five: nodes 3, 7, 4, 0 */
   SumElemFaceNormal(&pfx[3], &pfy[3], &pfz[3],
                  &pfx[7], &pfy[7], &pfz[7],
                  &pfx[4], &pfy[4], &pfz[4],
                  &pfx[0], &pfy[0], &pfz[0],
                  x[3], y[3], z[3], x[7], y[7], z[7],
                  x[4], y[4], z[4], x[0], y[0], z[0]);
   /* evaluate face six: nodes 4, 7, 6, 5 */
   SumElemFaceNormal(&pfx[4], &pfy[4], &pfz[4],
                  &pfx[7], &pfy[7], &pfz[7],
                  &pfx[6], &pfy[6], &pfz[6],
                  &pfx[5], &pfy[5], &pfz[5],
                  x[4], y[4], z[4], x[7], y[7], z[7],
                  x[6], y[6], z[6], x[5], y[5], z[5]);

}

/******************************************/

static inline
void SumElemStressesToNodeForces( const Real_t B[][8],
                                  const Real_t stress_xx,
                                  const Real_t stress_yy,
                                  const Real_t stress_zz,
                                  Real_t fx[], Real_t fy[], Real_t fz[] )
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
   for(Index_t i = 0; i < 8; i++) {
      fx[i] = -( stress_xx * B[0][i] );
      fy[i] = -( stress_yy * B[1][i]  );
      fz[i] = -( stress_zz * B[2][i] );
   }
}

/******************************************/

static inline
void IntegrateStressForElems( Domain &domain,
                              Real_t *sigxx, Real_t *sigyy, Real_t *sigzz,
                              Real_t *determ, Index_t numElem, Index_t numNode)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);


#if _OPENMP
   Index_t numthreads = omp_get_max_threads();
#else
   Index_t numthreads = 1;
#endif

   Index_t numElem8 = numElem * 8 ;
   Real_t *fx_elem;
   Real_t *fy_elem;
   Real_t *fz_elem;
   Real_t fx_local[8] ;
   Real_t fy_local[8] ;
   Real_t fz_local[8] ;


  if (numthreads > 1) {
     fx_elem = Allocate<Real_t>(numElem8) ;
     fy_elem = Allocate<Real_t>(numElem8) ;
     fz_elem = Allocate<Real_t>(numElem8) ;
  }


#if _CD && (SWITCH_5_1_1  > SEQUENTIAL_CD)
   CDHandle *cdh_5_1_1 = GetCurrentCD()->Create(CD_MAP_5_1_1 >> CDFLAG_SIZE, 
                            (string("Loop_IntegrateStressForElems")+GetCurrentCD()->label()).c_str(), 
                            CD_MAP_5_1_1 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_5_1_1)); 
#elif _CD && (SWITCH_5_1_1 == SEQUENTIAL_CD)
   CDHandle *cdh_5_1_1 = GetCurrentCD();
   CD_Complete(cdh_5_1_1); 
#endif

  // loop over all elements
//#pragma omp parallel for firstprivate(numElem)
  for( Index_t k=0 ; k<numElem ; ++k )
  {
#if _CD && (SWITCH_5_1_1  > SEQUENTIAL_CD || SWITCH_5_1_1 == SEQUENTIAL_CD)
      cdh_5_1_1->Begin("Loop_IntegrateStressForElems"); 
      cdh_5_1_1->Preserve(&domain, sizeof(domain), kCopy, "locDom_Loop_IntegrateStressForElems"); 
      cdh_5_1_1->Preserve(domain.serdes.SetOp(M__SERDES_ALL), kCopy, "AllMembers_Loop_IntegrateStressForElems"); 
#endif

      const Index_t* const elemToNode = domain.nodelist(k);
      Real_t B[3][8] ;// shape function derivatives
      Real_t x_local[8] ;
      Real_t y_local[8] ;
      Real_t z_local[8] ;

#if _CD && (SWITCH_6_0_0  > SEQUENTIAL_CD)
      CDHandle *cdh_6_0_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_6_0_0, cdh_6_0_0, "CollectDomainNodesToElemNodes1", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_6_0_0 == SEQUENTIAL_CD)
      CDHandle *cdh_6_0_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_6_0_0, cdh_6_0_0, "CollectDomainNodesToElemNodes1", M__SERDES_ALL, kCopy);
#endif

      // get nodal coordinates from global arrays and copy into local arrays.
      CollectDomainNodesToElemNodes(domain, elemToNode, x_local, y_local, z_local);

#if _CD && (SWITCH_6_0_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_6_0_0, cdh_6_0_0);
#elif _CD && (SWITCH_6_0_0 == SEQUENTIAL_CD)
      cdh_6_0_0->Detect(); 
#endif

#if _CD && (SWITCH_6_1_0  > SEQUENTIAL_CD)
      CDHandle *cdh_6_1_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_6_1_0, cdh_6_1_0, "CalcElemShapeFunctionDerivatives1", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_6_1_0 == SEQUENTIAL_CD)
      CDHandle *cdh_6_1_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_6_1_0, cdh_6_1_0, "CalcElemShapeFunctionDerivatives1", M__SERDES_ALL, kCopy);
#endif
      // Volume calculation involves extra work for numerical consistency
      CalcElemShapeFunctionDerivatives(x_local, y_local, z_local,
                                         B, &determ[k]);
#if _CD && (SWITCH_6_1_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_6_1_0, cdh_6_1_0);
#elif _CD && (SWITCH_6_1_0 == SEQUENTIAL_CD)
      cdh_6_1_0->Detect(); 
#endif

#if _CD && (SWITCH_6_2_0  > SEQUENTIAL_CD)
      CDHandle *cdh_6_2_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_6_2_0, cdh_6_2_0, "CalcElemNodeNormals", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_6_2_0 == SEQUENTIAL_CD)
      CDHandle *cdh_6_2_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_6_2_0, cdh_6_2_0, "CalcElemNodeNormals", M__SERDES_ALL, kCopy);
#endif
      CalcElemNodeNormals( B[0] , B[1], B[2],
                           x_local, y_local, z_local );
#if _CD && (SWITCH_6_2_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_6_2_0, cdh_6_2_0);
#elif _CD && (SWITCH_6_2_0 == SEQUENTIAL_CD)
      cdh_6_2_0->Detect(); 
#endif


#if _CD && (SWITCH_6_3_0  > SEQUENTIAL_CD)
      CDHandle *cdh_6_3_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_6_3_0, cdh_6_3_0, "SumElemStressesToNodeForces", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_6_3_0 == SEQUENTIAL_CD)
      CDHandle *cdh_6_3_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_6_3_0, cdh_6_3_0, "SumElemStressesToNodeForces", M__SERDES_ALL, kCopy);
#endif

    if (numthreads > 1) {
       // Eliminate thread writing conflicts at the nodes by giving
       // each element its own copy to write to
       SumElemStressesToNodeForces( B, sigxx[k], sigyy[k], sigzz[k],
                                    &fx_elem[k*8],
                                    &fy_elem[k*8],
                                    &fz_elem[k*8] ) ;
    }
    else {
       SumElemStressesToNodeForces( B, sigxx[k], sigyy[k], sigzz[k],
                                    fx_local, fy_local, fz_local ) ;

       // copy nodal force contributions to global force arrray.
       for( Index_t lnode=0 ; lnode<8 ; ++lnode ) {
          Index_t gnode = elemToNode[lnode];
          domain.fx(gnode) += fx_local[lnode];
          domain.fy(gnode) += fy_local[lnode];
          domain.fz(gnode) += fz_local[lnode];
       }
    }
#if _CD && (SWITCH_6_3_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_6_3_0, cdh_6_3_0);
#elif _CD && (SWITCH_6_3_0 == SEQUENTIAL_CD)
      cdh_6_3_0->Detect(); 
#endif

#if _CD && (SWITCH_5_1_1  > SEQUENTIAL_CD || SWITCH_5_1_1 == SEQUENTIAL_CD)
     cdh_5_1_1->Detect();
     CD_Complete(cdh_5_1_1);
#endif

  }


  //FIXME CDBegin in the inner scope and CDComplete in the outer scope.
#if _CD && (SWITCH_5_1_1  > SEQUENTIAL_CD)
   cdh_5_1_1->Destroy();
#elif _CD && (SWITCH_5_1_1 == SEQUENTIAL_CD)
   cdh_5_1_1->Begin(false, "After_Loop_IntegrateStressForElems");
#endif



  if (numthreads > 1) {
     // If threaded, then we need to copy the data out of the temporary
     // arrays used above into the final forces field
//#pragma omp parallel for firstprivate(numNode)
     for( Index_t gnode=0 ; gnode<numNode ; ++gnode )
     {
        Index_t count = domain.nodeElemCount(gnode) ;
        Index_t *cornerList = domain.nodeElemCornerList(gnode) ;
        Real_t fx_tmp = Real_t(0.0) ;
        Real_t fy_tmp = Real_t(0.0) ;
        Real_t fz_tmp = Real_t(0.0) ;
        for (Index_t i=0 ; i < count ; ++i) {
           Index_t elem = cornerList[i] ;
           fx_tmp += fx_elem[elem] ;
           fy_tmp += fy_elem[elem] ;
           fz_tmp += fz_elem[elem] ;
        }
        domain.fx(gnode) = fx_tmp ;
        domain.fy(gnode) = fy_tmp ;
        domain.fz(gnode) = fz_tmp ;
     }
     Release(&fz_elem) ;
     Release(&fy_elem) ;
     Release(&fx_elem) ;
  }

}

/******************************************/

static inline
void VoluDer(const Real_t x0, const Real_t x1, const Real_t x2,
             const Real_t x3, const Real_t x4, const Real_t x5,
             const Real_t y0, const Real_t y1, const Real_t y2,
             const Real_t y3, const Real_t y4, const Real_t y5,
             const Real_t z0, const Real_t z1, const Real_t z2,
             const Real_t z3, const Real_t z4, const Real_t z5,
             Real_t* dvdx, Real_t* dvdy, Real_t* dvdz)
{
   const Real_t twelfth = Real_t(1.0) / Real_t(12.0) ;

   *dvdx =
      (y1 + y2) * (z0 + z1) - (y0 + y1) * (z1 + z2) +
      (y0 + y4) * (z3 + z4) - (y3 + y4) * (z0 + z4) -
      (y2 + y5) * (z3 + z5) + (y3 + y5) * (z2 + z5);
   *dvdy =
      - (x1 + x2) * (z0 + z1) + (x0 + x1) * (z1 + z2) -
      (x0 + x4) * (z3 + z4) + (x3 + x4) * (z0 + z4) +
      (x2 + x5) * (z3 + z5) - (x3 + x5) * (z2 + z5);

   *dvdz =
      - (y1 + y2) * (x0 + x1) + (y0 + y1) * (x1 + x2) -
      (y0 + y4) * (x3 + x4) + (y3 + y4) * (x0 + x4) +
      (y2 + y5) * (x3 + x5) - (y3 + y5) * (x2 + x5);

   *dvdx *= twelfth;
   *dvdy *= twelfth;
   *dvdz *= twelfth;
}

/******************************************/

static inline
void CalcElemVolumeDerivative(Real_t dvdx[8],
                              Real_t dvdy[8],
                              Real_t dvdz[8],
                              const Real_t x[8],
                              const Real_t y[8],
                              const Real_t z[8])
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);


   VoluDer(x[1], x[2], x[3], x[4], x[5], x[7],
           y[1], y[2], y[3], y[4], y[5], y[7],
           z[1], z[2], z[3], z[4], z[5], z[7],
           &dvdx[0], &dvdy[0], &dvdz[0]);
   VoluDer(x[0], x[1], x[2], x[7], x[4], x[6],
           y[0], y[1], y[2], y[7], y[4], y[6],
           z[0], z[1], z[2], z[7], z[4], z[6],
           &dvdx[3], &dvdy[3], &dvdz[3]);
   VoluDer(x[3], x[0], x[1], x[6], x[7], x[5],
           y[3], y[0], y[1], y[6], y[7], y[5],
           z[3], z[0], z[1], z[6], z[7], z[5],
           &dvdx[2], &dvdy[2], &dvdz[2]);
   VoluDer(x[2], x[3], x[0], x[5], x[6], x[4],
           y[2], y[3], y[0], y[5], y[6], y[4],
           z[2], z[3], z[0], z[5], z[6], z[4],
           &dvdx[1], &dvdy[1], &dvdz[1]);
   VoluDer(x[7], x[6], x[5], x[0], x[3], x[1],
           y[7], y[6], y[5], y[0], y[3], y[1],
           z[7], z[6], z[5], z[0], z[3], z[1],
           &dvdx[4], &dvdy[4], &dvdz[4]);
   VoluDer(x[4], x[7], x[6], x[1], x[0], x[2],
           y[4], y[7], y[6], y[1], y[0], y[2],
           z[4], z[7], z[6], z[1], z[0], z[2],
           &dvdx[5], &dvdy[5], &dvdz[5]);
   VoluDer(x[5], x[4], x[7], x[2], x[1], x[3],
           y[5], y[4], y[7], y[2], y[1], y[3],
           z[5], z[4], z[7], z[2], z[1], z[3],
           &dvdx[6], &dvdy[6], &dvdz[6]);
   VoluDer(x[6], x[5], x[4], x[3], x[2], x[0],
           y[6], y[5], y[4], y[3], y[2], y[0],
           z[6], z[5], z[4], z[3], z[2], z[0],
           &dvdx[7], &dvdy[7], &dvdz[7]);

}

/******************************************/

static inline
void CalcElemFBHourglassForce(Real_t *xd, Real_t *yd, Real_t *zd,  Real_t hourgam[][4],
                              Real_t coefficient,
                              Real_t *hgfx, Real_t *hgfy, Real_t *hgfz )
{

   Real_t hxx[4];
   for(Index_t i = 0; i < 4; i++) {
      hxx[i] = hourgam[0][i] * xd[0] + hourgam[1][i] * xd[1] +
               hourgam[2][i] * xd[2] + hourgam[3][i] * xd[3] +
               hourgam[4][i] * xd[4] + hourgam[5][i] * xd[5] +
               hourgam[6][i] * xd[6] + hourgam[7][i] * xd[7];
   }
   for(Index_t i = 0; i < 8; i++) {
      hgfx[i] = coefficient *
                (hourgam[i][0] * hxx[0] + hourgam[i][1] * hxx[1] +
                 hourgam[i][2] * hxx[2] + hourgam[i][3] * hxx[3]);
   }
   for(Index_t i = 0; i < 4; i++) {
      hxx[i] = hourgam[0][i] * yd[0] + hourgam[1][i] * yd[1] +
               hourgam[2][i] * yd[2] + hourgam[3][i] * yd[3] +
               hourgam[4][i] * yd[4] + hourgam[5][i] * yd[5] +
               hourgam[6][i] * yd[6] + hourgam[7][i] * yd[7];
   }
   for(Index_t i = 0; i < 8; i++) {
      hgfy[i] = coefficient *
                (hourgam[i][0] * hxx[0] + hourgam[i][1] * hxx[1] +
                 hourgam[i][2] * hxx[2] + hourgam[i][3] * hxx[3]);
   }
   for(Index_t i = 0; i < 4; i++) {
      hxx[i] = hourgam[0][i] * zd[0] + hourgam[1][i] * zd[1] +
               hourgam[2][i] * zd[2] + hourgam[3][i] * zd[3] +
               hourgam[4][i] * zd[4] + hourgam[5][i] * zd[5] +
               hourgam[6][i] * zd[6] + hourgam[7][i] * zd[7];
   }
   for(Index_t i = 0; i < 8; i++) {
      hgfz[i] = coefficient *
                (hourgam[i][0] * hxx[0] + hourgam[i][1] * hxx[1] +
                 hourgam[i][2] * hxx[2] + hourgam[i][3] * hxx[3]);
   }

}

/******************************************/

static inline
void CalcFBHourglassForceForElems( Domain &domain,
                                   Real_t *determ,
                                   Real_t *x8n, Real_t *y8n, Real_t *z8n,
                                   Real_t *dvdx, Real_t *dvdy, Real_t *dvdz,
                                   Real_t hourg, Index_t numElem,
                                   Index_t numNode)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

#if _OPENMP
   Index_t numthreads = omp_get_max_threads();
#else
   Index_t numthreads = 1;
#endif
   /*************************************************
    *
    *     FUNCTION: Calculates the Flanagan-Belytschko anti-hourglass
    *               force.
    *
    *************************************************/
  
   Index_t numElem8 = numElem * 8 ;

   Real_t *fx_elem; 
   Real_t *fy_elem; 
   Real_t *fz_elem; 

   if(numthreads > 1) {
      fx_elem = Allocate<Real_t>(numElem8) ;
      fy_elem = Allocate<Real_t>(numElem8) ;
      fz_elem = Allocate<Real_t>(numElem8) ;
   }

   Real_t  gamma[4][8];

   gamma[0][0] = Real_t( 1.);
   gamma[0][1] = Real_t( 1.);
   gamma[0][2] = Real_t(-1.);
   gamma[0][3] = Real_t(-1.);
   gamma[0][4] = Real_t(-1.);
   gamma[0][5] = Real_t(-1.);
   gamma[0][6] = Real_t( 1.);
   gamma[0][7] = Real_t( 1.);
   gamma[1][0] = Real_t( 1.);
   gamma[1][1] = Real_t(-1.);
   gamma[1][2] = Real_t(-1.);
   gamma[1][3] = Real_t( 1.);
   gamma[1][4] = Real_t(-1.);
   gamma[1][5] = Real_t( 1.);
   gamma[1][6] = Real_t( 1.);
   gamma[1][7] = Real_t(-1.);
   gamma[2][0] = Real_t( 1.);
   gamma[2][1] = Real_t(-1.);
   gamma[2][2] = Real_t( 1.);
   gamma[2][3] = Real_t(-1.);
   gamma[2][4] = Real_t( 1.);
   gamma[2][5] = Real_t(-1.);
   gamma[2][6] = Real_t( 1.);
   gamma[2][7] = Real_t(-1.);
   gamma[3][0] = Real_t(-1.);
   gamma[3][1] = Real_t( 1.);
   gamma[3][2] = Real_t(-1.);
   gamma[3][3] = Real_t( 1.);
   gamma[3][4] = Real_t( 1.);
   gamma[3][5] = Real_t(-1.);
   gamma[3][6] = Real_t( 1.);
   gamma[3][7] = Real_t(-1.);


/*************************************************/
/*    compute the hourglass modes */

#if _CD && (SWITCH_7_0_0  > SEQUENTIAL_CD)
   CDHandle *cdh_7_0_0 = GetCurrentCD()->Create(CD_MAP_7_0_0 >> CDFLAG_SIZE, 
                            (string("Loop_CalcFBHourglassForceForElems")+GetCurrentCD()->label()).c_str(), 
                            CD_MAP_7_0_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_7_0_0)); 
#elif _CD && (SWITCH_7_0_0 == SEQUENTIAL_CD)
   CDHandle *cdh_7_0_0 = GetCurrentCD();
   CD_Complete(cdh_7_0_0); 
#endif

//#pragma omp parallel for firstprivate(numElem, hourg)
   for(Index_t i2=0;i2<numElem;++i2) {

#if _CD && (SWITCH_7_0_0  > SEQUENTIAL_CD || SWITCH_7_0_0 == SEQUENTIAL_CD)
     cdh_7_0_0->Begin("Loop_CalcFBHourglassForceForElems"); 
     cdh_7_0_0->Preserve(&domain, sizeof(domain), kCopy, "locDom_Loop_CalcFBHourglassForceForElems"); 
     cdh_7_0_0->Preserve(domain.serdes.SetOp(M__SERDES_ALL), kCopy, "AllMembers_Loop_CalcFBHourglassForceForElems"); 
#endif

      Real_t *fx_local, *fy_local, *fz_local ;
      Real_t hgfx[8], hgfy[8], hgfz[8] ;

      Real_t coefficient;

      Real_t hourgam[8][4];
      Real_t xd1[8], yd1[8], zd1[8] ;

      const Index_t *elemToNode = domain.nodelist(i2);
      Index_t i3=8*i2;
      Real_t volinv=Real_t(1.0)/determ[i2];
      Real_t ss1, mass1, volume13 ;


      for(Index_t i1=0;i1<4;++i1){

         Real_t hourmodx =
            x8n[i3] * gamma[i1][0] + x8n[i3+1] * gamma[i1][1] +
            x8n[i3+2] * gamma[i1][2] + x8n[i3+3] * gamma[i1][3] +
            x8n[i3+4] * gamma[i1][4] + x8n[i3+5] * gamma[i1][5] +
            x8n[i3+6] * gamma[i1][6] + x8n[i3+7] * gamma[i1][7];

         Real_t hourmody =
            y8n[i3] * gamma[i1][0] + y8n[i3+1] * gamma[i1][1] +
            y8n[i3+2] * gamma[i1][2] + y8n[i3+3] * gamma[i1][3] +
            y8n[i3+4] * gamma[i1][4] + y8n[i3+5] * gamma[i1][5] +
            y8n[i3+6] * gamma[i1][6] + y8n[i3+7] * gamma[i1][7];

         Real_t hourmodz =
            z8n[i3] * gamma[i1][0] + z8n[i3+1] * gamma[i1][1] +
            z8n[i3+2] * gamma[i1][2] + z8n[i3+3] * gamma[i1][3] +
            z8n[i3+4] * gamma[i1][4] + z8n[i3+5] * gamma[i1][5] +
            z8n[i3+6] * gamma[i1][6] + z8n[i3+7] * gamma[i1][7];

         hourgam[0][i1] = gamma[i1][0] -  volinv*(dvdx[i3  ] * hourmodx +
                                                  dvdy[i3  ] * hourmody +
                                                  dvdz[i3  ] * hourmodz );

         hourgam[1][i1] = gamma[i1][1] -  volinv*(dvdx[i3+1] * hourmodx +
                                                  dvdy[i3+1] * hourmody +
                                                  dvdz[i3+1] * hourmodz );

         hourgam[2][i1] = gamma[i1][2] -  volinv*(dvdx[i3+2] * hourmodx +
                                                  dvdy[i3+2] * hourmody +
                                                  dvdz[i3+2] * hourmodz );

         hourgam[3][i1] = gamma[i1][3] -  volinv*(dvdx[i3+3] * hourmodx +
                                                  dvdy[i3+3] * hourmody +
                                                  dvdz[i3+3] * hourmodz );

         hourgam[4][i1] = gamma[i1][4] -  volinv*(dvdx[i3+4] * hourmodx +
                                                  dvdy[i3+4] * hourmody +
                                                  dvdz[i3+4] * hourmodz );

         hourgam[5][i1] = gamma[i1][5] -  volinv*(dvdx[i3+5] * hourmodx +
                                                  dvdy[i3+5] * hourmody +
                                                  dvdz[i3+5] * hourmodz );

         hourgam[6][i1] = gamma[i1][6] -  volinv*(dvdx[i3+6] * hourmodx +
                                                  dvdy[i3+6] * hourmody +
                                                  dvdz[i3+6] * hourmodz );

         hourgam[7][i1] = gamma[i1][7] -  volinv*(dvdx[i3+7] * hourmodx +
                                                  dvdy[i3+7] * hourmody +
                                                  dvdz[i3+7] * hourmodz );

      }


      /* compute forces */
      /* store forces into h arrays (force arrays) */

      ss1=domain.ss(i2);
      mass1=domain.elemMass(i2);
      volume13=CBRT(determ[i2]);

      Index_t n0si2 = elemToNode[0];
      Index_t n1si2 = elemToNode[1];
      Index_t n2si2 = elemToNode[2];
      Index_t n3si2 = elemToNode[3];
      Index_t n4si2 = elemToNode[4];
      Index_t n5si2 = elemToNode[5];
      Index_t n6si2 = elemToNode[6];
      Index_t n7si2 = elemToNode[7];

      xd1[0] = domain.xd(n0si2);
      xd1[1] = domain.xd(n1si2);
      xd1[2] = domain.xd(n2si2);
      xd1[3] = domain.xd(n3si2);
      xd1[4] = domain.xd(n4si2);
      xd1[5] = domain.xd(n5si2);
      xd1[6] = domain.xd(n6si2);
      xd1[7] = domain.xd(n7si2);

      yd1[0] = domain.yd(n0si2);
      yd1[1] = domain.yd(n1si2);
      yd1[2] = domain.yd(n2si2);
      yd1[3] = domain.yd(n3si2);
      yd1[4] = domain.yd(n4si2);
      yd1[5] = domain.yd(n5si2);
      yd1[6] = domain.yd(n6si2);
      yd1[7] = domain.yd(n7si2);

      zd1[0] = domain.zd(n0si2);
      zd1[1] = domain.zd(n1si2);
      zd1[2] = domain.zd(n2si2);
      zd1[3] = domain.zd(n3si2);
      zd1[4] = domain.zd(n4si2);
      zd1[5] = domain.zd(n5si2);
      zd1[6] = domain.zd(n6si2);
      zd1[7] = domain.zd(n7si2);

      coefficient = - hourg * Real_t(0.01) * ss1 * mass1 / volume13;

#if _CD && (SWITCH_7_1_0  > SEQUENTIAL_CD)
      CDHandle *cdh_7_1_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_7_1_0, cdh_7_1_0, "CalcElemFBHourglassForce", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_7_1_0 == SEQUENTIAL_CD)
      CDHandle *cdh_7_1_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_7_1_0, cdh_7_1_0, "CalcElemFBHourglassForce", M__SERDES_ALL, kCopy);
#endif
      CalcElemFBHourglassForce(xd1,yd1,zd1,
                               hourgam,
                               coefficient, 
                               hgfx, hgfy, hgfz);

      // With the threaded version, we write into local arrays per elem
      // so we don't have to worry about race conditions
      if (numthreads > 1) {
         fx_local = &fx_elem[i3] ;
         fx_local[0] = hgfx[0];
         fx_local[1] = hgfx[1];
         fx_local[2] = hgfx[2];
         fx_local[3] = hgfx[3];
         fx_local[4] = hgfx[4];
         fx_local[5] = hgfx[5];
         fx_local[6] = hgfx[6];
         fx_local[7] = hgfx[7];

         fy_local = &fy_elem[i3] ;
         fy_local[0] = hgfy[0];
         fy_local[1] = hgfy[1];
         fy_local[2] = hgfy[2];
         fy_local[3] = hgfy[3];
         fy_local[4] = hgfy[4];
         fy_local[5] = hgfy[5];
         fy_local[6] = hgfy[6];
         fy_local[7] = hgfy[7];

         fz_local = &fz_elem[i3] ;
         fz_local[0] = hgfz[0];
         fz_local[1] = hgfz[1];
         fz_local[2] = hgfz[2];
         fz_local[3] = hgfz[3];
         fz_local[4] = hgfz[4];
         fz_local[5] = hgfz[5];
         fz_local[6] = hgfz[6];
         fz_local[7] = hgfz[7];
      }
      else {
         domain.fx(n0si2) += hgfx[0];
         domain.fy(n0si2) += hgfy[0];
         domain.fz(n0si2) += hgfz[0];

         domain.fx(n1si2) += hgfx[1];
         domain.fy(n1si2) += hgfy[1];
         domain.fz(n1si2) += hgfz[1];

         domain.fx(n2si2) += hgfx[2];
         domain.fy(n2si2) += hgfy[2];
         domain.fz(n2si2) += hgfz[2];

         domain.fx(n3si2) += hgfx[3];
         domain.fy(n3si2) += hgfy[3];
         domain.fz(n3si2) += hgfz[3];

         domain.fx(n4si2) += hgfx[4];
         domain.fy(n4si2) += hgfy[4];
         domain.fz(n4si2) += hgfz[4];

         domain.fx(n5si2) += hgfx[5];
         domain.fy(n5si2) += hgfy[5];
         domain.fz(n5si2) += hgfz[5];

         domain.fx(n6si2) += hgfx[6];
         domain.fy(n6si2) += hgfy[6];
         domain.fz(n6si2) += hgfz[6];

         domain.fx(n7si2) += hgfx[7];
         domain.fy(n7si2) += hgfy[7];
         domain.fz(n7si2) += hgfz[7];
      }


#if _CD && (SWITCH_7_1_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_7_1_0, cdh_7_1_0);
#elif _CD && (SWITCH_7_1_0 == SEQUENTIAL_CD)
      cdh_7_1_0->Detect(); 
#endif


#if _CD && (SWITCH_7_0_0  > SEQUENTIAL_CD || SWITCH_7_0_0 == SEQUENTIAL_CD)
     cdh_7_0_0->Detect();
     CD_Complete(cdh_7_0_0);
#endif

   }  // loop for CD7 ends

  //FIXME CDBegin in the inner scope and CDComplete in the outer scope.
#if _CD && (SWITCH_7_0_0  > SEQUENTIAL_CD)
   cdh_7_0_0->Destroy();
#elif _CD && (SWITCH_7_0_0 == SEQUENTIAL_CD)
   cdh_7_0_0->Begin(false, "After_Loop_CalcFBHourglassForceForElems");
#endif

   if (numthreads > 1) {
     // Collect the data from the local arrays into the final force arrays
//#pragma omp parallel for firstprivate(numNode)
      for( Index_t gnode=0 ; gnode<numNode ; ++gnode )
      {
         Index_t count = domain.nodeElemCount(gnode) ;
         Index_t *cornerList = domain.nodeElemCornerList(gnode) ;
         Real_t fx_tmp = Real_t(0.0) ;
         Real_t fy_tmp = Real_t(0.0) ;
         Real_t fz_tmp = Real_t(0.0) ;
         for (Index_t i=0 ; i < count ; ++i) {
            Index_t elem = cornerList[i] ;
            fx_tmp += fx_elem[elem] ;
            fy_tmp += fy_elem[elem] ;
            fz_tmp += fz_elem[elem] ;
         }
         domain.fx(gnode) += fx_tmp ;
         domain.fy(gnode) += fy_tmp ;
         domain.fz(gnode) += fz_tmp ;
      }
      Release(&fz_elem) ;
      Release(&fy_elem) ;
      Release(&fx_elem) ;
   }

}

/******************************************/

static inline
void CalcHourglassControlForElems(Domain& domain,
                                  Real_t determ[], Real_t hgcoef)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);


   Index_t numElem = domain.numElem() ;
   Index_t numElem8 = numElem * 8 ;
   Real_t *dvdx = Allocate<Real_t>(numElem8) ;
   Real_t *dvdy = Allocate<Real_t>(numElem8) ;
   Real_t *dvdz = Allocate<Real_t>(numElem8) ;
   Real_t *x8n  = Allocate<Real_t>(numElem8) ;
   Real_t *y8n  = Allocate<Real_t>(numElem8) ;
   Real_t *z8n  = Allocate<Real_t>(numElem8) ;

#if _CD && (SWITCH_6_4_0  > SEQUENTIAL_CD)
   CDHandle *cdh_6_4_0 = GetCurrentCD()->Create(CD_MAP_6_4_0 >> CDFLAG_SIZE, 
                            (string("Loop_CalcHourglassControlForElems")+GetCurrentCD()->label()).c_str(), 
                            CD_MAP_6_4_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_6_4_0)); 
#elif _CD && (SWITCH_6_4_0 == SEQUENTIAL_CD)
   CDHandle *cdh_6_4_0 = GetCurrentCD();
   CD_Complete(cdh_6_4_0); 
#endif

   /* start loop over elements */
//#pragma omp parallel for firstprivate(numElem)
   for (Index_t i=0 ; i<numElem ; ++i){

#if _CD && (SWITCH_6_4_0  > SEQUENTIAL_CD || SWITCH_6_4_0 == SEQUENTIAL_CD)
      CD_BEGIN(cdh_6_4_0, "Loop_CalcHourglassControlForElems"); 
      cdh_6_4_0->Preserve(&domain, sizeof(domain), kCopy, "locDom_Loop_CalcHourglassControlForElems"); 
      cdh_6_4_0->Preserve(domain.serdes.SetOp(M__SERDES_ALL), kCopy, "AllMembers_Loop_CalcHourglassControlForElems"); 
#endif

      Real_t  x1[8],  y1[8],  z1[8] ;
      Real_t pfx[8], pfy[8], pfz[8] ;

      Index_t* elemToNode = domain.nodelist(i);

#if _CD && (SWITCH_6_5_0  > SEQUENTIAL_CD)
      CDHandle *cdh_6_5_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_6_5_0, cdh_6_5_0, "CollectDomainNodesToElemNodes2", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_6_5_0 == SEQUENTIAL_CD)
      CDHandle *cdh_6_5_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_6_5_0, cdh_6_5_0, "CollectDomainNodesToElemNodes2", M__SERDES_ALL, kCopy);
#endif
      CollectDomainNodesToElemNodes(domain, elemToNode, x1, y1, z1);

#if _CD && (SWITCH_6_5_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_6_5_0, cdh_6_5_0);
#elif _CD && (SWITCH_6_5_0 == SEQUENTIAL_CD)
      cdh_6_5_0->Detect(); 
#endif



#if _CD && (SWITCH_6_6_0  > SEQUENTIAL_CD)
      CDHandle *cdh_6_6_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_6_6_0, cdh_6_6_0, "CalcElemVolumeDerivative", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_6_6_0 == SEQUENTIAL_CD)
      CDHandle *cdh_6_6_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_6_6_0, cdh_6_6_0, "CalcElemVolumeDerivative", M__SERDES_ALL, kCopy);
#endif

      CalcElemVolumeDerivative(pfx, pfy, pfz, x1, y1, z1);

      /* load into temporary storage for FB Hour Glass control */
      for(Index_t ii=0;ii<8;++ii){
         Index_t jj=8*i+ii;

         dvdx[jj] = pfx[ii];
         dvdy[jj] = pfy[ii];
         dvdz[jj] = pfz[ii];
         x8n[jj]  = x1[ii];
         y8n[jj]  = y1[ii];
         z8n[jj]  = z1[ii];
      }

      determ[i] = domain.volo(i) * domain.v(i);


#if _CD && (SWITCH_6_6_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_6_6_0, cdh_6_6_0);
#elif _CD && (SWITCH_6_6_0 == SEQUENTIAL_CD)
      cdh_6_6_0->Detect(); 
#endif


      /* Do a check for negative volumes */
      if ( domain.v(i) <= Real_t(0.0) ) {
//#if USE_MPI         
//         MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
//#else
//         exit(VolumeError);
//#endif
      }  // check ends

#if _CD && (SWITCH_6_4_0  > SEQUENTIAL_CD || SWITCH_6_4_0 == SEQUENTIAL_CD)
     cdh_6_4_0->Detect();
     CD_Complete(cdh_6_4_0);
#endif
   }  // loop ends

  //FIXME CDBegin in the inner scope and CDComplete in the outer scope.
#if _CD && (SWITCH_6_4_0  > SEQUENTIAL_CD)
   cdh_6_4_0->Destroy();
#elif _CD && (SWITCH_6_4_0 == SEQUENTIAL_CD) 
   CD_BEGIN(cdh_6_4_0, false, "After_Loop_CalcHourglassControlForElems");
#endif

#if _CD && (SWITCH_6_7_0  > SEQUENTIAL_CD)
   CDHandle *cdh_6_7_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_6_7_0, cdh_6_7_0, "CalcFBHourglassForceForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_6_7_0 == SEQUENTIAL_CD)
   CDHandle *cdh_6_7_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_6_7_0, cdh_6_7_0, "CalcFBHourglassForceForElems", M__SERDES_ALL, kCopy);
#endif
   if ( hgcoef > Real_t(0.) ) {
      CalcFBHourglassForceForElems( domain,
                                    determ, x8n, y8n, z8n, dvdx, dvdy, dvdz,
                                    hgcoef, numElem, domain.numNode()) ;
   }

#if _CD && (SWITCH_6_7_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_6_7_0, cdh_6_7_0);
#elif _CD && (SWITCH_6_7_0 == SEQUENTIAL_CD)
   cdh_6_7_0->Detect(); 
#endif

   Release(&z8n) ;
   Release(&y8n) ;
   Release(&x8n) ;
   Release(&dvdz) ;
   Release(&dvdy) ;
   Release(&dvdx) ;


   return ;
}

/******************************************/

static inline
void CalcVolumeForceForElems(Domain& domain)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   Index_t numElem = domain.numElem() ;
   if (numElem != 0) {
      Real_t  hgcoef = domain.hgcoef() ;
      Real_t *sigxx  = Allocate<Real_t>(numElem) ;
      Real_t *sigyy  = Allocate<Real_t>(numElem) ;
      Real_t *sigzz  = Allocate<Real_t>(numElem) ;
      Real_t *determ = Allocate<Real_t>(numElem) ;

#if _CD && (SWITCH_5_0_0  > SEQUENTIAL_CD)
      CDHandle *cdh_5_0_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_5_0_0, cdh_5_0_0, "InitStressTermsForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_0_0 == SEQUENTIAL_CD)
      CDHandle *cdh_5_0_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_0_0, cdh_5_0_0, "InitStressTermsForElems", M__SERDES_ALL, kCopy);
#endif
      /* Sum contributions to total stress tensor */
      InitStressTermsForElems(domain, sigxx, sigyy, sigzz, numElem);

#if _CD && (SWITCH_5_0_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_5_0_0, cdh_5_0_0);
#elif _CD && (SWITCH_5_0_0 == SEQUENTIAL_CD)
      cdh_5_0_0->Detect(); 
#endif


#if _CD && (SWITCH_5_1_0  > SEQUENTIAL_CD)
      CDHandle *cdh_5_1_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_5_1_0, cdh_5_1_0, "IntegrateStressForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_1_0 == SEQUENTIAL_CD)
      CDHandle *cdh_5_1_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_1_0, cdh_5_1_0, "IntegrateStressForElems", M__SERDES_ALL, kCopy);
#endif
      // call elemlib stress integration loop to produce nodal forces from
      // material stresses.
      IntegrateStressForElems( domain,
                               sigxx, sigyy, sigzz, determ, numElem,
                               domain.numNode()) ;

#if _CD && (SWITCH_5_1_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_5_1_0, cdh_5_1_0);
#elif _CD && (SWITCH_5_1_0 == SEQUENTIAL_CD)
      cdh_5_1_0->Detect(); 
#endif


      // check for negative element volume
//#pragma omp parallel for firstprivate(numElem)
      for ( Index_t k=0 ; k<numElem ; ++k ) {
         if (determ[k] <= Real_t(0.0)) {
//#if USE_MPI            
//            MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
//#else
//            exit(VolumeError);
//#endif
         }
      }


#if _CD && (SWITCH_5_2_0  > SEQUENTIAL_CD)
      CDHandle *cdh_5_2_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_5_2_0, cdh_5_2_0, "CalcHourglassControlForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_2_0 == SEQUENTIAL_CD)
      CDHandle *cdh_5_2_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_2_0, cdh_5_2_0, "CalcHourglassControlForElems", M__SERDES_ALL, kCopy);
#endif

      CalcHourglassControlForElems(domain, determ, hgcoef) ;

#if _CD && (SWITCH_5_2_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_5_2_0, cdh_5_2_0);
#elif _CD && (SWITCH_5_2_0 == SEQUENTIAL_CD)
      cdh_5_2_0->Detect(); 
#endif

      Release(&determ) ;
      Release(&sigzz) ;
      Release(&sigyy) ;
      Release(&sigxx) ;
   }

}

/******************************************/

static inline void CalcForceForNodes(Domain& domain)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   Index_t numNode = domain.numNode() ;

#if USE_MPI  
  CommRecv(domain, MSG_COMM_SBN, 3,
           domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
           true, false) ;
#endif 

#if _CD && (SWITCH_3_0_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_0_0 = 0;
   cdh_3_0_0 = GetLeafCD()->Create(CD_MAP_3_0_0 >> CDFLAG_SIZE, 
                                  (string("CalcForceForNodes")+GetCurrentCD()->label()).c_str(), 
                                   CD_MAP_3_0_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_3_0_0)); 
   CD_BEGIN(cdh_3_0_0, "CalcForceForNodes"); 
   cdh_3_0_0->Preserve(domain.serdes.SetOp(
                              (M__FX | M__FY | M__FZ |
                               M__X  | M__Y  | M__Z  |
                               M__XD | M__YD | M__ZD |
                               M__XDD| M__YDD| M__ZDD )
                              ), kCopy | kSerdes, "SWITCH_3_0_0_CalcForceForNodes"); 
//#elif _CD && (SWITCH_3_0_0 == SEQUENTIAL_CD)
//   CDHandle *cdh_3_0_0 = GetLeafCD();
//   CD_BEGIN(cdh_3_0_0, "CalcForceForNodes"); 
//   cdh_3_0_0->Preserve(domain.serdes.SetOp(
//                              (M__FX | M__FY | M__FZ |
//                               M__X  | M__Y  | M__Z  |
//                               M__XD | M__YD | M__ZD |
//                               M__XDD| M__YDD| M__ZDD )
//                              ), kCopy | kSerdes, "SWITCH_3_0_0_CalcForceForNodes"); 
#endif


//#pragma omp parallel for firstprivate(numNode)
  for (Index_t i=0; i<numNode; ++i) {
     domain.fx(i) = Real_t(0.0) ;
     domain.fy(i) = Real_t(0.0) ;
     domain.fz(i) = Real_t(0.0) ;
  }

#if _CD && (SWITCH_4_0_0  > SEQUENTIAL_CD)
   CDHandle *cdh_4_0_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_4_0_0, cdh_4_0_0, "CalcVolumeForceForElems", 
                              (M__FX | M__FY | M__FZ 
                              ), 
                              kCopy);
//#elif _CD && (SWITCH_4_0_0 == SEQUENTIAL_CD)
//   CDHandle *cdh_4_0_0 = GetCurrentCD();
//   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_4_0_0, cdh_4_0_0, "CalcVolumeForceForElems", 
//                              (M__FX | M__FY | M__FZ 
//                              ), 
//                              kCopy);
#endif

  /* Calcforce calls partial, force, hourq */
  CalcVolumeForceForElems(domain) ;

#if _CD && (SWITCH_4_0_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_4_0_0, cdh_4_0_0);
//#elif _CD && (SWITCH_4_0_0 == SEQUENTIAL_CD)
//   cdh_4_0_0->Detect(); 
#endif


#if _CD && (SWITCH_3_0_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_0_0, cdh_3_0_0);
//#elif _CD && (SWITCH_3_0_0 == SEQUENTIAL_CD)
//   cdh_3_0_0->Detect(); 
//   CD_Complete(cdh_3_0_0);
#endif



#if USE_MPI  
  Domain_member fieldData[3] ;
  fieldData[0] = &Domain::fx ;
  fieldData[1] = &Domain::fy ;
  fieldData[2] = &Domain::fz ;
  
  CommSend(domain, MSG_COMM_SBN, 3, fieldData,
           domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() +  1,
           true, false) ;
  CommSBN(domain, 3, fieldData) ;
#endif  

}

/******************************************/

static inline
void CalcAccelerationForNodes(Domain &domain, Index_t numNode)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
   

//#pragma omp parallel for firstprivate(numNode)
   for (Index_t i = 0; i < numNode; ++i) {
      domain.xdd(i) = domain.fx(i) / domain.nodalMass(i);
      domain.ydd(i) = domain.fy(i) / domain.nodalMass(i);
      domain.zdd(i) = domain.fz(i) / domain.nodalMass(i);
   }

}

/******************************************/

static inline
void ApplyAccelerationBoundaryConditionsForNodes(Domain& domain)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   Index_t size = domain.sizeX();
   Index_t numNodeBC = (size+1)*(size+1) ;

//#pragma omp parallel
   {
      if (!domain.symmXempty() != 0) {
//#pragma omp for nowait firstprivate(numNodeBC)
         for(Index_t i=0 ; i<numNodeBC ; ++i)
            domain.xdd(domain.symmX(i)) = Real_t(0.0) ;
      }

      if (!domain.symmYempty() != 0) {
//#pragma omp for nowait firstprivate(numNodeBC)
         for(Index_t i=0 ; i<numNodeBC ; ++i)
            domain.ydd(domain.symmY(i)) = Real_t(0.0) ;
      }

      if (!domain.symmZempty() != 0) {
//#pragma omp for nowait firstprivate(numNodeBC)
         for(Index_t i=0 ; i<numNodeBC ; ++i)
            domain.zdd(domain.symmZ(i)) = Real_t(0.0) ;
      }
   }

}

/******************************************/

static inline
void CalcVelocityForNodes(Domain &domain, const Real_t dt, const Real_t u_cut,
                          Index_t numNode)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

//#pragma omp parallel for firstprivate(numNode)
   for ( Index_t i = 0 ; i < numNode ; ++i )
   {
     Real_t xdtmp, ydtmp, zdtmp ;

     xdtmp = domain.xd(i) + domain.xdd(i) * dt ;
     if( FABS(xdtmp) < u_cut ) xdtmp = Real_t(0.0);
     domain.xd(i) = xdtmp ;

     ydtmp = domain.yd(i) + domain.ydd(i) * dt ;
     if( FABS(ydtmp) < u_cut ) ydtmp = Real_t(0.0);
     domain.yd(i) = ydtmp ;

     zdtmp = domain.zd(i) + domain.zdd(i) * dt ;
     if( FABS(zdtmp) < u_cut ) zdtmp = Real_t(0.0);
     domain.zd(i) = zdtmp ;
   }

}

/******************************************/

static inline
void CalcPositionForNodes(Domain &domain, const Real_t dt, Index_t numNode)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

//#pragma omp parallel for firstprivate(numNode)
   for ( Index_t i = 0 ; i < numNode ; ++i )
   {
     domain.x(i) += domain.xd(i) * dt ;
     domain.y(i) += domain.yd(i) * dt ;
     domain.z(i) += domain.zd(i) * dt ;
   }

}

/******************************************/

static inline
void LagrangeNodal(Domain& domain)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);


#ifdef SEDOV_SYNC_POS_VEL_EARLY
   Domain_member fieldData[6] ;
#endif

   const Real_t delt = domain.deltatime() ;
   Real_t u_cut = domain.u_cut() ;

//#if _CD && (SWITCH_3_0_0  > SEQUENTIAL_CD)
//   CDHandle *cdh_3_0_0 = 0;
//    assert(0);
//   cdh_3_0_0 = GetLeafCD()->Create(CD_MAP_3_0_0 >> CDFLAG_SIZE, 
//                                  (string("CalcForceForNodes")+GetCurrentCD()->label()).c_str(), 
//                                   CD_MAP_3_0_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_3_0_0)); 
//   CD_BEGIN(cdh_3_0_0, "CalcForceForNodes"); 
//   cdh_3_0_0->Preserve(domain.serdes.SetOp(
//                              (M__FX | M__FY | M__FZ |
//                               M__X  | M__Y  | M__Z  |
//                               M__XD | M__YD | M__ZD |
//                               M__XDD| M__YDD| M__ZDD )
//                              ), kCopy | kSerdes, "SWITCH_3_0_0_CalcForceForNodes"); 
//#elif _CD && (SWITCH_3_0_0 == SEQUENTIAL_CD)
//   CDHandle *cdh_3_0_0 = GetLeafCD();
//   CD_BEGIN(cdh_3_0_0, "CalcForceForNodes"); 
//   cdh_3_0_0->Preserve(domain.serdes.SetOp(
//                              (M__FX | M__FY | M__FZ |
//                               M__X  | M__Y  | M__Z  |
//                               M__XD | M__YD | M__ZD |
//                               M__XDD| M__YDD| M__ZDD )
//                              ), kCopy | kSerdes, "SWITCH_3_0_0_CalcForceForNodes"); 
//   cdh_3_0_0->Preserve(&(domain.deltatime()), sizeof(Real_t), kCopy, "deltatime"); 
//#endif

   /* time of boundary condition evaluation is beginning of step for force and
    * acceleration boundary conditions. */
   CalcForceForNodes(domain); 


#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD) && OPTIMIZE
   if(interval != 0 && ckpt_idx % interval == 0) 
   {
      st.BeginTimer();
      GetCurrentCD()->Preserve(domain.serdes.SetOp(preserve_vec_0), kCopy, "AfterCalcForceForNodes");
      st.EndTimer(0);
#ifdef _DEBUG_PRV
      if(st.myRank == 0) {
        printf("preserve vec 0 AfterCalcForceForNodes\n");
      }
#endif
   }

#endif



#if USE_MPI  
#ifdef SEDOV_SYNC_POS_VEL_EARLY
   CommRecv(domain, MSG_SYNC_POS_VEL, 6,
            domain.sizeX + 1, domain.sizeY + 1, domain.sizeZ + 1,
            false, false) ;
#endif
#endif

//#if _CD && (SWITCH_3_0_0  > SEQUENTIAL_CD)
//   CDMAPPING_END_NESTED(CD_MAP_3_0_0, cdh_3_0_0);
//#elif _CD && (SWITCH_3_0_0 == SEQUENTIAL_CD)
//   cdh_3_0_0->Detect(); 
//#endif


#if _CD && (SWITCH_3_1_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_1_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_1_0, cdh_3_1_0, "CalcAccelerationForNodes",
                              (M__FX | M__FY | M__FZ |
                               M__X  | M__Y  | M__Z  |
                               M__XD | M__YD | M__ZD |
                               M__XDD| M__YDD| M__ZDD),
                           kCopy);
#elif _CD && (SWITCH_3_1_0 == SEQUENTIAL_CD)
   CDHandle *cdh_3_1_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_1_0, cdh_3_1_0, "CalcAccelerationForNodes",
                              (M__FX | M__FY | M__FZ |
                               M__X  | M__Y  | M__Z  |
                               M__XD | M__YD | M__ZD |
                               M__XDD| M__YDD| M__ZDD),
                           kCopy);
#endif

   CalcAccelerationForNodes(domain, domain.numNode());

//#if _CD && (SWITCH_3_1_0  > SEQUENTIAL_CD)
//   CDMAPPING_END_NESTED(CD_MAP_3_1_0, cdh_3_1_0);
//#elif _CD && (SWITCH_3_1_0 == SEQUENTIAL_CD)
//   cdh_3_1_0->Detect(); 
//#endif
   

   
#if _CD && (SWITCH_3_2_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_2_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_2_0, cdh_3_2_0, "ApplyAccelerationBoundaryConditionsForNodes", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_3_2_0 == SEQUENTIAL_CD)
   CDHandle *cdh_3_2_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_2_0, cdh_3_2_0, "ApplyAccelerationBoundaryConditionsForNodes", M__SERDES_ALL, kCopy);
#endif
   ApplyAccelerationBoundaryConditionsForNodes(domain);

#if _CD && (SWITCH_3_2_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_2_0, cdh_3_2_0);
#elif _CD && (SWITCH_3_2_0 == SEQUENTIAL_CD)
   cdh_3_2_0->Detect(); 
#endif



#if _CD && (SWITCH_3_3_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_3_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_3_0, cdh_3_3_0, "CalcVelocityForNodes", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_3_3_0 == SEQUENTIAL_CD)
   CDHandle *cdh_3_3_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_3_0, cdh_3_3_0, "CalcVelocityForNodes", M__SERDES_ALL, kCopy);
#endif
   CalcVelocityForNodes( domain, delt, u_cut, domain.numNode()) ;
#if _CD && (SWITCH_3_3_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_3_0, cdh_3_3_0);
#elif _CD && (SWITCH_3_3_0 == SEQUENTIAL_CD)
   cdh_3_3_0->Detect(); 
#endif



#if _CD && (SWITCH_3_4_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_4_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_4_0, cdh_3_4_0, "CalcPositionForNodes", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_3_4_0 == SEQUENTIAL_CD)
   CDHandle *cdh_3_4_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_4_0, cdh_3_4_0, "CalcPositionForNodes", M__SERDES_ALL, kCopy);
#endif
   CalcPositionForNodes( domain, delt, domain.numNode() );
#if _CD && (SWITCH_3_4_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_4_0, cdh_3_4_0);
#elif _CD && (SWITCH_3_4_0 == SEQUENTIAL_CD)
   cdh_3_4_0->Detect(); 
#endif

#if _CD && (SWITCH_3_1_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_1_0, cdh_3_1_0);
#elif _CD && (SWITCH_3_1_0 == SEQUENTIAL_CD)
   cdh_3_1_0->Detect(); 
   CD_Complete(cdh_3_1_0); 
#endif
//#if _CD && (SWITCH_3_0_0  > SEQUENTIAL_CD)
//   CDMAPPING_END_NESTED(CD_MAP_3_0_0, cdh_3_0_0);
//#elif _CD && (SWITCH_3_0_0 == SEQUENTIAL_CD)
//   cdh_3_0_0->Detect(); 
//#endif



#if USE_MPI
#ifdef SEDOV_SYNC_POS_VEL_EARLY
   fieldData[0] = &Domain::x ;
   fieldData[1] = &Domain::y ;
   fieldData[2] = &Domain::z ;
   fieldData[3] = &Domain::xd ;
   fieldData[4] = &Domain::yd ;
   fieldData[5] = &Domain::zd ;

   CommSend(domain, MSG_SYNC_POS_VEL, 6, fieldData,
            domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
            false, false) ;
   CommSyncPosVel(domain) ;
#endif
#endif

#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD) && OPTIMIZE
   if(interval != 0 && ckpt_idx % interval == 0) 
   {
      st.BeginTimer();
      GetCurrentCD()->Preserve(domain.serdes.SetOp(preserve_vec_1), kCopy, "AfterCalcPositionForNodes");
      st.EndTimer(1);
#ifdef _DEBUG_PRV
      if(st.myRank == 0) {
        printf("preserve vec 1 AfterCalcPositionForNodes\n");
      }
#endif
   }

#endif

   return;
}

/******************************************/

static inline
Real_t CalcElemVolume( const Real_t x0, const Real_t x1,
               const Real_t x2, const Real_t x3,
               const Real_t x4, const Real_t x5,
               const Real_t x6, const Real_t x7,
               const Real_t y0, const Real_t y1,
               const Real_t y2, const Real_t y3,
               const Real_t y4, const Real_t y5,
               const Real_t y6, const Real_t y7,
               const Real_t z0, const Real_t z1,
               const Real_t z2, const Real_t z3,
               const Real_t z4, const Real_t z5,
               const Real_t z6, const Real_t z7 )
{
   //DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

  Real_t twelveth = Real_t(1.0)/Real_t(12.0);

  Real_t dx61 = x6 - x1;
  Real_t dy61 = y6 - y1;
  Real_t dz61 = z6 - z1;

  Real_t dx70 = x7 - x0;
  Real_t dy70 = y7 - y0;
  Real_t dz70 = z7 - z0;

  Real_t dx63 = x6 - x3;
  Real_t dy63 = y6 - y3;
  Real_t dz63 = z6 - z3;

  Real_t dx20 = x2 - x0;
  Real_t dy20 = y2 - y0;
  Real_t dz20 = z2 - z0;

  Real_t dx50 = x5 - x0;
  Real_t dy50 = y5 - y0;
  Real_t dz50 = z5 - z0;

  Real_t dx64 = x6 - x4;
  Real_t dy64 = y6 - y4;
  Real_t dz64 = z6 - z4;

  Real_t dx31 = x3 - x1;
  Real_t dy31 = y3 - y1;
  Real_t dz31 = z3 - z1;

  Real_t dx72 = x7 - x2;
  Real_t dy72 = y7 - y2;
  Real_t dz72 = z7 - z2;

  Real_t dx43 = x4 - x3;
  Real_t dy43 = y4 - y3;
  Real_t dz43 = z4 - z3;

  Real_t dx57 = x5 - x7;
  Real_t dy57 = y5 - y7;
  Real_t dz57 = z5 - z7;

  Real_t dx14 = x1 - x4;
  Real_t dy14 = y1 - y4;
  Real_t dz14 = z1 - z4;

  Real_t dx25 = x2 - x5;
  Real_t dy25 = y2 - y5;
  Real_t dz25 = z2 - z5;

#define TRIPLE_PRODUCT(x1, y1, z1, x2, y2, z2, x3, y3, z3) \
   ((x1)*((y2)*(z3) - (z2)*(y3)) + (x2)*((z1)*(y3) - (y1)*(z3)) + (x3)*((y1)*(z2) - (z1)*(y2)))

  Real_t volume =
    TRIPLE_PRODUCT(dx31 + dx72, dx63, dx20,
       dy31 + dy72, dy63, dy20,
       dz31 + dz72, dz63, dz20) +
    TRIPLE_PRODUCT(dx43 + dx57, dx64, dx70,
       dy43 + dy57, dy64, dy70,
       dz43 + dz57, dz64, dz70) +
    TRIPLE_PRODUCT(dx14 + dx25, dx61, dx50,
       dy14 + dy25, dy61, dy50,
       dz14 + dz25, dz61, dz50);

#undef TRIPLE_PRODUCT

  volume *= twelveth;

  return volume ;
}

/******************************************/

//inline
Real_t CalcElemVolume( const Real_t x[8], const Real_t y[8], const Real_t z[8] )
{
return CalcElemVolume( x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],
                       y[0], y[1], y[2], y[3], y[4], y[5], y[6], y[7],
                       z[0], z[1], z[2], z[3], z[4], z[5], z[6], z[7]);
}

/******************************************/

static inline
Real_t AreaFace( const Real_t x0, const Real_t x1,
                 const Real_t x2, const Real_t x3,
                 const Real_t y0, const Real_t y1,
                 const Real_t y2, const Real_t y3,
                 const Real_t z0, const Real_t z1,
                 const Real_t z2, const Real_t z3)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
   Real_t fx = (x2 - x0) - (x3 - x1);
   Real_t fy = (y2 - y0) - (y3 - y1);
   Real_t fz = (z2 - z0) - (z3 - z1);
   Real_t gx = (x2 - x0) + (x3 - x1);
   Real_t gy = (y2 - y0) + (y3 - y1);
   Real_t gz = (z2 - z0) + (z3 - z1);
   Real_t area =
      (fx * fx + fy * fy + fz * fz) *
      (gx * gx + gy * gy + gz * gz) -
      (fx * gx + fy * gy + fz * gz) *
      (fx * gx + fy * gy + fz * gz);
   return area ;
}

/******************************************/

static inline
Real_t CalcElemCharacteristicLength( const Real_t x[8],
                                     const Real_t y[8],
                                     const Real_t z[8],
                                     const Real_t volume)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   Real_t a, charLength = Real_t(0.0);

   a = AreaFace(x[0],x[1],x[2],x[3],
                y[0],y[1],y[2],y[3],
                z[0],z[1],z[2],z[3]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[4],x[5],x[6],x[7],
                y[4],y[5],y[6],y[7],
                z[4],z[5],z[6],z[7]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[0],x[1],x[5],x[4],
                y[0],y[1],y[5],y[4],
                z[0],z[1],z[5],z[4]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[1],x[2],x[6],x[5],
                y[1],y[2],y[6],y[5],
                z[1],z[2],z[6],z[5]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[2],x[3],x[7],x[6],
                y[2],y[3],y[7],y[6],
                z[2],z[3],z[7],z[6]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[3],x[0],x[4],x[7],
                y[3],y[0],y[4],y[7],
                z[3],z[0],z[4],z[7]) ;
   charLength = std::max(a,charLength) ;

   charLength = Real_t(4.0) * volume / SQRT(charLength);

   return charLength;
}

/******************************************/

static inline
void CalcElemVelocityGradient( const Real_t* const xvel,
                                const Real_t* const yvel,
                                const Real_t* const zvel,
                                const Real_t b[][8],
                                const Real_t detJ,
                                Real_t* const d )
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

  const Real_t inv_detJ = Real_t(1.0) / detJ ;
  Real_t dyddx, dxddy, dzddx, dxddz, dzddy, dyddz;
  const Real_t* const pfx = b[0];
  const Real_t* const pfy = b[1];
  const Real_t* const pfz = b[2];

  d[0] = inv_detJ * ( pfx[0] * (xvel[0]-xvel[6])
                     + pfx[1] * (xvel[1]-xvel[7])
                     + pfx[2] * (xvel[2]-xvel[4])
                     + pfx[3] * (xvel[3]-xvel[5]) );

  d[1] = inv_detJ * ( pfy[0] * (yvel[0]-yvel[6])
                     + pfy[1] * (yvel[1]-yvel[7])
                     + pfy[2] * (yvel[2]-yvel[4])
                     + pfy[3] * (yvel[3]-yvel[5]) );

  d[2] = inv_detJ * ( pfz[0] * (zvel[0]-zvel[6])
                     + pfz[1] * (zvel[1]-zvel[7])
                     + pfz[2] * (zvel[2]-zvel[4])
                     + pfz[3] * (zvel[3]-zvel[5]) );

  dyddx  = inv_detJ * ( pfx[0] * (yvel[0]-yvel[6])
                      + pfx[1] * (yvel[1]-yvel[7])
                      + pfx[2] * (yvel[2]-yvel[4])
                      + pfx[3] * (yvel[3]-yvel[5]) );

  dxddy  = inv_detJ * ( pfy[0] * (xvel[0]-xvel[6])
                      + pfy[1] * (xvel[1]-xvel[7])
                      + pfy[2] * (xvel[2]-xvel[4])
                      + pfy[3] * (xvel[3]-xvel[5]) );

  dzddx  = inv_detJ * ( pfx[0] * (zvel[0]-zvel[6])
                      + pfx[1] * (zvel[1]-zvel[7])
                      + pfx[2] * (zvel[2]-zvel[4])
                      + pfx[3] * (zvel[3]-zvel[5]) );

  dxddz  = inv_detJ * ( pfz[0] * (xvel[0]-xvel[6])
                      + pfz[1] * (xvel[1]-xvel[7])
                      + pfz[2] * (xvel[2]-xvel[4])
                      + pfz[3] * (xvel[3]-xvel[5]) );

  dzddy  = inv_detJ * ( pfy[0] * (zvel[0]-zvel[6])
                      + pfy[1] * (zvel[1]-zvel[7])
                      + pfy[2] * (zvel[2]-zvel[4])
                      + pfy[3] * (zvel[3]-zvel[5]) );

  dyddz  = inv_detJ * ( pfz[0] * (yvel[0]-yvel[6])
                      + pfz[1] * (yvel[1]-yvel[7])
                      + pfz[2] * (yvel[2]-yvel[4])
                      + pfz[3] * (yvel[3]-yvel[5]) );
  d[5]  = Real_t( .5) * ( dxddy + dyddx );
  d[4]  = Real_t( .5) * ( dxddz + dzddx );
  d[3]  = Real_t( .5) * ( dzddy + dyddz );

}

/******************************************/

//static inline
void CalcKinematicsForElems( Domain &domain, Real_t *vnew, 
                             Real_t deltaTime, Index_t numElem )
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

  // loop over all elements
//#pragma omp parallel for firstprivate(numElem, deltaTime)
  for( Index_t k=0 ; k<numElem ; ++k )
  {
    Real_t B[3][8] ; /** shape function derivatives */
    Real_t D[6] ;
    Real_t x_local[8] ;
    Real_t y_local[8] ;
    Real_t z_local[8] ;
    Real_t xd_local[8] ;
    Real_t yd_local[8] ;
    Real_t zd_local[8] ;
    Real_t detJ = Real_t(0.0) ;

    Real_t volume ;
    Real_t relativeVolume ;
    const Index_t* const elemToNode = domain.nodelist(k) ;

#if _CD && (SWITCH_5_4_0  > SEQUENTIAL_CD)
    CDHandle *cdh_5_4_0 = 0;
    CDMAPPING_BEGIN_NESTED(CD_MAP_5_4_0, cdh_5_4_0, "CollectDomainNodesToElemNodes3", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_4_0 == SEQUENTIAL_CD)
    CDHandle *cdh_5_4_0 = GetCurrentCD();
    CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_4_0, cdh_5_4_0, "CollectDomainNodesToElemNodes3", M__SERDES_ALL, kCopy);
#endif

    // get nodal coordinates from global arrays and copy into local arrays.
    CollectDomainNodesToElemNodes(domain, elemToNode, x_local, y_local, z_local);

#if _CD && (SWITCH_5_4_0  > SEQUENTIAL_CD)
    CDMAPPING_END_NESTED(CD_MAP_5_4_0, cdh_5_4_0);
#elif _CD && (SWITCH_5_4_0 == SEQUENTIAL_CD)
    cdh_5_4_0->Detect(); 
#endif

#if _CD && (SWITCH_5_5_0  > SEQUENTIAL_CD)
    CDHandle *cdh_5_5_0 = 0;
    CDMAPPING_BEGIN_NESTED(CD_MAP_5_5_0, cdh_5_5_0, "CalcElemVolume", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_5_0 == SEQUENTIAL_CD)
    CDHandle *cdh_5_5_0 = GetCurrentCD();
    CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_5_0, cdh_5_5_0, "CalcElemVolume", M__SERDES_ALL, kCopy);
#endif
    // volume calculations
    volume = CalcElemVolume(x_local, y_local, z_local );

    relativeVolume = volume / domain.volo(k) ;
    vnew[k] = relativeVolume ;
    domain.delv(k) = relativeVolume - domain.v(k) ;
#if _CD && (SWITCH_5_5_0  > SEQUENTIAL_CD)
    CDMAPPING_END_NESTED(CD_MAP_5_5_0, cdh_5_5_0);
#elif _CD && (SWITCH_5_5_0 == SEQUENTIAL_CD)
    cdh_5_5_0->Detect(); 
#endif

#if _CD && (SWITCH_5_6_0  > SEQUENTIAL_CD)
    CDHandle *cdh_5_6_0 = 0;
    CDMAPPING_BEGIN_NESTED(CD_MAP_5_6_0, cdh_5_6_0, "CalcElemCharacteristicLength", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_6_0 == SEQUENTIAL_CD)
    CDHandle *cdh_5_6_0 = GetCurrentCD();
    CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_6_0, cdh_5_6_0, "CalcElemCharacteristicLength", M__SERDES_ALL, kCopy);
#endif
    // set characteristic length
    domain.arealg(k) = CalcElemCharacteristicLength(x_local, y_local, z_local,
                                             volume);

#if _CD && (SWITCH_5_6_0  > SEQUENTIAL_CD)
    CDMAPPING_END_NESTED(CD_MAP_5_6_0, cdh_5_6_0);
#elif _CD && (SWITCH_5_6_0 == SEQUENTIAL_CD)
    cdh_5_6_0->Detect(); 
#endif

   
#if _CD && (SWITCH_5_7_0  > SEQUENTIAL_CD)
    CDHandle *cdh_5_7_0 = 0;
    CDMAPPING_BEGIN_NESTED(CD_MAP_5_7_0, cdh_5_7_0, "InnerLoopInCalcKinematicsForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_7_0 == SEQUENTIAL_CD)
    CDHandle *cdh_5_7_0 = GetCurrentCD();
    CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_7_0, cdh_5_7_0, "InnerLoopInCalcKinematicsForElems", M__SERDES_ALL, kCopy);
#endif
    // get nodal velocities from global array and copy into local arrays.
    for( Index_t lnode=0 ; lnode<8 ; ++lnode )
    {
      Index_t gnode = elemToNode[lnode];
      xd_local[lnode] = domain.xd(gnode);
      yd_local[lnode] = domain.yd(gnode);
      zd_local[lnode] = domain.zd(gnode);
    }

    Real_t dt2 = Real_t(0.5) * deltaTime;
    for ( Index_t j=0 ; j<8 ; ++j )
    {
       x_local[j] -= dt2 * xd_local[j];
       y_local[j] -= dt2 * yd_local[j];
       z_local[j] -= dt2 * zd_local[j];
    }
#if _CD && (SWITCH_5_7_0  > SEQUENTIAL_CD)
    CDMAPPING_END_NESTED(CD_MAP_5_7_0, cdh_5_7_0);
#elif _CD && (SWITCH_5_7_0 == SEQUENTIAL_CD)
    cdh_5_7_0->Detect(); 
#endif


#if _CD && (SWITCH_5_8_0  > SEQUENTIAL_CD)
    CDHandle *cdh_5_8_0 = 0;
    CDMAPPING_BEGIN_NESTED(CD_MAP_5_8_0, cdh_5_8_0, "CalcElemShapeFunctionDerivatives2", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_8_0 == SEQUENTIAL_CD)
    CDHandle *cdh_5_8_0 = GetCurrentCD();
    CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_8_0, cdh_5_8_0, "CalcElemShapeFunctionDerivatives2", M__SERDES_ALL, kCopy);
#endif
    CalcElemShapeFunctionDerivatives( x_local, y_local, z_local,
                                      B, &detJ );

#if _CD && (SWITCH_5_8_0  > SEQUENTIAL_CD)
    CDMAPPING_END_NESTED(CD_MAP_5_8_0, cdh_5_8_0);
#elif _CD && (SWITCH_5_8_0 == SEQUENTIAL_CD)
    cdh_5_8_0->Detect(); 
#endif


#if _CD && (SWITCH_5_9_0  > SEQUENTIAL_CD)
    CDHandle *cdh_5_9_0 = 0;
    CDMAPPING_BEGIN_NESTED(CD_MAP_5_9_0, cdh_5_9_0, "CalcElemVelocityGradient", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_5_9_0 == SEQUENTIAL_CD)
    CDHandle *cdh_5_9_0 = GetCurrentCD();
    CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_5_9_0, cdh_5_9_0, "CalcElemVelocityGradient", M__SERDES_ALL, kCopy);
#endif
    CalcElemVelocityGradient( xd_local, yd_local, zd_local,
                               B, detJ, D );
#if _CD && (SWITCH_5_9_0  > SEQUENTIAL_CD)
    CDMAPPING_END_NESTED(CD_MAP_5_9_0, cdh_5_9_0);
#elif _CD && (SWITCH_5_9_0 == SEQUENTIAL_CD)
    cdh_5_9_0->Detect(); 
#endif

    // put velocity gradient quantities into their global arrays.
    domain.dxx(k) = D[0];
    domain.dyy(k) = D[1];
    domain.dzz(k) = D[2];

  }   // loop ends

}

/******************************************/

static inline
void CalcLagrangeElements(Domain& domain, Real_t* vnew)
{

   Index_t numElem = domain.numElem() ;
   if (numElem > 0) {
      const Real_t deltatime = domain.deltatime() ;

      domain.AllocateStrains(numElem);

#if _CD && (SWITCH_4_1_0  > SEQUENTIAL_CD)
      CDHandle *cdh_4_1_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_4_1_0, cdh_4_1_0, "CalcKinematicsForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_4_1_0 == SEQUENTIAL_CD)
      CDHandle *cdh_4_1_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_4_1_0, cdh_4_1_0, "CalcKinematicsForElems", M__SERDES_ALL, kCopy);
#endif

      CalcKinematicsForElems(domain, vnew, deltatime, numElem) ;

#if _CD && (SWITCH_4_1_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_4_1_0, cdh_4_1_0);
#elif _CD && (SWITCH_4_1_0 == SEQUENTIAL_CD)
      cdh_4_1_0->Detect(); 
#endif


#if _CD && (SWITCH_4_2_0  > SEQUENTIAL_CD)
      CDHandle *cdh_4_2_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_4_2_0, cdh_4_2_0, "LoopInCalcLagrangeElements", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_4_2_0 == SEQUENTIAL_CD)
      CDHandle *cdh_4_2_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_4_2_0, cdh_4_2_0, "LoopInCalcLagrangeElements", M__SERDES_ALL, kCopy);
#endif

      // element loop to do some stuff not included in the elemlib function.
//#pragma omp parallel for firstprivate(numElem)
      for ( Index_t k=0 ; k<numElem ; ++k )
      {
         // calc strain rate and apply as constraint (only done in FB element)
         Real_t vdov = domain.dxx(k) + domain.dyy(k) + domain.dzz(k) ;
         Real_t vdovthird = vdov/Real_t(3.0) ;

         // make the rate of deformation tensor deviatoric
         domain.vdov(k) = vdov ;
         domain.dxx(k) -= vdovthird ;
         domain.dyy(k) -= vdovthird ;
         domain.dzz(k) -= vdovthird ;

         // See if any volumes are negative, and take appropriate action.
         if (vnew[k] <= Real_t(0.0))
         {
//#if USE_MPI           
//           MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
//#else
//           exit(VolumeError);
//#endif
         }
      }

#if _CD && (SWITCH_4_2_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_4_2_0, cdh_4_2_0);
#elif _CD && (SWITCH_4_2_0 == SEQUENTIAL_CD)
      cdh_4_2_0->Detect(); 
#endif

      domain.DeallocateStrains();
   }

}

/******************************************/

static inline
void CalcMonotonicQGradientsForElems(Domain& domain, Real_t vnew[])
{

   Index_t numElem = domain.numElem();

//#pragma omp parallel for firstprivate(numElem)
   for (Index_t i = 0 ; i < numElem ; ++i ) {

      const Real_t ptiny = Real_t(1.e-36) ;
      Real_t ax,ay,az ;
      Real_t dxv,dyv,dzv ;

      const Index_t *elemToNode = domain.nodelist(i);
      Index_t n0 = elemToNode[0] ;
      Index_t n1 = elemToNode[1] ;
      Index_t n2 = elemToNode[2] ;
      Index_t n3 = elemToNode[3] ;
      Index_t n4 = elemToNode[4] ;
      Index_t n5 = elemToNode[5] ;
      Index_t n6 = elemToNode[6] ;
      Index_t n7 = elemToNode[7] ;

      Real_t x0 = domain.x(n0) ;
      Real_t x1 = domain.x(n1) ;
      Real_t x2 = domain.x(n2) ;
      Real_t x3 = domain.x(n3) ;
      Real_t x4 = domain.x(n4) ;
      Real_t x5 = domain.x(n5) ;
      Real_t x6 = domain.x(n6) ;
      Real_t x7 = domain.x(n7) ;

      Real_t y0 = domain.y(n0) ;
      Real_t y1 = domain.y(n1) ;
      Real_t y2 = domain.y(n2) ;
      Real_t y3 = domain.y(n3) ;
      Real_t y4 = domain.y(n4) ;
      Real_t y5 = domain.y(n5) ;
      Real_t y6 = domain.y(n6) ;
      Real_t y7 = domain.y(n7) ;

      Real_t z0 = domain.z(n0) ;
      Real_t z1 = domain.z(n1) ;
      Real_t z2 = domain.z(n2) ;
      Real_t z3 = domain.z(n3) ;
      Real_t z4 = domain.z(n4) ;
      Real_t z5 = domain.z(n5) ;
      Real_t z6 = domain.z(n6) ;
      Real_t z7 = domain.z(n7) ;

      Real_t xv0 = domain.xd(n0) ;
      Real_t xv1 = domain.xd(n1) ;
      Real_t xv2 = domain.xd(n2) ;
      Real_t xv3 = domain.xd(n3) ;
      Real_t xv4 = domain.xd(n4) ;
      Real_t xv5 = domain.xd(n5) ;
      Real_t xv6 = domain.xd(n6) ;
      Real_t xv7 = domain.xd(n7) ;

      Real_t yv0 = domain.yd(n0) ;
      Real_t yv1 = domain.yd(n1) ;
      Real_t yv2 = domain.yd(n2) ;
      Real_t yv3 = domain.yd(n3) ;
      Real_t yv4 = domain.yd(n4) ;
      Real_t yv5 = domain.yd(n5) ;
      Real_t yv6 = domain.yd(n6) ;
      Real_t yv7 = domain.yd(n7) ;

      Real_t zv0 = domain.zd(n0) ;
      Real_t zv1 = domain.zd(n1) ;
      Real_t zv2 = domain.zd(n2) ;
      Real_t zv3 = domain.zd(n3) ;
      Real_t zv4 = domain.zd(n4) ;
      Real_t zv5 = domain.zd(n5) ;
      Real_t zv6 = domain.zd(n6) ;
      Real_t zv7 = domain.zd(n7) ;

      Real_t vol = domain.volo(i)*vnew[i] ;
      Real_t norm = Real_t(1.0) / ( vol + ptiny ) ;

      Real_t dxj = Real_t(-0.25)*((x0+x1+x5+x4) - (x3+x2+x6+x7)) ;
      Real_t dyj = Real_t(-0.25)*((y0+y1+y5+y4) - (y3+y2+y6+y7)) ;
      Real_t dzj = Real_t(-0.25)*((z0+z1+z5+z4) - (z3+z2+z6+z7)) ;

      Real_t dxi = Real_t( 0.25)*((x1+x2+x6+x5) - (x0+x3+x7+x4)) ;
      Real_t dyi = Real_t( 0.25)*((y1+y2+y6+y5) - (y0+y3+y7+y4)) ;
      Real_t dzi = Real_t( 0.25)*((z1+z2+z6+z5) - (z0+z3+z7+z4)) ;

      Real_t dxk = Real_t( 0.25)*((x4+x5+x6+x7) - (x0+x1+x2+x3)) ;
      Real_t dyk = Real_t( 0.25)*((y4+y5+y6+y7) - (y0+y1+y2+y3)) ;
      Real_t dzk = Real_t( 0.25)*((z4+z5+z6+z7) - (z0+z1+z2+z3)) ;

      /* find delvk and delxk ( i cross j ) */

      ax = dyi*dzj - dzi*dyj ;
      ay = dzi*dxj - dxi*dzj ;
      az = dxi*dyj - dyi*dxj ;

      domain.delx_zeta(i) = vol / SQRT(ax*ax + ay*ay + az*az + ptiny) ;

      ax *= norm ;
      ay *= norm ;
      az *= norm ;

      dxv = Real_t(0.25)*((xv4+xv5+xv6+xv7) - (xv0+xv1+xv2+xv3)) ;
      dyv = Real_t(0.25)*((yv4+yv5+yv6+yv7) - (yv0+yv1+yv2+yv3)) ;
      dzv = Real_t(0.25)*((zv4+zv5+zv6+zv7) - (zv0+zv1+zv2+zv3)) ;

      domain.delv_zeta(i) = ax*dxv + ay*dyv + az*dzv ;

      /* find delxi and delvi ( j cross k ) */

      ax = dyj*dzk - dzj*dyk ;
      ay = dzj*dxk - dxj*dzk ;
      az = dxj*dyk - dyj*dxk ;

      domain.delx_xi(i) = vol / SQRT(ax*ax + ay*ay + az*az + ptiny) ;

      ax *= norm ;
      ay *= norm ;
      az *= norm ;

      dxv = Real_t(0.25)*((xv1+xv2+xv6+xv5) - (xv0+xv3+xv7+xv4)) ;
      dyv = Real_t(0.25)*((yv1+yv2+yv6+yv5) - (yv0+yv3+yv7+yv4)) ;
      dzv = Real_t(0.25)*((zv1+zv2+zv6+zv5) - (zv0+zv3+zv7+zv4)) ;

      domain.delv_xi(i) = ax*dxv + ay*dyv + az*dzv ;

      /* find delxj and delvj ( k cross i ) */

      ax = dyk*dzi - dzk*dyi ;
      ay = dzk*dxi - dxk*dzi ;
      az = dxk*dyi - dyk*dxi ;

      domain.delx_eta(i) = vol / SQRT(ax*ax + ay*ay + az*az + ptiny) ;

      ax *= norm ;
      ay *= norm ;
      az *= norm ;

      dxv = Real_t(-0.25)*((xv0+xv1+xv5+xv4) - (xv3+xv2+xv6+xv7)) ;
      dyv = Real_t(-0.25)*((yv0+yv1+yv5+yv4) - (yv3+yv2+yv6+yv7)) ;
      dzv = Real_t(-0.25)*((zv0+zv1+zv5+zv4) - (zv3+zv2+zv6+zv7)) ;

      domain.delv_eta(i) = ax*dxv + ay*dyv + az*dzv ;


   }  // loop ends

}

/******************************************/

static inline
void CalcMonotonicQRegionForElems(Domain &domain, Int_t r,
                                  Real_t vnew[], Real_t ptiny)
{

   Real_t monoq_limiter_mult = domain.monoq_limiter_mult();
   Real_t monoq_max_slope = domain.monoq_max_slope();
   Real_t qlc_monoq = domain.qlc_monoq();
   Real_t qqc_monoq = domain.qqc_monoq();


//#pragma omp parallel for firstprivate(qlc_monoq, qqc_monoq, monoq_limiter_mult, monoq_max_slope, ptiny)
   for ( Index_t ielem = 0 ; ielem < domain.regElemSize(r); ++ielem ) {
      Index_t i = domain.regElemlist(r,ielem);
      Real_t qlin, qquad ;
      Real_t phixi, phieta, phizeta ;
      Int_t bcMask = domain.elemBC(i) ;
      Real_t delvm, delvp ;


      /*  phixi     */
      Real_t norm = Real_t(1.) / (domain.delv_xi(i)+ ptiny ) ;

      switch (bcMask & XI_M) {
         case XI_M_COMM: /* needs comm data */
         case 0:         delvm = domain.delv_xi(domain.lxim(i)); break ;
         case XI_M_SYMM: delvm = domain.delv_xi(i) ;       break ;
         case XI_M_FREE: delvm = Real_t(0.0) ;      break ;
         default:        /* ERROR */ ;              break ;
      }
      switch (bcMask & XI_P) {
         case XI_P_COMM: /* needs comm data */
         case 0:         delvp = domain.delv_xi(domain.lxip(i)) ; break ;
         case XI_P_SYMM: delvp = domain.delv_xi(i) ;       break ;
         case XI_P_FREE: delvp = Real_t(0.0) ;      break ;
         default:        /* ERROR */ ;              break ;
      }

      delvm = delvm * norm ;
      delvp = delvp * norm ;

      phixi = Real_t(.5) * ( delvm + delvp ) ;

      delvm *= monoq_limiter_mult ;
      delvp *= monoq_limiter_mult ;

      if ( delvm < phixi ) phixi = delvm ;
      if ( delvp < phixi ) phixi = delvp ;
      if ( phixi < Real_t(0.)) phixi = Real_t(0.) ;
      if ( phixi > monoq_max_slope) phixi = monoq_max_slope;


      /*  phieta     */
      norm = Real_t(1.) / ( domain.delv_eta(i) + ptiny ) ;

      switch (bcMask & ETA_M) {
         case ETA_M_COMM: /* needs comm data */
         case 0:          delvm = domain.delv_eta(domain.letam(i)) ; break ;
         case ETA_M_SYMM: delvm = domain.delv_eta(i) ;        break ;
         case ETA_M_FREE: delvm = Real_t(0.0) ;        break ;
         default:         /* ERROR */ ;                break ;
      }
      switch (bcMask & ETA_P) {
         case ETA_P_COMM: /* needs comm data */
         case 0:          delvp = domain.delv_eta(domain.letap(i)) ; break ;
         case ETA_P_SYMM: delvp = domain.delv_eta(i) ;        break ;
         case ETA_P_FREE: delvp = Real_t(0.0) ;        break ;
         default:         /* ERROR */ ;                break ;
      }

      delvm = delvm * norm ;
      delvp = delvp * norm ;

      phieta = Real_t(.5) * ( delvm + delvp ) ;

      delvm *= monoq_limiter_mult ;
      delvp *= monoq_limiter_mult ;

      if ( delvm  < phieta ) phieta = delvm ;
      if ( delvp  < phieta ) phieta = delvp ;
      if ( phieta < Real_t(0.)) phieta = Real_t(0.) ;
      if ( phieta > monoq_max_slope)  phieta = monoq_max_slope;

      /*  phizeta     */
      norm = Real_t(1.) / ( domain.delv_zeta(i) + ptiny ) ;

      switch (bcMask & ZETA_M) {
         case ZETA_M_COMM: /* needs comm data */
         case 0:           delvm = domain.delv_zeta(domain.lzetam(i)) ; break ;
         case ZETA_M_SYMM: delvm = domain.delv_zeta(i) ;         break ;
         case ZETA_M_FREE: delvm = Real_t(0.0) ;          break ;
         default:          /* ERROR */ ;                  break ;
      }
      switch (bcMask & ZETA_P) {
         case ZETA_P_COMM: /* needs comm data */
         case 0:           delvp = domain.delv_zeta(domain.lzetap(i)) ; break ;
         case ZETA_P_SYMM: delvp = domain.delv_zeta(i) ;         break ;
         case ZETA_P_FREE: delvp = Real_t(0.0) ;          break ;
         default:          /* ERROR */ ;                  break ;
      }

      delvm = delvm * norm ;
      delvp = delvp * norm ;

      phizeta = Real_t(.5) * ( delvm + delvp ) ;

      delvm *= monoq_limiter_mult ;
      delvp *= monoq_limiter_mult ;

      if ( delvm   < phizeta ) phizeta = delvm ;
      if ( delvp   < phizeta ) phizeta = delvp ;
      if ( phizeta < Real_t(0.)) phizeta = Real_t(0.);
      if ( phizeta > monoq_max_slope  ) phizeta = monoq_max_slope;

      /* Remove length scale */

      if ( domain.vdov(i) > Real_t(0.) )  {
         qlin  = Real_t(0.) ;
         qquad = Real_t(0.) ;
      }
      else {
         Real_t delvxxi   = domain.delv_xi(i)   * domain.delx_xi(i)   ;
         Real_t delvxeta  = domain.delv_eta(i)  * domain.delx_eta(i)  ;
         Real_t delvxzeta = domain.delv_zeta(i) * domain.delx_zeta(i) ;

         if ( delvxxi   > Real_t(0.) ) delvxxi   = Real_t(0.) ;
         if ( delvxeta  > Real_t(0.) ) delvxeta  = Real_t(0.) ;
         if ( delvxzeta > Real_t(0.) ) delvxzeta = Real_t(0.) ;

         Real_t rho = domain.elemMass(i) / (domain.volo(i) * vnew[i]) ;

         qlin = -qlc_monoq * rho *
            (  delvxxi   * (Real_t(1.) - phixi) +
               delvxeta  * (Real_t(1.) - phieta) +
               delvxzeta * (Real_t(1.) - phizeta)  ) ;

         qquad = qqc_monoq * rho *
            (  delvxxi*delvxxi     * (Real_t(1.) - phixi*phixi) +
               delvxeta*delvxeta   * (Real_t(1.) - phieta*phieta) +
               delvxzeta*delvxzeta * (Real_t(1.) - phizeta*phizeta)  ) ;
      }

      domain.qq(i) = qquad ;
      domain.ql(i) = qlin  ;


   }  // loop ends

}

/******************************************/

static inline
void CalcMonotonicQForElems(Domain& domain, Real_t vnew[])
{  
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
   //
   // initialize parameters
   // 

   const Real_t ptiny = Real_t(1.e-36) ;

   //
   // calculate the monotonic q for all regions
   //
   for (Index_t r=0 ; r<domain.numReg() ; ++r) {

      if (domain.regElemSize(r) > 0) {
         CalcMonotonicQRegionForElems(domain, r, vnew, ptiny) ;
      }

   }



}

/******************************************/
// NOTE for CD
static inline
void CalcQForElems(Domain& domain, Real_t vnew[])
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
   //
   // MONOTONIC Q option
   //

   Index_t numElem = domain.numElem() ;

   if (numElem != 0) {
      Int_t allElem = numElem +  /* local elem */
            2*domain.sizeX()*domain.sizeY() + /* plane ghosts */
            2*domain.sizeX()*domain.sizeZ() + /* row ghosts */
            2*domain.sizeY()*domain.sizeZ() ; /* col ghosts */

      domain.AllocateGradients(numElem, allElem);

#if USE_MPI      
      CommRecv(domain, MSG_MONOQ, 3,
               domain.sizeX(), domain.sizeY(), domain.sizeZ(),
               true, true) ;
#endif   

   
#if _CD && (SWITCH_4_3_0  > SEQUENTIAL_CD)
      CDHandle *cdh_4_3_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_4_3_0, cdh_4_3_0, "CalcMonotonicQGradientsForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_4_3_0 == SEQUENTIAL_CD)
      CDHandle *cdh_4_3_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_4_3_0, cdh_4_3_0, "CalcMonotonicQGradientsForElems", M__SERDES_ALL, kCopy);
#endif

      /* Calculate velocity gradients */
      CalcMonotonicQGradientsForElems(domain, vnew);

#if _CD && (SWITCH_4_3_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_4_3_0, cdh_4_3_0);
#elif _CD && (SWITCH_4_3_0 == SEQUENTIAL_CD)
      cdh_4_3_0->Detect(); 
#endif

#if USE_MPI      
      Domain_member fieldData[3] ;
      
      /* Transfer veloctiy gradients in the first order elements */
      /* problem->commElements->Transfer(CommElements::monoQ) ; */

      fieldData[0] = &Domain::delv_xi ;
      fieldData[1] = &Domain::delv_eta ;
      fieldData[2] = &Domain::delv_zeta ;

      CommSend(domain, MSG_MONOQ, 3, fieldData,
               domain.sizeX(), domain.sizeY(), domain.sizeZ(),
               true, true) ;

      CommMonoQ(domain) ;
#endif      


#if _CD && (SWITCH_4_4_0  > SEQUENTIAL_CD)
      CDHandle *cdh_4_4_0 = 0;
      CDMAPPING_BEGIN_NESTED(CD_MAP_4_4_0, cdh_4_4_0, "CalcMonotonicQForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_4_4_0 == SEQUENTIAL_CD)
      CDHandle *cdh_4_4_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_4_4_0, cdh_4_4_0, "CalcMonotonicQForElems", M__SERDES_ALL, kCopy);
#endif
      CalcMonotonicQForElems(domain, vnew) ;
#if _CD && (SWITCH_4_4_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_4_4_0, cdh_4_4_0);
#elif _CD && (SWITCH_4_4_0 == SEQUENTIAL_CD)
      cdh_4_4_0->Detect(); 
#endif


      // Free up memory
      domain.DeallocateGradients();

      /* Don't allow excessive artificial viscosity */
      Index_t idx = -1; 
      for (Index_t i=0; i<numElem; ++i) {
         if ( domain.q(i) > domain.qstop() ) {
            idx = i ;
            break ;
         }
      }

      if(idx >= 0) {
//#if USE_MPI         
//         MPI_Abort(MPI_COMM_WORLD, QStopError) ;
//#else
//         exit(QStopError);
//#endif
      }

   }

}

/******************************************/

static inline
void CalcPressureForElems(Real_t* p_new, Real_t* bvc,
                          Real_t* pbvc, Real_t* e_old,
                          Real_t* compression, Real_t *vnewc,
                          Real_t pmin,
                          Real_t p_cut, Real_t eosvmax,
                          Index_t length, Index_t *regElemList)
{

//#pragma omp parallel for firstprivate(length)
   for (Index_t i = 0; i < length ; ++i) {
      Real_t c1s = Real_t(2.0)/Real_t(3.0) ;
      bvc[i] = c1s * (compression[i] + Real_t(1.));
      pbvc[i] = c1s;
   }

//#pragma omp parallel for firstprivate(length, pmin, p_cut, eosvmax)
   for (Index_t i = 0 ; i < length ; ++i){
      Index_t elem = regElemList[i];
      
      p_new[i] = bvc[i] * e_old[i] ;

      if    (FABS(p_new[i]) <  p_cut   )
         p_new[i] = Real_t(0.0) ;

      if    ( vnewc[elem] >= eosvmax ) /* impossible condition here? */
         p_new[i] = Real_t(0.0) ;

      if    (p_new[i]       <  pmin)
         p_new[i]   = pmin ;
   }

}

/******************************************/

static inline
void CalcEnergyForElems(Real_t* p_new, Real_t* e_new, Real_t* q_new,
                        Real_t* bvc, Real_t* pbvc,
                        Real_t* p_old, Real_t* e_old, Real_t* q_old,
                        Real_t* compression, Real_t* compHalfStep,
                        Real_t* vnewc, Real_t* work, Real_t* delvc, Real_t pmin,
                        Real_t p_cut, Real_t  e_cut, Real_t q_cut, Real_t emin,
                        Real_t* qq_old, Real_t* ql_old,
                        Real_t rho0,
                        Real_t eosvmax,
                        Index_t length, Index_t *regElemList)
{

   Real_t *pHalfStep = Allocate<Real_t>(length) ;

//#pragma omp parallel for firstprivate(length, emin)
   for (Index_t i = 0 ; i < length ; ++i) {
      e_new[i] = e_old[i] - Real_t(0.5) * delvc[i] * (p_old[i] + q_old[i])
         + Real_t(0.5) * work[i];

      if (e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }


   CalcPressureForElems(pHalfStep, bvc, pbvc, 
                        e_new, compHalfStep, vnewc,
                        pmin, p_cut, eosvmax, length, regElemList);


//#pragma omp parallel for firstprivate(length, rho0)
   for (Index_t i = 0 ; i < length ; ++i) {
      Real_t vhalf = Real_t(1.) / (Real_t(1.) + compHalfStep[i]) ;

      if ( delvc[i] > Real_t(0.) ) {
         q_new[i] /* = qq_old[i] = ql_old[i] */ = Real_t(0.) ;
      }
      else {
         Real_t ssc = ( pbvc[i] * e_new[i]
                 + vhalf * vhalf * bvc[i] * pHalfStep[i] ) / rho0 ;

         if ( ssc <= Real_t(.1111111e-36) ) {
            ssc = Real_t(.3333333e-18) ;
         } else {
            ssc = SQRT(ssc) ;
         }

         q_new[i] = (ssc*ql_old[i] + qq_old[i]) ;
      }

      e_new[i] = e_new[i] + Real_t(0.5) * delvc[i]
         * (  Real_t(3.0)*(p_old[i]     + q_old[i])
              - Real_t(4.0)*(pHalfStep[i] + q_new[i])) ;
   }

//#pragma omp parallel for firstprivate(length, emin, e_cut)
   for (Index_t i = 0 ; i < length ; ++i) {

      e_new[i] += Real_t(0.5) * work[i];

      if (FABS(e_new[i]) < e_cut) {
         e_new[i] = Real_t(0.)  ;
      }
      if (     e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }

   CalcPressureForElems(p_new, bvc, pbvc, 
                        e_new, compression, vnewc,
                        pmin, p_cut, eosvmax, length, regElemList);


//#pragma omp parallel for firstprivate(length, rho0, emin, e_cut)
   for (Index_t i = 0 ; i < length ; ++i){
      const Real_t sixth = Real_t(1.0) / Real_t(6.0) ;
      Index_t elem = regElemList[i];
      Real_t q_tilde ;

      if (delvc[i] > Real_t(0.)) {
         q_tilde = Real_t(0.) ;
      }
      else {
         Real_t ssc = ( pbvc[i] * e_new[i]
                 + vnewc[elem] * vnewc[elem] * bvc[i] * p_new[i] ) / rho0 ;

         if ( ssc <= Real_t(.1111111e-36) ) {
            ssc = Real_t(.3333333e-18) ;
         } else {
            ssc = SQRT(ssc) ;
         }

         q_tilde = (ssc*ql_old[i] + qq_old[i]) ;
      }

      e_new[i] = e_new[i] - (  Real_t(7.0)*(p_old[i]     + q_old[i])
                               - Real_t(8.0)*(pHalfStep[i] + q_new[i])
                               + (p_new[i] + q_tilde)) * delvc[i]*sixth ;

      if (FABS(e_new[i]) < e_cut) {
         e_new[i] = Real_t(0.)  ;
      }
      if (     e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }


   CalcPressureForElems(p_new, bvc, pbvc, 
                        e_new, compression, vnewc,
                        pmin, p_cut, eosvmax, length, regElemList);


//#pragma omp parallel for firstprivate(length, rho0, q_cut)
   for (Index_t i = 0 ; i < length ; ++i){
      Index_t elem = regElemList[i];

      if ( delvc[i] <= Real_t(0.) ) {
         Real_t ssc = ( pbvc[i] * e_new[i]
                 + vnewc[elem] * vnewc[elem] * bvc[i] * p_new[i] ) / rho0 ;

         if ( ssc <= Real_t(.1111111e-36) ) {
            ssc = Real_t(.3333333e-18) ;
         } else {
            ssc = SQRT(ssc) ;
         }

         q_new[i] = (ssc*ql_old[i] + qq_old[i]) ;

         if (FABS(q_new[i]) < q_cut) q_new[i] = Real_t(0.) ;
      }
   }

   Release(&pHalfStep) ;


   return ;
}

/******************************************/
// FIXME : CD
static inline
void CalcSoundSpeedForElems(Domain &domain,
                            Real_t *vnewc, Real_t rho0, Real_t *enewc,
                            Real_t *pnewc, Real_t *pbvc,
                            Real_t *bvc, Real_t ss4o3,
                            Index_t len, Index_t *regElemList)
{

//#pragma omp parallel for firstprivate(rho0, ss4o3)
   for (Index_t i = 0; i < len ; ++i) {
      Index_t elem = regElemList[i];
      Real_t ssTmp = (pbvc[i] * enewc[i] + vnewc[elem] * vnewc[elem] *
                 bvc[i] * pnewc[i]) / rho0;
      if (ssTmp <= Real_t(.1111111e-36)) {
         ssTmp = Real_t(.3333333e-18);
      }
      else {
         ssTmp = SQRT(ssTmp);
      }
      domain.ss(elem) = ssTmp ;
   }

}

/******************************************/
//FIXME : CD
static inline
void EvalEOSForElems(Domain& domain, Real_t *vnewc,
                     Int_t numElemReg, Index_t *regElemList, Int_t rep)
{

   Real_t  e_cut = domain.e_cut() ;
   Real_t  p_cut = domain.p_cut() ;
   Real_t  ss4o3 = domain.ss4o3() ;
   Real_t  q_cut = domain.q_cut() ;

   Real_t eosvmax = domain.eosvmax() ;
   Real_t eosvmin = domain.eosvmin() ;
   Real_t pmin    = domain.pmin() ;
   Real_t emin    = domain.emin() ;
   Real_t rho0    = domain.refdens() ;

   // These temporaries will be of different size for 
   // each call (due to different sized region element
   // lists)
   Real_t *e_old = Allocate<Real_t>(numElemReg) ;
   Real_t *delvc = Allocate<Real_t>(numElemReg) ;
   Real_t *p_old = Allocate<Real_t>(numElemReg) ;
   Real_t *q_old = Allocate<Real_t>(numElemReg) ;
   Real_t *compression = Allocate<Real_t>(numElemReg) ;
   Real_t *compHalfStep = Allocate<Real_t>(numElemReg) ;
   Real_t *qq_old = Allocate<Real_t>(numElemReg) ;
   Real_t *ql_old = Allocate<Real_t>(numElemReg) ;
   Real_t *work = Allocate<Real_t>(numElemReg) ;
   Real_t *p_new = Allocate<Real_t>(numElemReg) ;
   Real_t *e_new = Allocate<Real_t>(numElemReg) ;
   Real_t *q_new = Allocate<Real_t>(numElemReg) ;
   Real_t *bvc = Allocate<Real_t>(numElemReg) ;
   Real_t *pbvc = Allocate<Real_t>(numElemReg) ;


   //loop to add load imbalance based on region number 
   for(Int_t j = 0; j < rep; j++) {

      /* compress data, minimal set */
//#pragma omp parallel
      {

//#pragma omp for nowait firstprivate(numElemReg)
         for (Index_t i=0; i<numElemReg; ++i) {
            Index_t elem = regElemList[i];
            e_old[i] = domain.e(elem) ;
            delvc[i] = domain.delv(elem) ;
            p_old[i] = domain.p(elem) ;
            q_old[i] = domain.q(elem) ;
            qq_old[i] = domain.qq(elem) ;
            ql_old[i] = domain.ql(elem) ;
         }

//#pragma omp for firstprivate(numElemReg)
         for (Index_t i = 0; i < numElemReg ; ++i) {
            Index_t elem = regElemList[i];
            Real_t vchalf ;
            compression[i] = Real_t(1.) / vnewc[elem] - Real_t(1.);
            vchalf = vnewc[elem] - delvc[i] * Real_t(.5);
            compHalfStep[i] = Real_t(1.) / vchalf - Real_t(1.);
         }

      /* Check for v > eosvmax or v < eosvmin */
         if ( eosvmin != Real_t(0.) ) {
//#pragma omp for nowait firstprivate(numElemReg, eosvmin)
            for(Index_t i=0 ; i<numElemReg ; ++i) {
               Index_t elem = regElemList[i];
               if (vnewc[elem] <= eosvmin) { /* impossible due to calling func? */
                  compHalfStep[i] = compression[i] ;
               }
            }
         }
         if ( eosvmax != Real_t(0.) ) {
//#pragma omp for nowait firstprivate(numElemReg, eosvmax)
            for(Index_t i=0 ; i<numElemReg ; ++i) {
               Index_t elem = regElemList[i];
               if (vnewc[elem] >= eosvmax) { /* impossible due to calling func? */
                  p_old[i]        = Real_t(0.) ;
                  compression[i]  = Real_t(0.) ;
                  compHalfStep[i] = Real_t(0.) ;
               }
            }
         }

//#pragma omp for nowait firstprivate(numElemReg)
         for (Index_t i = 0 ; i < numElemReg ; ++i) {
            work[i] = Real_t(0.) ; 
         }

      }  // pragma omp parallel ends



      CalcEnergyForElems(p_new, e_new, q_new, bvc, pbvc,
                         p_old, e_old,  q_old, compression, compHalfStep,
                         vnewc, work,  delvc, pmin,
                         p_cut, e_cut, q_cut, emin,
                         qq_old, ql_old, rho0, eosvmax,
                         numElemReg, regElemList);

   }  // loop ends


//#pragma omp parallel for firstprivate(numElemReg)
   for (Index_t i=0; i<numElemReg; ++i) {
      Index_t elem = regElemList[i];
      domain.p(elem) = p_new[i] ;
      domain.e(elem) = e_new[i] ;
      domain.q(elem) = q_new[i] ;
   }


   CalcSoundSpeedForElems(domain,
                          vnewc, rho0, e_new, p_new,
                          pbvc, bvc, ss4o3,
                          numElemReg, regElemList) ;

   Release(&pbvc) ;
   Release(&bvc) ;
   Release(&q_new) ;
   Release(&e_new) ;
   Release(&p_new) ;
   Release(&work) ;
   Release(&ql_old) ;
   Release(&qq_old) ;
   Release(&compHalfStep) ;
   Release(&compression) ;
   Release(&q_old) ;
   Release(&p_old) ;
   Release(&delvc) ;
   Release(&e_old) ;

}

/******************************************/
// NOTE : CD
static inline
void ApplyMaterialPropertiesForElems(Domain& domain, Real_t vnew[])
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   Index_t numElem = domain.numElem() ;

  if (numElem != 0) {

    /* Expose all of the variables needed for material evaluation */
    Real_t eosvmin = domain.eosvmin() ;
    Real_t eosvmax = domain.eosvmax() ;

//#pragma omp parallel
    {

       // Bound the updated relative volumes with eosvmin/max
       if (eosvmin != Real_t(0.)) {
//#pragma omp for firstprivate(numElem)
          for(Index_t i=0 ; i<numElem ; ++i) {
             if (vnew[i] < eosvmin)
                vnew[i] = eosvmin ;
          }
       }

       if (eosvmax != Real_t(0.)) {
//#pragma omp for nowait firstprivate(numElem)
          for(Index_t i=0 ; i<numElem ; ++i) {
             if (vnew[i] > eosvmax)
                vnew[i] = eosvmax ;
          }
       }


       // This check may not make perfect sense in LULESH, but
       // it's representative of something in the full code -
       // just leave it in, please
//#pragma omp for nowait firstprivate(numElem)
       for (Index_t i=0; i<numElem; ++i) {

          Real_t vc = domain.v(i) ;
          if (eosvmin != Real_t(0.)) {
             if (vc < eosvmin)
                vc = eosvmin ;
          }
          if (eosvmax != Real_t(0.)) {
             if (vc > eosvmax)
                vc = eosvmax ;
          }
          if (vc <= 0.) {
//#if USE_MPI             
//             MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
//#else
//             exit(VolumeError);
//#endif
          }


       } // end inner loop


    }  // pragma omp parallel ends

#if _CD && (SWITCH_4_5_0  > SEQUENTIAL_CD)
    CDHandle *cdh_4_5_0 = GetCurrentCD()->Create(CD_MAP_4_5_0 >> CDFLAG_SIZE, 
                            (string("Loop_EvalEOSForElems")+GetCurrentCD()->label()).c_str(), 
                            CD_MAP_4_5_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_4_5_0)); 
#elif _CD && (SWITCH_4_5_0 == SEQUENTIAL_CD)
    CDHandle *cdh_4_5_0 = GetCurrentCD();
    CD_Complete(cdh_4_5_0); 
#endif
    for (Int_t r=0 ; r<domain.numReg() ; r++) {
#if _CD && (SWITCH_4_5_0  > SEQUENTIAL_CD || SWITCH_4_5_0 == SEQUENTIAL_CD)
       CD_BEGIN(cdh_4_5_0, "Loop_EvalEOSForElems"); 
       cdh_4_5_0->Preserve(&domain, sizeof(domain), kCopy, "locDom_Loop_EvalEOSForElems"); 
       cdh_4_5_0->Preserve(domain.serdes.SetOp(M__SERDES_ALL), kCopy, "AllMembers_Loop_EvalEOSForElems"); 
#endif


       Index_t numElemReg = domain.regElemSize(r);
       Index_t *regElemList = domain.regElemlist(r);
       Int_t rep;

       //Determine load imbalance for this region
       //round down the number with lowest cost
       if(r < domain.numReg()/2)
	      rep = 1;
       //you don't get an expensive region unless you at least have 5 regions
       else if(r < (domain.numReg() - (domain.numReg()+15)/20))
         rep = 1 + domain.cost();
       //very expensive regions
       else
	      rep = 10 * (1+ domain.cost());


       EvalEOSForElems(domain, vnew, numElemReg, regElemList, rep);
#if _CD && (SWITCH_4_5_0  > SEQUENTIAL_CD || SWITCH_4_5_0 == SEQUENTIAL_CD)
       cdh_4_5_0->Detect();
       CD_Complete(cdh_4_5_0);
#endif

    }

#if _CD && (SWITCH_4_5_0  > SEQUENTIAL_CD)
   cdh_4_5_0->Destroy();
#elif _CD && (SWITCH_4_5_0 == SEQUENTIAL_CD)
   CD_BEGIN(cdh_4_5_0, false, "After_Loop_EvalEOSForElems");
#endif
  }   // if-statement ends

}

/******************************************/

static inline
void UpdateVolumesForElems(Domain &domain, Real_t *vnew,
                           Real_t v_cut, Index_t length)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   if (length != 0) {
//#pragma omp parallel for firstprivate(length, v_cut)
      for(Index_t i=0 ; i<length ; ++i) {
         Real_t tmpV = vnew[i] ;

         if ( FABS(tmpV - Real_t(1.0)) < v_cut )
            tmpV = Real_t(1.0) ;

         domain.v(i) = tmpV ;
      }
   }

   return ;
}

/******************************************/

static inline
void LagrangeElements(Domain& domain, Index_t numElem)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

  Real_t *vnew = Allocate<Real_t>(numElem) ;  /* new relative vol -- temp */

#if _CD && (SWITCH_3_5_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_5_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_5_0, cdh_3_5_0, "CalcLagrangeElements", 
                   (M__VOLO | M__V | M__X  | M__Y  | M__Z  | 
                                     M__XD | M__YD | M__ZD |
                                     M__DXX| M__DYY| M__DZZ|
                                     M__QQ | M__QL | M__SS |
                                     M__Q  | M__E  | 
                                     M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
                                     M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA),
                   kCopy);
   cdh_3_5_0->Preserve(&(domain.deltatime()), sizeof(Real_t), kCopy, "deltatime"); 
#elif _CD && (SWITCH_3_5_0 == SEQUENTIAL_CD)
   CDHandle *cdh_3_5_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_5_0, cdh_3_5_0, "CalcLagrangeElements", 
                   (M__VOLO | M__V | M__X  | M__Y  | M__Z  | 
                                     M__XD | M__YD | M__ZD |
                                     M__DXX| M__DYY| M__DZZ|
                                     M__QQ | M__QL | M__SS |
                                     M__Q  | M__E  | 
                                     M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
                                     M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA),
                    kCopy);
   cdh_3_5_0->Preserve(&(domain.deltatime()), sizeof(Real_t), kCopy, "deltatime"); 
#endif

   CalcLagrangeElements(domain, vnew) ;

#if _CD && (SWITCH_3_5_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_5_0, cdh_3_5_0);
#elif _CD && (SWITCH_3_5_0 == SEQUENTIAL_CD)
   cdh_3_5_0->Detect(); 
#endif


#if _CD && (SWITCH_3_6_0  >= SEQUENTIAL_CD)
   CDHandle *cdh_3_6_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_6_0, cdh_3_6_0, "CalcQForElems", 
                             (M__VOLO | M__V | M__X  | M__Y  | M__Z  | 
                                               M__XD | M__YD | M__ZD |
                                               M__DXX| M__DYY| M__DZZ|
                                               M__QQ | M__QL | M__SS |
                                               M__Q  | M__E  | 
                                               M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
                                               M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA),
                          kCopy);
//#elif _CD && (SWITCH_3_6_0 == SEQUENTIAL_CD)
//   CDHandle *cdh_3_6_0 = GetCurrentCD();
//   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_6_0, cdh_3_6_0, "CalcQForElems", 
//       M__SERDES_ALL, kCopy);
#endif

   /* Calculate Q.  (Monotonic q option requires communication) */
   CalcQForElems(domain, vnew) ;

//#if _CD && (SWITCH_3_6_0  > SEQUENTIAL_CD)
//   CDMAPPING_END_NESTED(CD_MAP_3_6_0, cdh_3_6_0);
//#elif _CD && (SWITCH_3_6_0 == SEQUENTIAL_CD)
//   cdh_3_6_0->Detect(); 
//#endif

#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD) && OPTIMIZE
   if(interval != 0 && ckpt_idx % interval == 0) 
   {
      st.BeginTimer();
      GetCurrentCD()->Preserve(domain.serdes.SetOp(preserve_vec_2), kCopy, "AfterCalcQForElems");
      st.EndTimer(2);

#ifdef _DEBUG_PRV
      if(st.myRank == 0) {
        printf("preserve vec 2 AfterCalcQForElems\n");
      }
#endif
   }

#endif


#if _CD && (SWITCH_3_7_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_7_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_7_0, cdh_3_7_0, "ApplyMaterialPropertiesForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_3_7_0 == SEQUENTIAL_CD)
   CDHandle *cdh_3_7_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_7_0, cdh_3_7_0, "ApplyMaterialPropertiesForElems", M__SERDES_ALL, kCopy);
#endif

   ApplyMaterialPropertiesForElems(domain, vnew) ;

#if _CD && (SWITCH_3_7_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_7_0, cdh_3_7_0);
#elif _CD && (SWITCH_3_7_0 == SEQUENTIAL_CD)
   cdh_3_7_0->Detect(); 
#endif



#if _CD && (SWITCH_3_8_0  > SEQUENTIAL_CD)
   CDHandle *cdh_3_8_0 = 0;
   CDMAPPING_BEGIN_NESTED(CD_MAP_3_8_0, cdh_3_8_0, "UpdateVolumesForElems", M__SERDES_ALL, kCopy);
#elif _CD && (SWITCH_3_8_0 == SEQUENTIAL_CD)
   CDHandle *cdh_3_8_0 = GetCurrentCD();
   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_3_8_0, cdh_3_8_0, "UpdateVolumesForElems", M__SERDES_ALL, kCopy);
#endif
  UpdateVolumesForElems(domain, vnew,
                        domain.v_cut(), numElem) ;

#if _CD && (SWITCH_3_8_0  > SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_8_0, cdh_3_8_0);
#elif _CD && (SWITCH_3_8_0 == SEQUENTIAL_CD)
   cdh_3_8_0->Detect(); 
#endif


#if _CD && (SWITCH_3_6_0  >= SEQUENTIAL_CD)
   CDMAPPING_END_NESTED(CD_MAP_3_6_0, cdh_3_6_0);
//#elif _CD && (SWITCH_3_6_0 == SEQUENTIAL_CD)
//   cdh_3_6_0->Detect(); 
#endif


  Release(&vnew);

}

/******************************************/

static inline
void CalcCourantConstraintForElems(Domain &domain, Index_t length,
                                   Index_t *regElemlist,
                                   Real_t qqc, Real_t& dtcourant)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);


#if _OPENMP   
   Index_t threads = omp_get_max_threads();
   static Index_t *courant_elem_per_thread;
   static Real_t *dtcourant_per_thread;
   static bool first = true;
   if (first) {
     courant_elem_per_thread = new Index_t[threads];
     dtcourant_per_thread = new Real_t[threads];
     first = false;
   }
#else
   Index_t threads = 1;
   Index_t courant_elem_per_thread[1];
   Real_t  dtcourant_per_thread[1];
#endif


//#pragma omp parallel firstprivate(length, qqc)
   {
      Real_t   qqc2 = Real_t(64.0) * qqc * qqc ;
      Real_t   dtcourant_tmp = dtcourant;
      Index_t  courant_elem  = -1 ;

#if _OPENMP
      Index_t thread_num = omp_get_thread_num();
#else
      Index_t thread_num = 0;
#endif      

//#pragma omp for 
      for (Index_t i = 0 ; i < length ; ++i) {
         Index_t indx = regElemlist[i] ;
         Real_t dtf = domain.ss(indx) * domain.ss(indx) ;

         if ( domain.vdov(indx) < Real_t(0.) ) {
            dtf = dtf
                + qqc2 * domain.arealg(indx) * domain.arealg(indx)
                * domain.vdov(indx) * domain.vdov(indx) ;
         }

         dtf = SQRT(dtf) ;
         dtf = domain.arealg(indx) / dtf ;

         if (domain.vdov(indx) != Real_t(0.)) {
            if ( dtf < dtcourant_tmp ) {
               dtcourant_tmp = dtf ;
               courant_elem  = indx ;
            }
         }
      }

      dtcourant_per_thread[thread_num]    = dtcourant_tmp ;
      courant_elem_per_thread[thread_num] = courant_elem ;
   }

   for (Index_t i = 1; i < threads; ++i) {
      if (dtcourant_per_thread[i] < dtcourant_per_thread[0] ) {
         dtcourant_per_thread[0]    = dtcourant_per_thread[i];
         courant_elem_per_thread[0] = courant_elem_per_thread[i];
      }
   }

   if (courant_elem_per_thread[0] != -1) {
      dtcourant = dtcourant_per_thread[0] ;
   }


   return ;

}

/******************************************/

static inline
void CalcHydroConstraintForElems(Domain &domain, Index_t length,
                                 Index_t *regElemlist, Real_t dvovmax, Real_t& dthydro)
{
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);


#if _OPENMP   
   Index_t threads = omp_get_max_threads();
   static Index_t *hydro_elem_per_thread;
   static Real_t *dthydro_per_thread;
   static bool first = true;
   if (first) {
     hydro_elem_per_thread = new Index_t[threads];
     dthydro_per_thread = new Real_t[threads];
     first = false;
   }
#else
   Index_t threads = 1;
   Index_t hydro_elem_per_thread[1];
   Real_t  dthydro_per_thread[1];
#endif

//#pragma omp parallel firstprivate(length, dvovmax)
   {
      Real_t dthydro_tmp = dthydro ;
      Index_t hydro_elem = -1 ;

#if _OPENMP      
      Index_t thread_num = omp_get_thread_num();
#else      
      Index_t thread_num = 0;
#endif      

//#pragma omp for
      for (Index_t i = 0 ; i < length ; ++i) {
         Index_t indx = regElemlist[i] ;

         if (domain.vdov(indx) != Real_t(0.)) {
            Real_t dtdvov = dvovmax / (FABS(domain.vdov(indx))+Real_t(1.e-20)) ;

            if ( dthydro_tmp > dtdvov ) {
                  dthydro_tmp = dtdvov ;
                  hydro_elem = indx ;
            }
         }
      }

      dthydro_per_thread[thread_num]    = dthydro_tmp ;
      hydro_elem_per_thread[thread_num] = hydro_elem ;
   }

   for (Index_t i = 1; i < threads; ++i) {
      if(dthydro_per_thread[i] < dthydro_per_thread[0]) {
         dthydro_per_thread[0]    = dthydro_per_thread[i];
         hydro_elem_per_thread[0] =  hydro_elem_per_thread[i];
      }
   }

   if (hydro_elem_per_thread[0] != -1) {
      dthydro =  dthydro_per_thread[0] ;
   }


   return ;
}

/******************************************/

static inline
void CalcTimeConstraintsForElems(Domain& domain) {
   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);

   // Initialize conditions to a very large value
   domain.dtcourant() = 1.0e+20;
   domain.dthydro() = 1.0e+20;


   for (Index_t r=0 ; r < domain.numReg() ; ++r) {

#if _CD && (SWITCH_3_9_0  > SEQUENTIAL_CD)
      CDHandle *cdh_3_9_0 = 0;
      CDMAPPING_BEGIN_NESTED_ONLY(CD_MAP_3_9_0, cdh_3_9_0, "CalcCourantConstraintForElems");
      cdh_3_9_0->Preserve(&(domain.dtcourant()), sizeof(Real_t), kCopy, "SWITCH_3_9_0_dtcourant"); 
#elif _CD && (SWITCH_3_9_0 == SEQUENTIAL_CD)
      CDHandle *cdh_3_9_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL_ONLY(CD_MAP_3_9_0, cdh_3_9_0, "CalcCourantConstraintForElems");
      cdh_3_9_0->Preserve(&(domain.dtcourant()), sizeof(Real_t), kCopy, "SWITCH_3_9_0_dtcourant"); 
#endif
      /* evaluate time constraint */
      CalcCourantConstraintForElems(domain, domain.regElemSize(r),
                                    domain.regElemlist(r),
                                    domain.qqc(),
                                    domain.dtcourant()) ;

#if _CD && (SWITCH_3_9_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_3_9_0, cdh_3_9_0);
#elif _CD && (SWITCH_3_9_0 == SEQUENTIAL_CD)
      cdh_3_9_0->Detect(); 
#endif


#if _CD && (SWITCH_3_10_0  > SEQUENTIAL_CD)
      CDHandle *cdh_3_10_0 = 0;
      CDMAPPING_BEGIN_NESTED_ONLY(CD_MAP_3_10_0, cdh_3_10_0, "CalcHydroConstraintForElems");
      cdh_3_10_0->Preserve(&(domain.dthydro()), sizeof(Real_t), kCopy, "SWITCH_3_10_0_dthydro"); 
#elif _CD && (SWITCH_3_10_0 == SEQUENTIAL_CD)
      CDHandle *cdh_3_10_0 = GetCurrentCD();
      CDMAPPING_BEGIN_SEQUENTIAL_ONLY(CD_MAP_3_10_0, cdh_3_10_0, "CalcHydroConstraintForElems");
      cdh_3_10_0->Preserve(&(domain.dthydro()), sizeof(Real_t), kCopy, "SWITCH_3_10_0_dthydro"); 
#endif
      /* check hydro constraint */
      CalcHydroConstraintForElems(domain, domain.regElemSize(r),
                                  domain.regElemlist(r),
                                  domain.dvovmax(),
                                  domain.dthydro()) ;
#if _CD && (SWITCH_3_10_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_3_10_0, cdh_3_10_0);
#elif _CD && (SWITCH_3_10_0 == SEQUENTIAL_CD)
      cdh_3_10_0->Detect(); 
#endif
   }

}

/******************************************/
//
//static inline
//void LagrangeLeapFrog(Domain& domain)
//{
//   DEBUG_PRINT("\n\n=====================================\n\t[%s]\n=====================================\n\n", __func__);
//#ifdef SEDOV_SYNC_POS_VEL_LATE
//   Domain_member fieldData[6] ;
//#endif
//
//
////#if _CD && (SWITCH_2_0_0  > SEQUENTIAL_CD)
////   CDHandle *cdh_2_0_0 = 0;
////   cdh_2_0_0 = GetLeafCD()->Create(CD_MAP_2_0_0 >> CDFLAG_SIZE, 
////                                  (string("LagrangeNodal")+GetCurrentCD()->label()).c_str(), 
////                                   CD_MAP_2_0_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_2_0_0)); 
////   CD_BEGIN(cdh_2_0_0, "LagrangeNodal"); 
////   cdh_2_0_0->Preserve(domain.serdes.SetOp(
////                              (M__FX | M__FY | M__FZ |
////                               M__X  | M__Y  | M__Z  |
////                               M__XD | M__YD | M__ZD |
////                               M__XDD| M__YDD| M__ZDD )
////                              ), kCopy | kSerdes, "SWITCH_2_0_0_LagrangeNodal"); 
////   cdh_2_0_0->Preserve(&(domain.deltatime()), sizeof(Real_t), kCopy, "deltatime"); 
////#elif _CD && (SWITCH_2_0_0 == SEQUENTIAL_CD)
////   CDHandle *cdh_2_0_0 = GetLeafCD();
////   CD_BEGIN(cdh_2_0_0, "LagrangeNodal"); 
////   cdh_2_0_0->Preserve(domain.serdes.SetOp(
////                              (M__FX | M__FY | M__FZ |
////                               M__X  | M__Y  | M__Z  |
////                               M__XD | M__YD | M__ZD |
////                               M__XDD| M__YDD| M__ZDD )
////                              ), kCopy | kSerdes, "SWITCH_2_0_0_LagrangeNodal"); 
////   cdh_2_0_0->Preserve(&(domain.deltatime()), sizeof(Real_t), kCopy, "deltatime"); 
////#endif
//
//   /* calculate nodal forces, accelerations, velocities, positions, with
//    * applied boundary conditions and slide surface considerations */
//   LagrangeNodal(domain);  
//
//#if _CD && (SWITCH_2_0_0  > SEQUENTIAL_CD)
//   CDMAPPING_END_NESTED(CD_MAP_2_0_0, cdh_2_0_0);
//#elif _CD && (SWITCH_2_0_0 == SEQUENTIAL_CD)
//   CDHandle *cdh_2_0_0 = GetCurrentCD();
//   cdh_2_0_0->Detect(); 
//#endif
//
//
//#ifdef SEDOV_SYNC_POS_VEL_LATE
//#endif
//
//
//#if _CD && (SWITCH_2_1_0  > SEQUENTIAL_CD)
//   CDHandle *cdh_2_1_0 = 0;
//   CDMAPPING_BEGIN_NESTED(CD_MAP_2_1_0, cdh_2_1_0, "LagrangeElements", 
//       (M__VOLO | M__V | M__X  | M__Y  | M__Z  | 
//                         M__XD | M__YD | M__ZD |
//                         M__DXX| M__DYY| M__DZZ|
//                         M__QQ | M__QL | M__SS |
//                         M__Q  | M__E  | 
//                         M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
//                         M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA ), 
//       kCopy);
//#elif _CD && (SWITCH_2_1_0 == SEQUENTIAL_CD)
//   CDHandle *cdh_2_1_0 = GetCurrentCD();
//   //delx_zeta,delv_xi,delx_eta,delv_eta,qq,ql,p,e,q,ss
//   CDMAPPING_BEGIN_SEQUENTIAL(CD_MAP_2_1_0, cdh_2_1_0, "LagrangeElements", 
//       (M__VOLO | M__V | M__X  | M__Y  | M__Z  | 
//                         M__XD | M__YD | M__ZD |
//                         M__DXX| M__DYY| M__DZZ|
//                         M__QQ | M__QL | M__SS |
//                         M__Q  | M__E  | 
//                         M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
//                         M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA), 
//       kCopy);
//   cdh_2_1_0->Preserve(&(domain.deltatime()), sizeof(Real_t), kCopy, "SWITCH_2_1_0_deltatime"); 
//#endif
//
//   /* calculate element quantities (i.e. velocity gradient & q), and update
//    * material states */
//   LagrangeElements(domain, domain.numElem());
//
//#if _CD && (SWITCH_2_1_0  > SEQUENTIAL_CD)
//   CDMAPPING_END_NESTED(CD_MAP_2_1_0, cdh_2_1_0);
//#elif _CD && (SWITCH_2_1_0 == SEQUENTIAL_CD)
//   cdh_2_1_0->Detect(); 
//#endif
//
//#if USE_MPI   
//#ifdef SEDOV_SYNC_POS_VEL_LATE
//   CommRecv(domain, MSG_SYNC_POS_VEL, 6,
//            domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
//            false, false) ;
//
//   fieldData[0] = &Domain::x ;
//   fieldData[1] = &Domain::y ;
//   fieldData[2] = &Domain::z ;
//   fieldData[3] = &Domain::xd ;
//   fieldData[4] = &Domain::yd ;
//   fieldData[5] = &Domain::zd ;
//   
//   CommSend(domain, MSG_SYNC_POS_VEL, 6, fieldData,
//            domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
//            false, false) ;
//#endif
//#endif   
//
//
//#if _CD && (SWITCH_2_2_0  > SEQUENTIAL_CD)
//   CDHandle *cdh_2_2_0 = 0;
//   CDMAPPING_BEGIN_NESTED_ONLY(CD_MAP_2_2_0, cdh_2_2_0, "CalcTimeConstraintsForElems");
//   cdh_2_2_0->Preserve(&(domain.dtcourant()), sizeof(Real_t), kCopy, "SWITCH_2_2_0_dtcourant"); 
//   cdh_2_2_0->Preserve(&(domain.dthydro()),   sizeof(Real_t), kCopy, "SWITCH_2_2_0_dthydro"); 
//#elif _CD && (SWITCH_2_2_0 == SEQUENTIAL_CD)
//   CDHandle *cdh_2_2_0 = GetCurrentCD();
//   CDMAPPING_BEGIN_SEQUENTIAL_ONLY(CD_MAP_2_2_0, cdh_2_2_0, "CalcTimeConstraintsForElems");
//   cdh_2_2_0->Preserve(&(domain.dtcourant()), sizeof(Real_t), kCopy, "SWITCH_2_2_0_dtcourant"); 
//   cdh_2_2_0->Preserve(&(domain.dthydro()),   sizeof(Real_t), kCopy, "SWITCH_2_2_0_dthydro"); 
//#endif
//
//   CalcTimeConstraintsForElems(domain);
//
//#if _CD && (SWITCH_2_2_0  > SEQUENTIAL_CD)
//   CDMAPPING_END_NESTED(CD_MAP_2_2_0, cdh_2_2_0);
//#elif _CD && (SWITCH_2_2_0 == SEQUENTIAL_CD)
//   cdh_2_2_0->Detect();
//   CD_Complete(cdh_2_2_0); 
//#endif
//
//#if USE_MPI   
//#ifdef SEDOV_SYNC_POS_VEL_LATE
//   CommSyncPosVel(domain) ;
//#endif
//#endif  
//
//}
//

//#define HISTO_SIZE 256
//bool openclose = false; 
//char histogram_filename[64] = {0};
//uint32_t bin_cnt = 0;
//// KL
//static inline
//void InsertBin(double sample)
//{
//
//   //   printf("prv time [\t%d] : %lf\n", idx, prv_time);
//   double shifted_sample = sample;
//   if(shifted_sample < 0) shifted_sample = 0.0;
//   uint32_t bin_idx = (uint32_t)(shifted_sample / 0.00005);
////   printf("(%d) prv time [\t%d] : %lf\n", INTERVAL, bin_idx, sample);
//   if(bin_idx >= HISTO_SIZE) {
//     bin_idx = HISTO_SIZE-1;
//   }
//   histogram[bin_idx]++;
//   latency_history[bin_cnt++] = bin_idx;
//}

/******************************************/

int main(int argc, char *argv[])
{
   Domain *locDom ;
   Int_t numRanks ;
   Int_t myRank ;
   struct cmdLineOpts opts;

#if USE_MPI   
   Domain_member fieldData ;

   MPI_Init(&argc, &argv) ;
   MPI_Comm_size(MPI_COMM_WORLD, &numRanks) ;
   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;
#else
   numRanks = 1;
   myRank = 0;
#endif   

   /* Set defaults that can be overridden by command line opts */
   opts.its = 9999999;
   opts.nx  = 30;
   opts.numReg = 11;
   opts.numFiles = (int)(numRanks+10)/9;
   opts.showProg = 0;
   opts.quiet = 0;
   opts.viz = 0;
   opts.balance = 1;
   opts.cost = 1;

   ParseCommandLineOptions(argc, argv, myRank, &opts);
   // FIXME
//   opts.nx  = 50;

   if ((myRank == 0) && (opts.quiet == 0)) {
      printf("Running problem size %d^3 per domain until completion\n", opts.nx);
      printf("Num processors: %d\n", numRanks);
#if _OPENMP
      printf("Num threads: %d\n", omp_get_max_threads());
#endif
      printf("Total number of elements: %lld\n\n", (long long int)numRanks*opts.nx*opts.nx*opts.nx);
      printf("To run other sizes, use -s <integer>.\n");
      printf("To run a fixed number of iterations, use -i <integer>.\n");
      printf("To run a more or less balanced region set, use -b <integer>.\n");
      printf("To change the relative costs of regions, use -c <integer>.\n");
      printf("To print out progress, use -p\n");
      printf("To write an output file for VisIt, use -v\n");
      printf("See help (-h) for more options\n\n");
   }

   // Set up the mesh and decompose. Assumes regular cubes for now
   Int_t col, row, plane, side;
   InitMeshDecomp(numRanks, myRank, &col, &row, &plane, &side);

   // Build the main data structure and initialize it
   locDom = new Domain(numRanks, col, row, plane, opts.nx,
                       side, opts.numReg, opts.balance, opts.cost) ;

#if USE_MPI   
   fieldData = &Domain::nodalMass ;

   // Initial domain boundary communication 
   CommRecv(*locDom, MSG_COMM_SBN, 1,
            locDom->sizeX() + 1, locDom->sizeY() + 1, locDom->sizeZ() + 1,
            true, false) ;
   CommSend(*locDom, MSG_COMM_SBN, 1, &fieldData,
            locDom->sizeX() + 1, locDom->sizeY() + 1, locDom->sizeZ() + 1,
            true, false) ;
   CommSBN(*locDom, 1, &fieldData) ;

   // End initialization
   MPI_Barrier(MPI_COMM_WORLD);
#endif   
   
   // BEGIN timestep to solution */
   Real_t start;
#if USE_MPI   
   start = MPI_Wtime();
#else
   start = clock();
#endif
   
#if _CD
   locDom->serdes.InitSerdesTable();
   CDHandle* root_cd = CD_Init(numRanks, myRank);
   root_cd->Begin(root_cd, "Root");
#endif

#if _CD && SWITCH_PRESERVE_INIT
   root_cd->Preserve(locDom, sizeof(locDom), kCopy, "locDom_Root");
   root_cd->Preserve(locDom->serdes.SetOp(M__SERDES_ALL), kCopy, "AllMembers_Root");
#endif


#if _CD && (SWITCH_0_0_0  > SEQUENTIAL_CD)
   CDHandle *cdh_0_0_0 = root_cd->Create(CD_MAP_0_0_0 >> CDFLAG_SIZE, 
                                         (string("MainLoop")+root_cd->label()).c_str(), 
                                         CD_MAP_0_0_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_0_0_0)); 
#endif

//   printf("cdhandle size:%zu\n", sizeof(*cdh_0_0_0));
#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD)
   CDHandle *cdh_1_0_0 = cdh_0_0_0;
#endif
   // Main loop start
//   opts.its = 4000;
//   opts.its = 600;
   if(myRank == 0) printf("dom size:%zu\n", sizeof(*locDom));
#if _CD   
   st.Initialize(myRank, numRanks, opts.its, interval);
#else
   unsigned nointerval = 0;
   st.Initialize(myRank, numRanks, opts.its, nointerval);
#endif

#if 0
   char *ckpt_var = getenv("CKPT_INTERVAL");
   if(ckpt_var != NULL) {
     interval = (uint32_t)atoi(ckpt_var);
   }
//   interval=1;
   uint32_t prv_cnt = 0;

   printf("interval:%u\n", interval);
   uint32_t history_size = (opts.its/interval);
   latency_history = new unsigned int[history_size](); // zero-initialized
 
   char hostname[32] = {0};
   gethostname(hostname, sizeof(hostname));
   char hostname_cat[9];
   memcpy(hostname_cat, hostname, 8);
   hostname_cat[8] = '\0';
   //printf("hostname %s\n", hostname);
   snprintf(histogram_filename, 64, "%s.%d", hostname_cat, myRank); 
 

   std::hash<std::string> ptr_hash; 
   uint64_t long_id = ptr_hash(std::string(hostname_cat));
   //printf("hostname %s %lu\n", histogram_filename, long_id);
   uint32_t node_id = ((uint32_t)(long_id >> 32)) ^ ((uint32_t)(long_id));
//   printf("hostname %s %lu (%u)\n", histogram_filename, long_id, node_id);

   MPI_Comm node_comm;
   int numNodeRanks = -1;
   int myNodeRank = -1;
   MPI_Comm_split(MPI_COMM_WORLD, node_id, myRank, &node_comm);
   MPI_Comm_size(node_comm, &numNodeRanks) ;
   MPI_Comm_rank(node_comm, &myNodeRank) ;

   // for rank 0s
   MPI_Comm zero_comm;
   int numZeroRanks = -1;
   int myZeroRank = -1;
   if(myNodeRank == 0) {
     MPI_Comm_split(MPI_COMM_WORLD, 0, myRank, &zero_comm);
   } else {
     MPI_Comm_split(MPI_COMM_WORLD, 1, myRank, &zero_comm);
   }
   MPI_Comm_size(zero_comm, &numZeroRanks);
   MPI_Comm_rank(zero_comm, &myZeroRank);


   printf("hostname %s %lu (%u): %d %d/%d %d/%d\n", histogram_filename, long_id, node_id, 
       myRank, myNodeRank, numNodeRanks, myZeroRank, numZeroRanks);

#endif
   double loop_time = 0.0;
   while((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) {
      double begin_tik = MPI_Wtime();
      // Main functions in the loop
#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD)
     if(interval != 0 && ckpt_idx % interval == 0) 
     {
        CD_BEGIN(cdh_1_0_0, "MainLoop"); 
        st.BeginTimer();
        cdh_1_0_0->Preserve(&(locDom->deltatime()), sizeof(Real_t), kCopy, "deltatime");
#if OPTIMIZE == 0
        cdh_1_0_0->Preserve(locDom->serdes.SetOp(preserve_vec_all), kCopy, "main_iter_prv");
#endif  
        st.EndTimer(-1);
        openclose = true; 
     }
#endif
//#if _CD
//      if(myRank == 0) {
//        printf("\n\n== Loop %d ==================================\n\n", counter++); //getchar();
//      }
//#endif
      TimeIncrement(*locDom);

//#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD)
//      cdh_1_0_0->Detect();
//      CD_Complete(cdh_1_0_0);
//      CD_BEGIN(cdh_1_0_0, "LagrangeNodal"); 
//#endif



#ifdef SEDOV_SYNC_POS_VEL_LATE
      Domain_member fieldData[6] ;
#endif

  Domain &domain = *locDom;
#if _CD && (SWITCH_2_0_0  >= SEQUENTIAL_CD)
      //CDHandle *cdh_2_0_0 = cdh_1_0_0;
      CDHandle *cdh_2_0_0 = cdh_1_0_0->Create(CD_MAP_2_0_0 >> CDFLAG_SIZE, 
                                  (string("LagrangeNodal")+cdh_1_0_0->label()).c_str(),
                                   CD_MAP_2_0_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_2_0_0));
      CD_BEGIN(cdh_2_0_0, "LagrangeNodal"); 
      cdh_2_0_0->Preserve(domain.serdes.SetOp(
                                (M__FX | M__FY | M__FZ |
                                 M__X  | M__Y  | M__Z  |
                                 M__XD | M__YD | M__ZD |
                                 M__XDD| M__YDD| M__ZDD )
                                ), kCopy | kSerdes, "SWITCH_2_0_0_LagrangeNodal"); 
#endif

      /* calculate nodal forces, accelerations, velocities, positions, with
       * applied boundary conditions and slide surface considerations */
      LagrangeNodal(*locDom);  


#if _CD && (SWITCH_2_0_0  >= SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_2_0_0, cdh_2_0_0);
#endif


#ifdef SEDOV_SYNC_POS_VEL_LATE
#endif

#if _CD && (SWITCH_2_1_0  > SEQUENTIAL_CD)
  
  CDHandle *cdh_2_1_0 = GetCurrentCD()->Create(CD_MAP_2_1_0 >> CDFLAG_SIZE, 
                                  (string("LagrangeElements")+GetCurrentCD()->label()).c_str(),
                                   CD_MAP_2_1_0 & CDFLAG_MASK, ERROR_FLAG_SHIFT(CD_MAP_2_1_0));
        cdh_2_1_0->Begin("LagrangeElements"); 
        cdh_2_1_0->Preserve(locDom->serdes.SetOp(
                 (M__VOLO | M__V | M__X  | M__Y  | M__Z  | 
                                   M__XD | M__YD | M__ZD |
                                   M__DXX| M__DYY| M__DZZ|
                                   M__QQ | M__QL | M__SS |
                                   M__Q  | M__E  | 
                                   M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
                                   M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA ) 
                  ), kCopy | kSerdes, "SWITCH_2_1_0_LagrangeElements"); 
#endif
      /* calculate element quantities (i.e. velocity gradient & q), and update
       * material states */
      LagrangeElements(*locDom, locDom->numElem());
      //LagrangeElements(domain, domain.numElem());
#if _CD && (SWITCH_2_1_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_2_1_0, cdh_2_1_0);
#endif

#if USE_MPI   
#ifdef SEDOV_SYNC_POS_VEL_LATE
      CommRecv(domain, MSG_SYNC_POS_VEL, 6,
               domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
               false, false) ;
   
      fieldData[0] = &Domain::x ;
      fieldData[1] = &Domain::y ;
      fieldData[2] = &Domain::z ;
      fieldData[3] = &Domain::xd ;
      fieldData[4] = &Domain::yd ;
      fieldData[5] = &Domain::zd ;
      
      CommSend(domain, MSG_SYNC_POS_VEL, 6, fieldData,
               domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
               false, false) ;
#endif
#endif   

#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD) && OPTIMIZE
   if(interval != 0 && ckpt_idx % interval == 0) 
   {
      st.BeginTimer();
      GetCurrentCD()->Preserve(domain.serdes.SetOp(preserve_vec_3), kCopy, "AfterUpdateVolumesForElems");
      st.EndTimer(3);

#ifdef _DEBUG_PRV
      if(st.myRank == 0) {
        printf("preserve vec 3 AfterUpdateVolumesForElems\n");
      }
#endif
   }
#endif




#if _CD && (SWITCH_2_2_0  > SEQUENTIAL_CD)
      CDHandle *cdh_2_2_0 = 0;
      CDMAPPING_BEGIN_NESTED_ONLY(CD_MAP_2_2_0, cdh_2_2_0, "CalcTimeConstraintsForElems");
      cdh_2_2_0->Preserve(&(domain.dtcourant()), sizeof(Real_t), kCopy, "SWITCH_2_2_0_dtcourant"); 
      cdh_2_2_0->Preserve(&(domain.dthydro()),   sizeof(Real_t), kCopy, "SWITCH_2_2_0_dthydro"); 
#elif _CD && (SWITCH_2_2_0 == SEQUENTIAL_CD)
      CDHandle *cdh_2_2_0 = GetLeafCD();
      CDMAPPING_BEGIN_SEQUENTIAL_ONLY(CD_MAP_2_2_0, cdh_2_2_0, "CalcTimeConstraintsForElems");
      cdh_2_2_0->Preserve(&(domain.dtcourant()), sizeof(Real_t), kCopy, "SWITCH_2_2_0_dtcourant"); 
      cdh_2_2_0->Preserve(&(domain.dthydro()),   sizeof(Real_t), kCopy, "SWITCH_2_2_0_dthydro"); 
#endif

      CalcTimeConstraintsForElems(domain);

#if _CD && (SWITCH_2_2_0  > SEQUENTIAL_CD)
      CDMAPPING_END_NESTED(CD_MAP_2_2_0, cdh_2_2_0);
#elif _CD && (SWITCH_2_2_0 == SEQUENTIAL_CD)
      cdh_2_2_0->Detect();
      CD_Complete(cdh_2_2_0); 
#endif

#if USE_MPI   
#ifdef SEDOV_SYNC_POS_VEL_LATE
      CommSyncPosVel(domain) ;
#endif
#endif  


#if _DEBUG
      if ((opts.showProg != 0) && (opts.quiet == 0) && (myRank == 0)) {
         printf("cycle = %d, time = %e, dt=%e\n",
                locDom->cycle(), double(locDom->time()), double(locDom->deltatime()) ) ;
      }
#endif

#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD)
     if(interval != 0 && ckpt_idx % interval == 0) 
     {
       st.FinishTimer();
     }
 
     ckpt_idx++;
 
     if(interval != 0 && ckpt_idx % interval == 0) 
     {
       CD_Complete(cdh_1_0_0);
       openclose = false; 
     }
#endif

#if _CD 
      double single_it = MPI_Wtime() - begin_tik;
      uint64_t domsize = locDom->serdes.SetOp(M__SERDES_ALL).GetTotalSize();
      loop_time += single_it;
      printf("single_it:%lf, %lu + %lu\n", single_it, domsize, sizeof(Domain));
#else
      double single_it = MPI_Wtime() - begin_tik;
      loop_time += single_it;
      printf("single_it:%lf\n", single_it);
#endif
   }

#if _CD && (SWITCH_1_0_0  >= SEQUENTIAL_CD)
   if(openclose) {
      CD_Complete(cdh_1_0_0);
   }
#endif

#if _CD && (SWITCH_0_0_0  > SEQUENTIAL_CD)
   cdh_0_0_0->Destroy();
//#elif _CD && (SWITCH_0_0_0 == SEQUENTIAL_CD)
#endif



   // Use reduced max elapsed time
   Real_t elapsed_time;
#if USE_MPI   
   elapsed_time = MPI_Wtime() - start;
#else
   elapsed_time = (clock() - start) / CLOCKS_PER_SEC;
#endif

   double elapsed_timeG;
#if USE_MPI   
   MPI_Reduce(&elapsed_time, &elapsed_timeG, 1, MPI_DOUBLE,
              MPI_MAX, 0, MPI_COMM_WORLD);
#else
   elapsed_timeG = elapsed_time;
#endif

#if _CD
   root_cd->Detect();
   root_cd->Complete();
#endif
  
//   printf("Done!"); getchar();

   // Write out final viz file */
   if (opts.viz) {
      DumpToVisit(*locDom, opts.numFiles, myRank, numRanks) ;
   }
   
   if ((myRank == 0) && (opts.quiet == 0)) {
      VerifyAndWriteFinalOutput(elapsed_timeG, *locDom, opts.nx, numRanks);
   }

   st.CheckTime();
#if _CD && ENABLE_HISTOGRAM
   st.Finalize();   
#endif

#if 0
//#if _CD && ENABLE_HISTOGRAM
   unsigned int *latency_history_global = NULL;
   unsigned int *latency_history_semiglobal = NULL;

   unsigned int *histogram_global = NULL; //[HISTO_SIZE]
   unsigned int *histogram_semiglobal = NULL; //[HISTO_SIZE]
   if(myRank == 0) {
     latency_history_global = new unsigned int[history_size * numRanks](); // zero-initialized
     histogram_global = new unsigned int[HISTO_SIZE * numRanks]();
   }
   if(myNodeRank == 0) {
     // numZeroRanks = 4
     // numRanks/numZeroRanks = 16
     latency_history_semiglobal = new unsigned int[history_size * numNodeRanks](); // zero-initialized
     histogram_semiglobal = new unsigned int[HISTO_SIZE * numNodeRanks]();
   }

   // Gather among ranks in the same node
   MPI_Gather(latency_history, history_size, MPI_UNSIGNED, 
              latency_history_semiglobal, history_size, MPI_UNSIGNED, 
              0, node_comm);

   MPI_Gather(histogram, HISTO_SIZE, MPI_UNSIGNED, 
              histogram_semiglobal, HISTO_SIZE, MPI_UNSIGNED, 
              0, node_comm);
   // Gather among zero ranks
   if(myNodeRank == 0) {
      MPI_Gather(latency_history_semiglobal, history_size * numNodeRanks, MPI_UNSIGNED, 
                 latency_history_global, history_size * numNodeRanks, MPI_UNSIGNED, 
                 0, zero_comm);
      MPI_Gather(histogram_semiglobal, HISTO_SIZE * numNodeRanks, MPI_UNSIGNED,
                 histogram_global, HISTO_SIZE * numNodeRanks, MPI_UNSIGNED, 
                 0, zero_comm);
   }

   if(myRank == 0) {
     char latency_history_filename[64];
     char unique_hist_filename[64];
     snprintf(latency_history_filename, 64, "latency.%s", histogram_filename); 
     snprintf(unique_hist_filename, 64, "histogram.%s", histogram_filename); 
     FILE *histfp = fopen(unique_hist_filename, "w");
     FILE *latencyfp = fopen(latency_history_filename, "w");
     for(uint32_t j=0; j<numRanks; j++) {
        uint32_t stride = HISTO_SIZE*j;
        for(uint32_t i=0; i<HISTO_SIZE; i++) {  
           fprintf(histfp, "%u,", histogram_global[i + stride]);
        }
        fprintf(histfp, "\n");

        stride = history_size*j;
        for(uint32_t i=0; i<history_size; i++) {   
           fprintf(latencyfp, "%u,", latency_history_global[i + stride]);
        }
        fprintf(latencyfp, "\n");
     }
     fprintf(histfp, "\n");
     fprintf(latencyfp, "\n");

     fclose(histfp);
     fclose(latencyfp);
   }

   printf("[Final] prv time  : %lf %lf %u %d\n",  prv_elapsed, prv_elapsed/ckpt_idx, prv_cnt, ckpt_idx);
{
   FILE *histfp = fopen(histogram_filename, "w");
//   uint32_t bin_idx=0;
//   for(auto hi=histogram.begin(); hi!=histogram.end(); ++hi) {
//      while(bin_idx < hi->first) {
//        fprintf(histfp, "0 ");
//        bin_idx++;
//      }
   for(uint32_t i=0; i<HISTO_SIZE; i++) {   
      fprintf(histfp, "%u ", histogram[i]);
   }
   fprintf(histfp, "\n");
   fprintf(histfp, "\n================================================================\n");
   for(uint32_t i=0; i<history_size; i++) {   
      fprintf(histfp, "%u,", latency_history[i]);
   }
   fprintf(histfp, "\n");
   fclose(histfp);
}
#endif
  
#if _CD
//   root_cd->Detect();
//   root_cd->Complete();
   CD_Finalize();
#endif

#if USE_MPI
   MPI_Finalize();
#endif

   return 0;
}
