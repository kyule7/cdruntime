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

#ifndef _PROFILER_INTERFACE_H 
#define _PROFILER_INTERFACE_H
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
#include "cd_handle.h"
#if CD_PROFILER_ENABLED
//#include "cd_internal.h"
//#include "cd_global.h"
//#include "cd_def_internal.h"
//#include <array>
//#include <vector>
//#include <list>
#include <string>
#include <map>
#include <cstdio>
#include "cd_def_interface.h"
#include "runtime_info.h"

#define LabelT std::string
using namespace common;

namespace cd {
  namespace interface {

    
enum PROFILER_TYPE {
  NULLPROFILER=0,
  CDPROFILER=1
};

class Profiler {
  friend class cd::CDHandle;
  friend class cd::internal::CD;
  friend class cd::internal::HeadCD;
  CDHandle *cdh_;
  bool reexecuted_;
  CD_CLOCK_T begin_clk_;
  CD_CLOCK_T end_clk_;
  CD_CLOCK_T sync_clk_;
  //static std::map<uint32_t,std::map<std::string,RuntimeInfo> > profMap;
//  static ProfMapType profMap;
  static uint32_t current_level_; // It is used to detect escalation
public:
  Profiler(void) : cdh_(NULL), reexecuted_(false) {}
  Profiler(CDHandle *cdh) : cdh_(cdh), reexecuted_(false) {}
  virtual ~Profiler(void) {}
  static Profiler *CreateProfiler(int prof_type=0, void *arg=NULL);
//  static void CreateRuntimeInfo(uint32_t level, const std::string &name);
  static void CreateRuntimeInfo(uint32_t phase);
  static void Print(void);
  static RuntimeInfo GetTotalInfo(std::map<uint32_t, RuntimeInfo> &runtime_info);
  virtual void InitViz(void){}
  virtual void FinalizeViz(void){}
  virtual void StartProfile() { BeginRecord(); }
  virtual void FinishProfile(void) { EndRecord(); }
private:
  void BeginRecord(void);
  void EndRecord(void);
  virtual void RecordProfile(ProfileType profile_type, uint64_t profile_data);
  virtual void RecordClockBegin(){}
  virtual void RecordClockEnd(){}
  virtual void Delete(void){}
  virtual LabelT label(void){ return std::string();}
//  virtual void GetProfile(const char *label)=0;
};

  } // interface ends
} // cd ends
#endif // profiler enabled

#endif
