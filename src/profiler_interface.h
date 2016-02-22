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
#include <cstdint>
#include <cstdio>

using namespace std;
#define LabelT string

namespace cd {
  namespace interface {

    
enum PROFILER_TYPE {
  NULLPROFILER=0,
  CDPROFILER=1
};


struct RuntimeInfo {
  uint64_t total_exec_;
  uint64_t reexec_;
  uint64_t prv_copy_;
  uint64_t prv_ref_;
  uint64_t msg_logging_;
  uint64_t sys_err_vec_;
  double   total_time_;
  double   reexec_time_;
  double   sync_time_;
  RuntimeInfo(void) 
    : total_exec_(0), reexec_(0), prv_copy_(0), prv_ref_(0), msg_logging_(0), sys_err_vec_(0),
      total_time_(0.0), reexec_time_(0.0), sync_time_(0.0) {}
  RuntimeInfo(const uint64_t &total_exec) 
    : total_exec_(total_exec), reexec_(0), prv_copy_(0), prv_ref_(0), msg_logging_(0), sys_err_vec_(0),
      total_time_(0.0), reexec_time_(0.0), sync_time_(0.0) {}
  RuntimeInfo(const RuntimeInfo &record) {
    total_exec_  = record.total_exec_;
    reexec_      = record.reexec_;
    prv_copy_    = record.prv_copy_;
    prv_ref_     = record.prv_ref_;
    msg_logging_ = record.msg_logging_;
    sys_err_vec_ = record.sys_err_vec_;
    total_time_  = record.total_time_;
    reexec_time_ = record.reexec_time_;
    sync_time_   = record.sync_time_;
  }
  std::string GetString(void);
  RuntimeInfo &operator+=(const RuntimeInfo &record) {
    total_exec_  += record.total_exec_;
    reexec_      += record.reexec_;
    prv_copy_    += record.prv_copy_;
    prv_ref_     += record.prv_ref_;
    msg_logging_ += record.msg_logging_;
    sys_err_vec_ |= record.sys_err_vec_;
    total_time_  += record.total_time_;
    reexec_time_ += record.reexec_time_;
    sync_time_   += record.sync_time_;
    return *this;
  }
  RuntimeInfo &operator=(const RuntimeInfo &record) {
    total_exec_  = record.total_exec_;
    reexec_      = record.reexec_;
    prv_copy_    = record.prv_copy_;
    prv_ref_     = record.prv_ref_;
    msg_logging_ = record.msg_logging_;
    sys_err_vec_ = record.sys_err_vec_;
    total_time_  = record.total_time_;
    reexec_time_ = record.reexec_time_;
    sync_time_   = record.sync_time_;
    return *this;
  }
  void MergeInfoPerLevel(const RuntimeInfo &info_per_level) {
    total_exec_  += info_per_level.total_exec_;
    reexec_      += info_per_level.reexec_;
    prv_copy_    += info_per_level.prv_copy_;
    prv_ref_     += info_per_level.prv_ref_;
    msg_logging_ += info_per_level.msg_logging_;
    sys_err_vec_ |= info_per_level.sys_err_vec_;
  }
};

class Profiler {
  friend class cd::CDHandle;
  CDHandle *cdh_;
  bool reexecuted_;
  clock_t begin_clk_;
  clock_t end_clk_;
  clock_t sync_clk_;
  static std::map<uint32_t,std::map<std::string,RuntimeInfo>> num_exec_map;
  static uint32_t current_level_; // It is used to detect escalation
public:
  Profiler() : cdh_(NULL), reexecuted_(false) {}
  Profiler(CDHandle *cdh) : cdh_(cdh), reexecuted_(false) {}
  virtual ~Profiler() {}
  static Profiler *CreateProfiler(int prof_type=0, void *arg=NULL);
  static void Print(void);
  static RuntimeInfo GetTotalInfo(void);
  virtual void InitViz(void){}
  virtual void FinalizeViz(void){}
  std::map<uint32_t,std::map<std::string,RuntimeInfo>> &GetProfInfo(void) { return Profiler::num_exec_map; }
  virtual void StartProfile() { BeginRecord(); }
  virtual void FinishProfile(void) { EndRecord(); }
private:
  void BeginRecord(void);
  void EndRecord(void);
  virtual void RecordProfile(ProfileType profile_type, uint64_t profile_data);
  virtual void RecordClockBegin(){}
  virtual void RecordClockEnd(){}
  virtual void Delete(void){}
  virtual LabelT label(void){ return string();}
//  virtual void GetProfile(const char *label)=0;
};

//class NullProfiler : public Profiler {
//public:
//  NullProfiler() {}
//  ~NullProfiler() {}
////  void GetProfile(const char *label) {}
//  void RecordProfile(ProfileType profile_type, uint64_t profile_data) {}
//  void RecordClockBegin() {}
//  void RecordClockEnd() {}
//  void StartProfile(const string &label) {}
//  void FinishProfile(void) {}
//  void InitViz(void) {}
//  void FinalizeViz(void) {}
//  void Delete(void) {}
//  LabelT label(void) {return LabelT();}
//};
//
//Profiler *CreateProfiler(PROFILER_TYPE prof_type, void *arg=NULL);


  } // interface ends
} // cd ends
#endif // profiler enabled

#endif
