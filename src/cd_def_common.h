#ifndef _CD_DEF_COMMON_H 
#define _CD_DEF_COMMON_H



#define CD_BIT_0     0x0000000000000001  
#define CD_BIT_1     0x0000000000000002                       
#define CD_BIT_2     0x0000000000000004                       
#define CD_BIT_3     0x0000000000000008                       
#define CD_BIT_4     0x0000000000000010                       
#define CD_BIT_5     0x0000000000000020                       
#define CD_BIT_6     0x0000000000000040                       
#define CD_BIT_7     0x0000000000000080                       
#define CD_BIT_8     0x0000000000000100                       
#define CD_BIT_9     0x0000000000000200                       
#define CD_BIT_10    0x0000000000000400                       
#define CD_BIT_11    0x0000000000000800                       
#define CD_BIT_12    0x0000000000001000                       
#define CD_BIT_13    0x0000000000002000                       
#define CD_BIT_14    0x0000000000004000                       
#define CD_BIT_15    0x0000000000008000                       
#define CD_BIT_16    0x0000000000010000                       
#define CD_BIT_17    0x0000000000020000                       
#define CD_BIT_18    0x0000000000040000                       
#define CD_BIT_19    0x0000000000080000                       
#define CD_BIT_20    0x0000000000100000                       
#define CD_BIT_21    0x0000000000200000                       
#define CD_BIT_22    0x0000000000400000                       
#define CD_BIT_23    0x0000000000800000                       
#define CD_BIT_24    0x0000000001000000                       
#define CD_BIT_25    0x0000000002000000                       
#define CD_BIT_26    0x0000000004000000                       
#define CD_BIT_27    0x0000000008000000                       
#define CD_BIT_28    0x0000000010000000                       
#define CD_BIT_29    0x0000000020000000                       
#define CD_BIT_30    0x0000000040000000                       
#define CD_BIT_31    0x0000000080000000                       

#define DEFAULT_MEDIUM kHDD

#define TWO_ARGS_MACRO(_IN0,_IN1,FUNC,...) FUNC
#define THREE_ARGS_MACRO(_IN0,_IN1,_IN2,FUNC,...) FUNC
#define FOUR_ARGS_MACRO(_IN0,_IN1,_IN2,_IN3,FUNC,...) FUNC
/** \addtogroup cd_defs 
 *@{
 *
 */
/**@brief Type for specifying whether a CD is strict or relaxed
 *
 * This type is used to specify whether a CD is strict or
 * relaxed. The full definition of the semantics of strict and
 * relaxed CDs can be found in the semantics document under
 * <http://lph.ece.utexas.edu/public/CDs>. In brief, concurrent
 * tasks (threads, MPI ranks, ...) in two different strict CDs
 * cannot communicate with one another and must first complete inner
 * CDs so that communicating tasks are at the same CD context. Tasks
 * in two different relaxed CDs may communicate (verified data
 * only). Relaxed CDs typically incur additional runtime overhead
 * compared to strict CDs.
 */
  enum CDType  { kStrict=CD_BIT_0,    //!< A strict CD
                 kRelaxed=CD_BIT_1,  //!< A relaxed CD
                 kDefaultCD=5 //!< Default is kStrict | kDRAM
               };

  
/** @brief Type to indicate where to preserve data
 *
 * \sa CD::GetPlaceToPreserve()
 */
  enum PrvMediumT { kDRAM=CD_BIT_2,  //!< Preserve to DRAM
                    kHDD=CD_BIT_3,   //!< Preserve to HDD
                    kSSD=CD_BIT_4,  //!< Preserve to SSD
                    kPFS=CD_BIT_5,   //!< Preserve to Parallel File System
                    kReserveMedium0=CD_BIT_6,  //!< Preserve to Parallel File System
                    kReserveMedium1=CD_BIT_7,  //!< Preserve to Parallel File System
                  };





/** @brief Type to indicate whether preserved data is from read-only
 * or potentially read/write application data
 *
 * \sa CDHandle::Preserve(), CDHandle::Complete()
 */
  enum PreserveUseT { kUnsure=CD_BIT_15,   //!< Not sure whether data being preserved will be written 
                                        //!< by the CD (treated as Read/Write for now, but may be optimized later)
                      kReadOnly=CD_BIT_16, //!< Data to be preserved is read-only within this CD
                      kReadWrite=CD_BIT_17 //!< Data to be preserved will be modified by this CD
                    };

/** @} */ // End group cd_defs ===========================================

    enum CtxtPrvMode { kExcludeStack=CD_BIT_18, 
                       kIncludeStack=CD_BIT_19
                     };

/**@addtogroup PGAS_funcs 
 * @{
 */

/** @brief Different types of PGAS memory behavior for relaxed CDs.
 *
 * Please see the CD semantics document at
 * <http://lph.ece.utexas.edu/public/CDs> for a full description of
 * PGAS semantics. In brief, because of the logging/tracking
 * requirements of relaxed CDs, it is important to identify which
 * memory accesses may be for communication between relaxed CDs
 * vs. memory accesses that are private (or temporarily privatized)
 * within this CD. 
 *
 */
  enum PGASUsageT {
    kShared = 0,        //!< Definitely shared for actual communication
    kPotentiallyShared, //!< Perhaps used for communication by this
                        //!< CD, essentially equivalent to kShared for CDs.
    kPrivatized,        //!< Shared in general, but not used for any
                        //!< communication during this CD.
    kPrivate            //!< Entirely private to this CD.
  };

/** @} */ // end PGAS_funcs ===========================================

/** \addtogroup tunable_api 
 * @{
 */
  enum TuningT {
    kON=1,
    kSeq=2,
    kNest=4,
    kAuto=8,
    kOFF=16
  };
  /** @} */ // End tunable_api group 

#endif
