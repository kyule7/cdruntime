#ifndef _RUNTIME_INFO
#define _RUNTIME_INFO
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <assert.h>
#include "cd_def_interface.h"
#include "cd_def_common.h"
#include "prof_entry.hpp"
//#define INVALID_NUM -1U
//class common::RuntimeInfo;
namespace common {

// This will generate the same hash as std::string
struct PrvProfEntry : public std::string {
  enum Type {
    kOutput=0x1, // 0 is input
    kRefer =0x2, // 0 is copy
  };
  uint64_t size_;
  uint32_t count_;
  uint32_t type_; // kCopy, kRef, kRegen, kCoop, kSerdes, kSource, kModify, kOutput

  void Update(uint64_t len, uint32_t type) { 
    size_ += len;
    count_++;
    type_ = type;
  }
  double GetVolume(bool is_avg) { 
    double prv_vol = 0.0;
    if(type_ != kRef) {
      if(is_avg)
        prv_vol += (double)size_ / count_;
      else
        prv_vol += (double)size_;
    }
    return prv_vol;
  }
//  PerPrvProfEntryType type_;
};

struct CDOverhead {
  double prv_elapsed_time_;
  double max_prv_elapsed_time_;
  double max_cdrt_elapsed_time_;
  double rst_elapsed_time_;
  double create_elapsed_time_;
  double destroy_elapsed_time_;
  double begin_elapsed_time_;
  double compl_elapsed_time_;
  double advance_elapsed_time_;
  CDOverhead(void) 
    : prv_elapsed_time_(0.0),
      max_prv_elapsed_time_(0.0),
      max_cdrt_elapsed_time_(0.0),
      rst_elapsed_time_(0.0),
      create_elapsed_time_(0.0),
      destroy_elapsed_time_(0.0),
      begin_elapsed_time_(0.0),
      compl_elapsed_time_(0.0),
      advance_elapsed_time_(0.0)
  {}
  void CopyCDOverhead(const CDOverhead &record) {
    prv_elapsed_time_     = record.prv_elapsed_time_; 
    max_prv_elapsed_time_ = record.max_prv_elapsed_time_; 
    max_cdrt_elapsed_time_ = record.max_cdrt_elapsed_time_; 
    rst_elapsed_time_     = record.rst_elapsed_time_; 
    create_elapsed_time_  = record.create_elapsed_time_;  
    destroy_elapsed_time_ = record.destroy_elapsed_time_; 
    begin_elapsed_time_   = record.begin_elapsed_time_;   
    compl_elapsed_time_   = record.compl_elapsed_time_;   
    advance_elapsed_time_ = record.advance_elapsed_time_;   
  }
  void MergeCDOverhead(const CDOverhead &info_per_level) {
    prv_elapsed_time_     += info_per_level.prv_elapsed_time_; 
    max_prv_elapsed_time_ += info_per_level.max_prv_elapsed_time_; 
    max_cdrt_elapsed_time_ += info_per_level.max_cdrt_elapsed_time_; 
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
  double GetCDRTOverhead(double create_time=0.0) {
    double rtovhd= (destroy_elapsed_time_
                  + begin_elapsed_time_
                  + compl_elapsed_time_
                  + advance_elapsed_time_);
    rtovhd = (max_cdrt_elapsed_time_ > rtovhd)? max_cdrt_elapsed_time_ : rtovhd;
    return rtovhd + create_time;
  }
  double GetCDRTOverheadLocal(double create_time=0.0) {
    double rtovhd= (destroy_elapsed_time_
                  + begin_elapsed_time_
                  + compl_elapsed_time_
                  + advance_elapsed_time_);
    return rtovhd + create_time;
  }
  double GetCDRTOverheadAll(void) {
    return ( create_elapsed_time_
          + destroy_elapsed_time_
          + begin_elapsed_time_
          + compl_elapsed_time_
          + advance_elapsed_time_);
  }
  double GetPreserveTime(void) {
    //printf("max prv:%lf (%lf, %lf)\n", max_prv_elapsed_time_, prv_elapsed_time_, rst_elapsed_time_);
    //return (prv_elapsed_time_ + max_prv_elapsed_time_ + rst_elapsed_time_);
    double prv_time = (max_prv_elapsed_time_ > prv_elapsed_time_)? max_prv_elapsed_time_ : prv_elapsed_time_;
    return (prv_time);
    //return (prv_time + rst_elapsed_time_);
  }
  double GetOverheadAll(double parent_create_time) {
//    return (GetPreserveTime()
//          + create_elapsed_time_
//          + destroy_elapsed_time_
//          + begin_elapsed_time_
//          + compl_elapsed_time_
//          + advance_elapsed_time_);
    return (GetPreserveTime() + GetCDRTOverhead(parent_create_time));
  }
};

struct CDOverheadVar : public CDOverhead {
  double prv_elapsed_time_var_;
  double max_prv_elapsed_time_var_;
  double max_cdrt_elapsed_time_var_;
  double rst_elapsed_time_var_;
  double create_elapsed_time_var_;
  double destroy_elapsed_time_var_;
  double begin_elapsed_time_var_;
  double compl_elapsed_time_var_;
  double advance_elapsed_time_var_;
  CDOverheadVar(void) 
    : CDOverhead(),
      prv_elapsed_time_var_(0.0),
      max_prv_elapsed_time_var_(0.0),
      max_cdrt_elapsed_time_var_(0.0),
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

static inline
void MergeMaps(std::map<std::string, PrvProfEntry>& lhs, 
         const std::map<std::string, PrvProfEntry>& rhs) {
  std::map<std::string, PrvProfEntry>::iterator left = lhs.begin();
  std::map<std::string, PrvProfEntry>::const_iterator right = rhs.begin();

  while (left != lhs.end() && right != rhs.end()) {
    if (right->first < left->first) {     // If the rhs value is less than the lhs value, 
      lhs.insert(left, *right);           // then insert it into the lhs map and skip past it.
      ++right;
    }
    else if (right->first == left->first) { // Otherwise, if the values are equal, 
      left->second = right->second;         // overwrite the lhs value and move both
      ++left; ++right;                      // iterators forward. 
    }
    else {   // Otherwise the rhs value is bigger, so skip past the lhs value. 
      ++left;
    }
  }

  // At this point we've exhausted one of the two ranges. 
  // Add what's left of the rhs values to the lhs map, 
  // since we know there are no duplicates there.
  lhs.insert(right, rhs.end());
}


// id is used when access to caches
//  1. profMap
//  2. phaseNodeCache
//  3. profPrvCache
struct RuntimeInfo : public CDOverhead {
  enum {
    kJSON = 1,
    kYAML = 2,
    kPROF = 3,
  };

