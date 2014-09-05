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

#ifndef _NODE_ID_H
#define _NODE_ID_H
#include "cd_global.h"
#include <ostream>
#include <utility>
namespace cd {

class NodeID 
{
  public:
  int color_;
  int task_;
  int size_;
  uint64_t sibling_id_{0};
  NodeID() 
    : color_(0), task_(0), size_(-1), sibling_id_(0) 
  {}
  NodeID(int color, int task, int size, uint64_t sibling_id)
    : color_(color), task_(task), size_(size), sibling_id_(sibling_id)
  {}
  NodeID(const NodeID& that)
    : color_(that.color_), task_(that.task_), size_(that.size_), sibling_id_(that.sibling_id_)
  {}
  NodeID(NodeID&& that)
    : task_(that.task_), size_(that.size_), sibling_id_(that.sibling_id_)
  {
    color_ = std::move(that.color_);
  }
  NodeID& operator=(const NodeID& that) {
    color_      = (that.color_);
    task_       = (that.task_);
    size_       = (that.size_);
    sibling_id_ = that.sibling_id_;
    return *this;
  }
  
};

std::ostream& operator<<(std::ostream& str, const NodeID& node_id);

}
#endif
