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

#ifndef _CD_ID_H
#define _CD_ID_H
#include "cd_global.h"
#include "node_id.h"
#include "cd_name_t.h"

using namespace cd;

namespace cd{

//extern uint64_t object_id_;
/*
CDID is for naming uniquely one of the CDs. 
So, it is a logical ID of our CD semantic which is independent on any other execution environment.
We can represent a CD with level and rank_in_a_level in the CD tree graph. 
So, these two numbers are needed, but they are not sufficient. 
One CD may create non-collectively, and just create one child CD, 
it does not reflect this new CD with this level and rank_in_level. 
To differentiate it, we need object_id which is unique ID in local address space. 
This local address space’s ID can be global with the combination of level+rank_in_a_level.
But it is still not sufficient, because we are reusing CD object by Begin/Complete. 
So in this case, we need sequential_id which is corresponding to the version number of a CD object.
We may need domain_id which is about physical location of the CDs, 
but we do not work on it, so do not use this for now.
it does not contain any information to access one of the processes (mpi ranks) which are belonging to one CD. 
So, we need this information to access a CD. 
Color is a kind of group number or an arbitrary type indicating a group of tasks corresponding to a CD.
task_in_a_color means an ID to access a task in that color. 
(ex. rank ID of a communicator in MPI context. Communicator corresponds to color_). 
size means the number of tasks in that color. With these three information, we can access to any tasks.

Q. # of sibing CDs should be here?

*/
class CDID {
  public:
    // Some physical information is desired in CDID to maximize locality when needed
    uint32_t domain_id_; 

    CDNameT  cd_name_;

    // Unique ID for each CD. It can be a communicator number. It increases at Create().
    // node_id_.color_ means a CD (color)
    // node_id_.task_in_color_ means task ID in that CD task group (color)
    // For now, node_id_.color_==0 is always Head.
    // But it can be more nicely managed distribuing Head for one process.
    NodeID   node_id_;

    // This will be local and unique within a process. It increases at creator or Create().
    uint32_t object_id_; // This will be local and unique within a process. It increases at creator or Create().

    // # of Begin/Complete pairs of each CD object. It increases at Begin()
    uint32_t sequential_id_;      

    // TODO Initialize member values to zero or something, for now I will put just zero but this is less efficient.
    CDID(); 
    CDID(const CDNameT& cd_name, const NodeID& new_node_id);

//    CDID(CDNameT&& cd_name, NodeID&& new_node_id);
    CDID(const CDID& that);

    // should be in CDID.h
    // old_cd_id should be passed by value
//    void UpdateCDID(uint64_t parent_level, const NodeID& new_node_id);

    void SetCDID(const NodeID& new_node_id);

    CDID& operator=(const CDID& that);

    //SZ: print function
    void Print ();
#ifdef comm_log
    void SetSequentialID(uint32_t seq_id) {sequential_id_ = seq_id;}
#endif
    bool operator==(const CDID& that) const;
    
    uint32_t level(void)         const { return cd_name_.level(); }
    uint32_t rank_in_level(void) const { return cd_name_.rank_in_level(); }
    uint32_t sibling_count(void) const { return cd_name_.size(); }
    ColorT   color(void)         const { return node_id_.color(); }
    int      task_in_color(void) const { return node_id_.task_in_color(); }
    int      head(void)          const { return node_id_.head(); }
    int      task_count(void)    const { return node_id_.size(); }
    uint32_t object_id(void)     const { return object_id_; }
    uint32_t sequential_id(void) const { return sequential_id_; }
    uint32_t domain_id(void)     const { return domain_id_; }
    CDNameT  cd_name(void)       const { return cd_name_; }
    NodeID   node_id(void)       const { return node_id_; }
    bool     IsHead(void)        const { return node_id_.IsHead(); }

    uint32_t GenMsgTag(uint64_t tag, uint16_t cd_tag) 
    {
      uint32_t msg_tag = cd_tag;
      msg_tag = (1 << 30) | msg_tag; // metadata is 2 bit
      msg_tag = ((tag & 0x3FFF) << 16) | msg_tag; // tag is 14 bit
      return msg_tag;
    }
    uint32_t GenMsgTag(ENTRY_TAG_T tag)
    {
      return ((uint32_t)(tag & 0xFFFFFFFF));
    }

