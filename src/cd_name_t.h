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

#include <ostream>

namespace cd {

class CDNameT {
  // Level in the CD hierarhcy. It increases at Create() and destroys at Destroy.
	uint32_t level_;  
  uint32_t rank_in_level_;
  uint32_t size_;    // The number of sibling CDs
public:
  CDNameT()
    : level_(0), rank_in_level_(0), size_(0) {}
    
  CDNameT(uint64_t level, uint64_t rank_in_level=0, uint64_t size=1)
    : level_(level), rank_in_level_(rank_in_level), size_(size)
  {}

  uint32_t level(void)         const { return level_; }
  uint32_t rank_in_level(void) const { return rank_in_level_; }
  uint32_t size(void)          const { return size_; }
  CDNameT(const CDNameT& parent_cdname, int num_children=1, int color=0)
  {
    level_         = parent_cdname.level() + 1;
    rank_in_level_ = num_children*(parent_cdname.rank_in_level()) + color;
    size_          = num_children;
  }

  CDNameT(CDNameT&& that) {
    level_         = that.level();
    rank_in_level_ = that.rank_in_level();
    size_          = that.size();
  }

  ~CDNameT()
  {}
 
  CDNameT& operator=(const CDNameT& that) {
    level_         = that.level();
    rank_in_level_ = that.rank_in_level();
    size_          = that.size();
    return *this;
  }

  bool operator==(const CDNameT& that) const {
    return (level_ == that.level()) && (rank_in_level_ == that.rank_in_level()) && (size_ == that.size());
  }
};

std::ostream& operator<<(std::ostream& str, const CDNameT& cd_name);

} // namespace cd ends
#endif 
