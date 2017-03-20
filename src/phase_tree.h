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
    uint32_t level_;
    uint32_t phase_;
    int64_t count_;
    int64_t interval_;
    int64_t errortype_;
    PhaseNode *parent_;
    PhaseNode *left_;
    PhaseNode *right_;
    std::list<PhaseNode *> children_;
    std::string label_;
    RuntimeInfo profile_;
    static uint32_t phase_gen;
    static uint32_t max_phase;
  public:
    // for cd::phaseTree
    PhaseNode(PhaseNode *parent, uint32_t level, const std::string &label) 
      : level_(level), phase_(phase_gen++), count_(0), interval_(-1), errortype_(-1), label_(label)
    {
      printf("PhaseNode %s\n", label.c_str());
      Init(parent, level);
    }
    // for tuned::phaseTree
    PhaseNode(PhaseNode *parent, uint32_t level, uint32_t phase)
      : level_(level), phase_(phase_gen++), count_(0), interval_(-1), errortype_(-1)
    {
      CD_ASSERT_STR(phase == phase_, "PhaseNode(%u == %u)\n", phase_, phase);
      Init(parent, level);
    }

    void Init(PhaseNode *parent, uint32_t level)
    {
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

    PhaseNode *GetNextNode(const std::string &label) 
    {
      PhaseNode *next = NULL;
      printf("%s %s == %s\n", __func__, label.c_str(), label_.c_str());
      if(label_ == label) {
        next = this;
      } else {
        for(auto it=children_.rbegin(); it!=children_.rend(); ++it) {
          printf("%s == %s\n", label.c_str(), (*it)->label_.c_str());
          if((*it)->label_ == label) { next = *it; break; }
        }
      }
      CD_ASSERT(next);
      return next;
    }

    uint32_t GetPhaseNode(uint32_t level, const std::string &label);
    PhaseNode *GetLeftMostNode(void)
    {
      PhaseNode *leftmost = NULL;
      if(left_ != NULL && left_->interval_ == 0) {
        assert(left_ != this);
        printf("%u ", phase_);
        leftmost = left_->GetLeftMostNode();
        printf(" -> %u", phase_);
      } else {
        printf("Left %u", phase_);
        leftmost = this;
      }
      return leftmost;
    }

    PhaseNode *GetRightMostNode(void)
    {
      PhaseNode *rightmost = NULL;
      if(right_ != NULL && right_->interval_ == 0) {
        printf("%u -> ", phase_);
        rightmost = right_->GetRightMostNode();
      } else {
        printf("%u Right", phase_);
        rightmost = this;
      }
      return rightmost;
    }

    void AddChild(PhaseNode *child) 
    {
//      printf("%s %p (%s, %u)\n", __func__, child, child->label_.c_str(), child->phase_);
      children_.push_back(child);
    }
    void Print(void); 
    void PrintInputYAML(void); 
    void PrintOutputYAML(void); 
    void PrintProfile(void); 
    std::string GetPhasePath(void);
    std::string GetPhasePath(const std::string &label);
};

struct PhaseTree {
    PhaseNode *root_;
    PhaseNode *current_;
  public:
    PhaseTree(void) : root_(NULL), current_(NULL) {}
    ~PhaseTree() {
      // If root_ == NULL,
      // phaseTree is not created.
      if(root_ != NULL) {
//        Print();
        root_->PrintInputYAML();
        root_->PrintOutputYAML();
        PrintProfile();
        printf("%s %p\n", __func__, root_);
        root_->Delete();
      } 
    }
  
    uint32_t Init(uint64_t level,  const std::string &label)
    { 
      root_ = new PhaseNode(NULL, level, label); 
      current_ = root_;
      return current_->phase_; 
    }
  
    void Print(int format=0) 
    { 
      switch(0) {
        case 0: root_->PrintInputYAML(); break;
        case 1: root_->Print(); break;
      }
    }
    
    void PrintProfile(void) 
    { root_->PrintProfile(); }
};

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
