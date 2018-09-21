#ifndef _PHASE_TREE
#define _PHASE_TREE

#include <vector>
#include <list>
#include <map>
#include <cstdint>
#include "runtime_info.h"
#include "cd_def_debug.h"
//class PhaseMap : public std::map<std::string, uint32_t> {
//  public:
//    //std::string prev_phase_;
//    uint32_t phase_gen_;
//};
typedef std::map<std::string, uint32_t> PhasePathType;
typedef std::map<uint32_t, std::map<std::string, uint32_t>> PhaseMapType;

namespace common {
//struct MeasuredProf {
//
//};
// PhaseNode is crated when Begin() is invoked.
//
struct PhaseNode {
    // unique identifier for each node in phaseTree
    uint32_t level_;
    uint32_t phase_;

    // info in task space
    uint32_t sibling_id_;
    uint32_t sibling_size_;
    uint32_t task_id_;
    uint32_t task_size_;

    // sequenial ID
    uint64_t seq_begin_; // records at the first begin
    uint64_t seq_end_;   // increments at every begin
    uint64_t seq_max_;   // accumulated until parent CD complete
    uint64_t seq_acc_;   // accumulated for every begin/compl
    uint64_t seq_acc_rb_;   // accumulated for rollback every begin/compl

    CDExecMode state_;

    // from execution profile. required for tuning 
    int64_t  count_;

    // from tuner
    int64_t  interval_;
    int64_t  errortype_;

    // links for phaseTree
    PhaseNode *parent_;
    // left and right is sort of caching 
    // *(--(parent->children_->find(this))) for left
    // *(++(parent->children_->find(this))) for right
    PhaseNode *left_;
    PhaseNode *right_;
    std::list<PhaseNode *> children_;

    // label corresponding to each node
    std::string label_;
    PrvMediumT medium_;

    // from execution profile
    RuntimeInfo profile_;
    static uint32_t phase_gen;
    static uint32_t max_phase;
    static int64_t last_completed_phase;
  public:
    // for cd::phaseTree
    PhaseNode(PhaseNode *parent, uint32_t level, const std::string &label, CDExecMode state) 
      : level_(level), phase_(phase_gen), 
        sibling_id_(0), sibling_size_(1), task_id_(0), task_size_(1), 
        /*seq_begin_(0), seq_end_(0),*/ seq_max_(0), seq_acc_(0), seq_acc_rb_(0),
        state_(state), count_(0), interval_(-1), errortype_(-1), 
        label_(label), medium_(kUndefined), profile_(phase_gen)
    {
      TUNE_DEBUG("PhaseNode %s\n", label.c_str());
      Init(parent, level);
      phase_gen++;
    }

    // Creator for tuned::phaseTree
    // tuned::phaseTree is generated by the script from tuner before actully
    // executing application. 
    PhaseNode(PhaseNode *parent, uint32_t level, uint32_t phase)
      : level_(level), phase_(phase_gen), 
        sibling_id_(0), sibling_size_(1), task_id_(0), task_size_(1), 
        /*seq_begin_(0), seq_end_(0),*/ seq_max_(0), seq_acc_(0), seq_acc_rb_(0),
        state_(kExecution), count_(0), interval_(-1), errortype_(-1), 
        label_("Undefined"), medium_(kUndefined), profile_(phase_gen)
    {
      CD_ASSERT_STR(phase == phase_, "PhaseNode(%u == %u)\n", phase_, phase);
      Init(parent, level);
      phase_gen++;
    }

    void Init(PhaseNode *parent, uint32_t level)
    {
      seq_begin_ = 0;
      seq_end_   = 0;
      if(parent != NULL) {
        parent_ = parent;
        if(parent_->children_.empty()) {
          left_  = NULL;
          right_ = NULL;
        } else {
          PhaseNode *prev_phase = parent_->children_.back();
          prev_phase->right_ = this;
          left_ = prev_phase;
          right_ = NULL;
          uint32_t last_seq_id = prev_phase->seq_end_;
          seq_begin_ = last_seq_id;
          seq_end_   = last_seq_id;
        }
        parent_->AddChild(this);
      } else { // for root
        parent_ = NULL;
        left_   = NULL;
        right_  = NULL;
      }
    } 

