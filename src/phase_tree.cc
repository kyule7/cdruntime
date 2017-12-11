#include "phase_tree.h"
#include "cd_global.h"
#include "cd_def_preserve.h"
#include "sys_err_t.h"
using namespace common;
using namespace std;

PhaseTree     cd::phaseTree;
PhasePathType cd::phasePath;
std::map<uint32_t, PhaseNode *> cd::phaseNodeCache;
//PhaseMapType  tuned::phaseMap;
PhaseTree     tuned::phaseTree; // Populated from config file
PhasePathType tuned::phasePath; // Populated from config file
std::map<uint32_t, PhaseNode *> tuned::phaseNodeCache;
uint32_t common::PhaseNode::phase_gen = 0;
uint32_t common::PhaseNode::max_phase = 0;
int64_t common::PhaseNode::last_completed_phase = HEALTHY;

static FILE *inYAML = NULL;
static FILE *outYAML = NULL;
static FILE *outJSON = NULL;
static FILE *outAll = NULL;
static char output_filepath[256];
static char output_basepath[512];
//static inline
//void AddIndent(int cnt)
//{
//  for(int i=0; i<cnt; i++) {
//    fprintf(outAll, "  ");
//  }
//}

//YKWON: This produces config.in (defined in CD_DEFAULT_OUTPUT_CONFIG_IN) for 
//       for baseline of config.yaml, which runtime actually refers. 
//       In addition to this file, user should add FAILURE information when 
//       error inject is on.
void PhaseNode::PrintInputYAML(bool first) 
{
  if(first) {
    assert(inYAML == NULL);
//    string &filepath = cd::output_basepath;
//    printf("in yaml filepath:%s , %s\n", filepath.c_str(), std::string(CD_DEFAULT_OUTPUT_CONFIG_IN).c_str());
//    filepath += std::string(CD_DEFAULT_OUTPUT_CONFIG_IN);
//<<<<<<< HEAD
//    sprintf(output_filepath, "%s%s", cd::output_basepath.c_str(), CD_DEFAULT_OUTPUT_CONFIG_IN);
//    //inYAML = fopen(output_filepath, "a");
//    inYAML = fopen(output_filepath, "w+");
//=======
    memset(output_filepath, '\0', 256);
    sprintf(output_filepath, "%s/%s", output_basepath, CD_DEFAULT_OUTPUT_CONFIG_IN);
    printf("%s file:%s\n", __func__, output_basepath);
    inYAML = fopen(output_filepath, "a");
//    printf("in yaml filepath:%s , %s\n", cd::output_basepath.c_str(), std::string(CD_DEFAULT_OUTPUT_CONFIG_IN).c_str());
//    inYAML = fopen((cd::output_basepath + std::string(CD_DEFAULT_OUTPUT_CONFIG_IN)).c_str(), "a");
  }

  std::string indent(level_<<1, ' ');
  std::string one_more_indent((level_+1)<<1, ' ');
  std::string two_more_indent((level_+2)<<1, ' ');
  fprintf(inYAML, "%s- CD_%u_%u :\n",          indent.c_str(), level_, phase_);
  fprintf(inYAML, "%s- label    : %s\n", one_more_indent.c_str(), label_.c_str());
  fprintf(inYAML, "%sinterval : %ld\n",    two_more_indent.c_str(), interval_);
  fprintf(inYAML, "%serrortype: 0x%lX\n", two_more_indent.c_str(), errortype_);
  //  fprintf(inYAML, "%s", profile_.GetRTInfoStr(level_+1).c_str());
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->PrintInputYAML(false);
  }

  if(first) {
    fclose(inYAML);
    inYAML = NULL;
  }
}

//YKWON: This is depreciated 
void PhaseNode::PrintOutputYAML(bool first) 
{
  if(first) {
    assert(outYAML == NULL);
//    string &filepath = cd::output_basepath;
//    printf("out yaml filepath:%s , %s\n", filepath.c_str(), std::string(CD_DEFAULT_OUTPUT_CONFIG_OUT).c_str());
//    filepath += std::string(CD_DEFAULT_OUTPUT_CONFIG_OUT);
//    outYAML = fopen(filepath.c_str(), "a");
    memset(output_filepath, '\0', 256);
    sprintf(output_filepath, "%s/%s", output_basepath, CD_DEFAULT_OUTPUT_CONFIG_OUT);
    printf("%s Output File:%s\n", __func__, output_filepath);
    outYAML = fopen(output_filepath, "a");
  }
  std::string indent(level_<<1, ' ');
  std::string one_more_indent((level_+1)<<1, ' ');
  std::string two_more_indent((level_+2)<<1, ' ');
  fprintf(outYAML, "%s- CD_%u_%u :\n",               indent.c_str(), level_, phase_);
  fprintf(outYAML, "%s- label    : %s\n",      one_more_indent.c_str(), label_.c_str());
  fprintf(outYAML, "%sinterval : %ld\n",    two_more_indent.c_str(), interval_);
  fprintf(outYAML, "%serrortype: 0x%lX\n", two_more_indent.c_str(), errortype_);
//  fprintf(outAll, "children size:%zu\n", children_.size()); getchar();
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->PrintOutputYAML(false);
  }
  if(first) {
    fclose(outYAML);
    outYAML = NULL;
  }
}

