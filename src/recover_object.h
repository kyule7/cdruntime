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

#ifndef _RECOVER_OBJECT_H
#define _RECOVER_OBJECT_H

/**
 * @file recover_object.h
 * @author Kyushick Lee, Mattan Erez
 * @date March 2015
 */
#include "cd_global.h"
#include "cd_def_internal.h"

#include "cd.h"
 
namespace cd {

/**@addtogroup register_detection_recovery  
 * @{
 *
 */


/**@class RecoverObject
 * @brief Recovery method that can be inherited and specialized by user
 *
 * The purpose of RecoverObject is to provide an interface to enable a
 * programer to create custom Recover routines. The idea is that for
 * each CD, each error type+location may be bound to a specialized
 * recovery routine, which is expressed through a Recover object. The
 * Recover object inherits the default RecoverObject and extends or
 * replaces the default restore+reexecute recovery method.
 *
 * @todo Write some example for custom recovery 
 * (see GVR interpolation example, although they do it between versions).
 *
 *
 * \sa CDHandle::RegisterRecovery()
 */

class RecoverObject {
public:

  /**@brief Recover method to be specialized by inheriting and overloading
   *
   * Recover uses methods that are internal to the CD itself and should
   * only be called by customized recovery routines that inherit from
   * RecoverObject and modify the Recover() method.
   *
   */
  virtual void Recover(CD* cd, //!< A pointer to the actual CD instance
                               //!< so that the internal methods can be called.
                       uint64_t error_name_mask,     //!< [in] Mask of all error/fail types that require recovery
                       uint64_t error_location_mask, //!< [in] Mask of all error/fail locations that require recovery
                       std::vector<SysErrT> errors   //!< [in] Errors/failures to recover from (typically just one).
                      ); 
  virtual void Recover(CD* cd, //!< A pointer to the actual CD instance
                               //!< so that the internal methods can be called.
                       uint64_t error_name_mask=0,    //!< [in] Mask of all error/fail types that require recovery
                       uint64_t error_location_mask=0 //!< [in] Mask of all error/fail locations that require recovery
                      ); 
};

/** @} */ // End detection_recovery group, but more methods later in CDHandle::CDAssert, etc..

} // namespace cd ends


#endif
