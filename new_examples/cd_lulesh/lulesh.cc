/*
  This is a Version 2.0 MPI + OpenMP implementation of LULESH

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
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#if _OPENMP
# include <omp.h>
#endif

#include "lulesh.h"
#include "float.h"
Int_t myRank ;
Int_t numRanks ;


unsigned int global_counter = 0;
bool r_regElemSize = false;  
bool r_regNumList = false;   
bool r_regElemlist = false;  
int intvl0 = -1;
int intvl1 = -1;
int intvl2 = -1;
double wait_time = 0.0;
double wait_end = 0.0;
double dump_end = 0.0;
double dump_phase[5] = { 0, 0, 0, 0, 0 };
double exec_phase[5] = { 0, 0, 0, 0, 0 };
double totl_phase[5] = { 0, 0, 0, 0, 0 };
double begn_time = 0.0;
double cmpl_time = 0.0;
double begn_end = 0.0;
double cmpl_end = 0.0;
unsigned long prv_len = 0;

//#define PRINT_ONE(...) 
#define PRINT_ONE(...) if(myRank == 1) fprintf(stdout, __VA_ARGS__)
#if _CD
#include "packer_prof.h"
CDHandle *cd_main_loop = NULL;
CDHandle *cd_child_loop = NULL;
CDHandle *root_cd = NULL;
CDHandle *leaf_lv = NULL;
//CDHandle *cd_main_dummy = NULL;;
CDHandle *dummy_cd = NULL;
packer::MagicStore magic __attribute__((aligned(0x1000)));
using namespace cd;
#if 0
uint64_t prvec_all = ( M__X  | M__Y  | M__Z  | 
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
                              M__NODALMASS |// M__NODELIST | 
                              M__LXIM | M__LXIP | M__LETAM | 
                              M__LETAP | M__LZETAM | M__LZETAP | 
                              M__ELEMBC | M__ELEMMASS | M__REGELEMSIZE 
//                              | M__REGELEMLIST | M__REGELEMLIST_INNER | M__REG_NUMLIST 
                              );
#endif
uint64_t prvec_all = ( M__X  | M__Y  | M__Z  | 
                              M__XD | M__YD | M__ZD |
                              M__XDD| M__YDD| M__ZDD|
                              M__DXX| M__DYY| M__DZZ|
                              M__FX | M__FY | M__FZ |
                              M__QQ | M__QL | M__SS |
                              M__Q  | M__E  | M__VOLO | 
                              M__V | M__P |// M__VNEW |
                              M__DELV | M__VDOV | M__AREALG |
                              M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA |
                              M__SYMMX | M__SYMMY | M__SYMMZ |
                              M__NODALMASS | //M__NODELIST | 
                              M__LXIM | M__LXIP | M__LETAM | 
                              M__LETAP | M__LZETAM | M__LZETAP | 
                              M__ELEMBC | M__ELEMMASS | M__REGELEMSIZE 
//                              | M__REGELEMLIST | M__REGNUMLIST 
                              );

uint64_t prvec_ref = ( M__SYMMX | M__SYMMY | M__SYMMZ |
                              M__NODALMASS | M__NODELIST | 
                              M__LXIM | M__LXIP | M__LETAM | 
                              M__LETAP | M__LZETAM | M__LZETAP | 
                              M__ELEMBC | M__ELEMMASS | M__REGELEMSIZE 
//                              | M__REGELEMLIST | M__REGELEMLIST_INNER | M__REG_NUMLIST 
                              );
// CalcForceForNodes
uint64_t prvec_f   = ( M__FX | M__FY | M__FZ );
uint64_t prvec_pos = (M__X | M__Y | M__Z);
uint64_t prvec_vel = (M__XD | M__YD | M__ZD);
uint64_t prvec_acc = (M__XDD | M__YDD | M__ZDD);
uint64_t prvec_ref_0 = (prvec_pos | prvec_vel | M__NODELIST 
                      | M__P | M__Q | M__V | M__VOLO | M__SS | M__ELEMMASS);
uint64_t prvec_symm = (M__SYMMX|M__SYMMY|M__SYMMZ);
// CalcAccelerationForNodes ~ CalcPositionForNodes
uint64_t prvec_1   = ( M__X  | M__Y  | M__Z  | 
                              M__XD | M__YD | M__ZD |
                              M__XDD| M__YDD| M__ZDD|
                              M__NODELIST);
uint64_t prvec_elem = (M__DELV | M__VDOV | M__AREALG);
uint64_t prvec_q    = (M__QL | M__QQ);
uint64_t prvec_matrl = (M__E | M__P | M__Q | M__SS | M__V);
uint64_t prvec_posall = (prvec_acc | prvec_vel | prvec_pos);


// CalcLagrangeElements ~ CalcQForElems
uint64_t prvec_2   = ( M__DELV | M__VDOV | M__AREALG | M__ELEMBC |
                              M__DXX| M__DYY| M__DZZ|
                              M__DELV_ZETA  | M__DELV_XI  | M__DELV_ETA |
                              M__DELX_ZETA  | M__DELX_XI  | M__DELX_ETA );

// ApplyMaterialPropertiesForElems ~ UpdateVolumesForElems
uint64_t prvec_3   = ( M__QQ | M__QL | M__SS |
                              M__Q  | M__E  | M__VOLO | M__V | M__P );

uint64_t prvec_readwrite_all  = ( prvec_f | prvec_posall
                             | prvec_elem | prvec_q | prvec_matrl );
uint64_t prvec_readonly_all = (prvec_all & (PRVEC_MASK & (~prvec_readwrite_all)));

#endif
Domain *pDomain = NULL;
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
   Index_t nd0i = elemToNode[0] ;
   Index_t nd1i = elemToNode[1] ;
   Index_t nd2i = elemToNode[2] ;
   Index_t nd3i = elemToNode[3] ;
   Index_t nd4i = elemToNode[4] ;
   Index_t nd5i = elemToNode[5] ;
   Index_t nd6i = elemToNode[6] ;
   Index_t nd7i = elemToNode[7] ;
   if(nd0i < 0 || nd0i > 1000000000 ||
      nd1i < 0 || nd1i > 1000000000 ||
      nd2i < 0 || nd2i > 1000000000 ||
      nd3i < 0 || nd3i > 1000000000 ||
      nd4i < 0 || nd4i > 1000000000 ||
      nd5i < 0 || nd5i > 1000000000 ||
      nd6i < 0 || nd6i > 1000000000 ||
      nd7i < 0 || nd7i > 1000000000 //||
      //      Domain::restarted 
      ) {
     const Index_t base = Index_t(8)*domain.prv_idx; 
     printf("[%d] CHECK nodelist size:%zu/%zu (%d), %d %d %d %d %d %d %d %d\n", myRank, domain.m_nodelist.size(), domain.m_nodelist.capacity(), domain.numElem(),
                                                            domain.m_nodelist[base],
                                                            domain.m_nodelist[base+1],
                                                            domain.m_nodelist[base+2],
                                                            domain.m_nodelist[base+3],
                                                            domain.m_nodelist[base+4],
                                                            domain.m_nodelist[base+5],
                                                            domain.m_nodelist[base+6],
                                                            domain.m_nodelist[base+7]
                                                            ); 
   }

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
   //
   // pull in the stresses appropriate to the hydro integration
   //

#pragma omp parallel for firstprivate(numElem)
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
  static unsigned long counter = 0;
#if _CD
  if(myRank == 0 && 0) {
    bool print_detail = false;
    if(Domain::restarted) { 
       print_detail = counter++ % 4096 == 1;
    } else {
       print_detail = counter++ % 4096 == 1;
    }
    if(print_detail) {
      if(Domain::restarted) {
        LULESH_PRINT("\n[%d]..before..restared......\n", myRank);
      }
      LULESH_PRINT("x: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n", x0, x1, x2, x3, x4, x5, x6, x7);
      LULESH_PRINT("y: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n", y0, y1, y2, y3, y4, y5, y6, y7);
      LULESH_PRINT("z: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n", z0, z1, z2, z3, z4, z5, z6, z7);
    }
    
  }
#endif
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
  Real_t vol = Real_t(8.) * ( fjxet * cjxet + fjyet * cjyet + fjzet * cjzet);
  *volume = vol;
#if _CD
  if(vol < 0) {
    if(Domain::restarted && myRank == 0) {
        LULESH_PRINT("\n[%d]....restarted......\n", myRank);
        LULESH_PRINT("x: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n", x0, x1, x2, x3, x4, x5, x6, x7);
        LULESH_PRINT("y: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n", y0, y1, y2, y3, y4, y5, y6, y7);
        LULESH_PRINT("z: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n", z0, z1, z2, z3, z4, z5, z6, z7);
//        const Index_t base = Index_t(8)*pDomain->prv_idx; 
        LULESH_PRINT("[%d] CHECK nodelist size:%zu/%zu (%d), %d %d %d %d %d %d %d %d\n", 
                myRank, pDomain->m_nodelist.size(), pDomain->m_nodelist.capacity(), pDomain->numElem(),
                pDomain->m_nodelist[base],
                pDomain->m_nodelist[base+1],
                pDomain->m_nodelist[base+2],
                pDomain->m_nodelist[base+3],
                pDomain->m_nodelist[base+4],
                pDomain->m_nodelist[base+5],
                pDomain->m_nodelist[base+6],
                pDomain->m_nodelist[base+7]
                ); 
        LULESH_PRINT("volume:%le\n", vol);
        assert(0);
    }
     counter = 0;
     *volume = 0.0;
  }
#endif
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
#if _OPENMP
   Index_t numthreads = omp_get_max_threads();
#else
   Index_t numthreads = 1;
#endif

   Index_t numElem8 = numElem * 8 ;
   Real_t *fx_elem = NULL;
   Real_t *fy_elem = NULL;
   Real_t *fz_elem = NULL;
   Real_t fx_local[8] ;
   Real_t fy_local[8] ;
   Real_t fz_local[8] ;


  if (numthreads > 1) {
     fx_elem = Allocate<Real_t>(numElem8) ;
     fy_elem = Allocate<Real_t>(numElem8) ;
     fz_elem = Allocate<Real_t>(numElem8) ;
  }
  // loop over all elements

//#pragma omp parallel for firstprivate(numElem)
  for( Index_t k=0 ; k<numElem ; ++k )
  {
    const Index_t* const elemToNode = domain.nodelist(k);
    Real_t B[3][8] ;// shape function derivatives
    Real_t x_local[8] ;
    Real_t y_local[8] ;
    Real_t z_local[8] ;

    // get nodal coordinates from global arrays and copy into local arrays.
    CollectDomainNodesToElemNodes(domain, elemToNode, x_local, y_local, z_local);

    // Volume calculation involves extra work for numerical consistency
    CalcElemShapeFunctionDerivatives(x_local, y_local, z_local,
                                         B, &determ[k]);

    CalcElemNodeNormals( B[0], B[1], B[2],
                         x_local, y_local, z_local );

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
  }

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


#pragma omp parallel for firstprivate(numElem, hourg)
   for(Index_t i2=0;i2<numElem;++i2){
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

      CalcElemFBHourglassForce(xd1,yd1,zd1,
                      hourgam,
                      coefficient, hgfx, hgfy, hgfz);

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
   }

   if (numthreads > 1) {
     // Collect the data from the local arrays into the final force arrays
#pragma omp parallel for firstprivate(numNode)
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
   Index_t numElem = domain.numElem() ;
   Index_t numElem8 = numElem * 8 ;
   Real_t *dvdx = Allocate<Real_t>(numElem8) ;
   Real_t *dvdy = Allocate<Real_t>(numElem8) ;
   Real_t *dvdz = Allocate<Real_t>(numElem8) ;
   Real_t *x8n  = Allocate<Real_t>(numElem8) ;
   Real_t *y8n  = Allocate<Real_t>(numElem8) ;
   Real_t *z8n  = Allocate<Real_t>(numElem8) ;

   if(domain.cycle() == 63) {
//      printf("stop here\n");


   }
   /* start loop over elements */
