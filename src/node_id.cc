
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

#include "node_id.h"
using namespace cd;


NodeID::NodeID(void)
  : color_(0), task_in_color_(0), head_(0), size_(-1) 
{}

NodeID::NodeID(const ColorT& color, int task, int head, int size)
  : color_(color), task_in_color_(task), head_(head), size_(size)
{}

NodeID::NodeID(const NodeID& that)
  : color_(that.color()), task_in_color_(that.task_in_color()), head_(that.head()), size_(that.size())
{}


NodeID &NodeID::operator=(const NodeID& that) 
{
  color_         = that.color();
  task_in_color_ = that.task_in_color();
  head_          = that.head();
  size_          = that.size();
  return *this;
}

bool NodeID::operator==(const NodeID& that) const 
{
  return (color_ == that.color()) && (task_in_color_ == that.task_in_color()) && (size_ == that.size());
}



void NodeID::init_node_id(ColorT color, int task_in_color, int head, int size)
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

ColorT NodeID::color(void)         const { return color_; }
int    NodeID::task_in_color(void) const { return task_in_color_; }
int    NodeID::head(void)          const { return head_; }
int    NodeID::size(void)          const { return size_; }
bool   NodeID::IsHead(void)        const { return head_ == task_in_color_; }
void   NodeID::set_head(int head)        { head_ = head; } 

void *NodeID::Serialize(uint32_t& len_in_bytes)
{
  Packer node_id_packer;
  node_id_packer.Add(NODEID_PACKER_COLOR, sizeof(ColorT), &color_);
  node_id_packer.Add(NODEID_PACKER_TASK_IN_COLOR, sizeof(int), &task_in_color_);
  node_id_packer.Add(NODEID_PACKER_HEAD, sizeof(int), &head_);
  node_id_packer.Add(NODEID_PACKER_SIZE, sizeof(int), &size_);
  return node_id_packer.GetTotalData(len_in_bytes);  
}

void NodeID::Deserialize(void* object) 
{
  Unpacker node_id_unpacker;
  uint32_t return_size;
  uint32_t dwGetID;
  color_         = *(ColorT *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
  task_in_color_ = *(int *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
  head_          = *(int *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
  size_          = *(int *)node_id_unpacker.GetNext((char *)object, dwGetID, return_size);
}

std::ostream &cd::operator<<(std::ostream &str, const NodeID &node_id)
{
  return str << '(' 
             << node_id.task_in_color() << "/" << node_id.size() 
             << ')';
}

std::string NodeID::GetString(void) const 
{
  return ( std::string("(") + std::to_string(task_in_color_) + std::string("/") + std::to_string(size_) + std::string(")") );
}
