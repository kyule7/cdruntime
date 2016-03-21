#ifndef _CD_DEF_INTERFACE_H
#define _CD_DEF_INTERFACE_H


#if CD_MPI_ENABLED
#define CD_CLOCK_T double
#define CLK_NORMALIZER (1.0)
#define CD_CLOCK MPI_Wtime
#else
#include <ctime>
#define CD_CLOCK_T clock_t
#define CLK_NORMALIZER CLOCKS_PER_SEC
#define CD_CLOCK clock
#endif



#endif