//#pragma omp parallel for firstprivate(numElem)
   for (Index_t i=0 ; i<numElem ; ++i){
      Real_t  x1[8],  y1[8],  z1[8] ;
      Real_t pfx[8], pfy[8], pfz[8] ;

      Index_t* elemToNode = domain.nodelist(i);
      CollectDomainNodesToElemNodes(domain, elemToNode, x1, y1, z1);

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

      /* Do a check for negative volumes */
      if ( domain.v(i) <= Real_t(0.0) ) {
#if USE_MPI         
         MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
#else
         exit(VolumeError);
#endif
      }
   }

   if ( hgcoef > Real_t(0.) ) {
      CalcFBHourglassForceForElems( domain,
                                    determ, x8n, y8n, z8n, dvdx, dvdy, dvdz,
                                    hgcoef, numElem, domain.numNode()) ;
   }

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
   Index_t numElem = domain.numElem() ;
   if (numElem != 0) {
      Real_t  hgcoef = domain.hgcoef() ;
      Real_t *sigxx  = Allocate<Real_t>(numElem) ;
      Real_t *sigyy  = Allocate<Real_t>(numElem) ;
      Real_t *sigzz  = Allocate<Real_t>(numElem) ;
      Real_t *determ = Allocate<Real_t>(numElem) ;

      /* Sum contributions to total stress tensor */
      InitStressTermsForElems(domain, sigxx, sigyy, sigzz, numElem);

      // call elemlib stress integration loop to produce nodal forces from
      // material stresses.
      // [CDs] determ got changed here, and need to examine prev/rstr
      IntegrateStressForElems( domain,
                               sigxx, sigyy, sigzz, determ, numElem,
                               domain.numNode()) ;

      // check for negative element volume
//#pragma omp parallel for firstprivate(numElem)
      for ( Index_t k=0 ; k<numElem ; ++k ) {
         if (determ[k] <= Real_t(0.0)) {
#if USE_MPI            
            MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
#else
            exit(VolumeError);
#endif
         }
      }

      CalcHourglassControlForElems(domain, determ, hgcoef) ;

      Release(&determ) ;
      Release(&sigzz) ;
      Release(&sigyy) ;
      Release(&sigxx) ;
   }
}

/******************************************/

static inline void CalcForceForNodes(Domain& domain)
{
  Index_t numNode = domain.numNode() ;

#if USE_MPI  
  CommRecv(domain, MSG_COMM_SBN, 3,
           domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
           true, false) ;
#endif  

#pragma omp parallel for firstprivate(numNode)
  for (Index_t i=0; i<numNode; ++i) {
     domain.fx(i) = Real_t(0.0) ;
     domain.fy(i) = Real_t(0.0) ;
     domain.fz(i) = Real_t(0.0) ;
  }
#if _CD && _CD_CDRT
  // Out{FX,FY,FZ} <- In{X,Y,Z,XD,YD,ZD,FX,FY,FZ,NODELIST
  //                     P,Q,V,VOLO,SS,ELEMMASS}
  #if _LEAF_LV && _SCR == 0
    #if _FGCD
   double now = MPI_Wtime();
   CDHandle *leaf_cd = GetCurrentCD()->Create(numRanks, "CalcForce", kStrict|kLocalMemory, 0x1);
    #else
  CDHandle *leaf_cd = GetLeafCD();
  double now = MPI_Wtime();
    #endif
  CD_Begin(leaf_cd, "CalcForce");
  double prv_start = MPI_Wtime();
  begn_end += prv_start - now;
  #else
  CDHandle *leaf_cd = GetCurrentCD();
  double prv_start = MPI_Wtime();
  #endif

  #if _LEAF_LV  && _SCR == 0
    prv_len += leaf_cd->Preserve(dynamic_cast<Internal *>(&domain), sizeof(Internal), kCopy, "LeafDomain");
  #endif

  #if _CD_DUMMY
  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_f), kRef, "CalcForceCopy"); 
    PRINT_ONE("Prv LeafCD       CalcForce %luMB (Ref)\n", prv_len/1000000); }
  #else
  if(domain.check_begin(intvl0)) {
    prv_len += cd_main_loop->Preserve(domain.SetOp(prvec_f), kCopy, "CalcForceCopy");
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_f), kRef, "CalcForceCopy"); 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_f), kRef, "CalcForceCopy"); 
    PRINT_ONE("Prv Parent       CalcForce %luMB\n", prv_len/1000000); }
  else if(domain.check_begin(intvl1)) {
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_f), kCopy, "CalcForceCopy");
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_f), kRef, "CalcForceCopy"); 
    PRINT_ONE("Prv Child        CalcForce %luMB\n", prv_len/1000000); }
  #endif
  else if(domain.check_begin(intvl2) || (_LEAF_LV && _SCR == 0)) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_f), kCopy, "CalcForceCopy");
    PRINT_ONE("Prv LeafCD       CalcForce %luMB\n", prv_len/1000000); }
  double app_start = MPI_Wtime();  
  double prvtime = app_start - prv_start;
  dump_phase[0] = prvtime;
  dump_end  += prvtime;
#else // _CD_CDRT
  double app_start = MPI_Wtime();  
#endif
  /* Calcforce calls partial, force, hourq */
  CalcVolumeForceForElems(domain) ;

  double app_end = MPI_Wtime();
  exec_phase[0] = app_end - app_start;
#if _CD && _CD_CDRT
  #if _CD_DUMMY
  if(domain.check_end(intvl0)) {
    prv_len += root_cd->Preserve(domain.SetOp(prvec_f), kOutput, "CalcForceCopy");
    PRINT_ONE("Prv PDummy       CalcForce %luMB\n", prv_len/1000000); }
  else if(domain.check_end(intvl1)) {
    prv_len += dummy_cd->Preserve(domain.SetOp(prvec_f), kOutput, "CalcForceCopy");
    PRINT_ONE("Prv CDummy       CalcForce %luMB\n", prv_len/1000000); }
  double now2 = MPI_Wtime();
  prvtime = now2 - app_end;
  dump_phase[0] += prvtime;
  dump_end  += prvtime;
  #endif

  #if _LEAF_LV && _SCR == 0
  now = MPI_Wtime();
  leaf_cd->Detect();
  leaf_cd->Complete();
    #if _FGCD
  leaf_cd->Destroy();
  double then = MPI_Wtime();
  cmpl_end += then - now;
  leaf_cd = GetCurrentCD()->Create("LeafCD", kStrict|kLocalMemory, 0x1);
  begn_end += MPI_Wtime() - then;
    #else
  cmpl_end += MPI_Wtime() - now;
    #endif
  #endif
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
   
#pragma omp parallel for firstprivate(numNode)
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
   Index_t size = domain.sizeX();
   Index_t numNodeBC = (size+1)*(size+1) ;

#pragma omp parallel
   {
      if (!domain.symmXempty() != 0) {
#pragma omp for nowait firstprivate(numNodeBC)
         for(Index_t i=0 ; i<numNodeBC ; ++i)
            domain.xdd(domain.symmX(i)) = Real_t(0.0) ;
      }

      if (!domain.symmYempty() != 0) {
#pragma omp for nowait firstprivate(numNodeBC)
         for(Index_t i=0 ; i<numNodeBC ; ++i)
            domain.ydd(domain.symmY(i)) = Real_t(0.0) ;
      }

      if (!domain.symmZempty() != 0) {
#pragma omp for nowait firstprivate(numNodeBC)
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

#pragma omp parallel for firstprivate(numNode)
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
#pragma omp parallel for firstprivate(numNode)
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
#ifdef SEDOV_SYNC_POS_VEL_EARLY
   Domain_member fieldData[6] ;
#endif

   const Real_t delt = domain.deltatime() ;
   Real_t u_cut = domain.u_cut() ;

   /* time of boundary condition evaluation is beginning of step for force and
    * acceleration boundary conditions. */
//   CD_Begin(root_cd, "Root");
//   root_cd->Preserve(locDom->SetOp(), kCopy, "Root_All");
   // Out{FX,FY,FZ} <- In{X,Y,Z,XD,YD,ZD,FX,FY,FZ,NODELIST
   //                     P,Q,V,VOLO,SS,ELEMMASS}
   CalcForceForNodes(domain);
   domain.CheckUpdate("CalcForceForNodes");

#if USE_MPI  
#ifdef SEDOV_SYNC_POS_VEL_EARLY
   CommRecv(domain, MSG_SYNC_POS_VEL, 6,
            domain.sizeX() + 1, domain.sizeY() + 1, domain.sizeZ() + 1,
            false, false) ;
#endif
#endif
   
#if _CD && _CD_CDRT
   // Out{XDD,YDD,ZDD} <- In{XDD,YDD,ZDD,FX,FY,FZ,SYMMX,SYMMY,SYMMZ}
  #if _LEAF_LV && _POS_VEL_ACC && _SCR == 0
  CDHandle *leaf_cd = GetLeafCD();
  double now = MPI_Wtime();
  CD_Begin(leaf_cd, "CalcPos");
  double prv_start = MPI_Wtime();
  begn_end += prv_start - now;
  #else
  CDHandle *leaf_cd = GetCurrentCD();
  double prv_start = MPI_Wtime();
  #endif
//  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) // leaf always preserve per loop 
//  {
//  #if _CD_DUMMY
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_posall), kRef, "PosVelAcc");
//  #else
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_posall), kCopy, "PosVelAcc");
//  #endif
////    if(myRank == 1) printf("Prv %s       PosVelAcc %luMB\n", domain.check_begin(intvl0)? "Parent":"Child", prv_len/1000000);
//    dump_end  += MPI_Wtime() - prv_start;
//    dump_phase[1] = MPI_Wtime() - prv_start;
//  } else {
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_posall), kCopy, "PosVelAcc");
//  }
  #if _LEAF_LV && _POS_VEL_ACC && _SCR == 0
    prv_len += leaf_cd->Preserve(dynamic_cast<Internal *>(&domain), sizeof(Internal), kCopy, "LeafDomain");
  #endif
  #if _CD_DUMMY
  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_posall), kRef, "PosVelAcc"); 
    PRINT_ONE("Prv LeafCD       PosVelAcc %luMB (Ref)\n", prv_len/1000000); }

  #else

  if(domain.check_begin(intvl0)) {
    prv_len += cd_main_loop->Preserve(domain.SetOp(prvec_posall), kCopy, "PosVelAcc");
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_posall), kRef, "PosVelAcc"); 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_posall), kRef, "PosVelAcc"); 
    PRINT_ONE("Prv Parent       PosVelAcc %luMB\n", prv_len/1000000); }
  else if(domain.check_begin(intvl1)) {
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_posall), kCopy, "PosVelAcc");
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_posall), kRef, "PosVelAcc"); 
    PRINT_ONE("Prv Child        PosVelAcc %luMB\n", prv_len/1000000); }

  #endif
  //else if(_LEAF_LV && _POS_VEL_ACC) { // leaf always preserve per loop 
  else if(domain.check_begin(intvl2) && _POS_VEL_ACC) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_posall), kCopy, "PosVelAcc");
    PRINT_ONE("Prv LeafCD       PosVelAcc %luMB\n", prv_len/1000000); }
  double app_start = MPI_Wtime();  
  double prvtime = app_start - prv_start;
  dump_phase[1] = prvtime;
  dump_end  += prvtime;
#else // _CD_CDRT
  double app_start = MPI_Wtime();  
#endif
   CalcAccelerationForNodes(domain, domain.numNode());
   
   ApplyAccelerationBoundaryConditionsForNodes(domain);

   domain.CheckUpdate("ApplyAccBoundaryCond");

   // Out{XD,YD,ZD} <- In{XD,YD,ZD,XDD,YDD,ZDD}
   CalcVelocityForNodes( domain, delt, u_cut, domain.numNode()) ;
   domain.CheckUpdate("CalcVelocity");

   // Out{X,Y,Z} <- In{X,Y,Z,XD,YD,ZD}
   CalcPositionForNodes( domain, delt, domain.numNode() );
   domain.CheckUpdate("CalcPosition");

  double app_end = MPI_Wtime();
  exec_phase[1] = app_end - app_start;
