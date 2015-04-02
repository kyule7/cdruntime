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

#ifndef _REGEN_OBJECT_H
#define _REGEN_OBJECT_H
/**
 * @file regen_object.h
 * @author Kyushick Lee, Mattan Erez
 * @date March 2015
 */

#include "cd_global.h"
#include "cd_def_internal.h" 
namespace cd {

/** \addtogroup preservation_funcs 
 *  The \ref preservation_funcs module contains all
 *  preservation/restoration related types and methods.
 *
 * @{
 *
 * @brief Interface for specifying regeneration functions for preserve/restore
 *
 * An interface for a data regeneration function that can be used to
 * restore "preserved" data instead of making a copy of the data to
 * be preserved.
 *
 * \sa PreserveMethodT, CDHandle::Preserve()
 */
class RegenObject {

  protected:
    /** @brief Pure virtual interface function for regenerating data as restoration type
     *
     * Must be implemented by programmer.
     *
     * \return Should return a CD error value if regeneration is not successful. 
     */
    virtual void Regenerate(CDHandle &handle); //!< Not supported yet.
    //TODO Is this needs to be a CDHandle or CD? If our policy is not to expose CD object directly then CDHandle is the one?
};

/** @} */ // End preservation_funcs group, but more methods later in CDHandle

} // namespace cd ends

#endif
