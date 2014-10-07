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

//uint64_t CDID::object_id_ = 0;


// TODO Initialize member values to zero or something, 
//for now I will put just zero but this is less efficient.

CDID::CDID() 
	: domain_id_(0), sequential_id_(0)
{}

CDID::CDID(const CDNameT& cd_name, const NodeID& new_node_id)
	: cd_name_(cd_name), node_id_(new_node_id)
{
  domain_id_ = 0;
  sequential_id_ = 0;

}
//CDID::CDID(CDNameT&& cd_name, NodeID&& new_node_id)
//	: cd_name_(std::move(cd_name)), node_id_(std::move(node_id)), domain_id_(0), sequential_id_(0)
//{}
CDID::CDID(const CDID& that)
  : cd_name_(that.cd_name()), node_id_(that.node_id())
{
  domain_id_     = that.domain_id();
  sequential_id_ = that.sequential_id();
}
// should be in CDID.h
// old_cd_id should be passed by value
//void CDID::UpdateCDID(const CDNameT& cd_name, const NodeID& node_id)
//{
//  object_id_ 		 = Util::GenObjectID();
//  sequential_id_ = 0;
//	cd_name_ 			 = cd_name;
//  node_id_ 			 = node_id;
//}

void CDID::SetCDID(const NodeID& node_id)
{
  node_id_ = node_id;
}

CDID& CDID::operator=(const CDID& that)
{
  domain_id_     = that.domain_id();
  sequential_id_ = that.sequential_id();
  object_id_     = that.object_id();
  return *this;
}


std::ostream& operator<<(std::ostream& str, const CDID& cd_id)
{
//  return str<< "Level: "<< cd_id.level_ << ", CDNode" << cd_id.node_id_.color_ << ", Obj# " << cd_id.object_id_ << ", Seq# " << cd_id.sequential_id_;
  return str<< "Level: "<< cd_id.level() << ", CDNode" << cd_id.color() << ", Obj# " << cd_id.object_id() << ", Seq# " << cd_id.sequential_id();
}

//std::ostream& operator<<(std::ostream& str, const NodeID& node_id)
//{
//  return str<< '(' << node_id.color_ << ", " << node_id.task_ << "/" << node_id.size_ << ')';
//}



//CDID::CDID(CDHandle* parent, const NodeID& new_node_id)
//{
//  if(parent != NULL) {
//    level_         = parent->ptr_cd_->GetCDID().level_ + 1;
//  }
//  else { // Root CD
//    level_         = 0;
//  }
//  node_id_ = new_node_id;
//  domain_id_     = 0; 
//  rank_in_level_    = 0;
//  sequential_id_ = 0;
//}
//
//CDID::CDID(CDHandle* parent, NodeID&& new_node_id)
//{
//  if(parent != NULL) {
//    level_         = parent->ptr_cd_->GetCDID().level_ + 1;
//  }
//  else { // Root CD
//    level_         = 0;
//  }
//  node_id_ = std::move(new_node_id);
//  domain_id_     = 0; 
//  rank_in_level_    = 0;
//  sequential_id_ = 0;
//}

#ifdef szhang
//SZ: reload '==' operator
bool CDID::operator==(const CDID &other) const {
  bool domain_id = (other.domain_id_ == this->domain_id_);
  //SZ: TODO: need to figure out how to match node_id_
  //bool node_id = (other.node_id_ == this->node_id_);
  //bool task_id = (other.task_id_ == this->task_id_);

  bool rank_in_level = (other.rank_in_level_ == this->rank_in_level_);
  bool level = (other.level_ == this->level_);
  bool sequential_id = (other.sequential_id_ == this->sequential_id_);

  //return (domain_id && node_id && task_id && level && sequential_id);
  return (domain_id && level && sequential_id && rank_in_level);
}

//SZ: print function
void CDID::Print ()
{
  std::cout << "CDID information:" << std::endl;
  std::cout << "    domain_id_: " << domain_id_ << std::endl;
  std::cout << "    rank_in_level_: " << rank_in_level_ << std::endl;
  //std::cout << "    node_id_: " << node_id_ << std::endl;
  //std::cout << "    task_id_: " << task_id_ << std::endl;

  std::cout << "    level_: " << level_ << std::endl;
  std::cout << "    object_id_: " << object_id_ << std::endl;
  std::cout << "    sequential_id_: " << sequential_id_ << std::endl;
}
#endif