#if _CD && _CD_CDRT
  #if _CD_DUMMY
  if(domain.check_end(intvl0)) {
    prv_len += root_cd->Preserve(domain.SetOp(prvec_posall), kOutput, "PosVelAcc");
    PRINT_ONE("Prv PDummy       PosVelAcc %luMB\n", prv_len/1000000); }
  else if(domain.check_end(intvl1)) {
    prv_len += dummy_cd->Preserve(domain.SetOp(prvec_posall), kOutput, "PosVelAcc");
    PRINT_ONE("Prv CDummy       PosVelAcc %luMB\n", prv_len/1000000); }
  double now2 = MPI_Wtime();
  prvtime = now2 - app_end;
  dump_phase[1] += prvtime;
  dump_end  += prvtime;
  #endif

  #if _LEAF_LV && _POS_VEL_ACC && _SCR == 0
  double then = MPI_Wtime();
  leaf_cd->Detect();
  leaf_cd->Complete();
  cmpl_end += MPI_Wtime() - then;
  #endif
#endif

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
   double wait_start = MPI_Wtime();
   CommSyncPosVel(domain) ;
   double wait_finish = MPI_Wtime() - wait_start;
   wait_end += wait_finish;
#endif
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

  // loop over all elements
#pragma omp parallel for firstprivate(numElem, deltaTime)
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

    // get nodal coordinates from global arrays and copy into local arrays.
    CollectDomainNodesToElemNodes(domain, elemToNode, x_local, y_local, z_local);

    // volume calculations
    volume = CalcElemVolume(x_local, y_local, z_local );
    relativeVolume = volume / domain.volo(k) ;
    vnew[k] = relativeVolume ;
    domain.delv(k) = relativeVolume - domain.v(k) ;

    // set characteristic length
    domain.arealg(k) = CalcElemCharacteristicLength(x_local, y_local, z_local,
                                             volume);

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

    CalcElemShapeFunctionDerivatives( x_local, y_local, z_local,
                                      B, &detJ );

    CalcElemVelocityGradient( xd_local, yd_local, zd_local,
                               B, detJ, D );

    // put velocity gradient quantities into their global arrays.
    domain.dxx(k) = D[0];
    domain.dyy(k) = D[1];
    domain.dzz(k) = D[2];
  }
}

/******************************************/

static inline
void CalcLagrangeElements(Domain& domain, Real_t* vnew)
{
   Index_t numElem = domain.numElem() ;
   if (numElem > 0) {
      const Real_t deltatime = domain.deltatime() ;

      domain.AllocateStrains(numElem);

      CalcKinematicsForElems(domain, vnew, deltatime, numElem) ;

      // element loop to do some stuff not included in the elemlib function.
#pragma omp parallel for firstprivate(numElem)
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
#if USE_MPI           
           MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
#else
           exit(VolumeError);
#endif
        }
      }
      domain.DeallocateStrains();
   }
}

/******************************************/

static inline
void CalcMonotonicQGradientsForElems(Domain& domain, Real_t vnew[])
{
   Index_t numElem = domain.numElem();

#pragma omp parallel for firstprivate(numElem)
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
   }
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

#pragma omp parallel for firstprivate(qlc_monoq, qqc_monoq, monoq_limiter_mult, monoq_max_slope, ptiny)
   for ( Index_t ielem = 0 ; ielem < domain.regElemSize(r); ++ielem ) {
      Index_t i = domain.regElemlist(r,ielem);
      Real_t qlin, qquad ;
      Real_t phixi, phieta, phizeta ;
      Int_t bcMask = domain.elemBC(i) ;
      Real_t delvm = 0.0, delvp =0.0;

      /*  phixi     */
      Real_t norm = Real_t(1.) / (domain.delv_xi(i)+ ptiny ) ;

      switch (bcMask & XI_M) {
         case XI_M_COMM: /* needs comm data */
         case 0:         delvm = domain.delv_xi(domain.lxim(i)); break ;
         case XI_M_SYMM: delvm = domain.delv_xi(i) ;       break ;
         case XI_M_FREE: delvm = Real_t(0.0) ;      break ;
         default:          fprintf(stderr, "Error in switch at %s line %d\n",
                                   __FILE__, __LINE__);
            delvm = 0; /* ERROR - but quiets the compiler */
            break;
      }
      switch (bcMask & XI_P) {
         case XI_P_COMM: /* needs comm data */
         case 0:         delvp = domain.delv_xi(domain.lxip(i)) ; break ;
         case XI_P_SYMM: delvp = domain.delv_xi(i) ;       break ;
         case XI_P_FREE: delvp = Real_t(0.0) ;      break ;
         default:          fprintf(stderr, "Error in switch at %s line %d\n",
                                   __FILE__, __LINE__);
            delvp = 0; /* ERROR - but quiets the compiler */
            break;
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
         default:          fprintf(stderr, "Error in switch at %s line %d\n",
                                   __FILE__, __LINE__);
            delvm = 0; /* ERROR - but quiets the compiler */
            break;
      }
      switch (bcMask & ETA_P) {
         case ETA_P_COMM: /* needs comm data */
         case 0:          delvp = domain.delv_eta(domain.letap(i)) ; break ;
         case ETA_P_SYMM: delvp = domain.delv_eta(i) ;        break ;
         case ETA_P_FREE: delvp = Real_t(0.0) ;        break ;
         default:          fprintf(stderr, "Error in switch at %s line %d\n",
                                   __FILE__, __LINE__);
            delvp = 0; /* ERROR - but quiets the compiler */
            break;
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
         default:          fprintf(stderr, "Error in switch at %s line %d\n",
                                   __FILE__, __LINE__);
            delvm = 0; /* ERROR - but quiets the compiler */
            break;
      }
      switch (bcMask & ZETA_P) {
         case ZETA_P_COMM: /* needs comm data */
         case 0:           delvp = domain.delv_zeta(domain.lzetap(i)) ; break ;
         case ZETA_P_SYMM: delvp = domain.delv_zeta(i) ;         break ;
         case ZETA_P_FREE: delvp = Real_t(0.0) ;          break ;
         default:          fprintf(stderr, "Error in switch at %s line %d\n",
                                   __FILE__, __LINE__);
            delvp = 0; /* ERROR - but quiets the compiler */
            break;
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
   }
}

/******************************************/

static inline
void CalcMonotonicQForElems(Domain& domain, Real_t vnew[])
{  
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

static inline
void CalcQForElems(Domain& domain, Real_t vnew[])
{
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

      /* Calculate velocity gradients */
      CalcMonotonicQGradientsForElems(domain, vnew);

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

      CalcMonotonicQForElems(domain, vnew) ;

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
#if USE_MPI         
         MPI_Abort(MPI_COMM_WORLD, QStopError) ;
#else
         exit(QStopError);
#endif
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
#pragma omp parallel for firstprivate(length)
   for (Index_t i = 0; i < length ; ++i) {
      Real_t c1s = Real_t(2.0)/Real_t(3.0) ;
      bvc[i] = c1s * (compression[i] + Real_t(1.));
      pbvc[i] = c1s;
   }

#pragma omp parallel for firstprivate(length, pmin, p_cut, eosvmax)
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

#pragma omp parallel for firstprivate(length, emin)
   for (Index_t i = 0 ; i < length ; ++i) {
      e_new[i] = e_old[i] - Real_t(0.5) * delvc[i] * (p_old[i] + q_old[i])
         + Real_t(0.5) * work[i];

      if (e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }

   CalcPressureForElems(pHalfStep, bvc, pbvc, e_new, compHalfStep, vnewc,
                        pmin, p_cut, eosvmax, length, regElemList);

#pragma omp parallel for firstprivate(length, rho0)
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

#pragma omp parallel for firstprivate(length, emin, e_cut)
   for (Index_t i = 0 ; i < length ; ++i) {

      e_new[i] += Real_t(0.5) * work[i];

      if (FABS(e_new[i]) < e_cut) {
         e_new[i] = Real_t(0.)  ;
      }
      if (     e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }

   CalcPressureForElems(p_new, bvc, pbvc, e_new, compression, vnewc,
                        pmin, p_cut, eosvmax, length, regElemList);

#pragma omp parallel for firstprivate(length, rho0, emin, e_cut)
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

   CalcPressureForElems(p_new, bvc, pbvc, e_new, compression, vnewc,
                        pmin, p_cut, eosvmax, length, regElemList);

#pragma omp parallel for firstprivate(length, rho0, q_cut)
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

static inline
void CalcSoundSpeedForElems(Domain &domain,
                            Real_t *vnewc, Real_t rho0, Real_t *enewc,
                            Real_t *pnewc, Real_t *pbvc,
                            Real_t *bvc, Real_t ss4o3,
                            Index_t len, Index_t *regElemList)
{
#pragma omp parallel for firstprivate(rho0, ss4o3)
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
#pragma omp parallel
      {
#pragma omp for nowait firstprivate(numElemReg)
         for (Index_t i=0; i<numElemReg; ++i) {
            Index_t elem = regElemList[i];
            e_old[i] = domain.e(elem) ;
            delvc[i] = domain.delv(elem) ;
            p_old[i] = domain.p(elem) ;
            q_old[i] = domain.q(elem) ;
            qq_old[i] = domain.qq(elem) ;
            ql_old[i] = domain.ql(elem) ;
         }

#pragma omp for firstprivate(numElemReg)
         for (Index_t i = 0; i < numElemReg ; ++i) {
            Index_t elem = regElemList[i];
            Real_t vchalf ;
            compression[i] = Real_t(1.) / vnewc[elem] - Real_t(1.);
            vchalf = vnewc[elem] - delvc[i] * Real_t(.5);
            compHalfStep[i] = Real_t(1.) / vchalf - Real_t(1.);
         }

      /* Check for v > eosvmax or v < eosvmin */
         if ( eosvmin != Real_t(0.) ) {
#pragma omp for nowait firstprivate(numElemReg, eosvmin)
            for(Index_t i=0 ; i<numElemReg ; ++i) {
               Index_t elem = regElemList[i];
               if (vnewc[elem] <= eosvmin) { /* impossible due to calling func? */
                  compHalfStep[i] = compression[i] ;
               }
            }
         }
         if ( eosvmax != Real_t(0.) ) {
#pragma omp for nowait firstprivate(numElemReg, eosvmax)
            for(Index_t i=0 ; i<numElemReg ; ++i) {
               Index_t elem = regElemList[i];
               if (vnewc[elem] >= eosvmax) { /* impossible due to calling func? */
                  p_old[i]        = Real_t(0.) ;
                  compression[i]  = Real_t(0.) ;
                  compHalfStep[i] = Real_t(0.) ;
               }
            }
         }

#pragma omp for nowait firstprivate(numElemReg)
         for (Index_t i = 0 ; i < numElemReg ; ++i) {
            work[i] = Real_t(0.) ; 
         }
      }
      CalcEnergyForElems(p_new, e_new, q_new, bvc, pbvc,
                         p_old, e_old,  q_old, compression, compHalfStep,
                         vnewc, work,  delvc, pmin,
                         p_cut, e_cut, q_cut, emin,
                         qq_old, ql_old, rho0, eosvmax,
                         numElemReg, regElemList);
   }

#pragma omp parallel for firstprivate(numElemReg)
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

static inline
void ApplyMaterialPropertiesForElems(Domain& domain, Real_t vnew[])
{
   Index_t numElem = domain.numElem() ;

  if (numElem != 0) {
    /* Expose all of the variables needed for material evaluation */
    Real_t eosvmin = domain.eosvmin() ;
    Real_t eosvmax = domain.eosvmax() ;

#pragma omp parallel
    {
       // Bound the updated relative volumes with eosvmin/max
       if (eosvmin != Real_t(0.)) {
#pragma omp for firstprivate(numElem)
          for(Index_t i=0 ; i<numElem ; ++i) {
             if (vnew[i] < eosvmin)
                vnew[i] = eosvmin ;
          }
       }

       if (eosvmax != Real_t(0.)) {
#pragma omp for nowait firstprivate(numElem)
          for(Index_t i=0 ; i<numElem ; ++i) {
             if (vnew[i] > eosvmax)
                vnew[i] = eosvmax ;
          }
       }

       // This check may not make perfect sense in LULESH, but
       // it's representative of something in the full code -
       // just leave it in, please
#pragma omp for nowait firstprivate(numElem)
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
#if USE_MPI             
             MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
#else
             exit(VolumeError);
#endif
          }
       }
    }

    for (Int_t r=0 ; r<domain.numReg() ; r++) {
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
    }

  }
}

/******************************************/

static inline
void UpdateVolumesForElems(Domain &domain, Real_t *vnew,
                           Real_t v_cut, Index_t length)
{
   if (length != 0) {
#pragma omp parallel for firstprivate(length, v_cut)
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
  Real_t *vnew = Allocate<Real_t>(numElem) ;  /* new relative vol -- temp */

#if _CD && _CD_CDRT
  #if _LEAF_LV && _LAGRANGE_ELEM && _SCR == 0
  CDHandle *leaf_cd = GetLeafCD();
  double now = MPI_Wtime();
  CD_Begin(leaf_cd, "LagrangeElem");
  double prv_start = MPI_Wtime();
  begn_end += prv_start - now;
  #else
  CDHandle *leaf_cd = GetCurrentCD();
  double prv_start = MPI_Wtime();
  #endif
//  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) // leaf always preserve per loop 
//  { 
//  #if _CD_DUMMY
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_elem), kRef, "LagrangeElem");
//  #else
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_elem), kCopy, "LagrangeElem");
//  #endif
////    if(myRank == 1) printf("Prv %s    LagrangeElem %luMB\n", domain.check_begin(intvl0)? "Parent":"Child", prv_len/1000000);
//    dump_end  += MPI_Wtime() - prv_start;
//    dump_phase[2] = MPI_Wtime() - prv_start;
//  } else {
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_elem), kCopy, "LagrangeElem");
//  }
  #if _LEAF_LV && _LAGRANGE_ELEM && _SCR == 0
    prv_len += leaf_cd->Preserve(dynamic_cast<Internal *>(&domain), sizeof(Internal), kCopy, "LeafDomain");
  #endif
  #if _CD_DUMMY
  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_elem), kRef, "LagrangeElem"); 
    PRINT_ONE("Prv LeafCD    LagrangeElem %luMB (Ref)\n", prv_len/1000000); }

  #else

  if(domain.check_begin(intvl0)) {
    prv_len += cd_main_loop->Preserve(domain.SetOp(prvec_elem), kCopy, "LagrangeElem");
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_elem), kRef, "LagrangeElem"); 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_elem), kRef, "LagrangeElem"); 
    PRINT_ONE("Prv Parent     LagrangeElem %luMB\n", prv_len/1000000); }
  else if(domain.check_begin(intvl1)) {
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_elem), kCopy, "LagrangeElem");
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_elem), kRef, "LagrangeElem"); 
    PRINT_ONE("Prv Child     LagrangeElem %luMB\n", prv_len/1000000); }

  #endif
  //else if(_LEAF_LV && _LAGRANGE_ELEM) { // leaf always preserve per loop 
  else if(domain.check_begin(intvl2) && _LAGRANGE_ELEM) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_elem), kCopy, "LagrangeElem");
    PRINT_ONE("Prv LeafCD    LagrangeElem %luMB\n", prv_len/1000000); }
