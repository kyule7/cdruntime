
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

#include "cd_config.h"
#include "node_id.h"
#include "util.h"

#include "cd_def_internal.h" 
using namespace cd;
using std::endl;


NodeID::NodeID(int head)
  : task_in_color_(-1), head_(head), size_(-1), color_id_(-1) 
{
#if _MPI_VER
  color_ = MPI_COMM_NULL;
#else
  color_ = -1; 
#endif
//  printf("NodeID 1 created\n"); getchar();
//  if(myTaskID == 0) {
//    printf("\nnodeid0\n"); 
//  }
//  assert(0);
}

NodeID::NodeID(const ColorT& color, int task, int head, int size)
  : task_in_color_(task), head_(head), size_(size), color_id_(-1), color_(color)
{
//  printf("NodeID 2 created\n"); getchar();
#if _MPI_VER
//  int ret = MPI_Comm_dup(color, &color_);
//  color_ = color;
//  assert(color_);
//  if(color_ != 0)
//    MPI_Comm_group(color_, &task_group_);

//  if(myTaskID == 0) {
//    printf("\nnodeid1\n"); getchar();
//  }
#endif
}

NodeID::NodeID(const NodeID& that)
  : task_in_color_(that.task_in_color_), head_(that.head_), size_(that.size_), color_id_(that.color_id_), color_(that.color_)
{
//  printf("NodeID 3 created %lu\n"); getchar();
#if _MPI_VER
//  int ret = MPI_Comm_dup(that.color_, &color_); 
//  color_ = that.color_;
//  printf("ret : %d %d\n", ret, MPI_SUCCESS);
//  assert(ret == MPI_SUCCESS);
//  assert(color_);
//  if(color_ != 0)
//    MPI_Comm_group(color_, &task_group_);
//  if(myTaskID == 0) {
//    printf("\nnodeid2\n"); getchar();
//  }
#else
  //printf("\nnodeid2222\n");
//  color_ = that.color_;
//  task_group_ = that.task_group_;
#endif
  task_group_ = that.task_group_;
}

NodeID::NodeID(NodeID &&that)
  : task_in_color_(std::move(that.task_in_color_))
  , head_(std::move(that.head_))
  , size_(std::move(that.size_))
  , color_id_(std::move(that.color_id_))
  , color_(std::move(that.color_))
  , task_group_(std::move(that.task_group_))
{}

NodeID &NodeID::operator=(const NodeID& that) 
{
//#if _MPI_VER
////  int ret = MPI_Comm_dup(that.color_, &color_);
//  color_ = that.color_;
////  printf("ret : %d\n", ret);
////  assert(ret == MPI_SUCCESS);
////  if(color_ != 0)
////    MPI_Comm_group(color_, &task_group_);
////  if(myTaskID == 0) {
////    printf("\nnodeid3\n"); getchar();
////  }
//#else
//  color_ = that.color_;
//  task_group_ = that.task_group_;
//#endif
  color_ = that.color_;
  task_group_ = that.task_group_;
  task_in_color_ = that.task_in_color_;
  head_          = that.head_;
  size_          = that.size_;
  color_id_      = that.color_id_;
  return *this;
}

bool NodeID::operator==(const NodeID& that) const 
{
  return (color_ == that.color()) && (task_in_color_ == that.task_in_color()) && (size_ == that.size());
}

void NodeID::init_node_id(ColorT color, int task_in_color, GroupT group, int head, int size)
{
#if _MPI_VER
  color_ = color;
  //int ret = MPI_Comm_dup(color, &color_);
  //assert(ret == MPI_SUCCESS);
  //assert(color_);
  if(color_ != 0)
    MPI_Comm_group(color_, &task_group_);
  //printf("\nnodeid4\n");
#else
  color_ = color;
  task_group_ = group;
#endif

  task_in_color_ = task_in_color;
  if(head == INVALID_HEAD_ID) {
    head_ = 0;
  } else {
    head_ = head;
  }
  size_ = size;
} 

ColorT NodeID::color(void)         const { return color_; }
GroupT &NodeID::group(void)              { return task_group_; }
int    NodeID::task_in_color(void) const { return task_in_color_; }
int    NodeID::head(void)          const { return head_; }
int    NodeID::size(void)          const { return size_; }
int    NodeID::color_id(void)         const { return color_id_; }
bool   NodeID::IsHead(void)        const { return head_ == task_in_color_; }
void   NodeID::set_head(int head)        { head_ = head; } 

void *NodeID::Serialize(uint64_t& len_in_bytes)
{
  TaskInfo *taskinfo = new TaskInfo(color_id_, task_in_color_, head_, size_);
  len_in_bytes = sizeof(TaskInfo);
  return taskinfo;
}

void NodeID::Deserialize(void* object) 
{
  TaskInfo *taskinfo = reinterpret_cast<TaskInfo *>(object);
  taskinfo->color_id_      = taskinfo->color_id_;
  taskinfo->task_in_color_ = taskinfo->task_in_color_;
  taskinfo->head_          = taskinfo->head_;
  taskinfo->size_          = taskinfo->size_;
}

std::ostream &cd::internal::operator<<(std::ostream &str, const NodeID &node_id)
{
  return str << '(' 
             << node_id.task_in_color() << "/" << node_id.size() 
             << ')';
}

string NodeID::GetString(void) const 
{
  return ( string("(") + to_string(task_in_color_) + string("/") + to_string(size_) + string(")") );
}

string NodeID::GetStringID(void) const 
{
  return ( to_string(task_in_color_) + string("_") + to_string(size_) );
}
