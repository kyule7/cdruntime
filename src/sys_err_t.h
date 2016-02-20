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

#ifndef _SYS_ERR_T_H 
#define _SYS_ERR_T_H


/**
 * @file sys_err_t.h
 * @author Kyushick Lee, Mattan Erez
 * @date March 2015
 *
 */
/**\addtogroup error_reporting Error Reporting 
 * The \ref error_reporting module includes the definition of types
 * and methods used for system and CD runtime error/failure reporting.
 * @{
 */

#include "cd_global.h"
#include <vector>
#include <map>
//#include "cd_def_internal.h" 

namespace cd {
/**
 * \brief Type for specifying system errors and failure names
 *
 * This type represents the interface between the user and the
 * system with respect to errors and failures. The intent is for
 * these error/failure names to be fairly comprehensive with respect
 * to system-based issues, while still providing general/abstract
 * enough names to be useful to the application programmer. The use
 * categories/names are meant to capture recovery strategies that
 * might be different based on the error/failure and be
 * comprehensive in that regard. The
 * DeclareErrName() method may be used to create a
 * programmer-defined error that may be associated with a
 * programmer-provided detection method.
 *
 * We considered doing
 * an extensible class hierarchy like GVR, but ended up with a
 * hybrid type system. There are predefined bit vector constants
 * for error/failure names and machine location names. These may be
 * extended by application programmers for specialized
 * detectors. These types are meant to capture abstract general
 * classes of errors that may be treated differently by recovery
 * functions and therefore benefit from easy-to-access and
 * well-defined names. Additional error/failure-specific
 * information will be represented by the SysErrInfo interface class
 * hierarchy, which may be extended by the programmer at compiler
 * time. Thus, each error/failure is a combination of 
 * SysErrNameT, SysErrLocT, and SysErrInfo.
 *
 *
 * __This needs more thought__
 *
 *
 */

/**@class SysErrInfo 
 * @brief Interface to error/failure-specific information 
 *
 * An abstract interface to specific error/failure information, such
 * as address range, core number, degradation, specific lost
 * functionality, ...
 *
 * This is an empty interface because the information is very much
 * error dependent. Also defining a few specific initial examples
 * below. This follows the GVR ideas pretty closely.
 *
 * \sa SoftMemErrInfo, DegradedMemErrInfo
 */
class SysErrInfo {
protected:
  uint64_t info_; //!< Information about system error component.
public:
  SysErrInfo(void) {}
  virtual ~SysErrInfo(void) {}
};

/**@class MemErrInfo
 * @brief Interface to memory-type error information
 *
 * This is meant to potentially be extended.
 *
 */
class MemErrInfo : public SysErrInfo {
public:
  MemErrInfo(void) {}
  virtual ~MemErrInfo(void) {}