//  if(myRank == 1) {
//    if(domain.check_begin(intvl0) || domain.check_begin(intvl1))
//      printf("Prv %s    LagrangeElem %luMB\n", domain.check_begin(intvl0)? "Parent":"Child", prv_len/1000000);
//    else
//      printf("Prv LeafCD    LagrangeElem %luMB\n", prv_len/1000000);
//  }
  double app_start = MPI_Wtime();  
  double prvtime = app_start - prv_start;
  dump_phase[2] = prvtime;
  dump_end  += prvtime;
#else // _CD_CDRT
  double app_start = MPI_Wtime();  
#endif

  // Out{DELV,VDOV,AREALG} <- In{X,Y,Z,XD,YD,ZD,NODELIST
  //                             DXX,DYY,DZZ,V,VOLO,DELV,VDOV,AREALG}
  CalcLagrangeElements(domain, vnew) ;
  domain.CheckUpdate("CalcLagrangeElements");

  double app_end = MPI_Wtime();
  exec_phase[2] = app_end - app_start;

#if _CD && _CD_CDRT
  #if _CD_DUMMY
  if(domain.check_end(intvl0)) {
    prv_len += root_cd->Preserve(domain.SetOp(prvec_elem), kOutput, "LagrangeElem");
    PRINT_ONE("Prv PDummy    LagrangeElem %luMB\n", prv_len/1000000); }
  else if(domain.check_end(intvl1)) {
    prv_len += dummy_cd->Preserve(domain.SetOp(prvec_elem), kOutput, "LagrangeElem");
    PRINT_ONE("Prv CDummy    LagrangeElem %luMB\n", prv_len/1000000); }
  double now1 = MPI_Wtime();
  prvtime = now1 - app_end;
  dump_phase[2] += prvtime;
  dump_end  += prvtime;
  #endif

  #if _LEAF_LV && _LAGRANGE_ELEM && _SCR == 0
  double then = MPI_Wtime();
  leaf_cd->Detect();
  leaf_cd->Complete();
  double now2 = MPI_Wtime();
  cmpl_end += now2 - then;
  #else
  double now2 = MPI_Wtime();
  #endif
  #if _LEAF_LV && _CALC_FOR_ELEM && _SCR == 0
  CD_Begin(leaf_cd, "CalcQForElems");
  prv_start = MPI_Wtime();
  begn_end += prv_start - now2;
  #else
  prv_start = MPI_Wtime();
  #endif

//  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) // leaf always preserve per loop 
//  {
//  #if _CD_DUMMY
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_q), kRef, "QforElem");
//  #else
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_q), kCopy, "QforElem");
//  #endif
////    if(myRank == 1) printf("Prv %s        QforElem %luMB\n", domain.check_begin(intvl0)? "Parent":"Child", prv_len/1000000);
//    dump_end  += MPI_Wtime() - prv_start;
//    dump_phase[3] = MPI_Wtime() - prv_start;
//  } else {
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_q), kCopy, "QforElem");
//  }
  #if _LEAF_LV && _CALC_FOR_ELEM && _SCR == 0
    prv_len += leaf_cd->Preserve(dynamic_cast<Internal *>(&domain), sizeof(Internal), kCopy, "LeafDomain");
  #endif
  #if _CD_DUMMY
  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_q), kRef, "QforElem"); 
    PRINT_ONE("Prv LeafCD        QforElem %luMB (Ref)\n", prv_len/1000000); }

  #else

  if(domain.check_begin(intvl0)) {
    prv_len += cd_main_loop->Preserve(domain.SetOp(prvec_q), kCopy, "QforElem");
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_q), kRef, "QforElem");
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_q), kRef, "QforElem"); 
    PRINT_ONE("Prv Parent         QforElem %luMB\n", prv_len/1000000); }
  else if(domain.check_begin(intvl1)) {
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_q), kCopy, "QforElem");
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_q), kRef, "QforElem"); 
    PRINT_ONE("Prv Child         QforElem %luMB\n", prv_len/1000000); }

  #endif
//  else if(_LEAF_LV  && _CALC_FOR_ELEM) { // leaf always preserve per loop 
  else if(domain.check_begin(intvl2) && _CALC_FOR_ELEM) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_q), kCopy, "QforElem");
    PRINT_ONE("Prv LeafCD        QforElem %luMB\n", prv_len/1000000); }
//  if(myRank == 1) {
//    if(domain.check_begin(intvl0) || domain.check_begin(intvl1))
//      printf("Prv %s        QforElem %luMB\n", domain.check_begin(intvl0)? "Parent":"Child", prv_len/1000000);
//    else
//      printf("Prv LeafCD        QforElem %luMB\n", prv_len/1000000);
//  }
  app_start = MPI_Wtime();  
  prvtime = app_start - prv_start;
  dump_phase[3] = prvtime;
  dump_end  += prvtime;
#else // _CD_CDRT
  app_start = MPI_Wtime();  
#endif

  /* Calculate Q.  (Monotonic q option requires communication) */
  // Out{QL,QQ} <- In{regElemSize,regElemlist,X,Y,Z,XD,YD,ZD,NODELIST
  //                  LXIM,LXIP,LETAM,LETAP,LZETAM,LZETAP,ELEMBC,
  //                  DELV_XI,DELV_ETA,DELV_ZETA,DELX_XI,DELX_ETA,DELX_ZETA,
  //                  Q,QL,QQ,VOLO,VDOV,ELEMMASS}
  CalcQForElems(domain, vnew) ;
  domain.CheckUpdate("CalcQForElems");
  app_end = MPI_Wtime();
  exec_phase[3] = app_end - app_start;

#if _CD && _CD_CDRT
  #if _CD_DUMMY
  if(domain.check_end(intvl0)) {
    prv_len += root_cd->Preserve(domain.SetOp(prvec_q), kOutput, "QforElem");
    PRINT_ONE("Prv PDummy        QforElem %luMB\n", prv_len/1000000); }
  else if(domain.check_end(intvl1)) {
    prv_len += dummy_cd->Preserve(domain.SetOp(prvec_q), kOutput, "QforElem");
    PRINT_ONE("Prv CDummy        QforElem %luMB\n", prv_len/1000000); }
  double now3 = MPI_Wtime();
  prvtime = now3 - app_end;
  dump_phase[3] += prvtime;
  dump_end  += prvtime;
  #endif

  #if _LEAF_LV && _CALC_FOR_ELEM && _SCR == 0
  then = MPI_Wtime();
  leaf_cd->Detect();
  leaf_cd->Complete();
  double now4 = MPI_Wtime();
  cmpl_end += now4 - then;
  #else
  double now4 = MPI_Wtime();
  #endif
  #if _LEAF_LV && _MATERIAL_PROP && _SCR == 0
  CD_Begin(leaf_cd, "ApplyMaterialPropertiesForElems");
  prv_start = MPI_Wtime();
  begn_end += prv_start - now4;
  #else
  prv_start = MPI_Wtime();
  #endif

//  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) // leaf always preserve per loop 
//  {
//  #if _CD_DUMMY
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_matrl), kRef, "MaterialforElem");
//  #else
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_matrl), kCopy, "MaterialforElem");
//  #endif
////    if(myRank == 1) printf("Prv %s MaterialForElem %luMB\n", domain.check_begin(intvl0)? "Parent":"Child", prv_len/1000000);
//    dump_end  += MPI_Wtime() - prv_start;
//    dump_phase[4] = MPI_Wtime() - prv_start;
//  } else {
//    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_matrl), kCopy, "MaterialforElem");
//  }
  #if _LEAF_LV   && _MATERIAL_PROP && _SCR == 0
    prv_len += leaf_cd->Preserve(dynamic_cast<Internal *>(&domain), sizeof(Internal), kCopy, "LeafDomain");
  #endif

  #if _CD_DUMMY
  if(domain.check_begin(intvl0) || domain.check_begin(intvl1)) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_matrl), kRef, "MaterialforElem"); 
    PRINT_ONE("Prv LeafCD MaterialForElem %luMB (Ref)\n", prv_len/1000000); }

  #else

  if(domain.check_begin(intvl0)) {
    prv_len += cd_main_loop->Preserve(domain.SetOp(prvec_matrl), kCopy, "MaterialforElem"); 
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_matrl), kRef, "MaterialforElem"); 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_matrl), kRef, "MaterialforElem"); 
    PRINT_ONE("Prv Parent MaterialForElem %luMB\n", prv_len/1000000); }
  else if(domain.check_begin(intvl1)) {
    prv_len += cd_child_loop->Preserve(domain.SetOp(prvec_matrl), kCopy, "MaterialforElem"); 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_matrl), kRef, "MaterialforElem"); 
    PRINT_ONE("Prv Child  MaterialForElem %luMB\n", prv_len/1000000); }
  #endif
  //else if(_LEAF_LV && _MATERIAL_PROP) { // leaf always preserve per loop 
  else if(domain.check_begin(intvl2) && _MATERIAL_PROP) { // leaf always preserve per loop 
    prv_len += leaf_cd->Preserve(domain.SetOp(prvec_matrl), kCopy, "MaterialforElem");
    PRINT_ONE("Prv LeafCD MaterialForElem %luMB\n", prv_len/1000000); }
