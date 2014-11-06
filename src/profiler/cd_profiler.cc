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


#include "cd_path.h"
#include "cd_global.h"
#include "cd_profiler.h"
#include "rdtsc.h"
#include <cstdint>
using namespace sight;
using namespace cd;

std::list<module*>     Module::mStack;
modularApp*            Module::ma;


#if _ENABLE_HIERGRAPH
//std::list<flowgraph*>  HierGraph::hgStack;
flowgraph*          HierGraph::fg;
#endif

std::list<scope*>      Scope::sStack;
graph*                 ScopeGraph::scopeGraph;

attrIf*                Attribute::attrScope;
std::list<attrAnd*>    Attribute::attrStack;
std::list<attr*>       Attribute::attrValStack;

std::list<CDNode*>     CDNode::cdStack;

std::vector<comparison*> Comparison::compTagStack;
//std::vector<int> Comparison::compKeyStack;
//std::list<comparison*> Conparison::compStack;









void CDProfiler::GetProfile(const char *label)  // it is called in Begin()
{
  int onOff = 0;
  if(CDPath::GetCurrentCD()->ptr_cd()->GetCDID().sequential_id_ < 2) onOff = 1;
    cout<<"GetProfile------   \t"<<label_.first << " - " << label << endl;

  if(label_.first != label) { 
    // diff
    //cout<<"label is diff"<<endl;
    //getchar();
    SetCollectProfile(true);

    // Dynamic Method Selection
    if(label_.first != "INITIAL_LABEL")
      FinishProfile();

    // Set label. INITIAL_LABEL should be unique for one CD
    // CD name cannot be "INITIAL_LABEL"
    if(label_.first != "INITIAL_LABEL") {
      label_.first = label;
      
    }
    else { // label_.first == "INITIAL_LABEL"
      label_.first = CDPath::GetCurrentCD()->GetName();
    }
//    cout<<"start profile------"<<endl;
//    getchar();
    label_.second = onOff;
    StartProfile();
  }
  else {
    // the same exec
    //cout<<"label is the same"<<endl;
    //getchar();
    SetCollectProfile(false);
  }
}


void CDProfiler::CollectProfile(void) // it is called in Complete()
{
  if( CheckCollectProfile() ) {
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

    //Dynamic Method Selection
    
    SetCollectProfile(false); // initialize this flag for the next Begin
  }
}

 

void CDProfiler::InitProfile(std::string label)
{
  sibling_id_ = 0;
  level_      = 0;
  label_.first = label;
  this_point_ = 0;
  that_point_ = 0;
  collect_profile_   = false;
  is_child_destroyed = false;
  profile_data_.clear();
}

void CDProfiler::InitViz()
{
  // Only root should call this (inside CD_Init)
  if( CDPath::GetCurrentCD() != CDPath::GetRootCD() ) assert(0);

//  cout<< "\n---------------- Initialize Visualization --------------\n" <<endl;
  //getchar();
  
//  if(!CDPath.empty()){
//    cout<<"\nMasterCD  "<< GetCDID().level_ <<" : ("<< GetRootCD()->GetNodeID() <<", "<< GetRootCD()->GetTaskID() <<")"<<endl;
//    getchar();
//  }
//  else {
//    cout<<"\nMasterCD  "<< GetCDID().level_ <<" : ("<< 0 <<", "<< 0 <<")"<<endl;
//    getchar();
//  }

  //SightInit(txt()<<"CDs", txt()<<"dbg_CDs_"<<GetCDID().node_id().color()<<"_"<< GetCDID().node_id().task_in_color() );
  SightInit(txt()<<"CDs", txt()<<"dbg_CDs_"<< CDPath::GetRootCD()->GetTaskID() );

  /// Title
  dbg << "<h1>Containment Domains Profiling and Visualization</h1>" << endl;

  /// Explanation
  dbg << "Some explanation on CD runtime "<<endl<<endl;

  Viz* newVizObj = NULL;  
#if _ENABLE_ATTR
  newVizObj = new Attribute();
  newVizObj->Init();
  vizStack_.push_back(new Attribute());
#endif

#if _ENABLE_COMP
  newVizObj = new Comparison();
  newVizObj->Init();
  vizStack_.push_back(new Comparison());
#endif

#if _ENABLE_MODULE
  newVizObj = new Module();
  newVizObj->Init();
  vizStack_.push_back(new Module());
#endif

#if _ENABLE_HIERGRAPH
  newVizObj = new HierGraph();
  newVizObj->Init();
  vizStack_.push_back(new HierGraph());
#endif

#if _ENABLE_SCOPE
  newVizObj = new Scope();
  newVizObj->Init();
  vizStack_.push_back(new Scope());
#elif _ENABLE_SCOPEGRAPH
  newVizObj = new ScopeGraph();
  newVizObj->Init();
  vizStack_.push_back(new ScopeGraph());
#endif

#if _ENABLE_CDNODE
  newVizObj = new CDNode();
  newVizObj->Init();
  vizStack_.push_back(new CDNode());
#endif

}

