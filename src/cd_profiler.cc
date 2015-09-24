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

#if _PROFILER

#include "cd_path.h"
#include "cd_global.h"
#include "cd_profiler.h"
#include "rdtsc.h"
#include <cstdint>
using namespace sight;
using namespace cd;
using std::endl;

std::list<Viz*> CDProfiler::vizStack_;
//std::list<module*>     Module::mStack;
//modularApp*            Module::ma;


//std::list<flowgraph*>  HierGraph::hgStack;
flowgraph *HierGraph::fg;

//std::list<scope*>      Scope::sStack;
graph *ScopeGraph::scopeGraph;

attrIf *Attribute::attrScope;
//std::list<attrAnd*>    Attribute::attrStack;
//std::list<attr*>       Attribute::attrValStack;

//std::list<CDNode*>     CDNode::cdStack;

//std::vector<comparison*> Comparison::compTagStack;
//std::vector<int> Comparison::compKeyStack;
//std::list<comparison*> Conparison::compStack;


// The Begin/Complete pairs that have the same label are assumed that
// they are identical sequential CDs.
// If the label is different from the new CDs, 
// it means it is newly created CD or the beginning of a new phase of sequential CD.
// Now, a new phase CDs is differentiated with the label for the CDs provided by user.
// After acquiring the profile data, 
// aggregate the data to master task
// Only master task will call sight APIs.
// Aggregated data from every task belonging to one CD node
// will be averaged out, and avg value with std will be shown.
//    
// <PROFILE_NAME>   | <SEND?>
// LOOP_COUNT       |   X
// EXEC_CYCLE       |   O
// PRV_COPY_DATA    |   O
// PRV_REF_DATA     |   O
// OVERLAPPED_DATA  |   O
// SYSTEM_BIT_VECTOR|   X
void CDProfiler::StartProfile(const char *label)
{
  dbg<< "\n\t-------- Start Profile --------\n" <<endl; //dbgBreak();
  cout << "current_label_ : " << current_label_ << ", label : " << label << endl;

  if(current_label_ != label) {
    cout << "diff" << endl;
    current_label_ = label;

    // Profile starts -- ATTR | COMP | MODULE | SCOPE | CDNODE -- 
    profile_data_[current_label_] = new uint64_t(MAX_PROFILE_DATA);
  }
  
//  profile_data_[current_label_][LOOP_COUNT]++;
  

  /// Timer on
  this_point_ = rdtsc();

  // Initialize sightObj_count
//  sightObj_count_ = getSOStackDepth();
//  sightObj_mark_ = markSOStack();

#if _ENABLE_ATTR
  vizStack_.push_back(new Attribute());
#endif

#if _ENABLE_COMP
  vizStack_.push_back(new Comparison());
#endif

#if _ENABLE_MODULE
  vizStack_.push_back(new Module());
#endif

#if _ENABLE_HIERGRAPH
  vizStack_.push_back(new HierGraph());
#endif

#if _ENABLE_SCOPE
  vizStack_.push_back(new Scope());
#endif

#if _ENABLE_SCOPEGRAPH
  vizStack_.push_back(new ScopeGraph());
#endif

#if _ENABLE_CDNODE
  vizStack_.push_back(new CDNode());
#endif

}


 

void CDProfiler::InitProfile(std::string label)
{
  sibling_id_ = 0;
  level_      = 0;
  current_label_ = label;
  this_point_ = 0;
  that_point_ = 0;
  collect_profile_   = false;
  is_child_destroyed = false;
  profile_data_.clear();
}

void CDProfiler::InitViz()
{
  // Only root should call this (inside CD_Init)
  if( GetCurrentCD() != GetRootCD() ) assert(0);

//  dbg<< "\n---------------- Initialize Visualization --------------\n" <<endl;
  //dbgBreak();

  //SightInit(txt()<<"CDs", txt()<<"dbg_CDs_"<<GetCDID().node_id().color()<<"_"<< GetCDID().node_id().task_in_color() );
  SightInit(txt()<<"CDs", txt()<<"dbg_CDs_"<< GetRootCD()->GetTaskID() );

  /// Title
  dbg << "<h1>Containment Domains Profiling and Visualization</h1>" << endl;

  /// Explanation
  dbg << "Some explanation on CD runtime "<<endl<<endl;


#if _ENABLE_ATTR
  attr *attrVal = new attr("INIT", 1);
  vizStack_.push_back(attrVal);
  attrScope = new attrIf(new attrEQ("INIT", 1));
#endif

#if _ENABLE_HIERGRAPH
  HierGraph::fg = new flowgraph("CD Hierarchical Graph App");
#endif

#if _ENABLE_SCOPEGRAPH
  ScopeGraph::scopeGraph = new graph();
#endif


}