  uint32_t id_;
  uint32_t exec_cnt_;
  uint32_t reex_cnt_;
  uint64_t prv_copy_;
  std::map<std::string, PrvProfEntry> input_;
  std::map<std::string, PrvProfEntry> output_;
  uint64_t prv_ref_;
  uint64_t restore_;
  uint64_t msg_logging_;
  uint64_t sys_err_vec_;
  double total_time_;
  double reex_time_;
  double sync_time_;
  double prsv_time_smpl_;
  double cdrt_time_smpl_;
  std::vector<float> exec_trace_;
  std::vector<float> prsv_trace_;
  std::vector<float> max_prsv_;
  std::vector<float> max_cdrt_;
  std::vector<float> cdrt_trace_;
  CD_CLOCK_T exec_clk_;
  CD_CLOCK_T reex_clk_;
  RuntimeInfo(uint32_t id) 
    : CDOverhead(), id_(id),
      exec_cnt_(0), reex_cnt_(0), prv_copy_(0), prv_ref_(0), restore_(0), msg_logging_(0), sys_err_vec_(0),
      total_time_(0.0), reex_time_(0.0), sync_time_(0.0), exec_clk_(0), reex_clk_(0), 
      prsv_time_smpl_(0), cdrt_time_smpl_(0)
  {}
//  RuntimeInfo(uonst uint64_t &total_exec) 
//    : CDOverhead(), 
//      exec_(total_exec), reex_cnt_(0), prv_copy_(0), prv_ref_(0), msg_logging_(0), sys_err_vec_(0),
//      total_time_(0.0), reex_time_(0.0), sync_time_(0.0)
//  {}
  void Copy(const RuntimeInfo &record) {
    exec_cnt_    = record.exec_cnt_;
    reex_cnt_    = record.reex_cnt_;
    prv_copy_    = record.prv_copy_;
    prv_ref_     = record.prv_ref_;
    msg_logging_ = record.msg_logging_;
    sys_err_vec_ = record.sys_err_vec_;
    total_time_  = record.total_time_;
    reex_time_   = record.reex_time_;
    sync_time_   = record.sync_time_;
    input_       = record.input_;
    output_      = record.output_;
    CopyCDOverhead(record);
  }
  RuntimeInfo(const RuntimeInfo &record) : CDOverhead() {
    Copy(record);
  }
  virtual std::string GetString(void);
  void GetPrvDetails(std::ostream &oss, const std::string &indent);
  std::string GetRTInfoStr(int cnt=0, int style=kJSON);
  virtual void Print(void);
  RuntimeInfo &operator+=(const RuntimeInfo &record) {
    Merge(record);
    return *this;
  }
  RuntimeInfo &operator=(const RuntimeInfo &record) {
    Copy(record);
    return *this;
  }
  void Merge(const RuntimeInfo &info_per_level) {
    exec_cnt_    += info_per_level.exec_cnt_;
    reex_cnt_    += info_per_level.reex_cnt_;
    prv_copy_    += info_per_level.prv_copy_;
    prv_ref_     += info_per_level.prv_ref_;
    msg_logging_ += info_per_level.msg_logging_;
    sys_err_vec_ |= info_per_level.sys_err_vec_;
    total_time_  += info_per_level.total_time_;
    reex_time_   += info_per_level.reex_time_;
    sync_time_   += info_per_level.sync_time_;
    MergeMaps(input_, info_per_level.input_);
    MergeMaps(output_, info_per_level.output_);
    MergeCDOverhead(info_per_level);
  }

