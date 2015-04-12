/*
Copyright 2014, The University of Texas at Austin 
All rights reserved.

THIS FILE IS PART OF THE CONTAINMENT DOMAINS RUNTIME LIBRARY

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CD_GLOBAL_H
#define _CD_GLOBAL_H

/**
 * @file cd_global.h
 * @author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * @date March 2015
 *
 * @brief Containment Domains API v0.2 (C++)
 */

/** 
 * \brief Declarations/Definitions for global usages in CD runtime.
 *
 * All user-visible CD API calls and definitions are under the CD namespace.
 * Internal CD API implementation components are under a separate CDInternal namespace.
 *
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
// This could be different from MPI program to PGAS program
// key is the unique ID from 0 for each CD node.
// value is the unique ID for mpi communication group or thread group.
// For MPI, there is a communicator number so it is the group ID,
// For PGAS, there should be a group ID, or we should generate it. 

#if _MPI_VER 
#include <mpi.h>
#define ROOT_COLOR    MPI_COMM_WORLD
#define INITIAL_COLOR MPI_COMM_NULL
#define ROOT_HEAD_ID  0
#else
#define ROOT_COLOR    0 
#define INITIAL_COLOR 0
#define ROOT_HEAD_ID  0
#endif

#if _SINGLE_VER
#define DEFINE_COLOR 0
#else
#define DEFINE_COLOR 1
#endif


#if DEFINE_COLOR
///@addtogroup cd_defs 
///@{
///@var typedef ColorT
///@brief ColorT is a type that signifies a task group entity. 
///       For MPI version, it corresponds to MPI_Comm.
///@typedef ColorT is used for non-single task program.OR
typedef MPI_Comm      ColorT;
///@}
#else
typedef int           ColorT;
#endif


namespace cd {
  class CDHandle;
  class DebugBuf;
#if _DEBUG
//  extern std::ostringstream dbg;
  extern DebugBuf dbg;
#define dbgBreak nullFunc
#else
#define dbg std::cout
#define dbgBreak nullFunc
#endif


/**@addtogroup internal_error_types  
 * @{
 *
 * @brief Type for specifying error return codes from an API call --
 * signifies some failure of the API call itself, not a system 
 * failure.
 *
 * Unlike SysErrNameT, CDErrT is not for system errors, but
 * rather errors originating from the CD framework itself.
 *
 * For now, only returning OK or error, but will get more elaborate
 * in future versions of this API.
 */
  enum CDErrT { kOK=0,        //!< Call executed without error       
                kAlreadyInit, //!< Init called more than once
                kError,       //!< Call did not execute as expected
                kExecutionModeError, //!< Errors during execution
                kOutOfMemory, //!< Error due to not enough memory
                kFileOpenError, //!< error during opening file
                kAppError 
              };

/** @} */ // end of internal_error_types


/**@addtogroup error_reporting 
 * @{
 *
 * @brief Type for specifying system error/failure names
 *
 * This type represents the interface between the user and the
 * system with respect to errors and failures. The intent is for
 * these error/failure names to be fairly comprehensive with respect
 * to system-based issues, while still providing general/abstract
 * enough names to be useful to the application programmer. The use
 * categories/names are meant to capture recovery strategies that
 * might be different based on the error/failure and be comprehensive in that regard. 
 *
 * \warning The SysErrNameT and SysErrLocT are extensible by a runtime
 * call to generate a new constant, but SysErrInfo is a class
 * hierarchy and extended at compile time by inhereting the
 * interface -- is this a problem? Should we go the GVR way with all
 * extensions done at runtime and all accesses with potential
 * runtime methods and no compile-time typing?
 *
 * \todo Is SysErrNameT comprehensive enough for portability?
 * \todo segv (segmentation violations) can be used as proxy for soft memory errors using the existing kernel infrastructure
 *
 * \sa SysErrLocT, DeclareErrName(), UndeclareErrName()
 */
  enum SysErrNameT  { kNoHWError=0,   //!< No errors/failures.
                      kSoftMem=1,     //!< Soft memory error.
                                      //!< (info includes address range and perhaps syndrome)
                      kDegradedMem=2, //!< Hard memory error that disabled some memory capacity.
                                      //!< (info includes address range(s))
                      kSoftComm=4,    //!< Soft communication error.
		                                  //!< (info includes message info)
                      kDegradedComm=8, //!< Some channel loss.
                      kSoftComp=16,    //!< Soft compute error.
		                                   //!< (info includes affected PC and perhaps bounds on the error?)
                      kDegradedResource=32, //!< Resource lost __some__ functionality.
                      kHardResource=64, //!< Resource entirely lost.
		                                    //!< (control/reachability failure).
                      kFileSys=128      //!< Some file system error.
                    };

/** @brief Type for specifying errors and failure location names
 *
 * Please see SysErrNameT for discussion of intent and defintions
 *
 * This is really not that suitable for all topologies, but the
 * intent really is to be rather comprehensive to maintain
 * portability -- how do we resolve this?
 *
 * \todo is SysErrLocT comprehensive enough for portability?
 *
 *
 * \sa SysErrNameT, DeclareErrLoc(), UndeclareErrLoc()
 */
  enum SysErrLocT   { kNoSysErr=0,      //!< No errors/failures
                      kIntraCore=1,     //!< Within a part of a core
                      kCore=2,          //!< A core
                      kProc=4,          //!< Processor
                      kNode=8,          //!< Same as processor?
                      kModule=16,       //!< Module
                      kCabinet=32,      //!< A cabinet
                      kCabinetGroup=64, //!< Some grouping of cabinets
                      kSystem=128       //!< Entire system
                    };