//  if(myRank == 0) {
//    if(domain.check_begin(intvl0) || domain.check_begin(intvl1))
//      printf("Prv %s MaterialForElem %luMB\n", domain.check_begin(intvl0)? "Parent":"Child", prv_len/1000000);
//    else
//      printf("Prv LeafCD MaterialForElem %luMB\n", prv_len/1000000);
//  }
  app_start = MPI_Wtime();  
  prvtime = app_start - prv_start;
  dump_phase[4] = prvtime;
  dump_end  += prvtime;
#else // _CD_CDRT
  app_start = MPI_Wtime();  
#endif


  // Out{E,P,Q,SS} <- In{regElemSize,regElemlist,E,P,Q,QL,QQ,V,DELV,SS}
  ApplyMaterialPropertiesForElems(domain, vnew) ;
  domain.CheckUpdate("ApplyMaterialPropertiesForElems");

  // Out{V} <- In{V}
  UpdateVolumesForElems(domain, vnew,
                        domain.v_cut(), numElem) ;
  domain.CheckUpdate("UpdateVolunesForElems");

  app_end = MPI_Wtime();
  exec_phase[4] = app_end - app_start;

#if _CD && _CD_CDRT

  #if _CD_DUMMY
  if(domain.check_end(intvl0)) {
    prv_len += root_cd->Preserve(domain.SetOp(prvec_matrl), kOutput, "MaterialforElem");
    PRINT_ONE("Prv PDummy MaterialforElem %luMB\n", prv_len/1000000); }
  else if(domain.check_end(intvl1)) { 
    prv_len += dummy_cd->Preserve(domain.SetOp(prvec_matrl), kOutput, "MaterialforElem");
    PRINT_ONE("Prv CDummy MaterialforElem %luMB\n", prv_len/1000000); }
  double now5 = MPI_Wtime();
  prvtime = now5 - app_end;
  dump_phase[4] += prvtime;
  dump_end  += prvtime;
  #endif

  #if _LEAF_LV && _MATERIAL_PROP && _SCR == 0
  then = MPI_Wtime();
  leaf_cd->Detect();
  leaf_cd->Complete();
  double now6 = MPI_Wtime();
  cmpl_end += now6 - then;
  #endif
#endif
  Release(&vnew);
}

/******************************************/

static inline
void CalcCourantConstraintForElems(Domain &domain, Index_t length,
                                   Index_t *regElemlist,
                                   Real_t qqc, Real_t& dtcourant)
{
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


#pragma omp parallel firstprivate(length, qqc)
   {
      Real_t   qqc2 = Real_t(64.0) * qqc * qqc ;
      Real_t   dtcourant_tmp = dtcourant;
      Index_t  courant_elem  = -1 ;

#if _OPENMP
      Index_t thread_num = omp_get_thread_num();
#else
      Index_t thread_num = 0;
#endif      

#pragma omp for 
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

#pragma omp parallel firstprivate(length, dvovmax)
   {
      Real_t dthydro_tmp = dthydro ;
      Index_t hydro_elem = -1 ;

#if _OPENMP      
      Index_t thread_num = omp_get_thread_num();
#else      
      Index_t thread_num = 0;
#endif      

#pragma omp for
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

   // Initialize conditions to a very large value
   domain.dtcourant() = 1.0e+20;
   domain.dthydro() = 1.0e+20;

   for (Index_t r=0 ; r < domain.numReg() ; ++r) {
      /* evaluate time constraint */
      CalcCourantConstraintForElems(domain, domain.regElemSize(r),
                                    domain.regElemlist(r),
                                    domain.qqc(),
                                    domain.dtcourant()) ;

      /* check hydro constraint */
      CalcHydroConstraintForElems(domain, domain.regElemSize(r),
                                  domain.regElemlist(r),
                                  domain.dvovmax(),
                                  domain.dthydro()) ;
   }
}

/******************************************/

static inline
void LagrangeLeapFrog(Domain& domain)
{
#ifdef SEDOV_SYNC_POS_VEL_LATE
   Domain_member fieldData[6] ;
#endif

#if _CD && _LEAF_LV && _FGCD == 0 && _SCR == 0
   double now = MPI_Wtime();
   CDHandle *leaf_cd = GetCurrentCD()->Create("LeafCD", kStrict|kLocalMemory, 0x1);
   begn_end += MPI_Wtime() - now;
#endif

   /* calculate nodal forces, accelerations, velocities, positions, with
    * applied boundary conditions and slide surface considerations */
   LagrangeNodal(domain);
//   domain.CheckUpdate("After LagrangeNodal");


#ifdef SEDOV_SYNC_POS_VEL_LATE
#endif

   /* calculate element quantities (i.e. velocity gradient & q), and update
    * material states */
   LagrangeElements(domain, domain.numElem());

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


   // O{time} <- I{regElemSize,regElemlist,VDOV,AREALG,SS}
   CalcTimeConstraintsForElems(domain);
   domain.CheckUpdate("CalcTimeConstraintsForElems");
#if _CD && _LEAF_LV && _SCR == 0
   double then = MPI_Wtime();
  #if _FGCD
   CDHandle *leaf_cd = GetLeafCD();
  #endif
   leaf_cd->Destroy();
   cmpl_end += MPI_Wtime() - then;
#endif

#if USE_MPI   
#ifdef SEDOV_SYNC_POS_VEL_LATE
   double wait_start = MPI_Wtime();
   CommSyncPosVel(domain) ;
   double wait_finish = MPI_Wtime() - wait_start;
   wait_end += wait_finish;
#endif
#endif   
}


/******************************************/

int main(int argc, char *argv[])
{
  Domain *locDom ;
//   Int_t numRanks ;
//   Int_t myRank ;
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
//   opts.nx=40;
   opts.its = 50;

   // overwrite parms
//   opts.nx  = 60;
   if ((myRank == 0) && (opts.quiet == 0)) {
      printf("Running problem size %d^3 per domain until completion\n", opts.nx);
      printf("Num processors: %d\n", numRanks);
#if _OPENMP
      printf("Num threads: %d\n", omp_get_max_threads());
#endif
      printf("Total number of elements: %lld\n\n", (long long int)(numRanks*opts.nx*opts.nx*opts.nx));
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


   pDomain = locDom;
#if USE_MPI   
   fieldData = &Domain::nodalMass ;

   // Initial domain boundary communication 
   CommRecv(*locDom, MSG_COMM_SBN, 1,
            locDom->sizeX() + 1, locDom->sizeY() + 1, locDom->sizeZ() + 1,
            true, false) ;
   CommSend(*locDom, MSG_COMM_SBN, 1, &fieldData,
            locDom->sizeX() + 1, locDom->sizeY() + 1, locDom->sizeZ() +  1,
            true, false) ;
   CommSBN(*locDom, 1, &fieldData) ;

   // End initialization
   MPI_Barrier(MPI_COMM_WORLD);
#endif   
   
   // BEGIN timestep to solution */
#if USE_MPI   
   double start = MPI_Wtime();
#else
   timeval start;
   gettimeofday(&start, NULL) ;
#endif
   
#if _CD
#if _CD_INCR_CKPT || _CD_FULL_CKPT
  int intvl[3] = {1, 1, 1}; 
#else
  int intvl[3] = {16, 4, 1}; 
#endif
  char *lulesh_intvl = getenv( "LULESH_LV0" );
  if(lulesh_intvl != NULL) {
    intvl[0] = atoi(lulesh_intvl);
    if(myRank == 4) printf("0 %s\n", lulesh_intvl);
    assert(intvl[0] > 0);
  }
  lulesh_intvl = getenv( "LULESH_LV1" );
  if(lulesh_intvl != NULL) {
    intvl[1] = atoi(lulesh_intvl);
    if(myRank == 4) printf("1 %s\n", lulesh_intvl);
    assert(intvl[1] > 0);
  }
  lulesh_intvl = getenv( "LULESH_LV2" );
  if(lulesh_intvl != NULL) {
    intvl[2] = atoi(lulesh_intvl);
    if(myRank == 4) printf("2 %s\n", lulesh_intvl);
    assert(intvl[1] > 0);
  }
  if(myRank == 4) printf("interval setting: lv0:%d lv1:%d lv2:%d\n", intvl[0], intvl[1], intvl[2]);
  intvl0 = intvl[0];
  intvl1 = intvl[1];
  intvl2 = intvl[2];

  root_cd = CD_Init(numRanks, myRank, kPFS);
  CD_Begin(root_cd, "Root");
#if _CD_CDRT 
  root_cd->Preserve(locDom->SetOp(prvec_readonly_all), kCopy, "ReadOnlyData");
  root_cd->Preserve(locDom->SetOp(prvec_f), kCopy, "CalcForceCopy");
  root_cd->Preserve(locDom->SetOp(prvec_posall), kCopy, "PosVelAcc");
  root_cd->Preserve(locDom->SetOp(prvec_elem), kCopy, "LagrangeElem");
  root_cd->Preserve(locDom->SetOp(prvec_q), kCopy, "QforElem");
  root_cd->Preserve(locDom->SetOp(prvec_matrl), kCopy, "MaterialforElem");
  //root_cd->Preserve(locDom->SetOp(M__SERDES_ALL), kCopy, "Root_All");
#elif _CD_INCR_CKPT 
  root_cd->Preserve(locDom->SetOp(prvec_readonly_all), kCopy, "ReadOnlyData");
#endif // do not preserve anything in loop for global ckpt case


  cd_main_loop = root_cd->Create("Parent", kStrict|kPFS, 0xF);
  cd_child_loop = NULL;
  //cd_main_dummy = root_cd;
  dummy_cd = NULL;
#endif

  bool is_main_loop_complete  = false;
  bool is_child_loop_complete = false;
  bool is_leaf_complete = false;
//  printf("prvec_all:%lx, prvec_wr:%lx, prvec_ro%lx\n", prvec_all, prvec_readwrite_all, prvec_readonly_all);
//debug to see region sizes
//   for(Int_t i = 0; i < locDom->numReg(); i++)
//      std::cout << "region" << i + 1<< "size" << locDom->regElemSize(i) <<std::endl;
//   bool leaf_first = true;
//   bool main_domain_preserved=false;
   //std::cout << "CD:" <<  _CD << ", CD_DUMMY:" << _CD_DUMMY << " ,LEAF_LV:" << _LEAF_LV <<std::endl;
   double loop_time = 0.0;
   double dump_time = 0.0;
   int total_its = opts.its;
//   double *local_loop = (double *)malloc(sizeof(double) * total_its * 2);
//   double *local_dump = (double *)malloc(sizeof(double) * total_its * 2);
//   double *local_wait = (double *)malloc(sizeof(double) * total_its * 2);
   std::vector<float> local_loop;
   std::vector<float> local_dump;
   std::vector<float> local_wait;
   std::vector<float> local_begn;
   std::vector<float> local_cmpl;
#if _CD_CDRT && _CD
   std::vector<float> local_dump0;
   std::vector<float> local_dump1;
   std::vector<float> local_dump2;
   std::vector<float> local_dump3;
   std::vector<float> local_dump4;
#endif
#if _CD
#if _CD_DUMMY
   CDPrvType prv_type = (CDPrvType)(kRef);
#else
   CDPrvType prv_type = kCopy;
#endif
   dummy_cd = root_cd;
#endif
   while((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) {
      dump_end = 0.0;
      begn_end = 0.0;
      cmpl_end = 0.0;
      prv_len = 0;
      double loop_start = MPI_Wtime();

      TimeIncrement(*locDom) ;
      double ts_loop = MPI_Wtime();
      wait_end = ts_loop - loop_start;
      locDom->CheckUpdate("TimeIncrement");
#if _CD
      int cycle = locDom->cycle();
      if(locDom->check_begin(intvl0)) 
      {
        is_main_loop_complete  = false;
        double ts0 = MPI_Wtime();
        CD_Begin(cd_main_loop, "MainLoop");
        begn_end = 0.0;
        cmpl_end = 0.0;
        wait_end = ts_loop - loop_start;

        begn_end += MPI_Wtime() - ts0;
//        if(myRank == 0) printf("0 Before Prsv:cycle:%d == %d, %lx\n", cycle, locDom->cycle(), prvec_readonly_all);
        if(IsReexec() && myRank == 1) {printf(">> Before Parent %4d, %6.3le %6.3le, %le \n", locDom->cycle(), locDom->time(), locDom->deltatime(), locDom->dthydro()); locDom->PrintDomain();}
        prv_len += cd_main_loop->Preserve(dynamic_cast<Internal *>(locDom), sizeof(Internal), kCopy, "MainLoopDomain");
        //main_domain_preserved = true;
//        cd_main_loop->Preserve(&(locDom->cycle()), sizeof(locDom->cycle()), kCopy, "MainLoopDomain");
        if(IsReexec() && myRank == 1) {printf("\n\n>> After Parent %4d, %6.3le %6.3le, %le \n", locDom->cycle(), locDom->time(), locDom->deltatime(), locDom->dthydro()); locDom->PrintDomain();}
        double dump_start = MPI_Wtime();
  #if _CD_CDRT
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_readonly_all), kRef, "ReadOnlyData-Main", "ReadOnlyData");
    #if _CD_DUMMY 
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_f),     prv_type, "CalcForceCopy"   );
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_posall),prv_type, "PosVelAcc"       );
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_elem),  prv_type, "LagrangeElem"    );
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_q),     prv_type, "QforElem"        );
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_matrl), prv_type, "MaterialforElem" );
    #endif
  #elif _CD_INCR_CKPT
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_readonly_all), kRef, "ReadOnlyData-Main", "ReadOnlyData");
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_readwrite_all), kCopy, "ReadWrite-Main");
  #elif _CD_FULL_CKPT
        prv_len += cd_main_loop->Preserve(locDom->SetOp(prvec_all), kCopy, "PrvAll-Main" );
  #endif
        dump_end  += MPI_Wtime() - dump_start;

  #if _CD_CDRT && _CD_CHILD
