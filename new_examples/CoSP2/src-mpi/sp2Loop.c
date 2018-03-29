/// \file
/// SP2 loop.

#include "sp2Loop.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sparseMatrix.h"
#include "sparseMath.h"
#include "haloExchange.h"
#include "performance.h"
#include "parallel.h"
#include "matrixio.h"
#include "constants.h"
#if _CD1
#include "cd.h"
#include "cd_cosp2.h"
#endif
/// \details
/// The secpnd order spectral projection algorithm.
void sp2Loop(struct SparseMatrixSt* xmatrix, struct DomainSt* domain)
{
#if _CD1
  cd_handle_t *lv1_cd = cd_create(getcurrentcd(), 1, "sp2Loop",
                                  kStrict | kHDD, 0xE);
  cd_begin(lv1_cd, "whileloop_sp2");

#if DO_PRV
  // preserve xmatrix and domain
  unsigned int prv_size_lv1 = 0;
  ////SZNOTE: sparse matrix is all zeros now, not need to preserve here..
  //prv_size_lv1 += preserveSparseMatrix(lv1_cd, xmatrix, "cd_lv1");

  prv_size_lv1 += preserveDomain(lv1_cd, domain, "cd_lv1");
#endif //DO_PRV
#endif

  HaloExchange* haloExchange;

  startTimer(sp2LoopTimer);

#ifdef DO_MPI
  // buffer allocation for communication
  if (getNRanks() > 1)
    haloExchange = initHaloExchange(domain);
#endif

  int hsize = xmatrix->hsize;
  // allocate for sparse matrix, and set all elements to 0
  SparseMatrix* x2matrix = initSparseMatrix(xmatrix->hsize, xmatrix->msize);

  // Do gershgorin normalization
  startTimer(normTimer);
  normalize(xmatrix);
  stopTimer(normTimer);
  
  // Sparse SP2 algorithm
 
  real_t idempErr = ZERO;
  real_t idempErr1 = ZERO;
  real_t idempErr2 = ZERO;

  real_t occ = hsize*HALF;

  real_t trX = ZERO;
  real_t trX2 = ZERO;

  real_t tr2XX2, trXOLD, limDiff;

  int iter = 0;
  int breakLoop = 0;

  if (printRank() && debug == 1)
    printf("\nSP2Loop:\n");

#if _CD2
  cd_handle_t *lv2_cd = cd_create(getcurrentcd(), 1, "sp2Loop_while",
                                  kStrict | kDRAM, 0xC);
  char iter_with_idx[8] = "-1";
#endif
  while ( breakLoop == 0 && iter < 100 )
  {
#if _CD2
    cd_begin(lv2_cd, "sp2Loop_while");
#if DO_PRV
    unsigned int prv_size_lv2 = 0;
    //// PRV via Copy
    // iter: iteration count
    cd_preserve(lv2_cd, &iter,  sizeof(int), kCopy, "sp2Loop_while_iter",  "sp2Loop_while_iter");
    prv_size_lv2 += sizeof(int);
    // preserve idempErr, idempErr1; idempErr2 not needed since RAW
    cd_preserve(lv2_cd, &idempErr,  sizeof(real_t), kCopy, "sp2Loop_while_idempErr",  "sp2Loop_while_idempErr");
    prv_size_lv2 += sizeof(real_t);
    cd_preserve(lv2_cd, &idempErr1, sizeof(real_t), kCopy, "sp2Loop_while_idempErr1", "sp2Loop_while_idempErr1");
    prv_size_lv2 += sizeof(real_t);

    // xmatrix
    printf("before preservations: xmatrix=%p, x2matrix=%p\n", xmatrix, x2matrix);
    prv_size_lv2 += preserveSparseMatrix(lv2_cd, xmatrix,  "sp2Loop_while_x");
    printf("between preservations: xmatrix=%p, x2matrix=%p\n", xmatrix, x2matrix);
    prv_size_lv2 += preserveSparseMatrix(lv2_cd, x2matrix, "sp2Loop_while_x2");
    printf("after preservations: xmatrix=%p, x2matrix=%p\n", xmatrix, x2matrix);
    // HaloExchange
    prv_size_lv2 += preserveHaloExchange(lv2_cd, haloExchange, "sp2Loop_while");

    //// PRV via Ref
    // preserve domain via kRef since it is read-only information
    cd_preserve(lv2_cd, domain, sizeof(Domain), kRef, "sp2Loop_while_domain", "sp2Loop_while_domain");
#endif //DO_PRV
#endif 

    trX = ZERO;
    trX2 = ZERO;

#ifdef DO_MPI
    if (getNRanks() > 1)
    {
      startTimer(exchangeTimer);
      // SZNOTE: post MPI_Irecv
      exchangeSetup(haloExchange, xmatrix, domain);
      stopTimer(exchangeTimer);
    }
#endif

    // Matrix multiply X^2
    startTimer(x2Timer);
    //--------------------------------------------------------------------------
    // This accounts for most computations (49.5% of Loop)
    // NOTE1: This doesn't involve communication and thus can have children 
    //        as many as the number of ranks
    // NOTE2: The inner most loop in sparseX2 is parallelized with OpenMP
    //        Therefore, the total number of inner loop is determined by given
    //        number of thread (set by $OMP_NUM_THREADS)
    sparseX2(&trX, &trX2, xmatrix, x2matrix, domain);
    //--------------------------------------------------------------------------
    stopTimer(x2Timer);

#ifdef DO_MPI
    // Reduce trace of X and X^2 across all processors
    if (getNRanks() > 1)
    {
      startTimer(reduceCommTimer);
      addRealReduce2(&trX, &trX2);
      stopTimer(reduceCommTimer);
      collectCounter(reduceCounter, 2 * sizeof(real_t));
    }
#endif

    if (printRank() && debug == 1) 
      printf("iter = %d  trX = %e  trX2 = %e\n", iter, trX, trX2);
 
    tr2XX2 = TWO*trX - trX2;
    trXOLD = trX;
    limDiff = ABS(trX2 - occ) - ABS(tr2XX2 - occ);

    if (limDiff > idemTol) 
    {
      // X = 2 * X - X^2
      trX = TWO * trX - trX2;

      startTimer(xaddTimer);
      sparseAdd(xmatrix, x2matrix, domain);
      stopTimer(xaddTimer);
    }
    else if (limDiff < -idemTol)
    {
      // X = X^2
      trX = trX2;

      startTimer(xsetTimer);
      sparseSetX2(xmatrix, x2matrix, domain);
      stopTimer(xsetTimer);
    }
    else 
    {
      trX = trXOLD;
      breakLoop = 1;
    }
         
    idempErr2 = idempErr1;
    idempErr1 = idempErr;
    idempErr = ABS(trX - trXOLD);    

    iter++;

    if (iter >= 25 && (idempErr >= idempErr2)) breakLoop = 1;

    // Exchange sparse matrix pieces across processors
#ifdef DO_MPI
    if (getNRanks() > 1)
    {
      startTimer(exchangeTimer);
      //------------------------------------------------------------------------
      // This accounts for 2nd most time (19.5% of Loop) for communication
      exchangeData(haloExchange, xmatrix, domain);
      //------------------------------------------------------------------------
      stopTimer(exchangeTimer);
    }
#endif
#if _CD2
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif
  } // end of while
#if _CD2
  cd_destroy(lv2_cd);
#endif
  stopTimer(sp2LoopTimer);

  // Multiply by 2
  sparseMultScalar(xmatrix, domain, TWO);
  
  // Report results
  reportResults(iter, xmatrix, x2matrix, domain);

#ifdef DO_MPI
  // Gather matrix to processor 0
  if (getNRanks() > 1)
    allGatherData(haloExchange, xmatrix, domain);
#endif

#ifdef DO_MPI
  if (getNRanks() > 1)
    destroyHaloExchange(haloExchange);
#endif
  destroySparseMatrix(x2matrix);

#if _CD1
  cd_detect(lv1_cd);
  cd_complete(lv1_cd);
  cd_destroy(lv1_cd);
#endif
}

