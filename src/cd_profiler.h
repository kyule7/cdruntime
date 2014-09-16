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

using namespace cd;
using namespace sight;
using namespace sight::structure;


#include "sight.h"
#include "cd_handle.h"
class Profiler {
public:
  Profiler() {}
  ~Profiler() {}
  virtual void GetProfile(void)=0;
  virtual void CollectProfile(void)=0;
  virtual void GetPrvData(void *data, 
                          uint64_t len_in_bytes,
                          uint32_t preserve_mask, 
                          const char *my_name, 
                          const char *ref_name, 
                          uint64_t ref_offset, 
                          const RegenObject * regen_object, 
                          PreserveUseT data_usage)=0;
  virtual void StartProfile(void)=0;
  virtual void FinishProfile(void)=0;
  virtual void InitViz(void)=0;
  virtual void FinalizeViz(void)=0;
};

class NullProfiler : public Profiler {
public:
  NullProfiler() {}
  ~NullProfiler() {}
  void GetProfile(void) {}
  void CollectProfile(void) {}
  void GetPrvData(void *data, 
                  uint64_t len_in_bytes,
                  uint32_t preserve_mask, 
                  const char *my_name, 
                  const char *ref_name, 
                  uint64_t ref_offset, 
                  const RegenObject * regen_object, 
                  PreserveUseT data_usage) {}
  void StartProfile(void) {}
  void FinishProfile(void) {}
  void InitViz(void) {}
  void FinalizeViz(void) {}
};

class CDProfiler : public Profiler {
  /// sight-related member data
  std::vector<Viz*> vizStack_;
  std::pair<std::string, int> label_;
  uint64_t  sibling_id_;
  uint64_t  level_;

  /// Profile-related meta data
  std::map<std::string, std::array<uint64_t, MAX_PROFILE_DATA>> profile_data_;
  bool is_child_destroyed;
  bool collect_profile_;
  bool usr_profile_enable;
//  std::vector<std::pair<std::string, long>>  usr_profile;


  /// Timer-related meta data
  uint64_t this_point_;
  uint64_t that_point_;

public:
  CDProfiler() 
    : sibling_id_(0), level_(0), this_point_(0), that_point_(0), collect_profile_(false), is_child_destroyed(false)
  {
    label_.first = "INITIAL_LABEL";
    profile_data_.clear();
  }
  ~CDProfiler() {}
  void InitProfile(std::string label="INITIAL_LABEL");
  void GetProfile(void);
  void CollectProfile(void);
  void GetPrvData(void *data, 
                  uint64_t len_in_bytes,
                  uint32_t preserve_mask, 
                  const char *my_name, 
                  const char *ref_name, 
                  uint64_t ref_offset, 
                  const RegenObject * regen_object, 
                  PreserveUseT data_usage);
  void StartProfile(void);
  void FinishProfile(void);
  void InitViz(void);
  void FinalizeViz(void);

private:
  void GetLocalAvg(void);
  // FIXME
  bool CheckCollectProfile(void);
  void SetCollectProfile(bool flag);
};

class Viz {
public:
  virtual void Create(void)=0;
  virtual void Destroy(void)=0;
  virtual void Init(void) { }
  virtual void Finalize(void) { }
  
};

class Module : public Viz {
  /// All modules that are currently live
  static std::list<module*> mStack;
  static modularApp*        ma;

  context usr_profile_input;
  context usr_profile_output;
  bool usr_profile_enable;
public:
  Module() : usr_profile_enable(false)
  void Create(void);
  void Destroy(void);
  void Init(void) {
    /// modularApp exists only one, and is created at init stage
    ma = new modularApp("CD Modular App");
  }
  void Finalize(void) {
    assert(ma);
    delete ma;
  }
  void SetUsrProfileInput(std::pair<std::string, long> name_list);
  void SetUsrProfileInput(std::initializer_list<std::pair<std::string, long>> name_list);
  void SetUsrProfileOutput(std::pair<std::string, long> name_list);
  void SetUsrProfileOutput(std::initializer_list<std::pair<std::string, long>> name_list);
  void AddUsrProfile(std::string key, long val, int mode);
};

class HierGraph : public Viz {
  static std::list<hierGraph*> hgStack;
  static hierGraphApp*         hga;

  HG_context usr_profile_input;
  HG_context usr_profile_output;
  bool usr_profile_enable;
public:
  HierGraph() : usr_profile_enable(false)
  void Create(void);
  void Destroy(void);
  void Init(void) {
    /// modularApp exists only one, and is created at init stage
    hga = new hierGraphApp("CD Hierarchical Graph App");
  }
  void Finalize(void) {
    assert(hga);
    delete hga;
  }
};

class Attribute : public Viz {
  static attrIf*             attrScope;
  static std::list<attrAnd*> attrStack;
  static std::list<attr*>    attrValStack;

public:
  void Create(void);
  void Destroy(void);
  void Init(void) {
    attrValStack.push_back(new attr("INIT", 1));
    attrScope = new attrIf(new attrEQ("INIT", 1));
  }
  void Finalize(void) {
    assert(attrScope);
    delete attrScope;
  
    assert(attrValStack.size()>0);
    assert(attrValStack.back() != NULL);
    delete attrValStack.back();
    attrValStack.pop_back();
  }
};

class Comparison : public Viz {

  static std::vector<comparison*> compTagStack;
//  static std::vector<int> compKeyStack;
//  static std::list<comparison*> compStack;
public:
  void Create(void);
  void Destroy(void);
};

class CDNode : public Viz {
  /// All CD Nodes that are currently live
  static std::list<CDNode*> cdStack;
public:
  void Create(void);
  void Destroy(void);
};

class Scope : public Viz {
protected:
  /// All scopes that are currently live
  static std::list<scope*> sStack;
public:
  void Create(void);
  void Destroy(void);
};

class ScopeGraph : public Scope {
  static graph* scopeGraph;
public:
  void Create(void);
  void Destroy(void);
  void Init(void) {
    /// graph exists only one. It is created at init stage
    scopeGraph = new graph();
  }
  void Finalize(void) {
    assert(scopeGraph);
    delete scopeGraph;
  }
};


#endif