void CDProfiler::FinalizeViz(void)
{
  dbg<< "\n---------------- Finalize Visualization --------------\n" <<endl;

  // Only root should call this (inside CD_Finalize)
  if( GetCurrentCD() != GetRootCD() ) assert(0);

  // Destroy SightObj
  dbg << "reached here? becore while in FinalizeViz() "<<vizStack_.size() << endl;

#if _ENABLE_SCOPEGRAPH
  assert(scopeGraph);
  delete scopeGraph;
#endif

#if _ENABLE_HIERGRAPH
  assert(HierGraph::fg);
  delete HierGraph::fg;
#endif

#if _ENABLE_ATTR
  assert(attrScope);
  delete attrScope;

  assert(vizStack_.size() > 0);
  assert(vizStack_.back() != NULL);
  delete vizStack_.back();
  vizStack_.pop_back();
#endif

}



void CDProfiler::ClearSightObj(void)
{
  for(auto it=vizStack_.begin(); it!=vizStack_.end(); ++it) {
    delete vizStack_.back();
    vizStack_.pop_back();
  }
}

void CDProfiler::FinishProfile(void) // it is called in Destroy()
{
  dbg<< "\n\t-------- Finish Profile --------\n" <<endl;

  // outputs the preservation / detection info
  /// Timer off
  uint64_t tmp_point = rdtsc();
  that_point_ = tmp_point;

  /// Loop Count (# of seq. CDs) + 1
//  (this->profile_data_)[current_label_][LOOP_COUNT] += 1;
//  (profile_data_)[current_label_][LOOP_COUNT] = GetCurrentCD()->ptr_cd()->GetCDID().sequential_id_;
  profile_data_[current_label_][LOOP_COUNT]++;

  /// Calcualate the execution time
  (profile_data_)[current_label_][EXEC_CYCLE] += (that_point_) - (this_point_);

  ClearSightObj();
}


bool CDProfiler::CheckCollectProfile(void)
{
  return collect_profile_;
}

void CDProfiler::SetCollectProfile(bool flag)
{
  collect_profile_ = flag;
}



void CDProfiler::GetPreserveInfo(void *data, 
                            uint64_t len_in_bytes,
                            CDPreserveT preserve_mask, 
                            const char *my_name, 
                            const char *ref_name, 
                            uint64_t ref_offset, 
                            const RegenObject * regen_object, 
                            PreserveUseT data_usage)
{
  if(preserve_mask == kCopy){

    profile_data_[current_label_][PRV_COPY_DATA] += len_in_bytes;
//    if( (this->parent_ != NULL) && check_overlap(this->parent_, ref_name) ){
      /// Sequential overlapped accumulated data. It will be calculated to OVERLAPPED_DATA/PRV_COPY_DATA
      /// overlapped data: overlapped data that is preserved only via copy method.
//      profile_data_[current_label_][OVERLAPPED_DATA] += len_in_bytes;
//      dbg<<"something is weird  "<<"level is "<<this->this_cd_->level_ << "length : "<<len_in_bytes<<endl;

  } else if(preserve_mask == kRef) {

    profile_data_[current_label_][PRV_REF_DATA] += len_in_bytes;

  } else {
                                                                                                                                  
    dbg<<"prvTy is not supported currently : "<< preserve_mask<<endl;          
//    exit(-1);

  }
}


void CDProfiler::GetLocalAvg(void)
{
  dbg<<"Master CD Get Local Avg"<<endl;
//  for(int i=1; i < MAX_PROFILE_DATA-1; ++i) {
//    profile_data_[current_label_][i] /= profile_data_[current_label_][LOOP_COUNT];
//  }
}



// -------------- CD Node -----------------------------------------------------------------------------

CDNode::CDNode(void)
{
//  CDNode* cdn = new CDNode(txt()<<current_label_, txt()<<GetCDID()); 
//  this->cdStack.push_back(cdn);
//  dbg << "{{{ CDNode Test -- "<<this->this_cd_->cd_id_<<", #cdStack="<<cdStack.size()<<endl;
}