// Only root call this
//YKWON: This proudces the file in JSON format for the estimator to find 
//       optimal mappipng. (estimation.json)
//TODO: check interval, reex_cnt, error_vec 
void PhaseNode::PrintOutputJson(void) 
{
//<<<<<<< HEAD
//  //FIXME(YKWON): Better to have different file handler
//  assert(outYAML == NULL);
//  assert(parent_ == NULL);
//  sprintf(output_filepath, "%s%s", cd::output_basepath.c_str(), CD_DEFAULT_OUTPUT_CONFIG_OUT);
//  printf("Output File:%s\n", output_filepath);
//  //outYAML = fopen(output_filepath, "a");
//  outYAML = fopen(output_filepath, "w+");
//=======
  assert(outJSON == NULL);
  assert(parent_ == NULL);
  memset(output_filepath, '\0', 256);
  sprintf(output_filepath, "%s/%s", output_basepath, CD_DEFAULT_OUTPUT_CONFIG_OUT);
  printf("%s Output File:%s\n", __func__, output_filepath);
  outJSON = fopen(output_filepath, "a");
  
  fprintf(outJSON, "{\n"
                   "  // global parameters\n"
                   "  \"global_param\" : {\n"
                   "    \"max_error\" : 20\n"
                   "  },\n"
                   "  // CD hierarchy\n"
                   "  \"CD info\" : {\n"
         );
  PrintOutputJsonInternal();
  fprintf(outJSON, "  } // CD info ends\n"
                   "}\n"
         );
  fclose(outJSON);
  outJSON = NULL;
  
}

static int adjust_tab = 2;
void PhaseNode::PrintOutputJsonInternal(void) 
{
  int tabsize = level_ + adjust_tab;
  std::string indent((tabsize)<<1, ' ');
  std::string one_more_indent((tabsize+1)<<1, ' ');
  std::string two_more_indent((tabsize+2)<<1, ' ');
//<<<<<<< HEAD
//  //TODO: lable may be better for CD name instead of level and phase
//  fprintf(outYAML, "%s\"CD_%u_%u\" : {\n",               indent.c_str(), level_, phase_);
//  //TODO: the estimator does not require "label".
//  fprintf(outYAML, "%s\"label\" : %s\n",        one_more_indent.c_str(), label_.c_str());
//  fprintf(outYAML, "%s\"interval\" : %ld\n",    one_more_indent.c_str(), interval_);
//  fprintf(outYAML, "%s\"errortype\" : 0x%lX\n", one_more_indent.c_str(), errortype_);
//  //YKWON: added the information about siblings
//  //FIXME: still this doesn't product the correct number of siblings
//  fprintf(outYAML, "%s\"siblingID\" : %8u\n",  one_more_indent.c_str(), sibling_id_);
//  fprintf(outYAML, "%s\"sibling #\" : %8u\n",  one_more_indent.c_str(), sibling_size_);
//  // This print exec_cnt, reex_cnt, tot_time, reex_time, vol_copy, vol_refer
//  // comm_log and error_ven, which are also printed in profile.out.
//  // TODO: let's change to be in B/ MB/ GB
//  fprintf(outYAML, "%s", profile_.GetRTInfoStr(tabsize + 1).c_str());
//  fprintf(outYAML, "%s\"ChildCDs\" : {\n", one_more_indent.c_str());
//=======
  fprintf(outJSON, "%s\"CD_%u_%u\" : {\n",              indent.c_str(), level_, phase_);
  fprintf(outJSON, "%s\"label\"    : %s\n",    one_more_indent.c_str(), label_.c_str());
  fprintf(outJSON, "%s\"interval\" : %ld\n",   one_more_indent.c_str(), interval_);
  fprintf(outJSON, "%s\"errortype\": 0x%lX\n", one_more_indent.c_str(), errortype_);
  fprintf(outJSON, "%s\"siblingID\" : %8u\n",  one_more_indent.c_str(), sibling_id_);
  fprintf(outJSON, "%s\"sibling #\" : %8u\n",  one_more_indent.c_str(), sibling_size_);
  fprintf(outJSON, "%s", profile_.GetRTInfoStr(tabsize + 1).c_str());
  fprintf(outJSON, "%s\"ChildCDs\" : {\n", one_more_indent.c_str());

  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->PrintOutputJsonInternal();
  }
  fprintf(outJSON, "%s}\n", one_more_indent.c_str());
  fprintf(outJSON, "%s}\n",          indent.c_str());
}