    void Delete(void) 
    {
      // Recursively reach the leaves, then delete from them to root.
      for(auto it=children_.begin(); it!=children_.end(); ++it) {
        (*it)->Delete();
      }
      left_ = NULL;
      right_ = NULL;
      delete this;
    }

    void GatherStats(void);

    double GetRuntimeOverhead(bool include_prv_time=false) {
      double create_time = (parent_!=NULL)? parent_->profile_.create_elapsed_time_ : 0.0;
      double prv_time = (include_prv_time)? profile_.GetPreserveTime() : 0.0;
      return profile_.GetCDRTOverhead(create_time + prv_time);
    }
    double GetRuntimeOverheadLocal(bool include_prv_time=false) {
      double create_time = (parent_!=NULL)? parent_->profile_.create_elapsed_time_ : 0.0;
      double prv_time = (include_prv_time)? profile_.GetPreserveTime() : 0.0;
      return profile_.GetCDRTOverheadLocal(create_time + prv_time);
    }
    double GetPrvBW(bool is_input=true) {
      return profile_.GetPrvVolume(is_input, false) / GetRuntimeOverhead(true);
    }

    PhaseNode *GetNextNode(const std::string &label) 
    {
      PhaseNode *next = NULL;
      TUNE_DEBUG("%s %s == %s\n", __func__, label.c_str(), label_.c_str());
      if(label_ == label) {
        next = this;
      } else {
        for(auto it=children_.rbegin(); it!=children_.rend(); ++it) {
          TUNE_DEBUG("%s == %s\n", label.c_str(), (*it)->label_.c_str());
          if((*it)->label_ == label) { next = *it; break; }
        }
      }
      CD_ASSERT(next);
      return next;
    }

    void UpdateTaskInfo(uint32_t rank_in_level, uint32_t sibling_count, 
                        uint32_t task_in_color, uint32_t task_count) 
    {
      sibling_id_   = rank_in_level;
      sibling_size_ = sibling_count;
      task_id_      = task_in_color;
      task_size_    = task_count;
    }

    uint32_t GetPhaseNode(uint32_t level, const std::string &label);
    PhaseNode *GetLeftMostNode(void)
    {
      PhaseNode *leftmost = NULL;
      if(left_ != NULL && left_->interval_ == 0) {
        assert(left_ != this);
        TUNE_DEBUG("%u ", phase_);
        leftmost = left_->GetLeftMostNode();
        TUNE_DEBUG(" -> %u", phase_);
      } else {
        TUNE_DEBUG("Left %u", phase_);
        leftmost = this;
      }
      return leftmost;
    }

    PhaseNode *GetRightMostNode(void)
    {
      PhaseNode *rightmost = NULL;
      if(right_ != NULL && right_->interval_ == 0) {
        TUNE_DEBUG("%u -> ", phase_);
        rightmost = right_->GetRightMostNode();
      } else {
        TUNE_DEBUG("%u Right", phase_);
        rightmost = this;
      }
      return rightmost;
    }

    void AddChild(PhaseNode *child) 
    {
//      TUNE_DEBUG("%s %p (%s, %u)\n", __func__, child, child->label_.c_str(), child->phase_);
      children_.push_back(child);
    }
    void Print(bool print_details, bool first, FILE *outfile=NULL);
    void PrintNode(bool print_details, FILE *outfile=NULL);
//    void PrintAll(bool print_details, bool first); 
    void PrintProfile(void);
    void PrintInputYAML(bool first); 
    void PrintOutputYAML(bool first); 
    static void PrintOutputJson(PhaseNode *root);
    void PrintOutputJsonInternal(void);
    std::string GetPhasePath(void);
    std::string GetPhasePath(const std::string &label);

    // When failed_phase is reached after failure, 
    // reset state_ to kExecution and measure reexec_time_
    // First, it goes up to the most parent node which is failed,
    // then reset every descendant nodes from the node. 
    void FinishRecovery(CD_CLOCK_T now) 
    {
      CD_ASSERT(state_ == kReexecution);
//      profile_.RecordRecoveryComplete(now);
      if(parent_ == NULL || parent_->state_ != kReexecution) { 
        ResetToExec();
      } else {
        parent_->FinishRecovery(now);
      }
    }
 
    // ResetToExec for every tasks 
    void ResetToExec(void) 
    {
      state_ = kExecution;
      for(auto it=children_.begin(); it!=children_.end(); ++it) {
        (*it)->ResetToExec();
      }
    }

