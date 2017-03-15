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

#include "cd_name_t.h"
#include "util.h"

using namespace cd;

CDNameT::CDNameT(void)
  : level_(0), phase_(0), rank_in_level_(0), size_(0) 
{}
  
CDNameT::CDNameT(uint64_t level, uint64_t phase, uint64_t rank_in_level, uint64_t size)
  : level_(level), phase_(phase), rank_in_level_(rank_in_level), size_(size)
{}

uint32_t CDNameT::level(void)         const { return level_; }
uint32_t CDNameT::phase(void)         const { return phase_; }
uint32_t CDNameT::rank_in_level(void) const { return rank_in_level_; }
uint32_t CDNameT::size(void)          const { return size_; }
void     CDNameT::IncLevel(void)            { level_++; }

CDNameT::CDNameT(const CDNameT &parent_cdname, int num_children, int color)
{
  level_         = parent_cdname.level() + 1;
  rank_in_level_ = num_children*(parent_cdname.rank_in_level()) + color;
  size_          = num_children;
  //cout << "level: " << level_ << " parent level : " << parent_cdname.level()
//            << ", rank_in_level created : " << rank_in_level_ 
//            << ", numchild: " << num_children << ", parent rank : " 
//            << parent_cdname.rank_in_level() 
//            << ", color : "<< color << endl; 
}

void CDNameT::copy(const CDNameT& that) 
{
  level_         = that.level();
  phase_         = that.phase();
  rank_in_level_ = that.rank_in_level();
  size_          = that.size();
}

CDNameT::CDNameT(const CDNameT& that)
{
  copy(that);
}

CDNameT &CDNameT::operator=(const CDNameT& that) 
{
  copy(that);
  return *this;
}

bool CDNameT::operator==(const CDNameT& that) const 
{
  return ((level_ == that.level()) && (phase_ == that.phase()) &&
          (rank_in_level_ == that.rank_in_level()) && (size_ == that.size()));
}

int CDNameT::CDTag(void) const { return ((level_ << 16) | rank_in_level_); }

ostream& cd::internal::operator<<(ostream& str, const CDNameT& cd_name)
{
  return str << "CD"<< cd_name.level() << "_"  << cd_name.rank_in_level();
}

string CDNameT::GetString(void) const {
  return ( string("CD") + to_string(level_) + string("_") + to_string(rank_in_level_) );
}

