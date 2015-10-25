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

#include "cd_features.h"
#if CD_PROFILER_ENABLED

#include <time.h>
#include "profiler_interface.h"
//#include "cd_internal.h"
#include "cd_global.h"
//#include "cd_def_internal.h"
#include "sight.h"

//using namespace cd;
using namespace std;
using namespace sight;
using namespace sight::structure;

namespace cd {

class Viz;
class Attribute;
class Comparison;
class Module;
class HierGraph;
class Scope;

class CDProfiler : public Profiler {
  /// sight-related member data
//  static std::list<Viz*> vizStack_;
#if _ENABLE_ATTR
 Attribute *attr_;
#endif

#if _ENABLE_COMP
  Comparison *comp_;  
#endif

#if _ENABLE_MODULE
  Module *module_;
#endif

#if _ENABLE_HIERGRAPH
  HierGraph *hierGraph_;
#endif

#if _ENABLE_SCOPE
  Scope *scope_;
#endif

  uint64_t  sibling_id_;
  uint64_t  level_;

  /// Profile-related meta data
//  std::map<std::string, uint64_t*> profile_data_;
  bool is_child_destroyed;
  bool collect_profile_;
  bool usr_profile_enable;
  bool isStarted;
//  std::vector<std::pair<std::string, long>>  usr_profile;
  int sightObj_count_;
  std::map<sight::structure::dbgStream*, int> sightObj_mark_;

  /// Timer-related meta data
  uint64_t this_point_;
  uint64_t that_point_;

  clock_t this_time_;
  clock_t that_time_;

  CDProfiler *parent_;

public:
  LabelT current_label_;
  std::map<std::string, array<uint64_t,MAX_PROFILE_DATA>> profile_data_;
  static std::map<std::string, bool> onOff_;
  CDProfiler(CDProfiler *prv_cdp) : parent_(prv_cdp) 
  {
    sibling_id_ = 0;
    level_ = 0;
    this_point_ = 0;
    that_point_ = 0;
    collect_profile_ = false;
    is_child_destroyed = false;
    isStarted = false;
    //profile_data_(MAX_PROFILE_DATA)
    
    current_label_ = "INITIAL_PROFILER_LABEL";
    profile_data_.clear();
  }
  ~CDProfiler() {}
  void InitProfile(std::string label="INITIAL_PROFILER_LABEL");
//  void GetProfile(const char *label);
  void RecordProfile(ProfileType profile_type, uint64_t profile_data);
  void RecordClockBegin();
  void RecordClockEnd();
  void StartProfile(const string &label);
  void FinishProfile(void);
  void InitViz(void);
  void FinalizeViz(void);
  LabelT label() { return current_label_; }
  void ClearSightObj(void);
  void CreateSightObj(void);
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
  friend class CDProfiler;
protected:
  CDProfiler *profiler_;
public:
  Viz(CDProfiler *profiler) : profiler_(profiler)
  {
//    std::cout << "Viz is called" << std::endl; //getchar();

  }
  virtual ~Viz(void) {
//    std::cout << "~Viz is called" << std::endl;
  }
  virtual sightObj *GetSightObj()=0;
};

#if _ENABLE_MODULE
class Module : public Viz {
  friend class CDProfiler;
  /// All modules that are currently live
  compModule *m;
  vector<port> externalOutputs;
  context usr_profile_input;
  context usr_profile_output;
  bool usr_profile_enable;
public:
  Module(CDProfiler *profiler, bool usr_profile_en=false);
  ~Module(void);
  void SetUsrProfileInput(std::pair<std::string, long> name_list);
  void SetUsrProfileInput(std::initializer_list<std::pair<std::string, long>> name_list);
  void SetUsrProfileOutput(std::pair<std::string, long> name_list);
  void SetUsrProfileOutput(std::initializer_list<std::pair<std::string, long>> name_list);
  void AddUsrProfile(std::string key, long val, int mode);
  sightObj *GetSightObj() { return m; }
};
#endif

#if _ENABLE_HIERGRAPH
class HierGraph : public Viz {
  friend class CDProfiler;
public:
  static flowgraph *fg;
public:
  HierGraph(CDProfiler *profiler);
  ~HierGraph(void);
  sightObj *GetSightObj() { return fg; }
};
#endif

#if _ENABLE_ATTR
class Attribute : public Viz {
  friend class CDProfiler;
  static attr   *attrInitVal;
  static attrIf *attrScope;
  
  attrAnd *attrKey;
  attr    *attrVal;
public:
  Attribute(CDProfiler *profiler);
  ~Attribute(void);
  sightObj *GetSightObj() { return attrVal; }
};
#endif

#if _ENABLE_COMP
class Comparison : public Viz {
  friend class CDProfiler;
  comparison *compTag;
//  static std::vector<int> compKeyStack;
//  static std::list<comparison*> compStack;
public:
  Comparison(CDProfiler *profiler);
  ~Comparison(void);
  sightObj *GetSightObj() { return compTag; }
};
#endif

#if _ENABLE_SCOPE
class Scope : public Viz {
  friend class CDProfiler;
public:
  /// All scopes that are currently live
  static graph* scopeGraph;
  scope *prv_s;
  scope *s;
public:
  Scope(Scope *parent_scope, CDProfiler *profiler);
  ~Scope(void);
  sightObj *GetSightObj() { return s; }
};


#endif



} // namespace cd ends
#endif
#endif