CDNode::~CDNode(void)
{
//  if(cdStack.back() != NULL){
//    dbg<<"add new info"<<endl;
///*
//    cdStack.back()->setStageNode( "preserve", "data_copy", profile_data_[current_label_]PRV_COPY_DATA]    );
//    cdStack.back()->setStageNode( "preserve", "data_overlapped", profile_data_[current_label_][OVERLAPPED_DATA]  );
//    cdStack.back()->setStageNode( "preserve", "data_ref", profile_data_[current_label_][PRV_REF_DATA]     );
//*/
//    
//    //scope s("Preserved Stats");
//    PreserveStageNode psn(txt()<<"Preserve Stage");
//    dbg << "data_copy="<<profile_data_[current_label_][PRV_COPY_DATA]<<endl;
//    dbg << "data_overlapped="<<profile_data_[current_label_][OVERLAPPED_DATA]<<endl;
//    dbg << "data_ref="<<profile_data_[current_label_][PRV_REF_DATA]<<endl;
//
//  }

//  dbg << " }}} CDNode Test -- "<<this->this_cd_->cd_id_<<", #cdStack="<<cdStack.size()<<endl;
//  assert(cdStack.size()>0);
//  assert(cdStack.back() != NULL);
//  delete cdStack.back();
//  cdStack.pop_back();
}

// -------------- Scope -------------------------------------------------------------------------------

Scope::Scope(void)
{
  std::pair<std::string, int> label = GetCurrentCD()->profiler_->label();

  /// create a new scope at each Begin() call
  s = new scope(txt()<<label.first<<", cd_id="<<GetCurrentCD()->node_id().color());
}

Scope::~Scope(void)
{
//  dbg << " >>> Scope  Test -- "<<this->this_cd_->cd_id_<<", #sStack="<<sStack.size()<<endl;
  assert(s != NULL);
  delete s;
}

ScopeGraph::ScopeGraph(void)
{
  dbg << "destroy scope" << endl;
  std::pair<std::string, int> label = GetCurrentCD()->profiler_->label();
  /// create a new scope at each Begin() call
  s = new scope(txt()<<label.first<<", cd_id="<<GetCurrentCD()->node_id().color());
  /// Connect edge between previous node to newly created node
  if(prv_s != NULL)
    scopeGraph->addDirEdge(prv_s->getAnchor(), s->getAnchor());
}

ScopeGraph::~ScopeGraph(void)
{
  dbg << "destroy scopegraph" << endl;
}


// -------------- Module ------------------------------------------------------------------------------
Module::Module(bool usr_profile_en)
{
//  CDHandle *cdh = GetCurrentCD();
  std::pair<std::string, int> label = GetCurrentCD()->profiler_->label();
  dbg<<"CreateModule call"<<endl;
  NodeID node_id = GetCurrentCD()->node_id();

  usr_profile_enable = usr_profile_en;
  if(usr_profile_enable==false) {
    dbg<<"\n[[[[Module object created]]]]"<<endl<<endl; //dbgBreak();
    m = new module( instance(txt()<<label.first<<"_"<<GetCurrentCD()->ptr_cd()->GetCDID().cd_name().GetCDName(), 1, 0), 
                    inputs(port(context("cd_id", GetCurrentCD()->ptr_cd()->GetCDID().cd_name().GetCDName(), 
                                        "sequential_id", (int)(GetCurrentCD()->ptr_cd()->GetCDID().sequential_id()),
                                        "label", label.first,
                                        "processID", (int)node_id.task_in_color()))),
                    namedMeasures("time", new timeMeasure()) );
//    mStack.push_back(m);
  }
  else {
  
//    dbg<<22222<<endl<<endl; dbgBreak();
    m = new module( instance(txt()<<label.first<<"_"<<GetCurrentCD()->ptr_cd()->GetCDID().cd_name().GetCDName(), 2, 2), 
                    inputs(port(context("cd_id", txt()<<node_id.task_in_color(), 
                                        "sequential_id", (int)(GetCurrentCD()->ptr_cd()->GetCDID().sequential_id()))),
                           port(usr_profile_input)),
                    namedMeasures("time", new timeMeasure()) );
    //this->mStack.push_back(m);
  }

//  dbg << "[[[ Module Test -- "<<this->this_cd_->cd_id_<<", #mStack="<<mStack.size()<<endl;
}