//        if(myRank == 0) printf("After MainLoop Prsv:cycle:%d == %d\n", cycle, locDom->cycle());
        CDHandle *cd_child_parent = cd_main_loop;
    #if _CD_DUMMY
        double ts1 = MPI_Wtime();
        dummy_cd = cd_main_loop->Create("LeafDummy", kStrict|kLocalDisk, 0x3);
        CD_Begin(dummy_cd, "ChildDummy");
        begn_end += MPI_Wtime() - ts1;
        cd_child_parent = dummy_cd;
    #endif
        cd_child_loop = cd_child_parent->Create("Loop Child", kStrict|kLocalDisk, 0x3);
  #endif
//        if(myRank==0) printf("Parent Begin:cycle:%d == %d (cycle)\n", locDom->cycle(), cycle);
      }

  #if _CD_CDRT && _CD_CHILD
      if(locDom->check_begin(intvl1)) {
        is_child_loop_complete = false;
        double ts2 = MPI_Wtime();
        CD_Begin(cd_child_loop, "LoopChild");
        begn_end += MPI_Wtime() - ts2;
        if(IsReexec() && myRank == 0) {printf("Before Child %4d, %6.3le %6.3le, %le ", locDom->cycle(), locDom->time(), locDom->deltatime(), locDom->dthydro()); locDom->Print();}
        prv_len += cd_child_loop->Preserve(dynamic_cast<Internal *>(locDom), sizeof(Internal), kCopy, "ChildLoopDomain");
        if(IsReexec() && myRank == 0) {printf("After Child %4d, %6.3le %6.3le, %le ", locDom->cycle(), locDom->time(), locDom->deltatime(), locDom->dthydro()); locDom->Print();}
        double dump_start = MPI_Wtime();
        // Preserve read-only data
        prv_len += cd_child_loop->Preserve(locDom->SetOp(prvec_readonly_all), kRef, "ReadOnlyData-Leaf", "ReadOnlyData");
        // Preserve read-write data
    #if _CD_DUMMY
        prv_len += cd_child_loop->Preserve(locDom->SetOp(prvec_f),     prv_type,  "CalcForceCopy"   );
        prv_len += cd_child_loop->Preserve(locDom->SetOp(prvec_posall),prv_type,  "PosVelAcc"       );
        prv_len += cd_child_loop->Preserve(locDom->SetOp(prvec_elem),  prv_type,  "LagrangeElem"    );
        prv_len += cd_child_loop->Preserve(locDom->SetOp(prvec_q),     prv_type,  "QforElem"        );
        prv_len += cd_child_loop->Preserve(locDom->SetOp(prvec_matrl), prv_type,  "MaterialforElem" );
    #endif
        dump_end  += MPI_Wtime() - dump_start;
    #if _LEAF_LV && _SCR
        leaf_lv = cd_child_loop->Create("LeafCD", kStrict|kLocalMemory, 0x1);
    #endif
//        if(myRank==0) printf("Child Begin :cycle:%d == %d (cycle)\n", locDom->cycle(), cycle);
      }
  #endif // _CD_CHILD ends
  #if _CD_CDRT && _LEAF_LV && _SCR
      if(locDom->check_begin(intvl2)) {
        double ts2 = MPI_Wtime();
        CD_Begin(leaf_lv, "CalcPos");
        begn_end += MPI_Wtime() - ts2;
        if(IsReexec() && myRank == 0) {printf("Before Leaf %4d, %6.3le %6.3le, %le ", locDom->cycle(), locDom->time(), locDom->deltatime(), locDom->dthydro()); locDom->Print();}
        prv_len += leaf_lv->Preserve(dynamic_cast<Internal *>(locDom), sizeof(Internal), kCopy, "LeafLoopDomain");
        if(IsReexec() && myRank == 0) {printf("After Leaf %4d, %6.3le %6.3le, %le ", locDom->cycle(), locDom->time(), locDom->deltatime(), locDom->dthydro()); locDom->Print();}
        double dump_start = MPI_Wtime();
        // Preserve read-only data
        prv_len += leaf_lv->Preserve(locDom->SetOp(prvec_readonly_all), kRef, "ReadOnlyData-Leaf", "ReadOnlyData");
        dump_end  += MPI_Wtime() - dump_start;
//        if(myRank==0) printf("Child Begin :cycle:%d == %d (cycle)\n", locDom->cycle(), cycle);
      }
  #endif // _LEAF_LV && _SCR ends

#endif // _CD ends
      LagrangeLeapFrog(*locDom) ;
//      locDom->CheckUpdate("After LagrangeLeapFrog");

      global_counter++;
      double compl_time_start = MPI_Wtime(); 
#if _CD 
  #if _CD_CDRT

    #if _LEAF_LV && _SCR
      if(locDom->check_end(intvl2)) {
        is_leaf_complete = true;
        leaf_lv->Detect();
        leaf_lv->Complete();
      }
    #endif

    #if _CD_CHILD
      if(locDom->check_end(intvl1))
      {
      #if _LEAF_LV && _SCR
        if(is_leaf_complete == false) {
          is_leaf_complete = true;
          leaf_lv->Detect();
          leaf_lv->Complete();
        }
        leaf_lv->Destroy();
      #endif
        cd_child_loop->Detect();
//        if(myRank==0) printf("child end:cycle:%d == %d\n", cycle, locDom->cycle());
        cd_child_loop->Complete( /*((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) == false*/ );
        is_child_loop_complete = true;
      }
    #endif //_CD_CHILD

      if(locDom->check_end(intvl0))
      { 
//        if(myRank==0) printf("parent end:cycle:%d == %d\n", cycle, locDom->cycle());
    #if _CD_CHILD
        if(is_child_loop_complete == false) {
          cd_child_loop->Detect();
          cd_child_loop->Complete( /*((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) == false*/ );
          is_child_loop_complete = true;
        }
        cd_child_loop->Destroy();
    #if _CD_DUMMY
        dummy_cd->Complete(); // happens every intvl1
        dummy_cd->Destroy();
    #endif
    #endif //_CD_CHILD
        cd_main_loop->Detect();
        //printf("0 complete:cycle:%d == %d\n", cycle, locDom->cycle());
        //main_domain_preserved = false;
        cd_main_loop->Complete( /*((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) == false*/ );
        is_main_loop_complete = true;
      }
  #else // _CD_CDRT ends
      if(locDom->check_end(intvl0))
      { 
        cd_main_loop->Detect();
        //printf("0 complete:cycle:%d == %d\n", cycle, locDom->cycle());
        //main_domain_preserved = false;
        cd_main_loop->Complete( /*((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) == false*/ );
        is_main_loop_complete = true;
      }
  #endif
#endif
      double now = MPI_Wtime();
      cmpl_end += now - compl_time_start;
      const double loop_end = now - loop_start;
//      for(int i=0; i<5; i++) { dump_end += dump_phase[i]; }
      loop_time += loop_end;
      wait_time += wait_end; 
      dump_time += dump_end;
      begn_time += begn_end;
      cmpl_time += cmpl_end;
//    local_loop[global_counter - 1] = loop_end;
//    local_dump[global_counter - 1] = dump_end;
//    local_wait[global_counter - 1] = wait_end;
      local_loop.push_back((float)loop_end);
      local_dump.push_back((float)dump_end);
      local_wait.push_back((float)wait_end);
      local_begn.push_back((float)begn_end);
      local_cmpl.push_back((float)cmpl_end);

      for(int i=0; i<5; i++) totl_phase[i] += exec_phase[i];
#if _CD_CDRT && _CD
      local_dump0.push_back((float)dump_phase[0]);
      local_dump1.push_back((float)dump_phase[1]);
      local_dump2.push_back((float)dump_phase[2]);
      local_dump3.push_back((float)dump_phase[3]);
      local_dump4.push_back((float)dump_phase[4]);
#endif
      if (myRank == 1) {
//      if ((opts.showProg != 0) && (opts.quiet == 0) && (myRank == 0)) 
         printf("cycle = %d (%u), t=%5.4e, dt=%5.4e, loop=%5.3e, dump=%5.3e, wait=%5.3e, begin=%5.3e, complete=%5.3e, vol=%lfMB (%5.3e, %5.3e, %5.3e, %5.3e, %5.3e),phase(%4.3f,%4.3f,%4.3f,%4.3f,%4.3f),dump(%4.3f,%4.3f,%4.3f,%4.3f,%4.3f)\n",
                locDom->cycle(), global_counter, double(locDom->time()), double(locDom->deltatime()), 
                loop_end, dump_end, wait_end, begn_end, cmpl_end, (double)prv_len/1000000,
                loop_time/global_counter, dump_time/global_counter, wait_time/global_counter, begn_time/global_counter, cmpl_time/global_counter, 
                exec_phase[0], exec_phase[1],exec_phase[2], exec_phase[3],exec_phase[4],
                dump_phase[0], dump_phase[1],dump_phase[2], dump_phase[3],dump_phase[4]) ;
      }
      for(int i=0; i<5; i++) { dump_phase[i] = 0; }