void CDProfiler::FinalizeViz(void)
{
//  cout<< "\n---------------- Finalize Visualization --------------\n" <<endl;

  // Only root should call this (inside CD_Finalize)
  if( CDPath::GetCurrentCD() != CDPath::GetRootCD() ) assert(0);

  // Destroy SightObj
  while( !vizStack_.empty() ) {
    assert(vizStack_.size()>0);
    assert(vizStack_.back() != NULL);
    vizStack_.back()->Finalize();
    delete vizStack_.back();
    vizStack_.pop_back();
  }
}


void CDProfiler::StartProfile()
{

  cout<< "\n\t-------- Start Profile --------\n" <<endl; getchar();

  profile_data_[label_.first] = new uint64_t(MAX_PROFILE_DATA);
// Profile starts -- ATTR | COMP | MODULE | SCOPE | CDNODE -- 
  /// Timer on
  this->this_point_ = rdtsc();

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
#elif _ENABLE_SCOPEGRAPH
  vizStack_.push_back(new ScopeGraph());
#endif

#if _ENABLE_CDNODE
  vizStack_.push_back(new CDNode());
#endif

}



void CDProfiler::FinishProfile(void) // it is called in Destroy()
{

  //cout<< "\n\t-------- Finish Profile --------\n" <<endl;
  //getchar();

// Profile starts -- COMP | MODULE | SCOPE | CDNODE -- 

  // outputs the preservation / detection info
  /// Timer off
  uint64_t tmp_point = rdtsc();
  that_point_ = tmp_point;

  /// Loop Count (# of seq. CDs) + 1
//  (this->profile_data_)[label_.first][LOOP_COUNT] += 1;
  (profile_data_)[label_.first][LOOP_COUNT] = CDPath::GetCurrentCD()->ptr_cd()->GetCDID().sequential_id_;

  /// Calcualate the execution time
  (profile_data_)[label_.first][EXEC_CYCLE] += (that_point_) - (this_point_);

  // Destroy SightObj
  while( !vizStack_.empty() ) {
    assert(vizStack_.size()>0);
    assert(vizStack_.back() != NULL);
    vizStack_.back()->Destroy();
    delete vizStack_.back();
    vizStack_.pop_back();
  }
}


bool CDProfiler::CheckCollectProfile(void)
{
  return collect_profile_;
}

void CDProfiler::SetCollectProfile(bool flag)
{
  collect_profile_ = flag;
}



void CDProfiler::GetPrvData(void *data, 
                            uint64_t len_in_bytes,
                            CDPreserveT preserve_mask, 
                            const char *my_name, 
                            const char *ref_name, 
                            uint64_t ref_offset, 
                            const RegenObject * regen_object, 
                            PreserveUseT data_usage)
{
  if(preserve_mask == kCopy){

    profile_data_[label_.first][PRV_COPY_DATA] += len_in_bytes;
//    if( (this->parent_ != NULL) && check_overlap(this->parent_, ref_name) ){
      /// Sequential overlapped accumulated data. It will be calculated to OVERLAPPED_DATA/PRV_COPY_DATA
      /// overlapped data: overlapped data that is preserved only via copy method.
//      profile_data_[label_.first][OVERLAPPED_DATA] += len_in_bytes;
//      cout<<"something is weird  "<<"level is "<<this->this_cd_->level_ << "length : "<<len_in_bytes<<endl;

  } else if(preserve_mask == kRef) {

    profile_data_[label_.first][PRV_REF_DATA] += len_in_bytes;

  } else {
                                                                                                                                  
    cout<<"prvTy is not supported currently : "<< preserve_mask<<endl;          
//    exit(-1);

  }
}