  template <typename T>
  RTInfo<T> GetRTInfo(int myTaskID=0) {
    localTaskID = myTaskID;
    return RTInfo<T>((double)exec_cnt_,
                     (double)reex_cnt_,
                     (double)prv_copy_,
                     (double)prv_ref_,
                     (double)restore_,
                     (double)msg_logging_,
                          total_time_,
                          reex_time_,
                          sync_time_,
                          prv_elapsed_time_,
                          max_prv_elapsed_time_,
                          max_cdrt_elapsed_time_,
                          rst_elapsed_time_,
                          create_elapsed_time_,
                          destroy_elapsed_time_,
                          begin_elapsed_time_,
                          compl_elapsed_time_,
                          advance_elapsed_time_
                    );

  }
  RTInfoInt GetRTInfoInt(void) {
    return RTInfoInt( exec_cnt_,
                      reex_cnt_,
                      prv_copy_,
                      prv_ref_,
                      restore_,
                      msg_logging_
                    );
  }
  RTInfoFloat GetRTInfoFloat(void) {
    return RTInfoFloat( total_time_,
                        reex_time_,
                        sync_time_,
                        prv_elapsed_time_,
                        max_prv_elapsed_time_,
                        max_cdrt_elapsed_time_,
                        rst_elapsed_time_,
                        create_elapsed_time_,
                        destroy_elapsed_time_,
                        begin_elapsed_time_,
                        compl_elapsed_time_,
                        advance_elapsed_time_
            );
  }

  void SetRTInfo(const RTInfo<double> &rt_info) {
    exec_cnt_             = rt_info.exec_cnt_;
    reex_cnt_             = rt_info.reex_cnt_;
    prv_copy_             = rt_info.prv_copy_;
    prv_ref_              = rt_info.prv_ref_;
    restore_              = rt_info.restore_;
    msg_logging_          = rt_info.msg_logging_;
    total_time_           = rt_info.total_time_;           
    reex_time_            = rt_info.reex_time_;
    sync_time_            = rt_info.sync_time_;
    prv_elapsed_time_     = rt_info.prv_elapsed_time_;
    max_prv_elapsed_time_ = rt_info.max_prv_elapsed_time_;
    max_cdrt_elapsed_time_= rt_info.max_cdrt_elapsed_time_;
    rst_elapsed_time_     = rt_info.rst_elapsed_time_;
    create_elapsed_time_  = rt_info.create_elapsed_time_;
    destroy_elapsed_time_ = rt_info.destroy_elapsed_time_;
    begin_elapsed_time_   = rt_info.begin_elapsed_time_;
    compl_elapsed_time_   = rt_info.compl_elapsed_time_;
    advance_elapsed_time_ = rt_info.advance_elapsed_time_;
  }
  void SetRTInfoInt(const RTInfoInt &rt_info) {
    exec_cnt_    = rt_info.exec_cnt_;
    reex_cnt_    = rt_info.reex_cnt_;
    prv_copy_    = rt_info.prv_copy_;
    prv_ref_     = rt_info.prv_ref_;
    restore_     = rt_info.restore_;
    msg_logging_ = rt_info.msg_logging_;
  }
  void SetRTInfoFloat(const RTInfoFloat &rt_info) {
    total_time_           = rt_info.total_time_;           
    reex_time_            = rt_info.reex_time_;
    sync_time_            = rt_info.sync_time_;
    prv_elapsed_time_     = rt_info.prv_elapsed_time_;
    max_prv_elapsed_time_ = rt_info.max_prv_elapsed_time_;
    max_cdrt_elapsed_time_= rt_info.max_cdrt_elapsed_time_;
    rst_elapsed_time_     = rt_info.rst_elapsed_time_;
    create_elapsed_time_  = rt_info.create_elapsed_time_;
    destroy_elapsed_time_ = rt_info.destroy_elapsed_time_;
    begin_elapsed_time_   = rt_info.begin_elapsed_time_;
    compl_elapsed_time_   = rt_info.compl_elapsed_time_;
    advance_elapsed_time_ = rt_info.advance_elapsed_time_;
  }

