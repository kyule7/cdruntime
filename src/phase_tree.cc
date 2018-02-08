#include "phase_tree.h"
#include "cd_global.h"
#include "cd_def_preserve.h"
#include "sys_err_t.h"
#include <iomanip>      // std::setw
using namespace common;
using namespace std;

std::map<uint32_t, CDProfiles> common::cd_prof_map;
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
const char *common::GetMedium(PrvMediumT medium)
{
  switch(medium) {
    case kLocalMemory:  return "LocalMemory";
    case kRemoteMemory: return "RemoteMemory";
    case kLocalDisk:    return "LocalDisk";
    case kGlobalDisk:   return "GlobalDisk";
    default:
#if CD_TUNING_ENABLED == 0 && CD_RUNTIME_ENABLED == 1
      ERROR_MESSAGE("Undefined medium:%d\n", medium);
#else
      return "Undefined";
#endif
  }
}
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
    memset(output_filepath, '\0', 256);
    sprintf(output_filepath, "%s/%s", output_basepath, CD_DEFAULT_OUTPUT_CONFIG_IN);
    printf("%s file:%s\n", __func__, output_basepath);
    inYAML = fopen(output_filepath, "a");
  }

  std::string indent(level_<<1, ' ');
  std::string one_more_indent((level_+1)<<1, ' ');
  std::string two_more_indent((level_+2)<<1, ' ');
  fprintf(inYAML, "%s- CD_%u_%u :\n",          indent.c_str(), level_, phase_);
  fprintf(inYAML, "%s- label    : %s\n", one_more_indent.c_str(), label_.c_str());
  fprintf(inYAML, "%sinterval : %ld\n",    two_more_indent.c_str(), interval_);
  fprintf(inYAML, "%serrortype: 0x%lX\n", two_more_indent.c_str(), errortype_);
  fprintf(inYAML, "%smedium: %s\n", two_more_indent.c_str(), GetMedium(medium_));
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
  fprintf(outYAML, "%smedium: %s\n", two_more_indent.c_str(), GetMedium(medium_));
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
  fprintf(outJSON, "    \"last_param\" : {}\n"
                   "  } // CD info ends\n"
                   "}\n"
         );
  fclose(outJSON);
  outJSON = NULL;
  
}

