#ifndef _CD_DEF_INTERFACE_H
#define _CD_DEF_INTERFACE_H


#if CD_MPI_ENABLED
  #define CD_CLOCK_T double
  #define CLK_NORMALIZER (1.0)
  #define CD_CLOCK MPI_Wtime
  #define CD_CLK_MEA(X) (X)
  #define CD_CLK_mMEA(X) ((X)*1000)
  #define CD_CLK_uMEA(X) ((X)*1000000)
#else
  #include <ctime>
  #define CD_CLOCK_T clock_t
  #define CLK_NORMALIZER CLOCKS_PER_SEC
  #define CD_CLOCK clock
  #define CD_CLK_MEA(X) ((double)((X))/CLOCKS_PER_SEC)
  #define CD_CLK_mMEA(X) ((((double)(X))/CLOCKS_PER_SEC)*1000)
  #define CD_CLK_uMEA(X) ((((double)(X))/CLOCKS_PER_SEC)*1000000)
#endif


#endif
