#ifndef _PHASE_TREE
#define _PHASE_TREE

#include <vector>
#include <list>
#include <map>
#include <cstdint>

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

struct PhaseNode {
    uint32_t level_;
    uint32_t phase_;
    int64_t interval_;
    int64_t errortype_;
    PhaseNode *parent_;
    std::list<PhaseNode *> children_;
    std::string label_;
    static uint32_t phase_gen;
  public:
    PhaseNode(PhaseNode *parent=NULL)  // only for tuned::PhaseTree
      : level_(-1), phase_(-1), interval_(-1), errortype_(-1), parent_(parent) {} 
    PhaseNode(PhaseNode *parent, uint32_t level, const std::string &label) 
      : level_(level), phase_(phase_gen++), interval_(-1), errortype_(-1), parent_(parent), 
        label_(label)
    { if(parent != NULL) { parent->AddChild(this); } }
  
    void Delete(void) 
    {
      // Recursively reach the leaves, then delete from them to root.
      for(auto it=children_.begin(); it!=children_.end(); ++it) {
        (*it)->Delete();
      }
      delete this;
    }
  
    uint32_t GetPhaseNode(uint32_t level, const std::string &label);
    void AddChild(PhaseNode *child) 
    {
//      printf("%s %p (%s, %u)\n", __func__, child, child->label_.c_str(), child->phase_);
      children_.push_back(child);
    }
    void Print(void); 
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
        Print();
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
  
    void Print(void) 
    { root_->Print(); }
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

} // namespace cd ends



namespace tuned {
extern common::PhaseTree phaseTree;     // Populated from config file
//extern PhaseMapType phaseMap;
extern PhasePathType phasePath;     // Populated from config file
                                    // TunedCDHandle will check tuned::phasePath
                                    // to determine the new phase at Begin
}

#endif