static int adjust_tab = 1;
void PhaseNode::PrintOutputJsonInternal(void) 
{
  int tabsize = level_ * 2  + adjust_tab;
  std::string indent((tabsize)<<1, ' ');
  std::string one_more_indent((tabsize+1)<<1, ' ');
  std::string two_more_indent((tabsize+2)<<1, ' ');
//<<<<<<< HEAD

//  fprintf(outYAML, "%s\"CD_%u_%u\" : {\n",               indent.c_str(), level_, phase_);

//  fprintf(outYAML, "%s\"label\" : %s\n",        two_more_indent.c_str(), label_.c_str());
//  fprintf(outYAML, "%s\"interval\" : %ld\n",    two_more_indent.c_str(), interval_);
//  fprintf(outYAML, "%s\"errortype\" : 0x%lX\n", two_more_indent.c_str(), errortype_);
//  //YKWON: added the information about siblings
//  fprintf(outYAML, "%s\"siblingID\" : %8u\n",  two_more_indent.c_str(), sibling_id_);
//  fprintf(outYAML, "%s\"sibling #\" : %8u\n",  two_more_indent.c_str(), sibling_size_);
//  // This print exec_cnt_cnt, reex_cnt, tot_time, reex_time, vol_copy, vol_refer
//  // comm_log and error_ven, which are also printed in profile.out.

//  fprintf(outYAML, "%s", profile_.GetRTInfoStr(tabsize + 1).c_str());
//  fprintf(outYAML, "%s\"ChildCDs\" : {\n", two_more_indent.c_str());
//=======
  //TODO: lable may be better for CD name instead of level and phase
  fprintf(outJSON, "%s\"CD_%u_%u\" : {\n",      one_more_indent.c_str(), level_, phase_);
  //TODO: the estimator does not require "label".
  fprintf(outJSON, "%s\"label\"    : %s,\n",    two_more_indent.c_str(), label_.c_str());
  fprintf(outJSON, "%s\"interval\" : %ld,\n",   two_more_indent.c_str(), interval_);
  fprintf(outJSON, "%s\"errortype\": 0x%lX,\n", two_more_indent.c_str(), errortype_);
  fprintf(outJSON, "%s\"medium\"   : %s\n",     two_more_indent.c_str(), GetMedium(medium_));
  fprintf(outJSON, "%s\"siblingID\" : %8u,\n",  two_more_indent.c_str(), sibling_id_);
  fprintf(outJSON, "%s\"sibling #\" : %8u,\n",  two_more_indent.c_str(), sibling_size_);
  std::ostringstream oss; 
  cd_prof_map[phase_].PrintJSON(oss, two_more_indent);
  // TODO: let's change to be in B/ MB/ GB
  profile_.GetPrvDetails(oss, two_more_indent);
//  cout << " DEBUG 22!!! \n" << oss.str() << endl; 
//  getchar();

  fprintf(outJSON, "%s", oss.str().c_str());
  //fprintf(outJSON, "%s", profile_.GetRTInfoStr(tabsize + 1).c_str());
  fprintf(outJSON, "%s\"ChildCDs\" : {\n", two_more_indent.c_str());

  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->PrintOutputJsonInternal();
//    if(it != children_.end() - 1)
//      fprintf(outJSON, ",\n");
//    else
//      fprintf(outJSON, "\n");
  }
  fprintf(outJSON, "%s} // ChildCDs ends\n", two_more_indent.c_str());
  fprintf(outJSON, "%s}", one_more_indent.c_str());
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
void PhaseNode::PrintNode(bool print_details, FILE *outfile)
{
  FILE *output_fp = outfile;
  std::string indent((level_)<<1, ' ');
  std::string one_more_indent((level_+1)<<1, ' ');
  fprintf(output_fp, "%sCD_%u_%u\n",                indent.c_str(), level_, phase_);
  fprintf(output_fp, "%s{\n",                       indent.c_str());
  fprintf(output_fp,   "%slabel      :%s\n",   one_more_indent.c_str(), label_.c_str());
  fprintf(output_fp,   "%sseq ID(%ld~%ld) :%ld(%ld) // # executions\n", one_more_indent.c_str(), seq_begin_, seq_end_, seq_acc_, seq_acc_rb_);
  fprintf(output_fp,   "%sstate      :%8u\n",  one_more_indent.c_str(), state_);
  fprintf(output_fp,   "%sinterval   :%8ld\n", one_more_indent.c_str(), interval_);
  if(print_details) {
    fprintf(output_fp, "%serrortype  :0x%lX\n", one_more_indent.c_str(), errortype_);
    fprintf(output_fp, "%smedium     :%s\n",    one_more_indent.c_str(), GetMedium(medium_));
    fprintf(output_fp, "%ssiblingID  :%8u\n",  one_more_indent.c_str(), sibling_id_);
    fprintf(output_fp, "%ssibling #  :%8u\n",  one_more_indent.c_str(), sibling_size_);
    fprintf(output_fp, "%stask ID    :%8u\n",  one_more_indent.c_str(), task_id_);
    fprintf(output_fp, "%stask #     :%8u\n",  one_more_indent.c_str(), task_size_);
    fprintf(output_fp, "%scount(tune):%8ld\n", one_more_indent.c_str(), count_);
    fprintf(output_fp, "%scount(cd)  :%8ld // # executions\n", one_more_indent.c_str(), seq_acc_);
    fprintf(output_fp, "%s# recreated:%8ld // # rexecutions\n", one_more_indent.c_str(), seq_acc_rb_);
    fprintf(output_fp, "%s", profile_.GetRTInfoStr(level_+1, RuntimeInfo::kPROF).c_str());
  }
}
void PhaseNode::Print(bool print_details, bool first, FILE *outfile) 
{
  if(first) {
//    assert(outAll == NULL);
//    string &filepath = cd::output_basepath;
//    printf("out yaml filepath:%s , %s\n", filepath.c_str(), std::string(CD_DEFAULT_OUTPUT_PROFILE).c_str());
//    filepath += std::string(CD_DEFAULT_OUTPUT_PROFILE);
//    outAll = fopen(filepath.c_str(), "a");
//    printf("profile out filepath:%s , %s\n", cd::output_basepath.c_str(), std::string(CD_DEFAULT_OUTPUT_PROFILE).c_str());
//    outAll = fopen((cd::output_basepath + std::string(CD_DEFAULT_OUTPUT_PROFILE)).c_str(), "a");
//    
    memset(output_filepath, '\0', 256);
    sprintf(output_filepath, "%s/%s", output_basepath, CD_DEFAULT_OUTPUT_PROFILE);
//    printf("[%s] %s\n", __func__, output_filepath);
    if(outfile == NULL) { // default
      outAll = fopen(output_filepath, "w+");
    } else {
      outAll = outfile;
    }
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
    fprintf(outAll, "%smedium     :%s\n",   one_more_indent.c_str(), GetMedium(medium_));
    fprintf(outAll, "%ssiblingID  :%8u\n",  one_more_indent.c_str(), sibling_id_);
    fprintf(outAll, "%ssibling #  :%8u\n",  one_more_indent.c_str(), sibling_size_);
    fprintf(outAll, "%stask ID    :%8u\n",  one_more_indent.c_str(), task_id_);
    fprintf(outAll, "%stask #     :%8u\n",  one_more_indent.c_str(), task_size_);
    fprintf(outAll, "%scount(tune):%8ld\n", one_more_indent.c_str(), count_);
    fprintf(outAll, "%scount(cd)  :%8ld // # executions\n", one_more_indent.c_str(), seq_acc_);
//    fprintf(outAll, "%s# recreated:%8ld // # rexecutions\n", one_more_indent.c_str(), seq_acc_rb_);
    
    std::ostringstream oss; 
    cd_prof_map[phase_].Print(oss, one_more_indent, "\n");
//    profile_.GetPrvDetails(oss, one_more_indent);
//    cout << " DEBUG!!! \n" << oss.str() << endl;
//    getchar();
    fprintf(outAll, "%s", oss.str().c_str());
//    fprintf(outAll, "%s", profile_.GetRTInfoStr(level_+1, RuntimeInfo::kPROF).c_str());
  }
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->Print(print_details, false);
  }
  fprintf(outAll, "%s}\n", indent.c_str());
  if(first) {
    if(outfile == NULL) { // default
      fclose(outAll);
    } else {
      fflush(outAll);
    }
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
  if(tuned::phaseNodeCache.empty() == false) {
    auto pt = tuned::phaseNodeCache.find(phase);
    if(pt == tuned::phaseNodeCache.end()) {
      for(auto it=tuned::phaseNodeCache.begin(); it!=tuned::phaseNodeCache.end(); ++it) {
        printf("[%d] phase %u \n", cd::myTaskID, it->first);
      }
      ERROR_MESSAGE("Phase %u is missing in tuned::phaseNodeCache (%zu)\n", 
          phase, tuned::phaseNodeCache.size());
    }
    const PhaseNode *pn = pt->second;
    cd::phaseTree.current_->errortype_ = pn->errortype_;
    cd::phaseTree.current_->medium_    = pn->medium_;
//      printf("(%s, %lx) <- (%s, %lx)\n", 
//          GetMedium(cd::phaseTree.current_->medium_),
//          cd::phaseTree.current_->errortype_, 
//          GetMedium(pn->medium_), 
//          pn->errortype_);
  } else {
//    printf("it is empty?\n");
  }
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

uint32_t PhaseTree::Init(uint64_t level,  const std::string &label)
{ 
  root_ = new PhaseNode(NULL, level, label, kExecution); 
  current_ = root_;
  cd::phaseNodeCache[current_->phase_] = root_;
  return current_->phase_; 
}


PhaseTree::~PhaseTree() {
  // If root_ == NULL,
  // phaseTree is not created.
  if(root_ != NULL) {
    root_->Delete();
  }
}


void PhaseNode::GatherStats(void)
{
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->GatherStats();
  }
  
  //char buf[64];
  //printf("[%s %d] level:%u, phase:%u, taskid:%u\n", __func__, cd::myTaskID, phase_, level_, task_id_);
  //YkWON: level_ and phase_ are swapped to fix
//  printf("[%s %d] level:%u, phase:%u, taskid:%u\n", __func__, cd::myTaskID, level_,  phase_, task_id_);
  RTInfo<double> rt_info = profile_.GetRTInfo<double>();
  RTInfo<double> &rt_info_avg = common::cd_prof_map[phase_].avg_;
  RTInfo<double> &rt_info_std = common::cd_prof_map[phase_].std_;
  RTInfo<DoubleInt> rt_info_loc = profile_.GetRTInfo<DoubleInt>(cd::myTaskID);
  RTInfo<DoubleInt> &rt_info_min = common::cd_prof_map[phase_].min_;
  RTInfo<DoubleInt> &rt_info_max = common::cd_prof_map[phase_].max_;
  //RTInfo<double> &rt_info_min_rank = common::cd_prof_map[phase_].min_rank_;
  //RTInfo<double> &rt_info_max_rank = common::cd_prof_map[phase_].max_rank_;
//  int &max_rank = common::cd_prof_map[phase_].max_rank_;
//  int &min_rank = common::cd_prof_map[phase_].min_rank_;
  DoubleInt max_result;
  DoubleInt min_result;
  //int myRank    = cd::myTaskID;
  RTInfo<double> rt_info_sqsum(rt_info);
  rt_info_sqsum.DoSq();
  //sprintf(buf, "Task %d", cd::myTaskID);
  //rt_info.Print(std::cout, buf);
  //sprintf(buf, "Task %d Sq", cd::myTaskID);
  //rt_info_sqsum.Print(std::cout, buf);
//  rt_info_int.Print(buf);
//  rt_info_float.Print(buf);
  //printf("\n----------- Before receive -----------\n");
//  MPI_Reduce(&rt_info_int, &rt_info_int_recv, sizeof(RTInfoInt)/sizeof(uint64_t), MPI_UNSIGNED_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
//  MPI_Reduce(&rt_info_float, &rt_info_float_recv, sizeof(RTInfoFloat)/sizeof(double), MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Allreduce(&rt_info_loc, &rt_info_min, rt_info_min.Length(), MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);
  MPI_Allreduce(&rt_info_loc, &rt_info_max, rt_info_max.Length(), MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);
  //MPI_Reduce(&rt_info, &rt_info_max, sizeof(RTInfo<double>)/sizeof(double), MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  //MPI_Reduce(&rt_info, &rt_info_min_rank, sizeof(RTInfo<double>)/sizeof(double), MPI_DOUBLE, MPI_MINLOC, 0, MPI_COMM_WORLD);
  //MPI_Reduce(&rt_info, &rt_info_max_rank, sizeof(RTInfo<double>)/sizeof(double), MPI_DOUBLE, MPI_MAXLOC, 0, MPI_COMM_WORLD);
  MPI_Reduce(&rt_info, &rt_info_avg, sizeof(RTInfo<double>)/sizeof(double), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&rt_info_sqsum, &rt_info_std, sizeof(RTInfo<double>)/sizeof(double), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if(cd::myTaskID == 0){
//      profile_.SetRTInfoInt(rt_info_int_recv);
//      profile_.SetRTInfoFloat(rt_info_float_recv);
      //printf("\n----------- After receive -----------\n");

      //getchar();
     //sprintf(buf, "Task %d Max", cd::myTaskID);
     //rt_info_max.Print(std::cout, buf);
      assert(cd::totalTaskSize > 0);
      rt_info_avg.Divide(cd::totalTaskSize); // first momentum
      rt_info_std.Divide(cd::totalTaskSize); // second momentum
      RTInfo<double> rt_info_avg_sqsum(rt_info_avg);
      rt_info_avg_sqsum.DoSq();
     //sprintf(buf, "Task %d 2nd", cd::myTaskID);
     //rt_info_std.Print(std::cout, buf);
      rt_info_std -= rt_info_avg_sqsum;
      rt_info_std.DoSqrt();
     //sprintf(buf, "Task %d Avg", cd::myTaskID);
     //rt_info_avg.Print(std::cout, buf);
     //sprintf(buf, "Task %d Std", cd::myTaskID);
     //rt_info_std.Print(std::cout, buf);
     //sprintf(buf, "Task %d All\n", cd::myTaskID);
     //common::cd_prof_map[phase_].Print(std::cout, " - ", "\n", buf);
      //printf("\n----------- Done  receive -----------\n");
      //getchar();
  }
}

void PhaseTree::PrintStats(void)
{
//  root_->GatherStats();
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

//      printf("basepath:%s\n", output_basepath);
//        Print();
      root_->PrintInputYAML(true); // profile.out
      root_->PrintOutputJson();    // estimation.json
      //FIXME(YKWON): This sometimes fails to produce profile.out
      root_->Print(true, true);
//      PrintProfile();
      fprintf(stdout, "%s %p\n", __func__, root_);
    }
  }

}