    uint32_t GenMsgTagForSameCD(int msg_type)
    {
      // MAX level : 64 (6 bit)
      // MAX number of CDs in a level : 4096 (12 bit)
      // MAX number of tasks in a CD  : 4096 (12 bit)
      uint32_t level         = cd_name_.level();
      uint32_t rank_in_level = cd_name_.rank_in_level();
      uint32_t task_in_color = node_id_.task_in_color();
      std::cout << "rank in level : "<<rank_in_level << std::endl;
      if(level > 64) assert(0); 
      if(rank_in_level > 4096) assert(0); 
      if(task_in_color > 4096) assert(0); 
      uint32_t tag = ((level << 24) | (rank_in_level << 12) | (task_in_color));
      tag &= 0x3FFFFFFF;
      tag |= msg_type;
      return tag;
    }

    uint16_t CDTag16(void)  { 
      // MAX level : 16 (4 bit)
      // MAX number of CDs in a level : 64 (6 bit)
      // MAX number of tasks in a CD  : 64 (6 bit)
      uint32_t level         = cd_name_.level();
      uint32_t rank_in_level = cd_name_.rank_in_level();
      uint32_t task_in_color = node_id_.task_in_color();
      if(level > 16) assert(0); 
      if(rank_in_level > 64) assert(0); 
      if(task_in_color > 64) assert(0); 
      return ((level << 12) | (rank_in_level << 6) | (task_in_color));
    }

    uint32_t CDTag32(void)  { 
      // MAX level : 64 (6 bit)
      // MAX number of CDs in a level : 4096 (12 bit)
      // MAX number of tasks in a CD  : 4096 (12 bit)
      uint32_t level         = cd_name_.level();
      uint32_t rank_in_level = cd_name_.rank_in_level();
      uint32_t task_in_color = node_id_.task_in_color();
      if(level > 64) assert(0); 
      if(rank_in_level > 4096) assert(0); 
      if(task_in_color > 4096) assert(0); 
      return ((level << 24) | (rank_in_level << 12) | (task_in_color));
    }

    uint64_t CDTag64(void)  { 
      // MAX level : 256 (8 bit)
      // MAX number of CDs in a level : 268435456 (28 bit)
      // MAX number of tasks in a CD  : 268435456 (28 bit)
      uint64_t level         = cd_name_.level();
      uint64_t rank_in_level = cd_name_.rank_in_level();
      uint64_t task_in_color = node_id_.task_in_color();
      if(level > 256) assert(0); 
      if(rank_in_level > 268435456) assert(0); 
      if(task_in_color > 268435456) assert(0); 
      return ((level << 56) | (rank_in_level << 28) | (task_in_color));
    }

    void ReadCDTag64(uint64_t cd_tag, uint32_t &level, uint32_t &rank_in_level, uint32_t &task_in_color) {
      level         = (cd_tag>>56) & 0xFF; // 8bit
      rank_in_level = (cd_tag>>28) & 0xFFFFFFF; // 28bit
      task_in_color = cd_tag       & 0xFFFFFFF; // 28bit
    }

    void ReadCDTag32(uint32_t cd_tag, uint32_t &level, uint32_t &rank_in_level, uint32_t &task_in_color) {
      level         = (cd_tag>>24) & 0x3F; // 6bit
      rank_in_level = (cd_tag>>12) & 0xFFF; // 12bit
      task_in_color = cd_tag       & 0xFFF; // 12bit 
    }

    void ReadCDTag16(uint16_t cd_tag, uint32_t &level, uint32_t &rank_in_level, uint32_t &task_in_color) {
      level         = (cd_tag>>12) & 0xF; // 4bit
      rank_in_level = (cd_tag>>6)  & 0x3F; // 6bit
      task_in_color = cd_tag       & 0x3F; // 6bit
    
    }
    
    void ReadMsgTag(uint32_t msg_tag) {
//      std::cout << "\nReadMsgTag : " << msg_tag << std::endl;
//      std::cout << ((msg_tag >> 30) & 0x3) << std::endl;
//      std::cout << ((msg_tag >> 16) & 0x3FFF) << std::endl;
      uint32_t level16=0;
      uint32_t rank16=0;
      uint32_t task16=0;
      ReadCDTag16(msg_tag & 0xFFFF, level16, rank16, task16);
//      std::cout << "In msgTag read, \n" << "16bit : "<< level16 << " , " << rank16 << " , " << task16 
//                                  << std::endl << std::endl;
    
    }

};

std::ostream& operator<<(std::ostream& str, const CDID& cd_id);

}

#endif 
