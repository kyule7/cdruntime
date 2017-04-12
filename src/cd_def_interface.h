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

#define CHECK_TYPE(X,Y) \
  (((X) & (Y)) == (Y))

enum CDOpState { kBegin = 0,
                 kComplete = 1,
                 kCreate = 2,
                 kDestroy = 3,
                 kPreserve = 4,
                 kNoop
               };
/** \addtogroup cd_defs 
 *@{
 */
/** 
 * @brief Type for specifying whether the current CD is executing
 * for the first time or is currently reexecuting as part of recovery.
 *
 * During reexecution, data is restored instead of being
 * preserved. Additionally, for relaxed CDs, some communication and
 * synchronization may not repeated and instead preserved (logged)
 * values are used. 
 * See <http://lph.ece.utexas.edu/public/CDs> for a detailed
 * description. Note that this is not part of the cd_internal
 * namespace because the application programmer may want to manipulate
 * this when specifying specialized recovery routines.
 */
  enum CDExecMode  {kExecution=0, //!< Execution mode 
                    kReexecution, //!< Reexecution mode
                    kSuspension   //!< Suspension mode (not active)
                     };

/** @} */ // End group cd_defs
namespace cd {
extern CD_CLOCK_T prof_begin_clk;
extern CD_CLOCK_T prof_end_clk;
extern CD_CLOCK_T prof_sync_clk;

extern CD_CLOCK_T begin_clk;
extern CD_CLOCK_T end_clk;
extern CD_CLOCK_T elapsed_time;
extern CD_CLOCK_T normal_sync_time;
extern CD_CLOCK_T reexec_sync_time;
extern CD_CLOCK_T recovery_sync_time;
extern CD_CLOCK_T prv_elapsed_time;
extern CD_CLOCK_T rst_elapsed_time;
extern CD_CLOCK_T create_elapsed_time;
extern CD_CLOCK_T destroy_elapsed_time;
extern CD_CLOCK_T begin_elapsed_time;
extern CD_CLOCK_T compl_elapsed_time;
extern CD_CLOCK_T advance_elapsed_time;

extern CD_CLOCK_T mailbox_elapsed_time;
}
// FIXME
//
// CD_CLOCK() --> CD_CLOCK(clk)
// CD_CLOCK(X) X = MPI_Wtime();
// CD_CLOCK(X) gettimeofday(&(X), NULL);
//double operator-(const timeval &lhs, const timeval &rhs)
//{
//  printf("first:%ld %ld\n", lhs.tv_sec, lhs.tv_usec);
//  printf("second:%ld %ld\n", rhs.tv_sec, rhs.tv_usec);
//  return ((double)(lhs.tv_sec - rhs.tv_sec)
//          + (lhs.tv_usec - rhs.tv_usec)*0.000001);
//}
//
//#define CD_CLOCK(X) gettimeofday(&(X), NULL)

#endif