void CDProfiles::Print(std::ostream &os, const std::string &head, const std::string &tail, const char *str)
{
  const int pz0 = 18;
  const int pz1 = 14;
  os << str  
  << head << std::left << std::setw(pz0) << "                         "      
                       << std::setw(pz1) << "      Min"
                       << std::setw(pz1) << "      Max"
                       << std::setw(pz1) << "      Avg"
                       << std::setw(pz1) << "      Std"
                       << tail
  << head << std::left << std::setw(pz0) << "num_exec     : "        
                       << std::setw(pz1) << min_.exec_cnt_              
                       << std::setw(pz1) << max_.exec_cnt_                
  //                     << std::setw(pz1) << "[" << min_rank_.exec_cnt_ << "] " << min_.exec_cnt_                
  //                     << std::setw(pz1) << "[" << max_rank_.exec_cnt_ << "] " << max_.exec_cnt_                  
                       << std::setw(pz1) << avg_.exec_cnt_                 
                       << std::setw(pz1) << std_.exec_cnt_                 
                       << tail
  << head << std::left << std::setw(pz0) << "num_reexec   : "      
                       << std::setw(pz1) << min_.reex_cnt_              
                       << std::setw(pz1) << max_.reex_cnt_                
                       << std::setw(pz1) << avg_.reex_cnt_               
                       << std::setw(pz1) << std_.reex_cnt_  
                       << tail

  << head << std::left << std::setw(pz0) << "vol_prv_copy : "    
                       << std::setw(pz1) << min_.prv_copy_            
                       << std::setw(pz1) << max_.prv_copy_              
                       << std::setw(pz1) << avg_.prv_copy_              
                       << std::setw(pz1) << std_.prv_copy_             
                       << tail
  << head << std::left << std::setw(pz0) << "vol_prv_ref  : "     
                       << std::setw(pz1) << min_.prv_ref_             
                       << std::setw(pz1) << max_.prv_ref_               
                       << std::setw(pz1) << avg_.prv_ref_               
                       << std::setw(pz1) << std_.prv_ref_              
                       << tail
  << head << std::left << std::setw(pz0) << "vol_restore  : "     
                       << std::setw(pz1) << min_.restore_             
                       << std::setw(pz1) << max_.restore_               
                       << std::setw(pz1) << avg_.restore_               
                       << std::setw(pz1) << std_.restore_              
                       << tail
  << head << std::left << std::setw(pz0) << "vol_msg_log  : " 
                       << std::setw(pz1) << min_.msg_logging_         
                       << std::setw(pz1) << max_.msg_logging_           
                       << std::setw(pz1) << avg_.msg_logging_           
                       << std::setw(pz1) << std_.msg_logging_          
                       << tail

  << head << std::left << std::setw(pz0) << "time_total   : "  
                       << std::setw(pz1) << min_.total_time_          
                       << std::setw(pz1) << max_.total_time_            
                       << std::setw(pz1) << avg_.total_time_            
                       << std::setw(pz1) << std_.total_time_           
                       << tail
  << head << std::left << std::setw(pz0) << "time_reexec  : " 
                       << std::setw(pz1) << min_.reex_time_         
                       << std::setw(pz1) << max_.reex_time_           
                       << std::setw(pz1) << avg_.reex_time_           
                       << std::setw(pz1) << std_.reex_time_          
                       << tail
  << head << std::left << std::setw(pz0) << "time_sync    : "   
                       << std::setw(pz1) << min_.sync_time_           
                       << std::setw(pz1) << max_.sync_time_             
                       << std::setw(pz1) << avg_.sync_time_             
                       << std::setw(pz1) << std_.sync_time_            
                       << tail
  << head << std::left << std::setw(pz0) << "time_preserve: "    
                       << std::setw(pz1) << min_.prv_elapsed_time_    
                       << std::setw(pz1) << max_.prv_elapsed_time_      
                       << std::setw(pz1) << avg_.prv_elapsed_time_      
                       << std::setw(pz1) << std_.prv_elapsed_time_     
                       << tail
  << head << std::left << std::setw(pz0) << "time_restore : "    
                       << std::setw(pz1) << min_.rst_elapsed_time_    
                       << std::setw(pz1) << max_.rst_elapsed_time_      
                       << std::setw(pz1) << avg_.rst_elapsed_time_      
                       << std::setw(pz1) << std_.rst_elapsed_time_     
                       << tail

  << head << std::left << std::setw(pz0) << "time_create  : " 
                       << std::setw(pz1) << min_.create_elapsed_time_ 
                       << std::setw(pz1) << max_.create_elapsed_time_   
                       << std::setw(pz1) << avg_.create_elapsed_time_   
                       << std::setw(pz1) << std_.create_elapsed_time_  
                       << tail
  << head << std::left << std::setw(pz0) << "time_destroy : "
                       << std::setw(pz1) << min_.destroy_elapsed_time_
                       << std::setw(pz1) << max_.destroy_elapsed_time_  
                       << std::setw(pz1) << avg_.destroy_elapsed_time_  
                       << std::setw(pz1) << std_.destroy_elapsed_time_ 
                       << tail
  << head << std::left << std::setw(pz0) << "time_begin   : "  
                       << std::setw(pz1) << min_.begin_elapsed_time_  
                       << std::setw(pz1) << max_.begin_elapsed_time_    
                       << std::setw(pz1) << avg_.begin_elapsed_time_    
                       << std::setw(pz1) << std_.begin_elapsed_time_   
                       << tail
  << head << std::left << std::setw(pz0) << "time_complete: " 
                       << std::setw(pz1) << min_.compl_elapsed_time_  
                       << std::setw(pz1) << max_.compl_elapsed_time_    
                       << std::setw(pz1) << avg_.compl_elapsed_time_    
                       << std::setw(pz1) << std_.compl_elapsed_time_ 
                       << tail
  << head << std::left << std::setw(pz0) << "time_advance : "
                       << std::setw(pz1) << min_.advance_elapsed_time_
                       << std::setw(pz1) << max_.advance_elapsed_time_  
                       << std::setw(pz1) << avg_.advance_elapsed_time_  
                       << std::setw(pz1) << std_.advance_elapsed_time_
                       << tail
  <<  std::endl;
}

