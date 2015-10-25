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

/**
 * @file node_id.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * \brief Node ID
 * Description on node id
 *
 * In MPI context, one process can be represented by a pair of communicator and MPI rank ID. 
 * But one process can belong to multiple communicators, so one pair of communicator and MPI rank ID do not uniquely represent a single process.
 * 
 * NodeID is a class for representing a task in one CD. Like MPI, one task can be represented by multiple pairs of color and task ID in the color.  
 * Color is a kind of group number or an arbitrary type indicating a group of tasks corresponding to a CD.
 * task_in_a_color means an ID to access a task in that color. 
 * (ex. rank ID of a communicator in MPI context. Communicator corresponds to color_). 
 * size means the number of tasks in that color. With these three information, we can access to any tasks.
 */

#include "cd_global.h"
#include "serializable.h"
//#include "cd_def_internal.h" 
//#include "packer.h"
//#include "unpacker.h"

using std::endl;

namespace cd {
  namespace internal {

/**@addtogroup cd_defs 
 * @{
 */
/**@class NodeID
 * @brief A type to uniquely identify a task in a CD.
 *
 */ 
class NodeID : public Serializable {
  friend class cd::CDHandle;
  friend class DataHandle;
//  friend class CD;
//  friend class HeadCD;

private:

  /** @brief Internal enumerators used for serialization.
   *
   */
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
  CommGroupT task_group_;

public:
  NodeID(void); 
  NodeID(const ColorT &color, int task, int head, int size);
  NodeID(const NodeID &that);
  ~NodeID(void){}

  NodeID &operator=(const NodeID &that);
  bool operator==(const NodeID &that) const;

  ColorT color(void)         const;
  int    task_in_color(void) const;
  int    head(void)          const;
  int    size(void)          const;
  bool   IsHead(void)        const;
  std::string GetString(void) const;
  std::string GetStringID(void) const;
private:
  void set_head(int head);
  void init_node_id(ColorT color, int task_in_color, int head, int size);
  void *Serialize(uint32_t &len_in_bytes);
  void Deserialize(void *object);
};

std::ostream& operator<<(std::ostream& str, const NodeID& node_id);

/** @} */ // End group cd_defs

  } // namespace internal ends
} // namespace cd ends


#endif