Module::~Module(void)
{
  dbg << "~Module is called" << endl;
  CDProfiler *profiler = (CDProfiler*)(GetCurrentCD()->profiler_);
  std::map<std::string, uint64_t*> profile_data = profiler->profile_data_;
  std::pair<std::string, int> label = GetCurrentCD()->profiler_->label();

  dbg<<"DestroyModule call"<<endl;
//  dbg << " ]]] Module Test -- "<<this->this_cd_->cd_id_<<", #mStack="<<mStack.size()<<endl;

  assert(m != NULL);

  dbg << "reach?" << endl;
  if(usr_profile_enable == false){
    m->setOutCtxt(0, context("data_copy=", (long)(profile_data[label.first][PRV_COPY_DATA]),
                             "data_overlapped=", (long)(profile_data[label.first][OVERLAPPED_DATA]),
                             "data_ref=" , (long)(profile_data[label.first][PRV_REF_DATA])));
  }
  else {
    m->setOutCtxt(1, usr_profile_output);
  }

  delete m;

/*
  mStack.back()->setOutCtxt(1, context("sequential id =" , (long)profile_data_[current_label_][PRV_REF_DATA],
                                       "execution cycle=", (long)profile_data_[current_label_][PRV_COPY_DATA],
                                       "estimated error rate=", (long)profile_data_[current_label_][OVERLAPPED_DATA]));
*/


  dbg << "\n [[[[[[[[[[[[ Module is deleted ]]]]]]]]]]]\n"<<endl;
}


void Module::SetUsrProfileInput(std::pair<std::string, long> name_list)
{
  AddUsrProfile(name_list.first, name_list.second, 0);
}

void Module::SetUsrProfileInput(std::initializer_list<std::pair<std::string, long>> name_list)
{
  for(auto il = name_list.begin() ;
      il != name_list.end(); ++il) {
    AddUsrProfile(il->first, il->second, 0);
  }
}

void Module::SetUsrProfileOutput(std::pair<std::string, long> name_list)
{
  AddUsrProfile(name_list.first, name_list.second, 1);
}

void Module::SetUsrProfileOutput(std::initializer_list<std::pair<std::string, long>> name_list)
{
  for(auto il = name_list.begin() ;
      il != name_list.end(); ++il) {
  
    AddUsrProfile(il->first, il->second, 1);
  }
}

void Module::AddUsrProfile(std::string key, long val, int mode)
{
  if(mode == 0) {
    usr_profile_input.add(key, val);
  }
  else if(mode == 1){
    usr_profile_output.add(key, val);
  }
  usr_profile_enable = true;
}





// -------------- Comparison --------------------------------------------------------------------------
Comparison::Comparison(void)
{

  compTag = new comparison(txt() << GetCurrentCD()->node_id().color());
//  comparison* comp = new comparison(node_id().color());
//  compStack.push_back(comp);

  //BeginComp();
//  for(vector<int>::const_iterator k=compKeyStack.begin(); 
//      k!=compKeyStack.end(); k++) {
//    compTagStack.push_back(new comparison(*k));
//  }
}

Comparison::~Comparison(void)
{
  //CompleteComp();
//  for(vector<comparison*>::reverse_iterator c=compTagStack.rbegin(); 
//      c!=compTagStack.rend(); c++) {
//    delete *c;
//  }
  assert(compTag != NULL);
  delete compTag;
//  assert(compTagStack.size()>0);
//  assert(compTagStack.back() != NULL);
//  delete compTagStack.back();
//  compTagStack.pop_back();
//  DeleteViz();

}




// -------------- HierGraph ---------------------------------------------------------------------------
HierGraph::HierGraph(void)
{
  dbg<<"CreateHierGraph call"<<endl;
  NodeID node_id = GetCurrentCD()->node_id();
  CDID   cd_id = GetCurrentCD()->ptr_cd()->GetCDID();

  fg->graphNodeStart(GetCurrentCD()->GetName(), cd_id.sequential_id(), cd_id.cd_name().GetCDName());
}


HierGraph::~HierGraph(void)
{
  fg->graphNodeEnd(GetCurrentCD()->GetName());
}

Attribute::Attribute(void)
{
  std::pair<std::string, int> label = GetCurrentCD()->profiler_->label();

  attrVal = new attr(label.first, label.second);
  attrKey = new attrAnd(new attrEQ(label.first, 1));
}

Attribute::~Attribute(void)
{
  assert(attrKey);
  delete attrKey;
  assert(attrVal);
  delete attrVal;
}

#endif
