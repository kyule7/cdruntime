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
//  object_id_     = Util::GenObjectID();
//  sequential_id_ = 0;
//  cd_name_       = cd_name;
//  node_id_       = node_id;
//}

void CDID::SetCDID(const NodeID& node_id)
{
  node_id_ = node_id;
}

CDID& CDID::operator=(const CDID& that)
{
  domain_id_     = that.domain_id();
  sequential_id_ = that.sequential_id();
  cd_name_       = that.cd_name();
  node_id_       = that.node_id();
//  object_id_     = that.object_id();
  return *this;
}


bool CDID::operator==(const CDID& that) const
{
  return (node_id_ == that.node_id()) && (cd_name_ == that.cd_name()) &&  (sequential_id_ == that.sequential_id());

}


std::ostream& operator<<(std::ostream& str, const CDID& cd_id)
{
  return str<< "Level: "<< cd_id.level() << ", CDNode" << cd_id.color() << ", Obj# " << cd_id.object_id() << ", Seq# " << cd_id.sequential_id();
}

//std::ostream& operator<<(std::ostream& str, const NodeID& node_id)
//{
//  return str<< '(' << node_id.color_ << ", " << node_id.task_ << "/" << node_id.size_ << ')';
//}



//SZ: print function
void CDID::Print ()
{
  PRINT_DEBUG("CDID information:\n");
  PRINT_DEBUG("    domain_id_: %ld\n"     , (unsigned long)domain_id_);
  PRINT_DEBUG("    object_id_: %ld\n"     , (unsigned long)object_id_);
  PRINT_DEBUG("    sequential_id_: %ld\n" , (unsigned long)sequential_id_);

  PRINT_DEBUG("    level: %ld\n"          , (unsigned long)level());
  PRINT_DEBUG("    rank_in_level: %ld\n"  , (unsigned long)rank_in_level());
  PRINT_DEBUG("    sibling_count: %ld\n"  , (unsigned long)sibling_count());

  PRINT_DEBUG("    task_count: %d\n"      , task_count());
  PRINT_DEBUG("    task_in_color: %d\n"   , task_in_color());
  PRINT_DEBUG("    head: %d\n"            , head());
  PRINT_DEBUG("    IsHead: %d\n"          , IsHead());
}