  double GetPrvVolume(bool is_input=true, bool is_avg=false);

  inline void RecordData(const std::string &entry_str, uint64_t len, uint32_t type, bool is_reexec)
  {
    CD_CLOCK_T now = CD_CLOCK();
    double elapsed = now - cd::begin_clk;
    prsv_time_smpl_      += elapsed;
    if(is_reexec == false) { // normal execution
      cd::prv_elapsed_time += elapsed; 
      cd::prv_elapsed_smpl += elapsed; 
      prv_elapsed_time_    += elapsed;

      if( CHECK_TYPE(type, kCopy) ) {
        prv_copy_ += len;
        input_[entry_str].Update(len, type);
//        printf(" elapsed: %lf, copy %lu \n", elapsed, len);
      } else if( CHECK_TYPE(type, kRef) ) {
        prv_ref_  += len;
//        input_[entry_str].Update(len, type);
//        printf(" elapsed: %lf, ref %lu \n", elapsed, len);
      } else if( CHECK_TYPE(type, kOutput) ) {
        output_[entry_str].Update(len, type);
//        printf(" elapsed: %lf, prv output %lu \n", elapsed, len);
      } else {
        assert(0);
        input_[entry_str].Update(len, type);
//        printf(" elapsed: %lf,unknown (%u) %lu \n", elapsed, type, len);
      }
  
    } else { // reexecution 
      cd::rst_elapsed_time += elapsed; 
      cd::rst_elapsed_smpl += elapsed; 
      rst_elapsed_time_    += elapsed; 
//      input_[entry_str].Update(len, type);
      restore_ += len;
    } 
  }

  // This is only called at Begin(),
  // In the case of is_reexec,
  // it additionally calculates
  // sync_time
  //    if(current_level_ == level) { 
  //      // If normal execution, Sequential CDs
  //      // If failure, Reexecution
  //    } else if(current_level_ < level) { 
  //      // If normal execution, Nested CDs
  //      // If failure, unexpected situation (mapping error)
  //    } else if(current_level_ > level) {
  //      // If normal execution, unexpected situation
  //      // If failure, escalation
  //      is_escalated = true;
  //    }
  inline void RecordBegin(bool is_reexec, bool need_sync)
  {
    CD_CLOCK_T now = CD_CLOCK();
    const double elapsed = now - cd::begin_clk;
    cd::begin_elapsed_time += elapsed;
    cd::begin_elapsed_smpl += elapsed;
    begin_elapsed_time_    += elapsed;
    cdrt_time_smpl_        += elapsed;

    if(need_sync) { // should also include recreated case.
      sync_time_  += now - cd::prof_sync_clk;
    }
    
    if(is_reexec) {
      reex_cnt_++;
      reex_clk_ = (reex_clk_ == 0)? now : reex_clk_;
    } else {
      reex_clk_ = 0;
    }
    exec_clk_ = (exec_clk_ == 0)? now : exec_clk_;
//    if(is_reexec == false) {
//      clk_ = now;
//    } else {
//      reex_cnt_clk_ = now;
//      reex_cnt_++;
//    }
  }

