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
/*
NodeID is a class for access to a task in one CD. 
Color is a kind of group number or an arbitrary type indicating a group of tasks corresponding to a CD.
task_in_a_color means an ID to access a task in that color. 
(ex. rank ID of a communicator in MPI context. Communicator corresponds to color_). 
size means the number of tasks in that color. With these three information, we can access to any tasks.
*/
class NodeID {
friend class CDHandle;
  ColorT color_;
  int task_in_color_;
  int head_;
  int size_;
public:
  NodeID() 
    : color_(0), task_in_color_(0), head_(0), size_(-1) 
  {}
  NodeID(const ColorT& color, int task, int head, int size)
    : color_(color), task_in_color_(task), head_(head), size_(size)
  {}
  NodeID(const NodeID& that)
    : color_(that.color()), task_in_color_(that.task_in_color()), head_(that.head()), size_(that.size())
  {}
  ~NodeID(){}
  NodeID& operator=(const NodeID& that) {
    color_         = that.color();
    task_in_color_ = that.task_in_color();
    head_          = that.head();
    size_          = that.size();
    return *this;
  }
  void init_node_id(ColorT color, int task_in_color, int head, int size)
  {
    color_ = color;
    task_in_color_ = task_in_color;
    if(head == INVALID_HEAD_ID) {
      head_ = 0;
    } else {
      head_ = head;
    }
    size_ = size;
  } 
  void set_head(int head) { head_ = head; } 
  ColorT color(void) const { return color_; }
  int task_in_color(void) const { return task_in_color_; }
  int head(void) const { return head_; }
  int size(void) const { return size_; }
  bool IsHead(void) const { return head_ == task_in_color_; }
};

std::ostream& operator<<(std::ostream& str, const NodeID& node_id);

} // namespace cd ends
#endif