#if _CD
      cd_update_profile();
#endif
    //leaf_first = false;
   } // while ends


#if _CD
   if(is_main_loop_complete == false) {
        //main_domain_preserved = false;
  #if _CD_CDRT && _CD_CHILD
      if(is_child_loop_complete == false) {
      #if _LEAF_LV && _SCR
        if(is_leaf_complete == false) {
          is_leaf_complete = true;
          leaf_lv->Detect();
          leaf_lv->Complete();
        }
        leaf_lv->Destroy();
      #endif
        cd_child_loop->Detect();
        cd_child_loop->Complete( /*((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) == false*/ );
      }
      cd_child_loop->Destroy();
    #if _CD_DUMMY
      dummy_cd->Complete(); 
      dummy_cd->Destroy();
    #endif
  #endif
      cd_main_loop->Detect();
      //main_domain_preserved = false;
      cd_main_loop->Complete( /*((locDom->time() < locDom->stoptime()) && (locDom->cycle() < opts.its)) == false*/ );
   }
   cd_main_loop->Destroy();
#endif

   // Use reduced max elapsed time
   double elapsed_time;
#if USE_MPI   
   elapsed_time = MPI_Wtime() - start;
#else
   timeval end;
   gettimeofday(&end, NULL) ;
   elapsed_time = (double)(end.tv_sec - start.tv_sec) + ((double)(end.tv_usec - start.tv_usec))/1000000 ;
#endif

   double elapsed_timeG;
#if USE_MPI   
   MPI_Reduce(&elapsed_time, &elapsed_timeG, 1, MPI_DOUBLE,
              MPI_MAX, 0, MPI_COMM_WORLD);
#else
   elapsed_timeG = elapsed_time;
#endif

#if _CD    
   char *execname = exec_name; 
   char *fname_last = start_date;
#else
   char execname[256] = "lulesh_bare";
   char fname_last[256];
   sprintf(fname_last, "%d", ((int)start) % 1000);
#endif

   double lt_loc = loop_time / global_counter;
   double dt_loc = dump_time / global_counter;
   double wt_loc = wait_time / global_counter;
   double cd_loc = (begn_time + cmpl_time) / global_counter;
   double aggregated_phase[5] = {0, 0, 0, 0, 0};
   for(int i=0; i<5; i++) { totl_phase[i] /= global_counter; }
#if _CD    
   const unsigned prof_elems = 13;
   double sendbuf[prof_elems] = { lt_loc, dt_loc, wt_loc, cd_loc, 
                         packer::time_copy.GetBW(), 
                         packer::time_write.GetBW(), 
                         packer::time_read.GetBW(), 
                         packer::time_posix_write.GetBW(), 
                         packer::time_posix_read.GetBW(), 
                         packer::time_posix_seek.GetBW(), 
                         packer::time_mpiio_write.GetBW(), 
                         packer::time_mpiio_read.GetBW(), 
                         packer::time_mpiio_seek.GetBW()
   };
#else
   const unsigned prof_elems = 4;
   double sendbuf[prof_elems] = { lt_loc, dt_loc, wt_loc, cd_loc };