    inline void PrintDetails(void) {
      if(cd::myTaskID == 0) { 
      printf("Phase: %s (err:%lx, intvl:%ld) (lv:%u,ph:%u,seq:%lu~%lu, %lu/%lu)\n"
             "(%s) siblingID:%u/%u, taskID:%u/%u, cnt:%ld\n",
      label_.c_str(), errortype_, interval_, level_, phase_, seq_begin_, seq_end_, seq_acc_, seq_acc_rb_,
      (state_ == kExecution)? "exec" : "reex", sibling_id_, sibling_size_, task_id_, task_size_, count_);
      }
      CD_DEBUG("Phase: %s (err:%lx, intvl:%ld) (lv:%u,ph:%u,seq:%lu~%lu, %lu/%lu)\n"
             "(%s) siblingID:%u/%u, taskID:%u/%u, cnt:%ld\n",
      label_.c_str(), errortype_, interval_, level_, phase_, seq_begin_, seq_end_, seq_acc_, seq_acc_rb_,
      (state_ == kExecution)? "exec" : "reex", sibling_id_, sibling_size_, task_id_, task_size_, count_);
    }      

    // Only seq_begin_ should be updated here.
    // seq_begin_ preverves sequential_id_.
    // Its purpose is to reinit seq_end_
    inline void MarkSeqID(int64_t seq_id) { 
      seq_begin_ = seq_id; 
      seq_end_   = seq_id;
    }

    inline void IncSeqID(bool err_free_exec) {
      if(err_free_exec) {
        seq_end_++; // reinit at failure
        seq_acc_++; // no reinit
      }
      seq_acc_rb_++; // total count
      seq_max_ = (seq_end_ > seq_max_)? seq_end_ : seq_max_; 
    }

    inline void ResetSeqID(uint32_t rollback_lv) {
      //PrintDetails();
      if(rollback_lv < level_) {
        seq_end_ = seq_begin_; 
      }
    }
};

struct PhaseTree {
    PhaseNode *root_;
    PhaseNode *current_;
  public:
    PhaseTree(void) : root_(NULL), current_(NULL) {}
    ~PhaseTree();
  
    uint32_t Init(uint64_t level,  const std::string &label);

    inline
    uint32_t ReInit(void)
    { 
      current_ = root_;
      return root_->phase_; 
    }
  
    void Print(int format=0) 
    {
      if(cd::myTaskID == 0) { 
      switch(format) {
        case 0: root_->PrintInputYAML(true); break;
        case 1: root_->Print(true, true); break;
      }
      }
    }
    
    void PrintProfile(void) 
    { root_->PrintProfile(); }

    void PrintStats(void);
};


struct CDProfiles {
  RTInfo<double> avg_;
  RTInfo<double> std_;
  RTInfo<DoubleInt> max_;
  //RTInfo<double> max_rank_;
  RTInfo<DoubleInt> min_;
  //RTInfo<double> min_rank_;
  int max_rank_;
  int min_rank_;
  //void Print(std::ostream &os, const char *str="");
  void Print(std::ostream &os, const std::string &head, const std::string &tail, const char *str="");
  void PrintJSON(std::ostream &os, const std::string &head);
};
extern std::map<uint32_t, CDProfiles> cd_prof_map;
const char *GetMedium(PrvMediumT medium);
} // namespace common ends

using namespace common;

namespace cd {
// phaseMap generates phase ID for CDNameT at each level.
// Different label can create different phase,
// therefore CD runtime compares the label of current CD being created
// with that of preceding sequential CD at the same level. See GetPhase()
// 
// The scope of phase ID is limited to the corresponding level,
// which means that the same label at different levels are different phases.
extern common::PhaseTree phaseTree;
extern PhasePathType phasePath;
//extern std::vector<PhaseNode *> phaseNodeCache;
extern std::map<uint32_t, PhaseNode *> phaseNodeCache;
extern uint32_t new_phase;
} // namespace cd ends



namespace tuned {
extern common::PhaseTree phaseTree;     // Populated from config file
extern std::map<uint32_t, PhaseNode *> phaseNodeCache;
//extern PhaseMapType phaseMap;
extern PhasePathType phasePath;     // Populated from config file
                                    // TunedCDHandle will check tuned::phasePath
                                    // to determine the new phase at Begin
}

#endif
