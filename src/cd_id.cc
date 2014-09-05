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

#include "cd_id.h"
#include "cd_handle.h"
#include "cd.h"
//#include "node_id.h"
using namespace cd;

uint64_t CDID::object_id_ = 0;

CDID::CDID() // TODO Initialize member values to zero or something, 
             //for now I will put just zero but this is less efficient.
//  : object_id_(CDID::object_id_)
{
  domain_id_     = 0; 
  level_         = 0;
  sibling_id_    = 0;
//      task_id_       = 0;
  sequential_id_ = 0;
}

CDID::CDID(uint64_t level, const NodeID& new_node_id)
//  : object_id_(cd::object_id)
{
  domain_id_     = 0; 
  level_         = level;
  sibling_id_    = 0;
  node_id_ = new_node_id;
  sequential_id_ = 0;
}

CDID::CDID(uint64_t level, NodeID&& new_node_id)
//  : object_id_(cd::object_id)
{
  domain_id_     = 0; 
  level_         = level;
  sibling_id_    = 0;
  node_id_ = std::move(new_node_id);
  sequential_id_ = 0;
}

//CDID::CDID(CDHandle* parent, const NodeID& new_node_id)
//{
//  if(parent != NULL) {
//  	level_         = parent->ptr_cd_->GetCDID().level_ + 1;
//  }
//  else { // Root CD
//  	level_         = 0;
//  }
//  node_id_ = new_node_id;
//  domain_id_     = 0; 
//  sibling_id_    = 0;
//  sequential_id_ = 0;
//}
//
//CDID::CDID(CDHandle* parent, NodeID&& new_node_id)
//{
//  if(parent != NULL) {
//  	level_         = parent->ptr_cd_->GetCDID().level_ + 1;
//  }
//  else { // Root CD
//  	level_         = 0;
//  }
//  node_id_ = std::move(new_node_id);
//  domain_id_     = 0; 
//  sibling_id_    = 0;
//  sequential_id_ = 0;
//}

CDID::CDID(const CDID& that)
//  : object_id_(that.object_id_)
{
  domain_id_      = that.domain_id_;
  level_         = that.level_;
  sibling_id_    = 0;
  node_id_       = that.node_id_;
  sequential_id_ = that.sequential_id_;
}
// should be in CDID.h
// old_cd_id should be passed by value
void CDID::UpdateCDID(uint64_t parent_level, const NodeID& new_node_id)
{
  level_ = parent_level++;
  object_id_++;
  node_id_ = new_node_id;
  sequential_id_ = 0;
}

void CDID::SetCDID(const NodeID& new_node_id)
{
  node_id_ = new_node_id;
}

CDID& CDID::operator=(const CDID& that)
{
  domain_id_      = that.domain_id_;
  level_         = that.level_;
//  node_id_       = that.node_id_;
  object_id_     = that.object_id_;
  sequential_id_ = that.sequential_id_;
  return *this;
}


std::ostream& operator<<(std::ostream& str, const CDID& cd_id)
{
  return str<< "Level: "<< cd_id.level_ << ", CDNode" << cd_id.node_id_.color_ << ", Obj# " << cd_id.object_id_ << ", Seq# " << cd_id.sequential_id_;
}

//std::ostream& operator<<(std::ostream& str, const NodeID& node_id)
//{
//  return str<< '(' << node_id.color_ << ", " << node_id.task_ << "/" << node_id.size_ << ')';
//}
