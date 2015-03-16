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

/**
 * @file cd_id.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * \brief CD ID
 *
 * Description on node id
 * CDID is for naming uniquely one of the CDs. 
 * So, it is a logical ID of our CD semantic which is independent on any other execution environment.
 * We can represent a CD with level and rank_in_a_level in the CD tree graph. 
 * So, these two numbers are needed, but they are not sufficient. 
 * One CD may create non-collectively, and just create one child CD, 
 * it does not reflect this new CD with this level and rank_in_level. 
 * To differentiate it, we need object_id which is unique ID in local address space. 
 * This local address space’s ID can be global with the combination of level+rank_in_a_level.
 * But it is still not sufficient, because we are reusing CD object by Begin/Complete. 
 * So in this case, we need sequential_id which is corresponding to the version number of a CD object.
 * We may need domain_id which is about physical location of the CDs, 
 * but we do not work on it, so do not use this for now.
 * it does not contain any information to access one of the processes (mpi ranks) which are belonging to one CD. 
 * So, we need this information to access a CD. 
 * Color is a kind of group number or an arbitrary type indicating a group of tasks corresponding to a CD.
 * task_in_a_color means an ID to access a task in that color. 
 * (ex. rank ID of a communicator in MPI context. Communicator corresponds to color_). 
 * size means the number of tasks in that color. With these three information, we can access to any tasks.
 * 
 * Q. # of sibing CDs should be here?
 *
 */
#include "cd_global.h"
#include "node_id.h"
#include "cd_name_t.h"

using namespace cd;

namespace cd{

  /** \addtogroup cd_defs CD-Related Definitions
   *
   * The \ref cd_defs module includes general CD-related type
   * definitions that don't fall into other type definitions (\ref
   * error_reporting and \ref preservation_funcs).
   *
   *@{
   *
   * @brief A type to uniquely identify a task in CD hierarchy.
   *
   * A CD name consists of its level in the CD tree (root=0)and the
   * its ID, or sequence number, within that level. 
   * 
   * \note Alternatives:
   * - The alternative of simply a unique name misses the idea of
   * levels in the tree; the idea of hierarchy is central to CDs so
   * this is a bad alternative.
   * - The alternative of naming a CD by its entire "branch" leads
   * to requiring the name to be parsed to identify the level; the
   * level is typically the most crucial information so this seems
   * unduly complex. 
   * - A third alternative is to store the branch information as a
   * std::vector, so the vector's length is the level. However, this
   * means the name is rather long.
   * 
   */ 
class CDID {
  public:
    uint32_t domain_id_; //!< Some physical information is desired in CDID to maximize locality when needed.

    CDNameT  cd_name_; //!< CD name

    NodeID   node_id_; //!< Unique ID for each CD. It can be a communicator number. It increases at Create().
                       //!< node_id_.color_ means a CD (color)
                       //!< node_id_.task_in_color_ means task ID in that CD task group (color)
                       //!< For now, node_id_.color_==0 is always Head.
                       //!< But it can be more nicely managed distribuing Head for one process.

    uint32_t object_id_; //!< This will be local and unique within a process. It increases at creator or Create().

    uint32_t sequential_id_; //!< Number of Begin/Complete pairs of each CD object. It increases at Begin() 
  public:
    // TODO Initialize member values to zero or something, for now I will put just zero but this is less efficient.
    CDID(void);
    CDID(const CDNameT& cd_name, const NodeID& new_node_id);
    CDID(const CDID& that);
    ~CDID(void) {}

    CDID &operator=(const CDID& that);
    bool operator==(const CDID& that) const;
   

    uint32_t level(void)         const;
    uint32_t rank_in_level(void) const;
    uint32_t sibling_count(void) const;
    ColorT   color(void)         const;
    int      task_in_color(void) const;
    int      head(void)          const;
    int      task_count(void)    const;
    uint32_t object_id(void)     const;
    uint32_t sequential_id(void) const;
    uint32_t domain_id(void)     const;
    CDNameT  cd_name(void)       const;
    NodeID   node_id(void)       const;
    bool     IsHead(void)        const;

    void SetCDID(const NodeID& new_node_id);
    void SetSequentialID(uint32_t seq_id);

    uint32_t GenMsgTag(ENTRY_TAG_T tag);
    uint32_t GenMsgTagForSameCD(int msg_type, int task_in_color);
 
   //SZ: print function
    void Print(void);
};

std::ostream& operator<<(std::ostream& str, const CDID& cd_id);

/** @} */ // End group cd_defs

} // namespace cd ends

#endif 
