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
//#include "cd_internal.h"
//#include "cd_global.h"
//#include "cd_def_internal.h"
//
 
#include "cd_handle.h"

#include <array>
#include <vector>
#include <list>
#include <cstdint>
#include <string>

using namespace std;
#define LabelT string

namespace cd {
  namespace interface {
enum PROFILER_TYPE {
  NULLPROFILER=0,
  CDPROFILER=1
};

class Profiler {
  friend class CDHandle;
public:
  Profiler() {}
  ~Profiler() {}
//  virtual void GetProfile(const char *label)=0;
  virtual void RecordProfile(ProfileType profile_type, uint64_t profile_data)=0;
  virtual void RecordClockBegin()=0;
  virtual void RecordClockEnd()=0;
  virtual void StartProfile(const string &label)=0;
  virtual void FinishProfile(void)=0;
  virtual void InitViz(void)=0;
  virtual void FinalizeViz(void)=0;
  virtual void ClearSightObj(void)=0;
  virtual LabelT label(void)=0;
};

class NullProfiler : public Profiler {
public:
  NullProfiler() {}
  ~NullProfiler() {}
//  void GetProfile(const char *label) {}
  void RecordProfile(ProfileType profile_type, uint64_t profile_data) {}
  void RecordClockBegin() {}
  void RecordClockEnd() {}
  void StartProfile(const string &label) {}
  void FinishProfile(void) {}
  void InitViz(void) {}
  void FinalizeViz(void) {}
  void ClearSightObj(void) {}
  LabelT label(void) {return LabelT();}
};

Profiler *CreateProfiler(PROFILER_TYPE prof_type, void *arg=NULL);


}
}
#endif
