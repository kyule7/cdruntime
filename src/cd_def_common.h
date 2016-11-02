#ifndef _CD_DEF_COMMON_H 
#define _CD_DEF_COMMON_H

#define BIT_0     0x0000000000000001  
#define BIT_1     0x0000000000000002                       
#define BIT_2     0x0000000000000004                       
#define BIT_3     0x0000000000000008                       
#define BIT_4     0x0000000000000010                       
#define BIT_5     0x0000000000000020                       
#define BIT_6     0x0000000000000040                       
#define BIT_7     0x0000000000000080                       
#define BIT_8     0x0000000000000100                       
#define BIT_9     0x0000000000000200                       
#define BIT_10    0x0000000000000400                       
#define BIT_11    0x0000000000000800                       
#define BIT_12    0x0000000000001000                       
#define BIT_13    0x0000000000002000                       
#define BIT_14    0x0000000000004000                       
#define BIT_15    0x0000000000008000                       
#define BIT_16    0x0000000000010000                       
#define BIT_17    0x0000000000020000                       
#define BIT_18    0x0000000000040000                       
#define BIT_19    0x0000000000080000                       
#define BIT_20    0x0000000000100000                       
#define BIT_21    0x0000000000200000                       
#define BIT_22    0x0000000000400000                       
#define BIT_23    0x0000000000800000                       
#define BIT_24    0x0000000001000000                       
#define BIT_25    0x0000000002000000                       
#define BIT_26    0x0000000004000000                       
#define BIT_27    0x0000000008000000                       
#define BIT_28    0x0000000010000000                       
#define BIT_29    0x0000000020000000                       
#define BIT_30    0x0000000040000000                       
#define BIT_31    0x0000000080000000                       
#define BIT_32    0x0000000100000000                       
#define BIT_33    0x0000000200000000                       
#define BIT_34    0x0000000400000000                       
#define BIT_35    0x0000000800000000                       
#define BIT_36    0x0000001000000000                       
#define BIT_37    0x0000002000000000                       
#define BIT_38    0x0000004000000000                       
#define BIT_39    0x0000008000000000                       
#define BIT_40    0x0000010000000000                       
#define BIT_41    0x0000020000000000                       
#define BIT_42    0x0000040000000000                       
#define BIT_43    0x0000080000000000                       
#define BIT_44    0x0000100000000000                       
#define BIT_45    0x0000200000000000                       
#define BIT_46    0x0000400000000000           
#define BIT_47    0x0000800000000000                       
#define BIT_48    0x0001000000000000                       
#define BIT_49    0x0002000000000000                       
#define BIT_50    0x0004000000000000                       
#define BIT_51    0x0008000000000000                       
#define BIT_52    0x0010000000000000           
#define BIT_53    0x0020000000000000                       
#define BIT_54    0x0040000000000000                       
#define BIT_55    0x0080000000000000                       
#define BIT_56    0x0100000000000000                       
#define BIT_57    0x0200000000000000                       
#define BIT_58    0x0400000000000000           
#define BIT_59    0x0800000000000000                       
#define BIT_60    0x1000000000000000                       
#define BIT_61    0x2000000000000000                       
#define BIT_62    0x4000000000000000                       
#define BIT_63    0x8000000000000000                       


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
                    kLocalFile=24,
                    kAllFile=56
                  };


/** @} */ // End group cd_defs ===========================================


/**@addtogroup preservation_funcs 
 * @{
 */

/** 
 * @brief Type for specifying preservation methods
 *
 * See <http://lph.ece.utexas.edu/public/CDs> for a detailed description. 
 *
 * The intent is for this to be used as a mask for specifying
 * multiple legal preservation methods so that the autotuner can
 * choose the appropriate one.
 *
 * \sa RegenObject, CDHandle::Preserve()
 */
  enum CDPreserveT  { kCopy=CD_BIT_8, //!< Prevervation via copy copies
                                   //!< the data to be preserved into
                                   //!< another storage/mem location
                                   //!< Preservation via reference
                      kRef=CD_BIT_9, //!< Preservation via reference     
                                  //!< indicates that restoration can
                                  //!< occur by restoring data that is
                                  //!< already preserved in another
                                  //!< CD. __Restriction:__ in the
                                  //!< current version of the API only
                                  //!< the parent can be used as a
                                  //!< reference. 
                      kRegen=CD_BIT_10, //!< Preservation via regenaration
                                    //!< is done by calling a
                                    //!< user-provided function to
                                    //!< regenerate the data during
                                    //!< restoration instead of copying
                                    //!< it from preserved storage.
                      kCoop=CD_BIT_11,  //!< This flag is used for preservation-via-reference 
                                    //!< in the case that the referred copy is in remote task.
                                    //!< This flag can be used with kCopy
                                    //!< such as kCopy | kCoop.
                                    //!< Then, this entry can be referred by lower level.
                      kSerdes=CD_BIT_12, //!< This flag indicates the preservation is done by
                                      //!< serialization, which mean it does not need to 
                                      //!< duplicate the data because serialized data is
                                      //!< already another form of preservation.
                                      //!< This can be used such as kCopy | kSerdes
                      kSource=CD_BIT_13, //!< Indicates application data
                      kModify=CD_BIT_14, //!< This indicates the modified data.
                                         //!< kModify is used for advance() call to
                                         //!< update preservation entries which are marked with it.
                      kReservedPrvT0=CD_BIT_15
                    };


/** @brief Type to indicate whether preserved data is from read-only
 * or potentially read/write application data
 *
 * \sa CDHandle::Preserve(), CDHandle::Complete()
 */
  enum PreserveUseT { kUnsure=CD_BIT_16,   //!< Not sure whether data being preserved will be written 
                                        //!< by the CD (treated as Read/Write for now, but may be optimized later)
                      kReadOnly=CD_BIT_17, //!< Data to be preserved is read-only within this CD
                      kReadWrite=CD_BIT_18 //!< Data to be preserved will be modified by this CD
                    };

/** @} */ // end of preservation_funcs

  enum CtxtPrvMode { kExcludeStack=CD_BIT_19, 
                     kIncludeStack=CD_BIT_20
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
