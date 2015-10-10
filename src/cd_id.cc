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
#include "cd_id.h"
#include "util.h"
using namespace cd;

// TODO Initialize member values to zero or something, 
//for now I will put just zero but this is less efficient.

CDID::CDID() 
  : domain_id_(0), sequential_id_(0)
{
  object_id_ = 0;
}

CDID::CDID(const CDNameT& cd_name, const NodeID& new_node_id)
  : cd_name_(cd_name), node_id_(new_node_id)
{
  domain_id_ = 0;
  sequential_id_ = 0;
//  if(Util::object_id.find(cd_name.level_) != Util::object_id.end()) {
//    object_id_ = Util::object_id[cd_name.level_];
//  }
//  else {
//    Util::object_id[cd_name.level_] = 0;
//    object_id_ = 0;
//  }
  object_id_ = 0;

}

CDID::CDID(const CDID& that)
  : cd_name_(that.cd_name()), node_id_(that.node_id())
{
  domain_id_     = that.domain_id();
  sequential_id_ = that.sequential_id();
//  uint32_t level = that.cd_name_.level_;
//  if(Util::object_id.find(level) != Util::object_id.end()) {
//    object_id_ = Util::object_id[level];
//  }
//  else {
//    Util::object_id[level] = 0;
//    object_id_ = 0;
//  }
  object_id_ = 0;
}

uint32_t CDID::level(void)         const { return cd_name_.level(); }
uint32_t CDID::rank_in_level(void) const { return cd_name_.rank_in_level(); }
uint32_t CDID::sibling_count(void) const { return cd_name_.size(); }
ColorT   CDID::color(void)         const { return node_id_.color(); }
int      CDID::task_in_color(void) const { return node_id_.task_in_color(); }
int      CDID::head(void)          const { return node_id_.head(); }
int      CDID::task_count(void)    const { return node_id_.size(); }
uint32_t CDID::object_id(void)     const { return object_id_; }
uint32_t CDID::sequential_id(void) const { return sequential_id_; }
uint32_t CDID::domain_id(void)     const { return domain_id_; }
CDNameT  CDID::cd_name(void)       const { return cd_name_; }
NodeID   CDID::node_id(void)       const { return node_id_; }
bool     CDID::IsHead(void)        const { return node_id_.IsHead(); }

string CDID::GetPhaseID(void) const { return to_string(cd_name_.level_) + string("_") + to_string(object_id_); }
void CDID::SetCDID(const NodeID& node_id)   { node_id_ = node_id; }
void CDID::SetSequentialID(uint32_t seq_id) { sequential_id_ = seq_id; }

CDID &CDID::operator=(const CDID& that)
{
  domain_id_     = that.domain_id();
  sequential_id_ = that.sequential_id();
  cd_name_       = that.cd_name();
  node_id_       = that.node_id();
  object_id_     = that.object_id();
  return *this;
}


bool CDID::operator==(const CDID& that) const
{
  return (node_id_ == that.node_id()) && (cd_name_ == that.cd_name()) &&  (sequential_id_ == that.sequential_id());
}


ostream& operator<<(ostream& str, const CDID& cd_id)
{
  return str<< "Level: "<< cd_id.level() << ", CDNode" << cd_id.color() << ", Obj# " << cd_id.object_id() << ", Seq# " << cd_id.sequential_id();
}


#if _MPI_VER

uint32_t CDID::GenMsgTag(ENTRY_TAG_T tag)
{
  // MAX tag value is 2^31-1
  return ((uint32_t)(tag & TAG_MASK(max_tag_bit)));
}

uint32_t CDID::GenMsgTagForSameCD(int msg_type, int task_in_color)
{
  // MSG TYPE  : 4  (2 bit)
  // MAX level : 64 (6 bit)
  // MAX number of CDs in a level : 4096 (12 bit)
  // MAX number of tasks in a CD  : 4096 (12 bit)
  uint32_t level         = cd_name_.level();
  uint32_t rank_in_level = cd_name_.rank_in_level();
  uint32_t tag = ((level << max_tag_rank_bit) | (rank_in_level << max_tag_task_bit) | (task_in_color));
  tag &= TAG_MASK(max_tag_bit);
  tag |= (1<<(max_tag_bit-1));
  return tag;
}
#endif

//SZ: print function
void CDID::Print(void)
{
  CD_DEBUG("CDID information:\n");
  CD_DEBUG("    domain_id_: %ld\n"     , (unsigned long)domain_id_);
  CD_DEBUG("    object_id_: %ld\n"     , (unsigned long)object_id_);
  CD_DEBUG("    sequential_id_: %ld\n" , (unsigned long)sequential_id_);

  CD_DEBUG("    level: %ld\n"          , (unsigned long)level());
  CD_DEBUG("    rank_in_level: %ld\n"  , (unsigned long)rank_in_level());
  CD_DEBUG("    sibling_count: %ld\n"  , (unsigned long)sibling_count());

  CD_DEBUG("    task_count: %d\n"      , task_count());
  CD_DEBUG("    task_in_color: %d\n"   , task_in_color());
  CD_DEBUG("    head: %d\n"            , head());
  CD_DEBUG("    IsHead: %d\n"          , IsHead());
}

string CDID::GetString(void) const 
{
  return ( string("CDName : ") + cd_name_.GetString() 
         + string(", Node ID : ") + node_id_.GetString() 
         + string(", Obj#") + to_string(object_id_) 
         + string(", Seq#") + to_string(sequential_id_) );
}