  // This is called at CDHandle::Complete(),
  // which means there is no error at the current CD.
  // This case is either forward execution, or reexecution not from current CD.
  // At this point, total_time_ is measured if there is no error.
  // Otherwise, reex_time_ is measured.
  inline void RecordComplete(bool is_reexec, bool lowest_level)
  {
    // When it is called, the current CD was executed without rollback.
    // if it is_reexec is true and phase == failed_phase, 
    // every parent that is reexecuted should call
    // RecordComplete(is_reexec) to record
    // reexecution portion of execution time.
    CD_CLOCK_T now = CD_CLOCK();
    const double elapsed = now - cd::begin_clk;
    cd::compl_elapsed_time += elapsed;
    cd::compl_elapsed_smpl += elapsed;
    compl_elapsed_time_    += elapsed;
    cdrt_time_smpl_        += elapsed;
    //if(is_reexec == false)  // normal execution
    const double cd_total_time = now - exec_clk_;
    total_time_ += cd_total_time;
    if(lowest_level) { 
      cd::body_elapsed_time += now - exec_clk_; 
      cd::body_elapsed_smpl += now - exec_clk_; }
    exec_cnt_   += 1;
    exec_clk_    = 0; 
    // reexecution not from the current CDs 
    if(reex_clk_ != 0) {
      //cd::reex_elapsed_time += now - reex_clk_;
      cd::reex_elapsed_smpl += now - reex_clk_;
      reex_time_            += now - reex_clk_;
      reex_clk_   = 0;
    }
// NOTE: max_prsv is gathered at Finalize()
// See void PhaseNode::GatherStats(void)
//    if(prv_elapsed_time_ > max_prv_elapsed_time_)
//      max_prv_elapsed_time_ = prv_elapsed_time_;

//    exec_trace_.push_back((float)cd_total_time);
//    prsv_trace_.push_back((float)prsv_time_smpl_);
//    cdrt_trace_.push_back((float)(cdrt_time_smpl_ + prsv_time_smpl_));
//    prsv_time_smpl_ = 0.0;
//    cdrt_time_smpl_ = 0.0;
    if(is_reexec == false) {
      exec_trace_.push_back((float)cd_total_time);
      prsv_trace_.push_back((float)prsv_time_smpl_);
      cdrt_trace_.push_back((float)(cdrt_time_smpl_ + prsv_time_smpl_));
      prsv_time_smpl_ = 0.0;
      cdrt_time_smpl_ = 0.0;

//      // reexecution not from the current CDs 
//      reex_time_ += now - reex_cnt_clk_;
//      //reex_cnt_      += 1;
    } else {
      assert(cd::is_error_free == false);
    }
  }
  

  // This is called at CD::Complete() in the case of error. 
  // RecordRollback() is always followed by GetCDToRecover()
  // Before invoking RecordRollback().
  // 
  // For example, if there is error, the order of calls will be
  // RecordBegin -> Rollback -> RecordBegin(reexec) 
  // -> ... -> FailedPoint -> ... -> RecordComplete
  // 
  // If it is just reexecution,
  // RecordBegin -> Rollback -> RecordBegin ->RecordComplete
  //
  // total_time_ measures the time between the very beginning and the end.
  // reex_time_ measures the time between error point (RecordRollback) and 
  // FailedPoint.
  //
  // At this point, reex_time is measured 
  inline void RecordRollback(bool is_first, CDOpState op=kNoop)
  {
    // if it is execution mode,
    // the reex_cnt_clk_ is set,
    // otherwise, just count reex_cnt_
    CD_CLOCK_T now = CD_CLOCK();
    
    if(is_first) {
      // When RecordRollback() is called for the first time when error happened,
      // Measure timer for complete.
      // CDHandle::Complete() will not call RecordComplete 
      // due to stack unwinding.
      cd::end_clk = now;
      const double elapsed = now - cd::begin_clk;
//      cd::body_elapsed_time       += elapsed; 
//      cd::body_elapsed_smpl       += elapsed; 
      if(op == kComplete) {
        cd::compl_elapsed_time += elapsed;
        cd::compl_elapsed_smpl += elapsed;
        compl_elapsed_time_    += elapsed;
        cdrt_time_smpl_        += elapsed;
      } else if (op == kBegin) {
        cd::begin_elapsed_time += elapsed;
        cd::begin_elapsed_smpl += elapsed;
        begin_elapsed_time_    += elapsed;
        cdrt_time_smpl_        += elapsed;
      } else if (op == kCreate) {
        cd::create_elapsed_time += elapsed;
        create_elapsed_time_    += elapsed;
        //cdrt_time_smpl_         += elapsed;
      } else {
        assert(0);
      }
    } else {
      sync_time_ += now - cd::prof_sync_clk;
    }
    cd::prof_sync_clk = now;
  }
 