/** @} */ // end of error_reporting



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
  enum CDPreserveT  { kCopy=1, //!< Prevervation via copy copies
                               //!< the data to be preserved into
                               //!< another storage/mem location
                               //!< Preservation via reference
											kRef=2, //!< Preservation via reference     
                              //!< indicates that restoration can
                              //!< occur by restoring data that is
                              //!< already preserved in another
                              //!< CD. __Restriction:__ in the
                              //!< current version of the API only
                              //!< the parent can be used as a
                              //!< reference. 
											kRegen=4, //!< Preservation via regenaration
                                //!< is done by calling a
                                //!< user-provided function to
                                //!< regenerate the data during
                                //!< restoration instead of copying
                                //!< it from preserved storage.
                      kCoop=8   //!< This flag is used for preservation-via-reference 
                                //!< in the case that the referred copy is in remote task.
                                //!< This flag can be used with kCopy
                                //!< such as kCopy | kCoop.
                                //!< Then, this entry can be referred by lower level.
                                
										};
/** @} */ // end of preservation_funcs




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
    enum CDType  { kStrict=1,   ///< A strict CD
              		 kRelaxed=2,   ///< A relaxed CD
                   kDefaultCD=5   ///< Default is strict CD
                 };

/** @brief Type to indicate whether preserved data is from read-only
 * or potentially read/write application data
 *
 * \sa CDHandle::Preserve(), CDHandle::Complete()
 */
    enum PreserveUseT { kUnsure =0, //!< Not sure whether data being preserved will be written 
                                    //!< by the CD (treated as Read/Write for now, but may be optimized later)
                        kReadOnly = 1, //!< Data to be preserved is read-only within this CD
                        kReadWrite = 2 //!< Data to be preserved will be modified by this CD
    };
  
/** @brief Type to indicate where to preserve data
 *
 * \sa CD::GetPlaceToPreserve()
 */
    enum PrvMediumT { kDRAM=4,  //!< Preserve to DRAM
                      kHDD=8,   //!< Preserve to HDD
                      kSSD=16,  //!< Preserve to SSD
                      kPFS=32   //!< Preserve to Parallel File System
                    };

/** @} */ // End group cd_defs ===========================================

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

  extern int myTaskID;
  extern int totalTaskSize;

 /** \addtogroup cd_accessor_funcs 
  * @{
  */

/**@brief Accessor function to current active CD.
 * 
 *  At any point after the CD runtime is initialized, each task is
 *  associated with a current CD instance. The current CD is the
 *  deepest CD in the tree visible from the task that has begun but
 *  has not yet completed. In other words, whenever a CD begins, it
 *  becomes the current CD. When a CD completes, its parent becomes
 *  the current CD.
 *
 *  @return returns a pointer to the handle of the current active CD; Returns
 *  0 if CD runtime is not yet initialized or because of a CD
 *  implementation bug.
 */
  CDHandle *GetCurrentCD(void);

 /**
  * @brief Accessor function to root CD of the application.
  * @return returns a pointer to the handle of the root CD; Returns
  * 0 if CD runtime is not yet initialized or because of a CD
  * implementation bug.
  */
  CDHandle *GetRootCD(void);

 /** @brief Accessor function to a CDHandle at a specific level in CDPath 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(void);

 /** @brief Accessor function to a CDHandle at a specific level in CDPath 
  *  @return Pointer to CDHandle at a level
  */
  CDHandle *GetParentCD(int current_level);

  /** @} */ // End cd_accessor_funcs group =====================================================

/**@class cd::DebugBuf
 * @brief Utility class for debugging.
 *
 *  Like other parallel programming models, it is hard to debug CD runtime. 
 *  So, debugging information are printed out to file using this class.
 *  The global object of this class, dbg, is defined.
 * 
 *  The usage of this class is like below.
 *  \n
 *  \n
 *  DebugBuf dbgApp;
 *  dbgApp.open("./file_path_to_write"); 
 *  dbgApp << "Debug Information" << endl;
 *  dbg.flush(); 
 *  dbg.close();
 */ 
  class DebugBuf: public std::ostringstream {
    std::ofstream ofs_;
  public:
    ~DebugBuf() {}
    DebugBuf(void) {}
    DebugBuf(const char *filename) 
      : ofs_(filename) {}
/**
 * @brief Open a file to write debug information.
 *
 */
    void open(const char *filename)
    {
      ofs_.open(filename);
    }

/**
 * @brief Close the file where debug information is written.
 *
 */    
    void close(void)
    {
      ofs_.close();
    }

 
/**
 * @brief Flush the buffer that contains debugging information to the file.
 *        After flushing it, it clears the buffer for the next use.
 *
 */
    void flush(void) {
      ofs_ << str();
      ofs_.flush();
      str("");
      clear();
    }
      
  };


/**@class cd::Tag
 * @brief Utility class to generate tag.
 *
 */ 
  class Tag : public std::string {
    std::ostringstream _oss;
  public:
    Tag() {}
    ~Tag() {}
    template <typename T>
    Tag &operator<<(const T &that) {
      _oss << that;
      return *this;
    }
    std::string str(void) {
      return _oss.str();
    }
  }; 


  extern CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium);
  extern void CD_Finalize(DebugBuf *debugBuf=NULL);



}

#define CD_Begin(X) if((X)->ctxt_prv_mode() == kExcludeStack) setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin();
//#define CD_Begin(X) (X)->Begin(); if((X)->ctxt_prv_mode() ==CD::kExcludeStack) (X)->jmp_val_=setjmp((X)->jmp_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff();
#define CD_Complete(X) (X)->Complete()   

#endif
