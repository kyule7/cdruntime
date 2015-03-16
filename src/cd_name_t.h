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


#ifndef _CD_NAME_T_H
#define _CD_NAME_T_H
/**
 * @file cd_name_t.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * \brief Containment Domains namespace for global functions, types, and main interface
 *
 * All user-visible CD API calls and definitions are under the CD namespace.
 * Internal CD API implementation components are under a separate cd_internal
 * namespace; the CDInternal class of namespace cd serves as an interface where necessary.
 *
 */

#include "cd_global.h"

using std::endl;

namespace cd {
/** \addtogroup cd_defs
 *
 *
 *@{
 *
 * @brief A type to uniquely name a CD in the tree
 *
 * A CD name consists of its level in the CD tree (root=0) and the
 * its ID, or sequence number, within that level. 
 *
 */ 

class CDNameT {
  // Level in the CD hierarhcy. It increases at Create() and destroys at Destroy.
	uint32_t level_;  
  uint32_t rank_in_level_;
  uint32_t size_;    // The number of sibling CDs
public:
  CDNameT(void);
  CDNameT(uint64_t level, uint64_t rank_in_level=0, uint64_t size=1);
  CDNameT(const CDNameT &parent_cdname, int num_children, int color);
  CDNameT(const CDNameT& that);
  ~CDNameT(void) {}

  uint32_t level(void) const;
  uint32_t rank_in_level(void) const;
  uint32_t size(void) const;
  void IncLevel(void);
 
  CDNameT& operator=(const CDNameT& that);
  bool operator==(const CDNameT& that) const;
  int CDTag(void) const; 
};


std::ostream& operator<<(std::ostream& str, const CDNameT& cd_name);

/** @} */ // End group cd_defs

} // namespace cd ends

#endif 
