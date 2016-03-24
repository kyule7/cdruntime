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

#include "cd_config.h"
#include "cd_features.h"

#if CD_PROFILER_ENABLED

//#include "cd_path.h"
//#include "cd_global.h"
#include "profiler_interface.h"
#include "cd_def_internal.h" 

//#include "cd_internal.h"
#include "cd_global.h"
//#include "cd_handle.h"
using namespace cd;
using namespace cd::interface;
using namespace std;

CD_CLOCK_T cd::prof_begin_clk;
CD_CLOCK_T cd::prof_end_clk;
CD_CLOCK_T cd::prof_sync_clk;
std::map<uint32_t,std::map<std::string,RuntimeInfo>> Profiler::num_exec_map;
uint32_t Profiler::current_level_ = 0; // It is used to detect escalation

Profiler *Profiler::CreateProfiler(int prof_type, void *arg)
{
  switch(prof_type) {
    case 0 :
      return new Profiler(static_cast<CDHandle *>(arg));
//    case CDPROFILER : 
//      return dynamic_cast<Profiler *>(new CDProfiler(static_cast<CDProfiler *>(arg)));
    default :
      ERROR_MESSAGE("Undifined Profiler Type!\n");
  }
  return NULL;

}

void Profiler::CreateRuntimeInfo(uint32_t level, const std::string &name) 
{
  auto rit = num_exec_map[level].find(name);
  if(rit == num_exec_map[level].end()) { 
    num_exec_map[level][name] = RuntimeInfo(0);
  }
}

void Profiler::BeginRecord(void)
{
  const uint32_t level = cdh_->level();
  string name = cdh_->GetLabel();
  bool is_escalated = false;
  bool is_reexecuted = (cdh_->GetExecMode() == kReexecution);
  if(current_level_ == level) { 
    // If normal execution, Sequential CDs
    // If failure, Reexecution
  } else if(current_level_ < level) { 
    // If normal execution, Nested CDs
    // If failure, unexpected situation (mapping error)
  } else if(current_level_ > level) {
    // If normal execution, unexpected situation
    // If failure, escalation
    is_escalated = true;
  }
  current_level_ = level;
  auto rit = num_exec_map[level].find(name);

  if(rit == num_exec_map[level].end()) { 
    num_exec_map[level][name] = RuntimeInfo(1);
  } else {
//    printf("Exec %s %s\n", cdh_->GetName(), name.c_str());
    num_exec_map[level][name].total_exec_ += 1;
  }

//  if(cdh_->recreated() || is_reexecuted) {
//    printf("RecreatedReexec %s %s\n", GetName().c_str(), name.c_str());
//    num_exec_map[level][name].reexec_ += 1;
//  }

  if( is_reexecuted || ((cdh_->recreated() == true)) ) {
    if(myTaskID == 0) printf("%sRe-exec %s %s (%d %d %d)\n",string(cdh_->level(), '\t').c_str(),  cdh_->GetName(), 
        name.c_str(), cdh_->GetCDType(), cdh_->GetCDLoggingMode(), cdh_->GetCommLogMode());
//    sync_clk_ = CD_CLOCK();
//    num_exec_map[level][name].reexec_ += 1;
//    num_exec_map[level][name].total_time_ += (double)(end_clk_ - begin_clk_) / CLK_NORMALIZER;
    if(is_reexecuted)
      num_exec_map[level][name].sync_time_  += (double)(CD_CLOCK() - prof_sync_clk) / CLK_NORMALIZER;
    reexecuted_ = true;
  }
  else {

    if(myTaskID == 0) printf("%sBegin Exec %s %s (%d %d %d)\n", string(cdh_->level(), '\t').c_str(), cdh_->GetName(), 
        name.c_str(), cdh_->GetCDType(), cdh_->GetCDLoggingMode(), cdh_->GetCommLogMode());
  } 
//  else if(cdh_->recreated()) {
//
//    printf("RecreatedReexec %s %s\n", cdh_->GetName(), name.c_str());
//    num_exec_map[level][name].reexec_ += 1;
//
//    reexecuted_ = true;
//  }
  begin_clk_ = CD_CLOCK();
}

void Profiler::EndRecord(void)
{
  const uint32_t level = cdh_->level();
  const string name = cdh_->GetLabel();
//  if(current_level_ != level)
//    printf("\n\nSomething is wrong : %s\n\n", name.c_str());

  end_clk_ = CD_CLOCK();
  //sync_clk_ = end_clk_;
  num_exec_map[level][name].total_time_ += (double)(end_clk_ - begin_clk_) / CLK_NORMALIZER;

  if(reexecuted_ || cdh_->recreated()) {
    if(myTaskID == 0) 
      printf("%sEnd Rexec %s %s (%d %d %d)\n", string(cdh_->level(), '\t').c_str(), cdh_->GetName(), name.c_str(), 
          cdh_->GetCDType(), cdh_->GetCDLoggingMode(), cdh_->GetCommLogMode());
    num_exec_map[level][name].total_time_ += (double)(end_clk_ - begin_clk_) / CLK_NORMALIZER;
    num_exec_map[level][name].reexec_time_ += (double)(end_clk_ - begin_clk_) / CLK_NORMALIZER;
    num_exec_map[level][name].reexec_ += 1;
    reexecuted_ = false;
  }
  else {
    if(myTaskID == 0) 
      printf("%sEnd Exec %s %s (%d %d %d)\n", string(cdh_->level(), '\t').c_str(), cdh_->GetName(), name.c_str(), 
          cdh_->GetCDType(), cdh_->GetCDLoggingMode(), cdh_->GetCommLogMode());
  } 
//  else if(cdh_->recreated()) {
//    num_exec_map[level][name].total_time_ += (double)(end_clk_ - begin_clk_) / CLK_NORMALIZER;
//    reexecuted_ = false;
//  }
}