void CDProfiles::PrintJSON(std::ostream &os, const std::string &head)
{
  os   
  << head << "\"exec\" : "            << max_.exec_cnt_             << ",\n"
  << head << "\"reexec\" : "          << max_.reex_cnt_             << ",\n"
  << head << "\"prv_copy\" : "        << max_.prv_copy_             << ",\n"
  << head << "\"prv_ref\" : "         << max_.prv_ref_              << ",\n"
  << head << "\"restore\" : "         << max_.restore_              << ",\n"
  << head << "\"msg_log\" : "         << max_.msg_logging_          << ",\n"
  << head << "\"total_time\" : "      << max_.total_time_           << ",\n"
  << head << "\"reex_time\" : "       << max_.reex_time_            << ",\n"
  << head << "\"sync_time\" : "       << max_.sync_time_            << ",\n"
  << head << "\"prv_time\" : "        << max_.prv_elapsed_time_     << ",\n"
  << head << "\"rst_time\" : "        << max_.rst_elapsed_time_     << ",\n"
  << head << "\"create_time\" : "     << max_.create_elapsed_time_  << ",\n"
  << head << "\"destroy_time\" : "    << max_.destroy_elapsed_time_ << ",\n"
  << head << "\"begin_time\" : "      << max_.begin_elapsed_time_   << ",\n"
  << head << "\"compl_time\" : "      << max_.compl_elapsed_time_   << ",\n"
  << head << "\"advance_time\" : "    << max_.advance_elapsed_time_ << ",\n"
  // min << std::endl
  << head << "\"min exec\" : "        << min_.exec_cnt_             << ",\n"
  << head << "\"min reexec\" : "      << min_.reex_cnt_             << ",\n"
  << head << "\"min prv_copy\" : "    << min_.prv_copy_             << ",\n"
  << head << "\"min prv_ref\" : "     << min_.prv_ref_              << ",\n"
  << head << "\"min restore\" : "     << min_.restore_              << ",\n"
  << head << "\"min msg_log\" : "     << min_.msg_logging_          << ",\n"
  << head << "\"min total_time\" : "  << min_.total_time_           << ",\n"
  << head << "\"min reex_time\" : "   << min_.reex_time_            << ",\n"
  << head << "\"min sync_time\" : "   << min_.sync_time_            << ",\n"
  << head << "\"min prv_time\" : "    << min_.prv_elapsed_time_     << ",\n"
  << head << "\"min rst_time\" : "    << min_.rst_elapsed_time_     << ",\n"
  << head << "\"min create_time\" : " << min_.create_elapsed_time_  << ",\n"
  << head << "\"min destroy_time\" : "<< min_.destroy_elapsed_time_ << ",\n"
  << head << "\"min begin_time\" : "  << min_.begin_elapsed_time_   << ",\n"
  << head << "\"min compl_time\" : "  << min_.compl_elapsed_time_   << ",\n"
  << head << "\"min advance_time\" : "<< min_.advance_elapsed_time_ << ",\n"
  // avg << std::endl
  << head << "\"avg exec\" : "        << avg_.exec_cnt_             << ",\n"
  << head << "\"avg reexec\" : "      << avg_.reex_cnt_             << ",\n"
  << head << "\"avg prv_copy\" : "    << avg_.prv_copy_             << ",\n"
  << head << "\"avg prv_ref\" : "     << avg_.prv_ref_              << ",\n"
  << head << "\"avg restore\" : "     << avg_.restore_              << ",\n"
  << head << "\"avg msg_log\" : "     << avg_.msg_logging_          << ",\n"
  << head << "\"avg total_time\" : "  << avg_.total_time_           << ",\n"
  << head << "\"avg reex_time\" : "   << avg_.reex_time_            << ",\n"
  << head << "\"avg sync_time\" : "   << avg_.sync_time_            << ",\n"
  << head << "\"avg prv_time\" : "    << avg_.prv_elapsed_time_     << ",\n"
  << head << "\"avg rst_time\" : "    << avg_.rst_elapsed_time_     << ",\n"
  << head << "\"avg create_time\" : " << avg_.create_elapsed_time_  << ",\n"
  << head << "\"avg destroy_time\" : "<< avg_.destroy_elapsed_time_ << ",\n"
  << head << "\"avg begin_time\" : "  << avg_.begin_elapsed_time_   << ",\n"
  << head << "\"avg compl_time\" : "  << avg_.compl_elapsed_time_   << ",\n"
  << head << "\"avg advance_time\" : "<< avg_.advance_elapsed_time_ << ",\n"
  // std << std::endl
  << head << "\"std exec\" : "        << std_.exec_cnt_             << ",\n"
  << head << "\"std reexec\" : "      << std_.reex_cnt_             << ",\n"
  << head << "\"std prv_copy\" : "    << std_.prv_copy_             << ",\n"
  << head << "\"std prv_ref\" : "     << std_.prv_ref_              << ",\n"
  << head << "\"std restore\" : "     << std_.restore_              << ",\n"
  << head << "\"std msg_log\" : "     << std_.msg_logging_          << ",\n"
  << head << "\"std total_time\" : "  << std_.total_time_           << ",\n"
  << head << "\"std reex_time\" : "   << std_.reex_time_            << ",\n"
  << head << "\"std sync_time\" : "   << std_.sync_time_            << ",\n"
  << head << "\"std prv_time\" : "    << std_.prv_elapsed_time_     << ",\n"
  << head << "\"std rst_time\" : "    << std_.rst_elapsed_time_     << ",\n"
  << head << "\"std create_time\" : " << std_.create_elapsed_time_  << ",\n"
  << head << "\"std destroy_time\" : "<< std_.destroy_elapsed_time_ << ",\n"
  << head << "\"std begin_time\" : "  << std_.begin_elapsed_time_   << ",\n"
  << head << "\"std compl_time\" : "  << std_.compl_elapsed_time_   << ",\n"
  << head << "\"std advance_time\" : "<< std_.advance_elapsed_time_ << ",\n";
}
