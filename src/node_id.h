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
#include "serializable.h"
#include "packer.h"
#include "unpacker.h"
#include <ostream>
#include <utility>


namespace cd {
/*
In MPI context, one process can be represented by a pair of communicator and MPI rank ID. 
But one process can belong to multiple communicators, so one pair of communicator and MPI rank ID do not uniquely represent a single process.

NodeID is a class for representing a task in one CD. Like MPI, one task can be represented by multiple pairs of color and task ID in the color.  
Color is a kind of group number or an arbitrary type indicating a group of tasks corresponding to a CD.
task_in_a_color means an ID to access a task in that color. 
(ex. rank ID of a communicator in MPI context. Communicator corresponds to color_). 
size means the number of tasks in that color. With these three information, we can access to any tasks.
*/
class NodeID : public Serializable {
friend class CDHandle;
  enum { 
    NODEID_PACKER_COLOR=0,
    NODEID_PACKER_TASK_IN_COLOR,
    NODEID_PACKER_HEAD,
    NODEID_PACKER_SIZE 
  };
 
  ColorT color_;
  int task_in_color_;
  int head_;
  int size_;
  GroupT task_group_;
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

  bool operator==(const NodeID& that) const {
    return (color_ == that.color()) && (task_in_color_ == that.task_in_color()) && (size_ == that.size());
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

  void * Serialize(uint32_t& len_in_bytes)
  {
    std::cout << "\nNode ID Serialize\n" << std::endl;
    Packer node_id_packer;
    node_id_packer.Add(NODEID_PACKER_COLOR, sizeof(ColorT), &color_);
    node_id_packer.Add(NODEID_PACKER_TASK_IN_COLOR, sizeof(int), &task_in_color_);
    node_id_packer.Add(NODEID_PACKER_HEAD, sizeof(int), &head_);
    node_id_packer.Add(NODEID_PACKER_SIZE, sizeof(int), &size_);
    std::cout << "\nNode ID Serialize Done\n" << std::endl;
    return node_id_packer.GetTotalData(len_in_bytes);  
  }

  void Deserialize(void* object) 
  {
    std::cout << "\nNode ID Deserialize\n" << std::endl;
    Unpacker node_id_unpacker;
    uint32_t return_size;
    uint32_t dwGetID;
    color_ = *(ColorT *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
    std::cout << "first unpacked thing in node_id : " << dwGetID << ", return size : " << return_size << std::endl;

    task_in_color_ = *(int *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
    std::cout << "second unpacked thing in node_id : " << dwGetID << ", return size : " << return_size << std::endl;

    head_ = *(int *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
    std::cout << "third unpacked thing in node_id : " << dwGetID << ", return size : " << return_size << std::endl;

    size_ = *(int *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
    std::cout << "fourth unpacked thing in node_id : " << dwGetID << ", return size : " << return_size << std::endl;
  }


};

std::ostream& operator<<(std::ostream& str, const NodeID& node_id);

} // namespace cd ends
#endif