  virtual uint64_t get_pa_start(void){ return 0; } //!< Starting physical address
  virtual uint64_t get_va_start(void){ return 0; } //!< Starting virtual address
  virtual uint64_t get_length(void){ return 0; }  //!< Length of affected access
  virtual char    *get_data(void){ return 0; }    //!< Data value read (erroneous)
};

/**@class SoftMemErrInfo
 * @brief Interface to soft memory error information
 *
 * This is meant to potentially be extended.
 *
 */
class SoftMemErrInfo : public MemErrInfo {
protected:
  uint64_t pa_start_;     //!< Starting physical address
  uint64_t va_start_;     //!< Starting virtual address
  uint64_t length_;       //!< Length of affected access
  char    *data_;         //!< Data value read (erroneous)
  uint64_t syndrome_len_; //!< Length of syndrome
  char    *syndrome_;     //!< Value of syndrome
public:
  SoftMemErrInfo(void) {}
  ~SoftMemErrInfo(void) {}
  uint64_t get_pa_start(void);  //!< Starting physical address
  uint64_t get_va_start(void);  //!< Starting virtual address
  uint64_t get_length(void);    //!< Length of affected access
  char    *get_data(void);      //!< Data value read (erroneous)
  uint64_t get_syndrome_len(void); //!< Length of syndrome
  char    *get_syndrome(void);     //!< Value of syndrome
};

/**@class DegradedMemErrInfo 
 * @brief Interface to degraded memory error information
 *
 * This is meant to potentially be extended.
 *
 */
class DegradedMemErrInfo : public MemErrInfo {
protected:
  std::vector<uint64_t> pa_starts_;     //!< Starting physical addresses
  std::vector<uint64_t> va_starts_;     //!< Starting virtual addresses
  std::vector<uint64_t> lengths_;       //!< Lengths of affected regions
public:
  DegradedMemErrInfo(void) {}
  ~DegradedMemErrInfo(void) {}
  std::vector<uint64_t> get_pa_starts(void);//!< Starting physical addresses
  std::vector<uint64_t> get_va_starts(void);//!< Starting virtual addresses
  std::vector<uint64_t> get_lengths(void);  //!< Lengths of affected regions
};


/**@class SysErrT 
 * @brief Data structure for specifying errors and failure
 *
 * should come up with a reasonable way to allow some extensible class hierarchy for various error types.
 * currently, it is more like a bit vector
 * This type represents the interface between the user and the
 * system with respect to errors and failures. We considered doing
 * an extensible class hierarchy like GVR, but ended up with
 * predefined bitvector constants because of the pain involved in
 * setting up and using deep class hierarchies. However, the bitmask
 * way is dangerously narrow and may lead to less portable (and less
 * future-proof code). Basically we chose C over C++ style here :-(
 * \n
 * \n
 * __This needs more thought__
 * \n
 * Type for specifying system errors and failure names.
 * 
 * This type represents the interface between the user and the system with respect to errors and failures. 
 * The intent is for these error/failure names to be fairly comprehensive with respect to system-based issues, 
 * while still providing general/abstract enough names to be useful to the application programmer. 
 * The use categories/names are meant to capture recovery strategies 
 * that might be different based on the error/failure and be comprehensive in that regard. 
 * The DeclareErrName() method may be used to create a programmer-defined error 
 * that may be associated with a programmer-provided detection method.
 * 
 * We considered doing an extensible class hierarchy like GVR, but ended up with a hybrid type system. 
 * There are predefined bit vector constants for error/failure names and machine location names. 
 * These may be extended by application programmers for specialized detectors. 
 * These types are meant to capture abstract general classes of errors that may be treated differently by recovery functions 
 * and therefore benefit from easy-to-access and well-defined names. 
 * Additional error/failure-specific information will be represented by the SysErrInfo interface class hierarchy, 
 * which may be extended by the programmer at compiler time. 
 * Thus, each error/failure is a combination of SysErrNameT, SysErrLocT, and SysErrInfo.
 * 
 */
class SysErrT {
public:
  /// Name of a system error.
  SysErrNameT  sys_err_name_; 

  /// Location of a system error.
  SysErrLocT   sys_err_loc_; 
 
  /// Detail information of system error.
  SysErrInfo*  error_info_;   

  SysErrT(void) {}
  ~SysErrT(void) {}
  
///@brief Print system error information.
  char* printSysErrInfo(void) { return NULL; } 
};

struct SystemConfig {
  std::map<uint64_t, float> failure_rate_;
  float &operator[](const uint64_t &idx) {
    return failure_rate_[idx];
  }
  static void LoadSystemConfig(char *configFile=NULL);
};

extern SystemConfig system_config;

/** @brief Create a new error/failure type name
 *
 * \return Returns a "constant" corresponding to a free bit location in the SysErrNameT bitvector.
 * \sa SysErrNameT, SysErrLocT, UndeclareErrName()
 */
uint32_t DeclareErrName(const char* name_string //!< user-specified name for a new error/failure type
            	          );


/** @brief Free a name that was created with DeclareErrorName()
 *
 * \return Returns kOK on success.
 *
 * \sa SysErrNameT, SysErrLocT, DeclareErrName()
 */
CDErrT UndeclareErrName(uint error_name_id //!< ID to free
		                    );



/** @brief Create a new error/failure type name
 *
 * \return Returns a "constant" corresponding to a free bit location in the SysErrNameT bitvector.
 *
 * \sa SysErrNameT, SysErrLocT, UndeclareErrLoc()
 */
uint32_t DeclareErrLoc(const char* name_string //!< user-specified name for a new error/failure location
	                     );


/** @brief Free a name that was created with DeclareErrLoc()
 *
 * \return Returns kOK on success.
 *
 * \sa SysErrNameT, SysErrLocT, DeclareErrLoc()
 */
CDErrT UndeclareErrLoc(uint error_name_id //!< ID to free
                       );

//std::ostream& operator<<(const std::ostream& str, const SysErrT& sys_err)
//{
//  return str << sys_err.printSysErrInfo();
//}


/** @} */ // End error_reporting group


} // namespace cd ends


#endif