//void PhaseNode::Print(void) 
//{
//  std::string indent((level_)<<1, ' ');
//  std::string one_more_indent((level_+1)<<1, ' ');
//  fprintf(outAll, "%sCD_%u_%u\n",                 indent.c_str(), level_, phase_);
//  fprintf(outAll, "%slabel:%s\n",        one_more_indent.c_str(), label_.c_str());
//  fprintf(outAll, "%sinterval:%ld\n",    one_more_indent.c_str(), interval_);
//  fprintf(outAll, "%serrortype:0x%lX\n", one_more_indent.c_str(), errortype_);
////  fprintf(outAll, "children size:%zu\n", children_.size()); getchar();
//  for(auto it=children_.begin(); it!=children_.end(); ++it) {
//    (*it)->Print();
//  }
//  AddIndent(level_);
//  fprintf(outAll, "}\n");
//}

void PhaseNode::Print(bool print_details, bool first) 
{
  if(first) {
//    assert(outAll == NULL);
//    string &filepath = cd::output_basepath;
//    printf("out yaml filepath:%s , %s\n", filepath.c_str(), std::string(CD_DEFAULT_OUTPUT_PROFILE).c_str());
//    filepath += std::string(CD_DEFAULT_OUTPUT_PROFILE);
//    outAll = fopen(filepath.c_str(), "a");
//    printf("profile out filepath:%s , %s\n", cd::output_basepath.c_str(), std::string(CD_DEFAULT_OUTPUT_PROFILE).c_str());
//    outAll = fopen((cd::output_basepath + std::string(CD_DEFAULT_OUTPUT_PROFILE)).c_str(), "a");
    memset(output_filepath, '\0', 256);
    sprintf(output_filepath, "%s/%s", output_basepath, CD_DEFAULT_OUTPUT_PROFILE);
//    printf("[%s] %s\n", __func__, output_filepath);
    outAll = fopen(output_filepath, "w+");
  }
  std::string indent((level_)<<1, ' ');
  std::string one_more_indent((level_+1)<<1, ' ');
  fprintf(outAll, "%sCD_%u_%u\n",                indent.c_str(), level_, phase_);
  fprintf(outAll, "%s{\n",                       indent.c_str());
  fprintf(outAll,   "%slabel      :%s\n",   one_more_indent.c_str(), label_.c_str());
  fprintf(outAll,   "%sstate      :%8u\n",  one_more_indent.c_str(), state_);
  fprintf(outAll,   "%sinterval   :%8ld\n", one_more_indent.c_str(), interval_);
  if(print_details) {
    fprintf(outAll, "%serrortype  :0x%lX\n", one_more_indent.c_str(), errortype_);
    fprintf(outAll, "%ssiblingID  :%8u\n",  one_more_indent.c_str(), sibling_id_);
    fprintf(outAll, "%ssibling #  :%8u\n",  one_more_indent.c_str(), sibling_size_);
    fprintf(outAll, "%stask ID    :%8u\n",  one_more_indent.c_str(), task_id_);
    fprintf(outAll, "%stask #     :%8u\n",  one_more_indent.c_str(), task_size_);
    fprintf(outAll, "%scount(tune):%8ld\n", one_more_indent.c_str(), count_);
    fprintf(outAll, "%scount(cd)  :%8ld // # executions\n", one_more_indent.c_str(), seq_acc_);
    fprintf(outAll, "%s# recreated:%8ld // # rexecutions\n", one_more_indent.c_str(), seq_acc_rb_);
    fprintf(outAll, "%s", profile_.GetRTInfoStr(level_+1, RuntimeInfo::kPROF).c_str());
  }
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->Print(print_details, false);
  }
  fprintf(outAll, "%s}\n", indent.c_str());
  if(first) {
    fclose(outAll);
  }
}

