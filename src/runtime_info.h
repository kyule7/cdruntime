#ifndef _RUNTIME_INFO
#define _RUNTIME_INFO
#include <cstdint>
#include <map>
#include <string>
#include "cd_def_interface.h"
//class common::RuntimeInfo;
namespace common {

struct CDOverhead {
  double prv_elapsed_time_;
  double rst_elapsed_time_;
  double create_elapsed_time_;
  double destroy_elapsed_time_;
  double begin_elapsed_time_;
  double compl_elapsed_time_;
  double advance_elapsed_time_;
  CDOverhead(void) 
    : prv_elapsed_time_(0.0),
      rst_elapsed_time_(0.0),
      create_elapsed_time_(0.0),
      destroy_elapsed_time_(0.0),
      begin_elapsed_time_(0.0),
      compl_elapsed_time_(0.0),
      advance_elapsed_time_(0.0)
  {}
  void CopyCDOverhead(const CDOverhead &record) {
    prv_elapsed_time_     = record.prv_elapsed_time_; 
    rst_elapsed_time_     = record.rst_elapsed_time_; 
    create_elapsed_time_  = record.create_elapsed_time_;  
    destroy_elapsed_time_ = record.destroy_elapsed_time_; 
    begin_elapsed_time_   = record.begin_elapsed_time_;   
    compl_elapsed_time_   = record.compl_elapsed_time_;   
    advance_elapsed_time_ = record.advance_elapsed_time_;   
  }
  void MergeCDOverhead(const CDOverhead &info_per_level) {
    prv_elapsed_time_     += info_per_level.prv_elapsed_time_; 
    rst_elapsed_time_     += info_per_level.rst_elapsed_time_; 
    create_elapsed_time_  += info_per_level.create_elapsed_time_;  
    destroy_elapsed_time_ += info_per_level.destroy_elapsed_time_; 
    begin_elapsed_time_   += info_per_level.begin_elapsed_time_;   
    compl_elapsed_time_   += info_per_level.compl_elapsed_time_;   
    advance_elapsed_time_ += info_per_level.advance_elapsed_time_;   
  }
  CDOverhead(const CDOverhead &record) {
    CopyCDOverhead(record);
  }
  std::string GetOverheadStr(void);
  virtual std::string GetString(void) { return GetOverheadStr(); }
  virtual void Print(void);
  CDOverhead &operator+=(const CDOverhead &record) {
    MergeCDOverhead(record);
    return *this;
  }
  CDOverhead &operator=(const CDOverhead &record) {
    CopyCDOverhead(record);
    return *this;
  }
};

struct CDOverheadVar : public CDOverhead {
  double prv_elapsed_time_var_;
  double rst_elapsed_time_var_;
  double create_elapsed_time_var_;
  double destroy_elapsed_time_var_;
  double begin_elapsed_time_var_;
  double compl_elapsed_time_var_;
  double advance_elapsed_time_var_;
  CDOverheadVar(void) 
    : CDOverhead(),
      prv_elapsed_time_var_(0.0),
      rst_elapsed_time_var_(0.0),
      create_elapsed_time_var_(0.0),
      destroy_elapsed_time_var_(0.0),
      begin_elapsed_time_var_(0.0),
      compl_elapsed_time_var_(0.0),
      advance_elapsed_time_var_(0.0)
  {}
  std::string GetOverheadVarStr(void);
  void PrintInfo(void);
};

struct RuntimeInfo : public CDOverhead {
  uint64_t total_exec_;
  uint64_t reexec_;
  uint64_t prv_copy_;
//  static std::map<std::string, std::string, uint64_t> per_entry_vol;
  uint64_t prv_ref_;
  uint64_t restore_;
  uint64_t msg_logging_;
  uint64_t sys_err_vec_;
  double total_time_;
  double reexec_time_;
  double sync_time_;
  CD_CLOCK_T begin_;
  RuntimeInfo(void) 
    : CDOverhead(),
      total_exec_(0), reexec_(0), prv_copy_(0), prv_ref_(0), msg_logging_(0), sys_err_vec_(0),
      total_time_(0.0), reexec_time_(0.0), sync_time_(0.0)
  {}
  RuntimeInfo(const uint64_t &total_exec) 
    : CDOverhead(),
      total_exec_(total_exec), reexec_(0), prv_copy_(0), prv_ref_(0), msg_logging_(0), sys_err_vec_(0),
      total_time_(0.0), reexec_time_(0.0), sync_time_(0.0)
  {}
  void copy(const RuntimeInfo &record) {
    total_exec_  = record.total_exec_;
    reexec_      = record.reexec_;
    prv_copy_    = record.prv_copy_;
    prv_ref_     = record.prv_ref_;
    msg_logging_ = record.msg_logging_;
    sys_err_vec_ = record.sys_err_vec_;
    total_time_  = record.total_time_;
    reexec_time_ = record.reexec_time_;
    sync_time_   = record.sync_time_;
    CopyCDOverhead(record);
//    prv_elapsed_time_     = record.prv_elapsed_time_; 
//    create_elapsed_time_  = record.create_elapsed_time_;  
//    destroy_elapsed_time_ = record.destroy_elapsed_time_; 
//    begin_elapsed_time_   = record.begin_elapsed_time_;   
//    compl_elapsed_time_   = record.compl_elapsed_time_;   
//    advance_elapsed_time_   = record.advance_elapsed_time_;   
  }
  RuntimeInfo(const RuntimeInfo &record) : CDOverhead() {
    copy(record);
  }
  virtual std::string GetString(void);
  std::string GetRTInfoStr(int cnt=0);
  virtual void Print(void);
  RuntimeInfo &operator+=(const RuntimeInfo &record) {
    MergeInfoPerLevel(record);
    return *this;
  }
  RuntimeInfo &operator=(const RuntimeInfo &record) {
    copy(record);
    return *this;
  }
  void MergeInfoPerLevel(const RuntimeInfo &info_per_level) {
    total_exec_  += info_per_level.total_exec_;
    reexec_      += info_per_level.reexec_;
    prv_copy_    += info_per_level.prv_copy_;
    prv_ref_     += info_per_level.prv_ref_;
    msg_logging_ += info_per_level.msg_logging_;
    sys_err_vec_ |= info_per_level.sys_err_vec_;
    MergeCDOverhead(info_per_level);
//    prv_elapsed_time_     += info_per_level.prv_elapsed_time_; 
//    create_elapsed_time_  += info_per_level.create_elapsed_time_;  
//    destroy_elapsed_time_ += info_per_level.destroy_elapsed_time_; 
//    begin_elapsed_time_   += info_per_level.begin_elapsed_time_;   
//    compl_elapsed_time_   += info_per_level.compl_elapsed_time_;   
//    advance_elapsed_time_   += info_per_level.advance_elapsed_time_;   
  }
};

typedef std::map<uint32_t, RuntimeInfo *> ProfMapType;
extern ProfMapType profMap;
} // namespace common ends
#endif
