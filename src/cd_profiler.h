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

#ifndef _CD_PROFILER_H 
#define _CD_PROFILER_H
/**
 * @file cd_profiler.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * \brief Interface classes for profiling
 *
 * \TODO More descriptiong needed.
 *
 */

#if _PROFILER

#include "cd.h"
#include "cd_global.h"
#include "cd_handle.h"
#include <array>
#include <vector>
#include <list>
#include <cstdint>
#include "sight.h"

using namespace cd;
using namespace sight;
using namespace sight::structure;
#define LabelT std::pair<std::string, int>

class Viz;

class Profiler {
public:
  Profiler() {}
  ~Profiler() {}
  virtual void GetProfile(const char *label)=0;
  virtual void CollectProfile(void)=0;
  virtual void GetPrvData(void *data, 
                          uint64_t len_in_bytes,
                          uint32_t preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject * regen_object, 
                          PreserveUseT data_usage){}
  virtual void StartProfile()=0;
  virtual void FinishProfile(void)=0;
  virtual void InitViz(void)=0;
  virtual void FinalizeViz(void)=0;
  virtual LabelT label(void)=0;
  virtual void ClearSightObj(void)=0;
};

class NullProfiler : public Profiler {
public:
  NullProfiler() {}
  ~NullProfiler() {}
  void GetProfile(const char *label) {}
  void CollectProfile(void) {}
  void GetPrvData(void *data, 
                  uint64_t len_in_bytes,
                  uint32_t preserve_mask, 
                  const char *my_name, 
                  const char *ref_name, 
                  uint64_t ref_offset, 
                  const RegenObject * regen_object, 
                  PreserveUseT data_usage) {}
  void StartProfile() {}
  void FinishProfile(void) {}
  void InitViz(void) {}
  void FinalizeViz(void) {}
  LabelT label(void) {return LabelT();}
};

class CDProfiler : public Profiler {
  /// sight-related member data
  static std::list<Viz*> vizStack_;
  LabelT label_;
  uint64_t  sibling_id_;
  uint64_t  level_;

  /// Profile-related meta data
//  std::map<std::string, uint64_t*> profile_data_;
  bool is_child_destroyed;
  bool collect_profile_;
  bool usr_profile_enable;
//  std::vector<std::pair<std::string, long>>  usr_profile;
  int sightObj_count_;
  std::map<sight::structure::dbgStream*, int> sightObj_mark_;
  /// Timer-related meta data
  uint64_t this_point_;
  uint64_t that_point_;

public:
  std::map<std::string, uint64_t*> profile_data_;
  CDProfiler() 
  {
    sibling_id_ = 0;
    level_ = 0;
    this_point_ = 0;
    that_point_ = 0;
    collect_profile_ = false;
    is_child_destroyed = false;
    //profile_data_(MAX_PROFILE_DATA)
    
    label_.first = "INITIAL_LABEL";
    profile_data_.clear();
  }
  ~CDProfiler() {}
  void InitProfile(std::string label="INITIAL_LABEL");
  void GetProfile(const char *label);
  void CollectProfile(void);
  void GetPrvData(void *data, 
                  uint64_t len_in_bytes,
                  CDPreserveT preserve_mask, 
                  const char *my_name, 
                  const char *ref_name, 
                  uint64_t ref_offset, 
                  const RegenObject * regen_object, 
                  PreserveUseT data_usage);
  void StartProfile();
  void FinishProfile(void);
  void InitViz(void);
  void FinalizeViz(void);
  LabelT label() { return label_; }
  void ClearSightObj(void);

private:
  void GetLocalAvg(void);
  // FIXME
  bool CheckCollectProfile(void);
  void SetCollectProfile(bool flag);
};


// The family of Viz class are all wrapper for sight-related widgets.
// There are multiple kinds of widgets in sight which performs differently with its own purpose.
// From the perspective of CDs, it manages every sight objects with an unified interface which is Viz class,
// and when we want a specific visualization with a new widget in sight, we simply inherit Viz class to extend it.
// By default, there are hiergraph (flowgraph) / module / scope / graph / comparison / attribute widgets supported by CDs.
class Viz {
public:
  Viz(void) {
    std::cout << "Viz is called" << std::endl; //getchar();

  }
  virtual ~Viz(void) {
    std::cout << "~Viz is called" << std::endl;
  }
  virtual sightObj *GetSightObj()=0;
};

class Module : public Viz {
  /// All modules that are currently live
  module *m;
  context usr_profile_input;
  context usr_profile_output;
  bool usr_profile_enable;
public:
  Module(bool usr_profile_en=false);
  ~Module(void);
  void SetUsrProfileInput(std::pair<std::string, long> name_list);
  void SetUsrProfileInput(std::initializer_list<std::pair<std::string, long>> name_list);
  void SetUsrProfileOutput(std::pair<std::string, long> name_list);
  void SetUsrProfileOutput(std::initializer_list<std::pair<std::string, long>> name_list);
  void AddUsrProfile(std::string key, long val, int mode);
  sightObj *GetSightObj() { return m; }
};

class HierGraph : public Viz {
public:
  static flowgraph *fg;
public:
  HierGraph(void);
  ~HierGraph(void);
  sightObj *GetSightObj() { return fg; }
};

class Attribute : public Viz {
  static attrIf *attrScope;
  attrAnd *attrKey;
  attr    *attrVal;
public:
  Attribute(void);
  ~Attribute(void);
  sightObj *GetSightObj() { return attrVal; }
};

class Comparison : public Viz {
  comparison *compTag;
//  static std::vector<int> compKeyStack;
//  static std::list<comparison*> compStack;
public:
  Comparison(void);
  ~Comparison(void);
  sightObj *GetSightObj() { return compTag; }
};

class CDNode : public Viz {
  /// All CD Nodes that are currently live
  CDNode *cdViz;
public:
  CDNode(void);
  ~CDNode(void);
};

class Scope : public Viz {
public:
  /// All scopes that are currently live
  scope *s;
public:
  Scope(void);
  ~Scope(void);
  sightObj *GetSightObj() { return s; }
};

class ScopeGraph : public Scope {
public:
  static graph* scopeGraph;
  scope *prv_s;
public:
  ScopeGraph(void);
  ~ScopeGraph(void);
};

#endif
#endif