void CDProfiler::GetLocalAvg(void)
{
  cout<<"Master CD Get Local Avg"<<endl;
//  for(int i=1; i < MAX_PROFILE_DATA-1; ++i) {
//    profile_data_[label_.first][i] /= profile_data_[label_.first][LOOP_COUNT];
//  }
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
// -------------- CD Node -----------------------------------------------------------------------------

void CDNode::Create(void)
{
//  CDNode* cdn = new CDNode(txt()<<label_.first, txt()<<GetCDID()); 
//  this->cdStack.push_back(cdn);
//  dbg << "{{{ CDNode Test -- "<<this->this_cd_->cd_id_<<", #cdStack="<<cdStack.size()<<endl;
}

void CDNode::Destroy(void)
{
//  if(cdStack.back() != NULL){
//    cout<<"add new info"<<endl;
///*
//    cdStack.back()->setStageNode( "preserve", "data_copy", profile_data_[label_.first]PRV_COPY_DATA]    );
//    cdStack.back()->setStageNode( "preserve", "data_overlapped", profile_data_[label_.first][OVERLAPPED_DATA]  );
//    cdStack.back()->setStageNode( "preserve", "data_ref", profile_data_[label_.first][PRV_REF_DATA]     );
//*/
//    
//    //scope s("Preserved Stats");
//    PreserveStageNode psn(txt()<<"Preserve Stage");
//    dbg << "data_copy="<<profile_data_[label_.first][PRV_COPY_DATA]<<endl;
//    dbg << "data_overlapped="<<profile_data_[label_.first][OVERLAPPED_DATA]<<endl;
//    dbg << "data_ref="<<profile_data_[label_.first][PRV_REF_DATA]<<endl;
//
//  }

//  dbg << " }}} CDNode Test -- "<<this->this_cd_->cd_id_<<", #cdStack="<<cdStack.size()<<endl;
//  assert(cdStack.size()>0);
//  assert(cdStack.back() != NULL);
//  delete cdStack.back();
//  cdStack.pop_back();
}

// -------------- Scope -------------------------------------------------------------------------------

void Scope::Create(void)
{

//#if _ENABLE_ATTR
//  attrValStack.push_back(new attr(label_.first, label_.second));
//  attrStack.push_back(new attrAnd(new attrEQ(label_.first, 1)));
////   attrStack.push_back(new attrAnd(new attrEQ("txtOn", 1)));
//#endif
  
  std::pair<std::string, int> label = CDPath::GetCurrentCD()->profiler_->label();
  /// create a new scope at each Begin() call
  scope* s = new scope(txt()<<label.first<<", cd_id="<<CDPath::GetCurrentCD()->node_id().color());

#if _ENABLE_GRAPH
  /// Connect edge between previous node to newly created node
  if(this->sStack.size()>0)
    this->scopeGraph->addDirEdge(this->sStack.back()->getAnchor(), s->getAnchor());
#endif
  /// push back this node into sStack to manage scopes
  this->sStack.push_back(s);
//  dbg << "<<< Scope  Test -- "<<this->this_cd_->cd_id_<<", #sStack="<<sStack.size()<<endl;
}


void Scope::Destroy(void)
{
//  dbg << " >>> Scope  Test -- "<<this->this_cd_->cd_id_<<", #sStack="<<sStack.size()<<endl;
  assert(sStack.size()>0);
  assert(sStack.back() != NULL);
  delete sStack.back();
  sStack.pop_back();

//  assert(attrStack.size()>0);
//  assert(attrStack.back() != NULL);
//  delete attrStack.back();
//  attrStack.pop_back();
//
//  assert(attrValStack.size()>0);
//  assert(attrValStack.back() != NULL);
//  delete attrValStack.back();
//  attrValStack.pop_back();
}

// -------------- Module ------------------------------------------------------------------------------
void Module::Create(void)
{
  std::pair<std::string, int> label = CDPath::GetCurrentCD()->profiler_->label();
//  cout<<"CreateModule call"<<endl;
  NodeID node_id = CDPath::GetCurrentCD()->node_id();
  if(usr_profile_enable==false) {
//    cout<<11111<<endl<<endl; getchar();
    module* m = new module( instance(txt()<<CDPath::GetCurrentCD()->GetName(), 1, 1), 
                            inputs(port(context("cd_id", (int)node_id.color(), 
                                                "sequential_id", (int)(CDPath::GetCurrentCD()->ptr_cd()->GetCDID().sequential_id()),
                                                "label", label.first,
                                                "processID", (int)node_id.task_in_color()))),
                            namedMeasures("time", new timeMeasure()) );
    mStack.push_back(m);
  }
  else {
  
//    cout<<22222<<endl<<endl; getchar();
    module* m = new module( instance(txt()<<label.first<<"_"<<node_id.color(), 2, 2), 
                            inputs(port(context("cd_id", txt()<<node_id.task_in_color(), 
                                                "sequential_id", (int)(CDPath::GetCurrentCD()->ptr_cd()->GetCDID().sequential_id()))),
                                   port(usr_profile_input)),
                            namedMeasures("time", new timeMeasure()) );
    this->mStack.push_back(m);
  }

//  dbg << "[[[ Module Test -- "<<this->this_cd_->cd_id_<<", #mStack="<<mStack.size()<<endl;
}


void Module::Destroy(void)
{
  CDProfiler *profiler = (CDProfiler*)(CDPath::GetCurrentCD()->profiler_);
  std::map<std::string, uint64_t*> profile_data = profiler->profile_data_;
  std::pair<std::string, int> label = CDPath::GetCurrentCD()->profiler_->label();
//  cout<<"DestroyModule call"<<endl;
//  dbg << " ]]] Module Test -- "<<this->this_cd_->cd_id_<<", #mStack="<<mStack.size()<<endl;
  assert(mStack.size()>0);
  assert(mStack.back() != NULL);
  mStack.back()->setOutCtxt(0, context("data_copy=", (long)(profile_data[label.first][PRV_COPY_DATA]),
                                       "data_overlapped=", (long)(profile_data[label.first][OVERLAPPED_DATA]),
                                       "data_ref=" , (long)(profile_data[label.first][PRV_REF_DATA])));
  if(usr_profile_enable) {
    mStack.back()->setOutCtxt(1, usr_profile_output);
  }
/*
  mStack.back()->setOutCtxt(1, context("sequential id =" , (long)profile_data_[label_.first][PRV_REF_DATA],
                                       "execution cycle=", (long)profile_data_[label_.first][PRV_COPY_DATA],
                                       "estimated error rate=", (long)profile_data_[label_.first][OVERLAPPED_DATA]));
*/
  delete mStack.back();
  mStack.pop_back();
}

// -------------- Comparison --------------------------------------------------------------------------
void Comparison::Create(void)
{
//  compKeyStack.push_back(node_id().color());

  compTagStack.push_back(new comparison(txt() << CDPath::GetCurrentCD()->node_id().color()));
//  comparison* comp = new comparison(node_id().color());
//  compStack.push_back(comp);

  //BeginComp();
//  for(vector<int>::const_iterator k=compKeyStack.begin(); 
//      k!=compKeyStack.end(); k++) {
//    compTagStack.push_back(new comparison(*k));
//  }
}

void Comparison::Destroy(void)
{
  //CompleteComp();
//  for(vector<comparison*>::reverse_iterator c=compTagStack.rbegin(); 
//      c!=compTagStack.rend(); c++) {
//    delete *c;
//  }

  assert(compTagStack.size()>0);
  assert(compTagStack.back() != NULL);
  delete compTagStack.back();
  compTagStack.pop_back();


}




#if _ENABLE_HIERGRAPH
// -------------- HierGraph ---------------------------------------------------------------------------
//int flowgraphID=0;
//int verID=10;
void HierGraph::Create(void)
{
//  cout<<"CreateHierGraph call"<<endl;
  NodeID node_id = CDPath::GetCurrentCD()->node_id();
  CDID   cd_id = CDPath::GetCurrentCD()->ptr_cd()->GetCDID();
  fg->graphNodeStart(CDPath::GetCurrentCD()->GetName(), cd_id.sequential_id(), cd_id.cd_name().GetCDName());
//  if(usr_profile_enable==false) {
//    cout<< "Default flowgraph visualization. input#: 1, output#: 1\n"<<endl; //getchar();
//    flowgraph* hg = new flowgraph( HG_instance(txt()<<label_.first, 1, 1, flowgraphID, (int)(node_id.color()), verID), 
//                                   HG_inputs(HG_port(HG_context("cd_id", (int)(node_id.color()), 
//                                                                "sequential_id", (int)(CDPath::GetCurrentCD()->ptr_cd_->GetCDID().sequential_id_),
//                                                                "label", label_.first,
//                                                                "processID", (int)node_id.task_in_color()))),
//                               namedMeasures("time", new timeMeasure()) );
//    hgStack.push_back(hg);
//  }
//  else {
//  
//    cout<< "User-specified flowgraph visualization. input#: 2, output#: 2\n"<<endl; //getchar();
//    flowgraph* hg = new flowgraph( HG_instance(txt()<<label_.first, 2, 2, flowgraphID, (int)node_id.color(), verID), 
//                                   HG_inputs(HG_port(HG_context("cd_id", txt()<<node_id.task_in_color(), 
//                                                                "sequential_id", (int)(CDPath::GetCurrentCD()->ptr_cd_->GetCDID().sequential_id_))),
//                                             HG_port(usr_profile_input)),
//                                   namedMeasures("time", new timeMeasure()) );
//    hgStack.push_back(hg);
//  }
//
//   flowgraphID+=1;
}


void HierGraph::Destroy(void)
{
  fg->graphNodeEnd(CDPath::GetCurrentCD()->GetName());
//  cout<<"DestroyHierGraph call"<<endl;
//  assert(hgStack.size()>0);
//  assert(hgStack.back() != NULL);
//  hgStack.back()->setOutCtxt(0, HG_context("data_copy=", (long)(profile_data_[label_.first][PRV_COPY_DATA]),
//                                        "data_overlapped=", (long)(profile_data_[label_.first][OVERLAPPED_DATA]),
//                                        "data_ref=" , (long)(profile_data_[label_.first][PRV_REF_DATA])));
//  if(usr_profile_enable) {
//    hgStack.back()->setOutCtxt(1, usr_profile_output);
//  }
///*
//  hgStack.back()->setOutCtxt(1, HG_context("sequential id =" , (long)profile_data_[label_.first][PRV_REF_DATA],
//                                       "execution cycle=", (long)profile_data_[label_.first][PRV_COPY_DATA],
//                                       "estimated error rate=", (long)profile_data_[label_.first][OVERLAPPED_DATA]));
//*/
//  delete hgStack.back();
//  hgStack.pop_back();
}
#endif

void Attribute::Create(void)
{
  
  std::pair<std::string, int> label = CDPath::GetCurrentCD()->profiler_->label();
  attrValStack.push_back(new attr(label.first, label.second));
  attrStack.push_back(new attrAnd(new attrEQ(label.first, 1)));
//   attrStack.push_back(new attrAnd(new attrEQ("txtOn", 1)));

}

void Attribute::Destroy(void)
{
  assert(attrStack.size()>0);
  assert(attrStack.back() != NULL);
  delete attrStack.back();
  attrStack.pop_back();

  assert(attrValStack.size()>0);
  assert(attrValStack.back() != NULL);
  delete attrValStack.back();
  attrValStack.pop_back();
}