#endif
   double sendbufstd[prof_elems];//{ lt_loc * lt_loc, dt_loc * dt_loc, wt_loc * wt_loc, cd_loc * cd_loc };
   for(int i=0; i<prof_elems; i++) { sendbufstd[i] = sendbuf[i] * sendbuf[i]; }
   double recvbufmax[prof_elems] = { 0, 0, 0, 0 };
   double recvbufmin[prof_elems] = { 0, 0, 0, 0 };
   double recvbufavg[prof_elems] = { 0, 0, 0, 0 };
   double recvbufstd[prof_elems] = { 0, 0, 0, 0 };
   MPI_Reduce(totl_phase, aggregated_phase, 5, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   MPI_Reduce(sendbuf, recvbufmax, prof_elems, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
   MPI_Reduce(sendbuf, recvbufmin, prof_elems, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
   MPI_Reduce(sendbuf, recvbufavg, prof_elems, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   MPI_Reduce(sendbufstd, recvbufstd, prof_elems, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   for(int i=0; i<prof_elems; i++) { 
     recvbufavg[i] /= numRanks; // avg
     recvbufstd[i] /= numRanks; // avg of 2nd momentum
     recvbufstd[i] =  sqrt(recvbufstd[i] - recvbufavg[i] * recvbufavg[i]);
   }
   if(myRank == 0) {
     for(int i=0; i<5; i++) { aggregated_phase[i] /= numRanks; }
     fprintf(stdout, "loop %5.4lf, dump %5.4lf, wait %5.4lf, cdrt %5.4lf\n", lt_loc, dt_loc, wt_loc, cd_loc);
     fprintf(stdout, "avg, %5.4lf, %5.4lf, %5.4lf, %5.4lf\n"
                     "std, %5.4lf, %5.4lf, %5.4lf, %5.4lf\n"
                     "min, %5.4lf, %5.4lf, %5.4lf, %5.4lf\n"
                     "max, %5.4lf, %5.4lf, %5.4lf, %5.4lf\n", 
         recvbufavg[0], recvbufavg[1], recvbufavg[2], recvbufavg[3], 
         recvbufstd[0], recvbufstd[1], recvbufstd[2], recvbufstd[3], 
         recvbufmin[0], recvbufmin[1], recvbufmin[2], recvbufmin[3], 
         recvbufmax[0], recvbufmax[1], recvbufmax[2], recvbufmax[3]);
     for(int i=4; i<prof_elems; i++) {
       fprintf(stdout, "%5.4lf (%5.4lf) %5.4lf~%5.4lf\n", recvbufavg[i], recvbufstd[i], recvbufmin[i], recvbufmax[i]);
     }
     fprintf(stdout, "phase: %5.4lf %5.4lf %5.4lf %5.4lf %5.4lf\n",
         aggregated_phase[0],
         aggregated_phase[1],
         aggregated_phase[2],
         aggregated_phase[3],
         aggregated_phase[4]
         );
   }
   if(0) 
   {
     double *total_loop = NULL;
     double *total_dump = NULL;
     double *total_wait = NULL;
     int total_stat_cnt = total_its * numRanks;
     if(myRank == 0) {
       total_loop = (double *)malloc(total_stat_cnt * sizeof(double));
       total_dump = (double *)malloc(total_stat_cnt * sizeof(double));
       total_wait = (double *)malloc(total_stat_cnt * sizeof(double));
     }
     MPI_Gather(local_loop.data(), total_its, MPI_FLOAT, total_loop, total_its, MPI_FLOAT, 0, MPI_COMM_WORLD);
     MPI_Gather(local_dump.data(), total_its, MPI_FLOAT, total_dump, total_its, MPI_FLOAT, 0, MPI_COMM_WORLD);
     MPI_Gather(local_wait.data(), total_its, MPI_FLOAT, total_wait, total_its, MPI_FLOAT, 0, MPI_COMM_WORLD);
     if(myRank == 0) {
       char tmpfile[128];
       
       // local stats /////////////////////////
       sprintf(tmpfile, "local_stats.%s.%d.%d.%s", execname, numRanks, opts.nx, fname_last);
       FILE *lfp = fopen(tmpfile, "w"); 
       fprintf(lfp, "%s\n", execname);
       if(lfp == 0) { printf("failed to open %s\n", tmpfile); assert(lfp); }
       double loop_min = DBL_MAX;
       double loop_max = 0;
       double dump_min = DBL_MAX;
       double dump_max = 0;
       double wait_min = DBL_MAX;
       double wait_max = 0;
       for(int i=0; i<total_stat_cnt; i++) {
         fprintf(lfp, "%lf,", total_loop[i]);
         if(total_loop[i] < loop_min) loop_min = total_loop[i];
         if(total_loop[i] > loop_max) loop_max = total_loop[i];
       }
       fprintf(lfp, "\n");
       for(int i=0; i<total_stat_cnt; i++) {
         fprintf(lfp, "%lf,", total_dump[i]);
         if(total_dump[i] < dump_min) dump_min = total_dump[i];
         if(total_dump[i] > dump_max) dump_max = total_dump[i];
       }
       fprintf(lfp, "\n");
       for(int i=0; i<total_stat_cnt; i++) {
         fprintf(lfp, "%lf,", total_wait[i]);
         if(total_wait[i] < wait_min) wait_min = total_wait[i];
         if(total_wait[i] > wait_max) wait_max = total_wait[i];
       }
       fprintf(lfp, "\n");
       fclose(lfp);
       ////////////////////////////////////////
       // global stats /////////////////////////
       sprintf(tmpfile, "global_stats.%s.%d.%d.%s", execname, numRanks, opts.nx, fname_last);
       FILE *gfp = fopen(tmpfile, "w"); 
       fprintf(gfp, "loop,         dump,         wait\n");
       fprintf(gfp, "avg, %le, %le, %le\nstd, %le, %le, %le\nmin, %le, %le, %le\nmax, %le, %le, %le\nmin, %le, %le, %le\nmax, %le, %le, %le\n", 
           recvbufavg[0], recvbufavg[1], recvbufavg[2], 
           recvbufstd[0], recvbufstd[1], recvbufstd[2], 
           loop_min     ,      dump_min,      wait_min, 
           loop_max     ,      dump_max,      wait_max,
           recvbufmin[0], recvbufmin[1], recvbufmin[2], 
           recvbufmax[0], recvbufmax[1], recvbufmax[2]);
       fclose(gfp);
       ////////////////////////////////////////
       // histogram ///////////////////////////
       sprintf(tmpfile, "histogram.%s.%d.%d.%s", execname, numRanks, opts.nx, fname_last);
       FILE *hfp = fopen(tmpfile, "w"); 
       fprintf(hfp, "%s\n", execname);
       if(hfp == 0) { printf("failed to open %s\n", tmpfile); assert(hfp); }
       // based on optimal bin size calculation,
       // https://www.fmrib.ox.ac.uk/datasets/techrep/tr00mj2/tr00mj2/node24.html
       double samples = pow(total_stat_cnt, -0.333333);
       double binsize_loop = 3.49 * recvbufstd[0] * samples;
       double binsize_dump = 3.49 * recvbufstd[1] * samples;
       double binsize_wait = 3.49 * recvbufstd[2] * samples;
       double window_loop = loop_max - loop_min;
       double window_dump = dump_max - dump_min;
       double window_wait = wait_max - wait_min;
       int    bincnt_loop = window_loop / binsize_loop;
       int    bincnt_dump = window_dump / binsize_dump;
       int    bincnt_wait = window_wait / binsize_wait;
       printf("loop: %lf~%lf=%lf %lf (%d)\n", loop_min, loop_max, window_loop, binsize_loop, bincnt_loop);
       printf("dump: %lf~%lf=%lf %lf (%d)\n", dump_min, dump_max, window_dump, binsize_dump, bincnt_dump);
       printf("wait: %lf~%lf=%lf %lf (%d)\n", wait_min, wait_max, window_wait, binsize_wait, bincnt_wait);
       unsigned *hist_loop = (unsigned *)calloc(bincnt_loop, sizeof(unsigned));
       unsigned *hist_dump = (unsigned *)calloc(bincnt_dump, sizeof(unsigned));
       unsigned *hist_wait = (unsigned *)calloc(bincnt_wait, sizeof(unsigned));
       for(int i=0; i<total_stat_cnt; i++) { 
         int idx = (total_loop[i] - loop_min)/binsize_loop; 
         idx = (idx >= bincnt_loop)? bincnt_loop : idx;
         idx = (idx < 0)? 0 : idx;
         hist_loop[idx]++; }
       for(int i=0; i<total_stat_cnt; i++) { 
         int idx = (total_dump[i] - dump_min)/binsize_dump; 
         idx = (idx >= bincnt_dump)? bincnt_dump : idx;
         idx = (idx < 0)? 0 : idx;
         hist_dump[idx]++; }
       for(int i=0; i<total_stat_cnt; i++) { 
         int idx = (total_wait[i] - wait_min)/binsize_wait; 
         idx = (idx >= bincnt_wait)? bincnt_wait : idx;
         idx = (idx < 0)? 0 : idx;
         hist_wait[idx]++; }
       // X-axis for loop
       for(int i=0; i<bincnt_loop; i++, loop_min += binsize_loop) { fprintf(hfp, "%lf,", loop_min); }
       fprintf(hfp, "\n");
       for(int i=0; i<bincnt_loop; i++) { fprintf(hfp, "%u,", hist_loop[i]); }
       fprintf(hfp, "\n");
       // X-axis for dump
       for(int i=0; i<bincnt_dump; i++, dump_min += binsize_dump) { fprintf(hfp, "%lf,", dump_min); }
       fprintf(hfp, "\n");
       for(int i=0; i<bincnt_dump; i++) { fprintf(hfp, "%u,", hist_dump[i]); }
       fprintf(hfp, "\n");
       // X-axis for wait
       for(int i=0; i<bincnt_wait; i++, wait_min += binsize_wait) { fprintf(hfp, "%lf,", wait_min); }
       fprintf(hfp, "\n");
       for(int i=0; i<bincnt_wait; i++) { fprintf(hfp, "%u,", hist_wait[i]); }
       fprintf(hfp, "\n");
       fclose(hfp);
       free(hist_loop);
       free(hist_dump);
       free(hist_wait);
       ////////////////////////////////////////
       free(total_loop);
       free(total_dump);
       free(total_wait);
     } // rank 0 ends
   } else {

#if 0
     MPI_File fh;
     char trace_fname[256];
     if(myRank == 0) {
       sprintf(trace_fname, "result_trace.%s.%d.%d.%d", execname, numRanks, opts.nx, ((int)start) % 1000);
       PMPI_Bcast(&trace_fname, 256, MPI_CHAR, 0, MPI_COMM_WORLD);
     } else {
       PMPI_Bcast(&trace_fname, 256, MPI_CHAR, 0, MPI_COMM_WORLD);
     }
     MPI_File_open(MPI_COMM_WORLD, trace_fname, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
 
     // Set up datatype for file view
     MPI_Datatype contig = MPI_FLOAT;
     uint64_t elemsize = local_loop.size();
     uint64_t viewsize = elemsize * sizeof(float); // should be len in byte
     MPI_Datatype ftype;
     MPI_Type_contiguous(elemsize, MPI_FLOAT, &contig);
     MPI_Type_create_resized(contig, 0, (numRanks)*viewsize, &ftype);
     MPI_Type_commit(&contig);
     MPI_Type_commit(&ftype);
   
     MPI_Info info = MPI_INFO_NULL;
     uint64_t view_offset = myRank * viewsize; // in byte
 
     MPI_File_set_view(fh, view_offset, MPI_FLOAT, ftype, "native", info);
     printf("[%d/%d] Opened file%d : %s, viewsize:%lu, view_offset:%lu, chunk:%lu\n", 
            myRank, numRanks, myRank, trace_fname, viewsize, view_offset, (numRanks)*viewsize);
     int offset = 0;
     MPI_File_write_at_all(fh, offset, local_loop.data(), local_loop.size(), MPI_FLOAT, MPI_STATUS_IGNORE); offset += elemsize;
     MPI_File_write_at_all(fh, offset, local_dump.data(), local_dump.size(), MPI_FLOAT, MPI_STATUS_IGNORE); offset += elemsize;
     MPI_File_write_at_all(fh, offset, local_wait.data(), local_wait.size(), MPI_FLOAT, MPI_STATUS_IGNORE); offset += elemsize;
     MPI_File_write_at_all(fh, offset, local_begn.data(), local_begn.size(), MPI_FLOAT, MPI_STATUS_IGNORE); offset += elemsize;
     MPI_File_write_at_all(fh, offset, local_cmpl.data(), local_cmpl.size(), MPI_FLOAT, MPI_STATUS_IGNORE); offset += elemsize;
#else

   float *total_loop = NULL;
   float *total_dump = NULL;
   float *total_wait = NULL;
   float *total_begn = NULL;
   float *total_cmpl = NULL;

#if _CD_CDRT && _CD
   float *total_dump0 = NULL;
   float *total_dump1 = NULL;
   float *total_dump2 = NULL;
   float *total_dump3 = NULL;
   float *total_dump4 = NULL;
#endif

   unsigned int min_global_counter = 0;
   assert(global_counter >= local_loop.size());
   assert(global_counter >= local_dump.size());
   assert(global_counter >= local_wait.size());
   assert(global_counter >= local_begn.size());
   assert(global_counter >= local_cmpl.size());
   MPI_Reduce(&global_counter, &min_global_counter, 1, MPI_UNSIGNED, MPI_MIN, 0, MPI_COMM_WORLD);
   global_counter = min_global_counter;
   int tot_elems = global_counter * numRanks;
   if(myRank == 0) {
     total_loop = (float *)malloc(tot_elems * sizeof(float));
     total_dump = (float *)malloc(tot_elems * sizeof(float));
     total_wait = (float *)malloc(tot_elems * sizeof(float));
     total_begn = (float *)malloc(tot_elems * sizeof(float));
     total_cmpl = (float *)malloc(tot_elems * sizeof(float));

#if _CD_CDRT && _CD
     total_dump0 = (float *)malloc(tot_elems * sizeof(float));
     total_dump1 = (float *)malloc(tot_elems * sizeof(float));
     total_dump2 = (float *)malloc(tot_elems * sizeof(float));
     total_dump3 = (float *)malloc(tot_elems * sizeof(float));
     total_dump4 = (float *)malloc(tot_elems * sizeof(float));
#endif
   }
   MPI_Gather(local_loop.data(), global_counter, MPI_FLOAT, total_loop, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_dump.data(), global_counter, MPI_FLOAT, total_dump, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_wait.data(), global_counter, MPI_FLOAT, total_wait, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_begn.data(), global_counter, MPI_FLOAT, total_begn, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_cmpl.data(), global_counter, MPI_FLOAT, total_cmpl, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);

#if _CD_CDRT && _CD
   MPI_Gather(local_dump0.data(), global_counter, MPI_FLOAT, total_dump0, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_dump1.data(), global_counter, MPI_FLOAT, total_dump1, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_dump2.data(), global_counter, MPI_FLOAT, total_dump2, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_dump3.data(), global_counter, MPI_FLOAT, total_dump3, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Gather(local_dump4.data(), global_counter, MPI_FLOAT, total_dump4, global_counter, MPI_FLOAT, 0, MPI_COMM_WORLD);
#endif

   if(myRank == 0) {
     char tmpfile[256];
//     sprintf(tmpfile, "time_trace.%s.%d.%d.%d", execname, numRanks, opts.nx, ((int)start) % 10000);
     sprintf(tmpfile, "time_trace.%s.%d.%d.%s.json", execname, opts.nx, numRanks, fname_last);
     FILE *tfp = fopen(tmpfile, "w"); 
     if(tfp == 0) { printf("failed to open %s\n", tmpfile); assert(tfp); }

     fprintf(tfp, "{\n");
     fprintf(tfp, "  \"name\":\"%s\",\n", execname);
     fprintf(tfp, "  \"input\":%d,\n", opts.nx);
     fprintf(tfp, "  \"nTask\":%d,\n", numRanks);
     fprintf(tfp, "  \"prof\": {\n");
     fprintf(tfp, "    \"loop\": [%f", total_loop[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_loop[i]); } fprintf(tfp, "],\n");
     fprintf(tfp, "    \"dump\": [%f", total_dump[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_dump[i]); } fprintf(tfp, "],\n");
#if _CD_CDRT && _CD
     fprintf(tfp, "    \"dump0\": [%f", total_dump0[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_dump0[i]); } fprintf(tfp, "],\n");
     fprintf(tfp, "    \"dump1\": [%f", total_dump1[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_dump1[i]); } fprintf(tfp, "],\n");
     fprintf(tfp, "    \"dump2\": [%f", total_dump2[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_dump2[i]); } fprintf(tfp, "],\n");
     fprintf(tfp, "    \"dump3\": [%f", total_dump3[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_dump3[i]); } fprintf(tfp, "],\n");
     fprintf(tfp, "    \"dump4\": [%f", total_dump4[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_dump4[i]); } fprintf(tfp, "],\n");
#endif
     fprintf(tfp, "    \"wait\": [%f", total_wait[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_wait[i]); } fprintf(tfp, "],\n");
     fprintf(tfp, "    \"cdrt\": [%f", total_begn[0] + total_cmpl[0]); for(int i=1; i<tot_elems; i++) { fprintf(tfp, ",%f", total_begn[i] + total_cmpl[i]); } fprintf(tfp, "]\n");
     fprintf(tfp, "  }\n");
     fprintf(tfp, "}\n");
     free(total_loop);
     free(total_dump);
     free(total_wait);
     free(total_begn);
     free(total_cmpl);

#if _CD_CDRT && _CD
     free(total_dump0);
     free(total_dump1);
     free(total_dump2);
     free(total_dump3);
     free(total_dump4);
#endif
     fclose(tfp);
   }

#endif
   }
//   free(local_loop);
//   free(local_dump);
//   free(local_wait);
   // Write out final viz file */
   if (opts.viz) {
      DumpToVisit(*locDom, opts.numFiles, myRank, numRanks) ;
   }
   
   if ((myRank == 0) && (opts.quiet == 0)) {
      VerifyAndWriteFinalOutput(elapsed_timeG, *locDom, opts.nx, numRanks);
   }

   if(myRank == 0) {
     printf("Loop time:%lf, Dump time:%lf\n", loop_time/global_counter, dump_time/global_counter);
   }
#if _CD
  root_cd->Complete();
  CD_Finalize();
#endif


#if USE_MPI
   MPI_Finalize() ;
#endif

   return 0 ;
}



char id2str[TOTAL_IDS][MAX_ID_STR_LEN] = {
      "ID__INVALID" 
    , "ID__INTERNAL"
    , "ID__X"
    , "ID__Y"
    , "ID__Z"
    , "ID__XD"
    , "ID__YD"
    , "ID__ZD"
    , "ID__XDD"
    , "ID__YDD"
    , "ID__ZDD"
    , "ID__FX"
    , "ID__FY"
    , "ID__FZ"
    , "ID__NODALMASS"
    , "ID__SYMMX"
    , "ID__SYMMY"
    , "ID__SYMMZ"
    , "ID__NODELIST"
    , "ID__LXIM"
    , "ID__LXIP"
    , "ID__LETAM"
    , "ID__LETAP"
    , "ID__LZETAM"
    , "ID__LZETAP"
    , "ID__ELEMBC"
    , "ID__DXX"
    , "ID__DYY"
    , "ID__DZZ"
    , "ID__DELV_XI"
    , "ID__DELV_ETA"
    , "ID__DELV_ZETA"
    , "ID__DELX_XI"
    , "ID__DELX_ETA"
    , "ID__DELX_ZETA"
    , "ID__E"
    , "ID__P"
    , "ID__Q"
    , "ID__QL"
    , "ID__QQ"
    , "ID__V"
    , "ID__VOLO"
    , "ID__VNEW"
    , "ID__DELV"
    , "ID__VDOV"
    , "ID__AREALG"
    , "ID__SS"
    , "ID__ELEMMASS"
    , "ID__REGELEMSIZE"
    , "ID__REGNUMLIST"
    , "ID__REGELEMLIST"
    , "ID__COMMBUFSEND"
    , "ID__COMMBUFRECV"
};
#if _CD 
bool Domain::restarted = false;
#endif