/// \details
/// Report density matrix results
void reportResults(int iter, struct SparseMatrixSt* xmatrix, struct SparseMatrixSt* x2matrix, struct DomainSt* domain)
{
  int hsize = xmatrix->hsize;

  int sumIIA= 0;
  int sumIIC= 0;
  int maxIIA= 0;
  int maxIIC= 0;

  #pragma omp parallel for reduction(+:sumIIA,sumIIC) reduction(max:maxIIA,maxIIC)  
  for (int i = domain->localRowMin; i < domain->localRowMax; i++)
  {
    sumIIA += xmatrix->iia[i];
    sumIIC += x2matrix->iia[i];
    maxIIA = MAX(maxIIA, xmatrix->iia[i]);
    maxIIC = MAX(maxIIC, x2matrix->iia[i]);
  }

#ifdef DO_MPI
  // Collect number of non-zeroes and max non-zeroes per row across ranks
  if (getNRanks() > 1)
  {
    startTimer(reduceCommTimer);
    addIntReduce2(&sumIIA, &sumIIC);
    stopTimer(reduceCommTimer);
    collectCounter(reduceCounter, 2 * sizeof(int));

    startTimer(reduceCommTimer);
    maxIntReduce2(&maxIIA, &maxIIC);
    stopTimer(reduceCommTimer);
    collectCounter(reduceCounter, 2 * sizeof(int));
  }
#endif

  if (printRank())
  {
    printf("\nResults:\n");
    printf("X2 Sparsity CCN = %d, fraction = %e avg = %g, max = %d\n", sumIIC, 
      (real_t)sumIIC/(real_t)(hsize*hsize), (real_t)sumIIC/(real_t)hsize, maxIIC);

    printf("D Sparsity AAN = %d, fraction = %e avg = %g, max = %d\n", sumIIA, 
      (real_t)sumIIA/(real_t)(hsize*hsize), (real_t)sumIIA/(real_t)hsize, maxIIA);

    printf("Number of iterations = %d\n", iter);
  }
}