void Profiler::RecordProfile(ProfileType profile_type, uint64_t profile_data)
{
  const uint32_t level = cdh_->level();
  string name = cdh_->GetLabel();
  switch(profile_type) {
    case PRV_COPY_DATA: {
      num_exec_map[level][name].prv_copy_ = profile_data;
      break;
    }
    case PRV_REF_DATA : {
      num_exec_map[level][name].prv_ref_ = profile_data;
      break;
    }
    case MSG_LOGGING : {
      num_exec_map[level][name].msg_logging_ = profile_data;
      break;
    }
    default:
      ERROR_MESSAGE("Invalid profile type to record : %d\n", profile_type);
  }
}

string RuntimeInfo::GetString(void)
{
  char stringout[512];
  snprintf(stringout, 512, 
    "# Execution:\t%lu\n# Reexecution:\t%lu\nTotal Execution Time:\t%lf[s]\nReexecution Time:\t%lf[s]\nSync Time (CDs):\t%lf[s]\nPreservation(Total):\t%lu[B]\nPreservation(Ref):\t%lu[B]\nComm Logging:\t%lu[B]\nError Vector:\t%lx\n-- CD Overhead -----------\nPreservation:\t%lf[s]\nCreate CD:\t%lf[s]\nDestroy CD:\t%lf[s]\nBegin CD:\t%lf[s]\nComplete CD:\t%lf[s]\n", 
                         total_exec_,
                         reexec_,
                         total_time_,
                         reexec_time_,
                         sync_time_,
                         prv_copy_,
                         prv_ref_,
                         msg_logging_,
                         sys_err_vec_,
                         prv_elapsed_time_, 
                         create_elapsed_time_,
                         destroy_elapsed_time_,
                         begin_elapsed_time_,
                         compl_elapsed_time_ 
                        );
  return string(stringout);
}

void RuntimeInfo::Print(void)
{
  printf("%s", GetString().c_str());
}

string CDOverhead::GetString(void)
{
  char stringout[512];
  snprintf(stringout, 512, 
    "Preservation:%lf[s]\nCreate CD   :%lf[s]\nDestroy CD  :%lf[s]\nBegin CD    :%lf[s]\nComplete CD :%lf[s]\n", 
                         prv_elapsed_time_, 
                         create_elapsed_time_,
                         destroy_elapsed_time_,
                         begin_elapsed_time_,
                         compl_elapsed_time_ 
                        );
  return string(stringout);
}

void CDOverhead::Print(void)
{
  printf("%s", GetString().c_str());
}

string CDOverheadVar::GetStringInfo(void)
{
  char stringout[512];
  snprintf(stringout, 512, 
    "Preservation:%lf[s] (var:%lf)\nCreate CD   :%lf[s] (var:%lf)\nDestroy CD  :%lf[s] (var:%lf)\nBegin CD    :%lf[s] (var:%lf)\nComplete CD :%lf[s] (var:%lf)\n", 
                         prv_elapsed_time_, 
                         prv_elapsed_time_var_, 
                         create_elapsed_time_,
                         create_elapsed_time_var_,
                         destroy_elapsed_time_,
                         destroy_elapsed_time_var_,
                         begin_elapsed_time_,
                         begin_elapsed_time_var_,
                         compl_elapsed_time_,
                         compl_elapsed_time_var_ 
                        );
  return string(stringout);
}

void CDOverheadVar::PrintInfo(void)
{
  printf("%s", GetString().c_str());
}

void Profiler::Print(void) 
{
  for(auto it=num_exec_map.begin(); it!=num_exec_map.end(); ++it) {
    CD_DEBUG("Level %u --------------------------------\n", it->first);
    for(auto jt=it->second.begin(); jt!=it->second.end(); ++jt) {
      CD_DEBUG("\n[%s]\n%s\n", jt->first.c_str(), jt->second.GetString().c_str());
    }
    CD_DEBUG("\n");
  }
  CD_DEBUG("-----------------------------------------\n");
}


RuntimeInfo Profiler::GetTotalInfo(std::map<uint32_t, RuntimeInfo> &runtime_info) 
{
  RuntimeInfo info_total;
  for(auto it=num_exec_map.begin(); it!=num_exec_map.end(); ++it) {
    RuntimeInfo info_per_level;
    CD_DEBUG("\n-- Level %u --------------------------------\n", it->first);
    //if(myTaskID == 0)
//      printf("-- Level %u --------------------------------\n", it->first);
    for(auto jt=it->second.begin(); jt!=it->second.end(); ++jt) { //map<string,RuntimeInfo>>
      CD_DEBUG("\n%s : %s\n", jt->first.c_str(), jt->second.GetString().c_str());
      info_per_level += jt->second;
    }
    CD_DEBUG("-- Summary --\n");
    CD_DEBUG("%s\n", info_per_level.GetString().c_str());
//    printf("%s\n", info_per_level.GetString().c_str());
    
//    printf("%s", info_per_level.GetString().c_str());
    info_total.MergeInfoPerLevel(info_per_level);
    runtime_info[it->first] = info_per_level;
  }
  CD_DEBUG("-----------------------------------------\n");
  CD_DEBUG("Total Summary ---------------------------\n");
  CD_DEBUG("%s", info_total.GetString().c_str());
  CD_DEBUG("-----------------------------------------\n");
  return info_total;
}

#endif // profiler enabled