void PhaseNode::PrintProfile(void) 
{
  profile_.Print();
////  fprintf(outAll, "children size:%zu\n", children_.size()); getchar();
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->PrintProfile();
  }
//  AddIndent(level_);
//  fprintf(outAll, "}\n");
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
  TUNE_DEBUG("@@ %s ## lv:%u, label:%s\n", __func__, level, label.c_str()); //getchar();
//  if(cd::myTaskID == 0) fprintf(outAll, "@@ %s ## lv:%u, label:%s\n", __func__, level, label.c_str()); //getchar();
  uint32_t phase = -1;
  std::string phase_path = GetPhasePath(label);
  auto it = cd::phasePath.find(phase_path);

  // If there is no label before, it is a new phase!
  if(it == cd::phasePath.end()) {
    // Parent's state is inherited to its child
    // If parent begins a child for the first time, 
    // it must be kExecution. 
    // Before reaching a new phase node, it tries to reach it with reexecution
    // mode, resetting execution mode may be missed!!
    CD_ASSERT(state_ == kExecution);
    cd::phaseTree.current_ = new PhaseNode(cd::phaseTree.current_, level, label, state_);
    phase        = cd::phaseTree.current_->phase_;
//    tuned::phaseMap[level][label] = phase;
    cd::phasePath[phase_path]  = phase;
//    TUNE_DEBUG("phase:%u, current:%p\n", phase, cd::phaseTree.current_);
    cd::phaseNodeCache[phase]  = cd::phaseTree.current_;

    //phaseMap[level][label] = cd::phaseTree.current_->phase_;
    TUNE_DEBUG("New Phase! %u %s\n", phase, phase_path.c_str());
//    if(cd::myTaskID == 0) fprintf(outAll, "New Phase! %u at lv#%u %s\n", phase, level, phase_path.c_str());
  } else {
    cd::phaseTree.current_ = cd::phaseNodeCache[it->second];
    // Parent's state is inherited to its child
    if(state_ == kReexecution) {
      cd::phaseTree.current_->state_ = kReexecution;
    }
    phase = it->second;
    TUNE_DEBUG("Old Phase! %u %s\n", phase, phase_path.c_str()); //getchar();
//    if(cd::myTaskID == 0) fprintf(outAll, "Old Phase! %u at lv#%u%s\n", phase, level, phase_path.c_str()); //getchar();
  }

#if CD_TUNING_ENABLED == 0 && CD_RUNTIME_ENABLED == 1
  auto pt = tuned::phaseNodeCache.find(phase);
  assert(pt != tuned::phaseNodeCache.end());
  cd::phaseTree.current_->errortype_ = pt->second->errortype_;
#endif

//  // First visit
//  if(last_completed_phase != phase) {
//    seq_begin_ = seq_end_;
//  }

//  profMap[phase] = &cd::phaseTree.current_->profile_;getchar();
  return phase;
}

// for tuned::CDHandle
// returns phase ID from label, 
// and update phaseNodeCache
//uint32_t PhaseNode::GetPhase(const string &label)
//{
//  fprintf(outAll, "## %s ## label:%s\n", __func__, label.c_str());
//  std::string phase_path = GetPhasePath(label);
//  auto it = tuned::phasePath.find(phase_path);
//  assert(it != tuned::phasePath.end());
//  assert(tuned::phaseNodeCache.find(it->second) != tuned::phaseNodeCache.end());
//  return tuned::phaseNodeCache[it->second];
//}


PhaseTree::~PhaseTree() {
  // If root_ == NULL,
  // phaseTree is not created.
  if(root_ != NULL) {
    root_->Delete();
  }
}

void PhaseTree::PrintStats(void)
{
  // If root_ == NULL,
  // phaseTree is not created.
  if(cd::myTaskID == 0) {
    if(root_ != NULL) {
      char *cd_config_file = getenv("CD_OUTPUT_BASE");
      if(cd_config_file != NULL) {
        strcpy(output_basepath, cd_config_file);
      }
      else {
        strcpy(output_basepath, CD_DEFAULT_OUTPUT_BASE);
      }

      printf("basepath:%s\n", output_basepath);
//        Print();
      root_->PrintInputYAML(true);
//      root_->PrintOutputYAML(true);
      root_->PrintOutputJson();
      //FIXME(YKWON): This sometimes fails to produce profile.out
      root_->Print(true, true);
//      PrintProfile();
      fprintf(stdout, "%s %p\n", __func__, root_);
    }
  }

}
