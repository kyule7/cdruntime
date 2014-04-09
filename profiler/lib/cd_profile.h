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

/**
 * @file cd_profile.h
 * @author Kyushick Lee, Jinsuk Chung, Song Zhang, Seong-Lyong Gong, Derong Liu, Mattan Erez
 * @date April 2014
 *
 * @brief Containment Domains Profile and Visualization v0.1 (C++)
 */

#ifndef _CD_PROFILE_H 
#define _CD_PROFILE_H

#include "sight.h"
#include <map>
#include <vector>
#include <array>
//#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <ctime>

/**
 * @mainpage Profilation and Visualization of Containment Domains 
 * 
 * The purpose of this document is to describe the process of profiling 
 * and visualizing CDs runtime behavior using "Sight".
 *
 * @section sect_intro Containment Domains Visualization Overview
 * Containment Domains (CDs) are a new approach 
 * that achieves efficient and scalale resilience in large applications 
 * with a full system approach instead of one-size-fits-all strategy.
 *
 * CDs framework can adapt large-overhead resilience scheme to the application need
 * that requires some information of application.
 *
 * The profiler can help to extract some useful information in the application,
 * and feed it to auto-tuner that decide the most efficient mapping hierarhcy 
 * of CDs in the corresponding application.
 * 
 * The scale of the application is normally huge, 
 * and profiling for the actual execution would be also overhead for time aspect.
 * Therefore, profiling should be preceded by scaling down the application size.
 * 
 * To sum up, there are three phases regarding profiling of CD-enabled applicaiton.
 * (1) Scaling down the application
 * (2) Data extraction at runtime
 * (3) Scaling up the result from profiler
 *
 * \tableofcontents
 *
 * \todo
 *
 * The current status of profiler (updated at 04.04.2014)
 *  [DONE]
 *   + Profiling Loop count (# of Sequential CDs)
 *   + Profiling Execution cycle in the body domain
 *   + Profiling Preserved data volume
 *   ++ Preserved-via-Copy volume
 *   ++ Preserved-via-Reference volume
 *   ++ Overlapped data volume among Preserved-via-Copy volume
 *
 *  [TODO]
 *   + Substitute some string for all the number information of data volume or loop count,
 *     and put those number separately somewhere in output config file
 *   + Involving scaling factor in profiler to scale application to run at ease for programmer
 *   + Profiling detection mechanism from bitvector
 *   + Different Phase support
 *
 *  [TODO associated with Sight]
 *   + Showing Parallel CD figure (the figure in SC12 paper, not tree shape)
 *   + Zoomable functionality in CD hierarchy graph
 *
 * This documentation regarding CD profiler is organized 
 * around the following "modules", 
 * which can also be accessed through the "Modules" tab on the HTML docs.
 *
 * - \ref cd_profiler
 * - \ref rdtsc
 * 
 * \example lulesh.cc
 *
 */


/** \page examples_page Implementation
 *
 * \section sec_examples Implementation
 * \subsection Profiling Mode
 */

/**
 * \brief
 *
 * All user-visible CD API calls which are the same as CD runtime API calls.
 * Internal CD API implementation components are under a separate internal namespace.
 * List of profile for CD information-----------------------------------------
 *  (1) The number of sequential CDs (Loop Count)
 *  (2) Body execution time
 *  (3) Perservation volume
 *      - 
 *  (4) Overlapped perservation volume
 *      - How much of each preservation is overlapped with parent 
 *       (or with previous iterations of a loop assuming “advance” semantics
 *  (5) Errors/failures a CD can handle (bitvector)
 * Future works--------------------------------------------------------------
 *  (6) Preservation execution overlap
 *  (7) Execution time for Preservation via Regeneration
 *  (8) Alternate recovery time
 *  (9) Application memory usage 
 *     -(without preservation, so that we know how much free space there is for preserving in memory)
 *  (10) Detection routine execution time
 *
 */

//#define PROFILE_DATA_NUM 6

using namespace std;
using namespace sight;

