#include "phase_tree.h"

using namespace common;
using namespace std;

PhaseTree     cd::phaseTree;
PhasePathType cd::phasePath;
std::map<uint32_t, PhaseNode *> cd::phaseNodeCache;
//PhaseMapType  tuned::phaseMap;
PhaseTree     tuned::phaseTree; // Populated from config file
PhasePathType tuned::phasePath; // Populated from config file
uint32_t common::PhaseNode::phase_gen = 0;

static inline
void AddIndent(int cnt)
{
  for(int i=0; i<cnt; i++) {
    printf("    ");
  }
}

void PhaseNode::Print(void) 
{
  AddIndent(level_);
  printf("CD_%u_%u\n", level_, phase_);
  AddIndent(level_);
  printf("{\n");
  AddIndent(level_+1); printf("label:%s\n", label_.c_str());
  AddIndent(level_+1); printf("interval:%ld\n", interval_);
  AddIndent(level_+1); printf("errortype:0x%lX\n", errortype_);
//  printf("children size:%zu\n", children_.size()); getchar();
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->Print();
  }
  AddIndent(level_);
  printf("}\n");
}

std::string PhaseNode::GetPhasePath(const std::string &label)
{
  if(parent_ !=NULL)
    return (GetPhasePath() + std::string("_") + label);
  else
    return label;
}

std::string PhaseNode::GetPhasePath(void)
{
  if(parent_ !=NULL)
    return (parent_->GetPhasePath() + std::string("_") + label_);
  else
    return label_;
}

// Create or Get phase depending on label
// Update phaseTree.current_, and returns phase ID number.
// If there is no phase for the unique label, create a new PhaseNode
// cd_name_.phase_ = cd::phaseTree->target_->GetPhaseNode();
uint32_t PhaseNode::GetPhaseNode(uint32_t level, const string &label)
{
  printf("## %s ## lv:%u, label:%s\n", __func__, level, label.c_str());
  uint32_t phase = -1;
  std::string phase_path = GetPhasePath(label);
  auto it = cd::phasePath.find(phase_path);

  // If there is no label before, it is a new phase!
  if(it == cd::phasePath.end()) {
    cd::phaseTree.current_ = new PhaseNode(cd::phaseTree.current_, level, label);
    uint32_t phase        = cd::phaseTree.current_->phase_;
//    tuned::phaseMap[level][label] = phase;
    cd::phasePath[phase_path]  = phase;
    printf("phase:%u, current:%p\n", phase, cd::phaseTree.current_);
    cd::phaseNodeCache[phase]  = cd::phaseTree.current_;

    //phaseMap[level][label] = cd::phaseTree.current_->phase_;
    printf("New Phase! %u %s\n", phase, phase_path.c_str());
  } else {
    cd::phaseTree.current_ = cd::phaseNodeCache[it->second];
    phase = it->second;
    printf("Old Phase! %u %s\n", phase, phase_path.c_str()); //getchar();
  }
  return phase;
}