  // When failed_phased is reached, 
  // RecordRecoveryComplete() is called
  // to record reex_time_ for every uncompleted CDs.
  // For the leaf CD, RecordRecoveryComplete() is called,
  // then RecordComplete() is called to measure total_time_
  inline void RecordRecoveryComplete(CD_CLOCK_T now)
  {
    //cd::reex_elapsed_time += now - reex_clk_;
    cd::reex_elapsed_smpl += now - reex_clk_;
    reex_time_            += now - reex_clk_;
    reex_cnt_  += 1;
  }
  
  inline void RecordCreate(double elapsed)
  {
    cd::create_elapsed_time += elapsed;
    cd::create_elapsed_smpl += elapsed;
    create_elapsed_time_    += elapsed;
    //cdrt_time_smpl_         += elapsed; // create should be
                                          // parent's 
  }

  inline void RecordDestory(double elapsed)
  {
    cd::destroy_elapsed_time += elapsed;
    cd::destroy_elapsed_smpl += elapsed;
    destroy_elapsed_time_    += elapsed;
    cdrt_time_smpl_          += elapsed;
  }

  void PrintTraces(FILE *fout, const char *indent) {
    {
      fprintf(fout, "%s\"exec_trace\"    : [", indent);
      auto jt=exec_trace_.begin();
      if(jt!=exec_trace_.end()) {
        fprintf(fout, "%f", *jt);
        ++jt;
      } else {
        fprintf(fout, "0");
      }
      for(;jt!=exec_trace_.end(); ++jt) {
        fprintf(fout, ",%f", *jt);
      }
      fprintf(fout, "],\n");
    }
    {
      fprintf(fout, "%s\"cdrt_trace\"    : [", indent);
      auto jt=cdrt_trace_.begin();
      if(jt!=cdrt_trace_.end()) {
        fprintf(fout, "%f", *jt);
        ++jt;
      } else {
        fprintf(fout, "0");
      }
      for(;jt!=cdrt_trace_.end(); ++jt) {
        fprintf(fout, ",%f", *jt);
      }
      fprintf(fout, "],\n");
    }
    {
      fprintf(fout, "%s\"prsv_trace\"    : [", indent);
      auto jt=prsv_trace_.begin();
      if(jt!=prsv_trace_.end()) {
        fprintf(fout, "%f", *jt);
        ++jt;
      } else {
        fprintf(fout, "0");
      }
      for(;jt!=prsv_trace_.end(); ++jt) {
        fprintf(fout, ",%f", *jt);
      }
      fprintf(fout, "],\n");
    }
    {
//      printf("max prsv %zu\n", max_prsv_.size());
      fprintf(fout, "%s\"max_prsv\"    : [", indent);
      auto jt=max_prsv_.begin();
      if(jt!=max_prsv_.end()) {
        fprintf(fout, "%f", *jt);
        ++jt;
      } else {
        fprintf(fout, "0");
      }
      for(;jt!=max_prsv_.end(); ++jt) {
        fprintf(fout, ",%f", *jt);
      }
      fprintf(fout, "],\n");
    }
    {
      fprintf(fout, "%s\"max_cdrt\"    : [", indent);
      auto jt=max_cdrt_.begin();
      if(jt!=max_cdrt_.end()) {
        fprintf(fout, "%f", *jt);
        ++jt;
      } else {
        fprintf(fout, "0");
      }
      for(;jt!=max_cdrt_.end(); ++jt) {
        fprintf(fout, ",%f", *jt);
      }
      fprintf(fout, "],\n");
    }
  }

}; // RuntimeInfo ends

//struct __PerPrvProfEntryType {
//  uint32_t copy_:1;
//  uint32_t ref_:1;
//  uint32_t copy_:1;
//};
//
//struct PerPrvProfEntryType {
//  __PerPrvProfEntryType type_;
//  uint32_t code_;
//};



typedef std::map<uint32_t, RuntimeInfo *> ProfMapType;
//typedef std::vector<std::map<PerPrvProfile>> ProfPrvCacheType;
extern ProfMapType profMap;
//extern ProfPrvCache profPrvCache;

// profPrvCache[phase] is a map containing per-preservation-entry ID to size
// per-preservation-entry ID is 64-bit hash number, and
// can be transformed to string.
// profPrvCache[phase][stringID].size_ += size;
// profPrvCache[phase][stringID].type_ = type;

} // namespace common ends
#endif