namespace cd{

/// Type definition for cd_info variable. 
/// CDTreeType contains every information for profiling (per-CDs(all CDs) information)
/// LevelType contains per-level information
/// CDInfoSet contains per-CD information
//typedef CDInfoSet std::array<uint64_t, PROFILE_DATA_NUM>;
//typedef LevelType std::vector< CDInfoSet* >;
//typedef CDTreeType std::map<uint64_t, std::vector<CDInfoSet*> >;
//using std::array<uint64_t, PROFILE_DATA_NUM> = typename CDInfoSet;
//using std::map<uint64_t, std::vector<CDInfoSet*> > = typename CDTreeType;


//-- Class Declaration ---------------------------------------------------------------

/// enumerators 
enum CDExecutionMode  { kExec =0, kReexec };
enum CDType           { kStrict=0, kRelaxed };
enum CDErrType        { kOK =0, kAlreadyInit, kError };
enum PreserveType     { kCopy =0b001, kRef =0b010, kRegen =0b100 };
enum DataType         { kMemory=0, kFile };
enum SystemErrName    { 
  kErrOK =0, kSoftMem = 0b1, kHardMem = 0b01, kSoftComp = 0b001, 
  kHardResource =0b0001, kSoftComm = 0b00001, kHardComm = 0b000001 
};
enum SystemErrLoc     { 
  kLocOK =0, kIntraCore = 0b1, kCore = 0b01, kProc = 0b001, 
  kNode = 0b0001, kModule = 0b00001, kCabinet = 0b000001, 
  kCabinetGroup =0b0000001, kSystem = 0b00000001 
};
enum CDPGASType       { kShared = 0, kPotentialShared, kTempPrivate, kPrivate };
/// Profile-related enumerator
enum ProfileType      { LOOP_COUNT, EXEC_CYCLE, PRV_COPY_DATA, PRV_REF_DATA, OVERLAPPED_DATA, SYSTEM_BIT_VECTOR, MAX_PROFILE_DATA };
enum ProfileFormat    { PRV, REC, BODY, MAX_FORMAT };

class CD;
class CDHandle;
class CDHandleCore;
class CDEntry;

//-- Class Definition ----------------------------------------------------------------

struct SystemErrT {
  SystemErrLoc  error_name_;
  SystemErrName  error_location_;
  void* error_info_;
};

class CDEvent{
  CDErrType Wait(){
    // STUB
  }
  bool Test(){
    // STUB
  }
  bool Reset(){
    // STUB
  }
};

/** @brief class CD
 *
 */
class CD{
private:
  CDExecutionMode                 cd_execution_mode_;
public:
  uint64_t                        sys_detect_bit_vector_;
  std::map<std::string, CDEntry*> entry_directory_;
  CDType cd_type_;
  CD(void);
  CD(CDType cd_type);
  ~CD();
};


/** @brief class CDEntry
 *
 */
class CDEntry{
  void* data_p_;
  uint64_t len_in_bytes_;
  std::string data_name_;
  PreserveType prvTy_;
public:
  CDEntry(void* data_p, uint64_t len_in_bytes, PreserveType prvTy, const std::string& data_name);
  ~CDEntry(); 
};


/** @brief class CDHandleCore
 *
 */
class CDHandleCore{
public:
  /// CD is actual data container regarding meta data for preservation and restoration
  CD*       ptr_cd_;

  /// CD hierarchy information (level_ is important)
  /// We should think about why we put level_ here.
  CDHandleCore* parent_cd_;
  uint64_t  level_;

  /// CD Identification information
  uint64_t  cd_id_;
  uint64_t  seq_id_;
  uint64_t  task_id_;
  uint64_t  process_id_;  
public:
  CDHandleCore();
  CDHandleCore(CDHandleCore* parent, CDType cd_type, uint64_t level);
 ~CDHandleCore();
  CDHandleCore* Create(CDType cd_type, CDErrType* cd_err_t);  //non-collective 
  CDErrType Destroy();
  CDErrType Begin();
  CDErrType Complete();
  CDErrType Detect();
  CDErrType Preserve (void* data_p, uint64_t len_in_bytes, 
                      PreserveType prvTy, std::string ref_name);
  
  void IncSeqID(){ seq_id_++; };
};


/** @brief class CDHandle
 *
 *
 *
 */
struct CDHandle {
  /// Wrapping CDHandleCore
  CDHandleCore* this_cd_; 

  /// Hierarchy-related meta data
  CDHandle* parent_;
  std::map<uint64_t, CDHandle*> children_;
  uint64_t  sibling_id_;
  uint64_t  level_;

  /// Profile-related meta data
  uint64_t profile_data_[MAX_PROFILE_DATA];
  bool     is_destroyed_;
  
  /// Timer-related meta data
  uint64_t this_point_;
  uint64_t that_point_;

  /// Member Fuction
  CDHandle();
  CDHandle(CDHandle* parent, CDType cd_type, uint64_t level);
 ~CDHandle();
  CDHandle*  Create(CDType cd_type, CDErrType* cd_err_t);  //non-collective 
  CDErrType Destroy();
  CDErrType Begin();
  CDErrType Complete();
  CDErrType Detect();
  CDErrType Preserve(void* data_p, uint64_t len_in_bytes, PreserveType prvTy, std::string ref_name);

  /// Profiler functions to extract useful information in application
  void Print_Profile(std::map<uint64_t, std::map<uint64_t,uint64_t*> >::iterator it, txt* cdstr_buf);
  void Print_Profile();

  /// Profiler functions to generate CD hierarchy graph using "Sight"
  void Print_Graph(CDHandle* current_cdh, graph& cd_graph, scope& cd_node);
  void Print_Graph();

};

//-- Global Function Declaration -----------------------------------------------------
CDHandle* Init(CDErrType cd_err_t, bool collective=true);
void Finalize();
CDHandle* GetRoot();

//-- Profiler-related function declaration -------------------------------------------
bool check_overlap(CDHandle* parent, std::string& ref_name);
//std::string adjust_tab(int lev);
string put_tabs(int tab_count);


};  // namespace cd end


#endif
