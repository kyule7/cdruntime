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

/** The current status of profiler (updated at 04.04.2014)
 *  [DONE]
 *   + Profiling Loop count (# of Sequential CDs)
 *   + Profiling Execution cycle in the body domain
 *   + Profiling Preserved data volume
 *   ++ Preserved-via-Copy volume
 *   ++ Preserved-via-Reference volume
 *   ++ Overlapped data volume among Preserved-via-Copy volume
 *
 *  [TODO]
 *   + Substitute some string for all the number information of data volume or loop count,
 *     and put those number separately somewhere in output config file
 *   + Involving scaling factor in profiler to scale application to run at ease for programmer
 *   + Profiling detection mechanism from bitvector
 *   + Different Phase support
 *
 *  [TODO associated with Sight]
 *   + Showing Parallel CD figure (the figure in SC12 paper, not tree shape)
 *   + Zoomable functionality in CD hierarchy graph
 *
 */

// ----------------------------------------------------------------------------------------------------

#include "cd_profile.h"
#include "rdtsc.h"
#include "sight.h"
#include <map>
#include <vector>
#include <array>
//#include <assert.h>
#include <stdint.h>
#include <string.h>
//#include <ctime>
#if CDNODE_VIZ
#define NODE_VIZ 1
#else
#define NODE_VIZ 0
#endif


//-- Profiler (cd_profiler.cc) -------------------------------------------------------

using namespace std;
using namespace sight;

namespace cd {



//-- Global Variable Definition --------------------------------------------------------
/// cd_info : LOOP_COUNT, EXEC_CYCLE, PRV_COPY_DATA, PRV_REF_DATA, OVERLAPPED_DATA, BANDWIDTH, ERROR_RATE
std::map<uint64_t, std::map<uint64_t,uint64_t*> > cd_info;
CDHandle* root;
uint64_t profile_version = 0;
uint64_t total_profile[MAX_PROFILE_DATA] =  {0,};

/// CD ID and Level Generation (Temporary for profiler)
uint64_t cd_id_gen;

//-- Class Member Function Definition ------------------------------------------------

/// class CD
CD::CD(void)  
{
  cd_type_ = kStrict;  
  cd_execution_mode_ = kExec;
}
CD::CD(CDType cd_type)
{
  cd_type_  = cd_type;
  cd_execution_mode_ = kExec;
}
CD::~CD()
{ 
  ///---cout<<"CD destroied"<<endl;
}

/** class CDEntry
*
*/
CDEntry::CDEntry(void* data_p, uint64_t len_in_bytes, PreserveType prvTy, const std::string& data_name)
          : data_p_(data_p), len_in_bytes_(len_in_bytes), prvTy_(prvTy), data_name_(data_name)
{ 
  ///---cout<<"CDEntry constructed"<<endl; 
}

CDEntry::~CDEntry() 
{
  ///---cout<<"CDEntry destroyed"<<endl;
}

//------ CDHandleCore ---------------------------------------------------------------

CDHandleCore::CDHandleCore()
{
  ptr_cd_ = new CD(kStrict);
  parent_cd_ = nullptr;
  level_ = 0;
  // id info
  cd_id_gen++;
  cd_id_   = cd_id_gen;
  seq_id_  =  0;
  task_id_ = cd_id_gen;
  process_id_ = cd_id_gen*4; 

//  cout<<"Construct CDHandleCore: "<<cd_id_<< endl;
}

CDHandleCore::CDHandleCore(CDHandleCore* parent_core, CDType cd_type, uint64_t parent_level)
{
  ptr_cd_ = new CD(cd_type);
  parent_cd_ = parent_core;
  level_ = parent_level+1;
  // id info
  cd_id_gen++;
  cd_id_   = cd_id_gen;
  seq_id_  =  0;
  task_id_ = cd_id_gen;
  process_id_ = cd_id_gen*4; 

//  cout<<"Construct CDHandleCore: "<<cd_id_<< endl;
}

CDHandleCore* CDHandleCore::Create(CDType cd_type, CDErrType* cd_err_t) {

  CDHandleCore* new_cd_core = new CDHandleCore(this, cd_type, this->level_);
  return new_cd_core;
}
CDHandleCore::~CDHandleCore()
{
//  cout<<"Delete CDHandleCore: "<<cd_id_<< endl;
  //getchar();
  delete ptr_cd_;
  ///---cout<<"Destroy this CDHandle of level "<<this->level_<<endl;

//  cout<<"CDHandleCore Destroy call"<<endl;
  //cd_info[this->level_].pop_back();                                //  FIXME
}
CDErrType CDHandleCore::Destroy()
{
  delete this;
  return kOK;
}

/** CDHandleCore::Begin()
 */
CDErrType CDHandleCore::Begin()
{
  return kOK;
}

/** CDHandleCore::Complete()
 */
CDErrType CDHandleCore::Complete()
{
  /// Increase the seq. ID by one
  this->IncSeqID();

// It deletes entry directory in the CD. 
// We might modify this in the profiler to support the overlapped data among sequential CDs.
  for(auto it = this->ptr_cd_->entry_directory_.begin(); it != this->ptr_cd_->entry_directory_.end(); ++it){
    delete it->second;
  }      

  return kOK;
}

CDErrType CDHandleCore::Preserve(void* data_p, uint64_t len_in_bytes, PreserveType prvTy, std::string ref_name)
{
  this->ptr_cd_->entry_directory_[ref_name] = new CDEntry(data_p, len_in_bytes, prvTy, ref_name);

  return kOK;
}

/*  CD::Detect()
 */
CDErrType CDHandleCore::Detect()
{
  return kOK;
}

// ------------- CDHandle ----------------------------------------------------------

  // CDHandle
  //check if the node is already created before,
  //if not, create a new node.
  //if it is, pass
  //like it populate profile_data_ in the CDHandle object,
  //I think it should populate CDHandleNod it the CDHandle object.
CDHandle::CDHandle()
{
  this_cd_ = new CDHandleCore(); 
  parent_ = nullptr;  // Root has no parent
  sibling_id_ = 0;    // Root has no sibling. There exists only one CD in level 0
//children_id_;

//  if(parent_ != nullptr) {
//      (parent_->children_)[sibling_id_] = this;
//  }
  level_ = this_cd_->level_;
  cd_info[level_][sibling_id_] = profile_data_;  //cd_info has the pointer of profile_data_

  for(int i=0; i<MAX_PROFILE_DATA; ++i) {
    profile_data_[i] = 0;
  }
  children_.clear();
//  cout<<"level (map size) : "<< cd_info.size() << "sibling (map size) : "<<cd_info[level_].size() <<endl;
  
//  cout<<"level (map size) : "<< cd_info.size() << "sibling (map size) : "<<this->children_.size() <<endl;
//getchar();
}

CDHandle::CDHandle(CDHandle* parent, CDType cd_type, uint64_t level)
{
  this_cd_ = new CDHandleCore(parent->this_cd_, cd_type, level); //Should be cdproxy
//  this_cd_ = parent->this_cd_->Create(cd_type, cd_err_t, level);
  parent_ = parent;
  sibling_id_ = parent_->children_.size();  // Getting sibling_id_ from parent
//children_id_;
  if(parent_ != nullptr) {
      (parent_->children_)[sibling_id_] = this;
  }
  //cout<<parent_->children_.size()<<endl;
  //getchar();
  
  level_ = this_cd_->level_;
  cd_info[level_][sibling_id_] = profile_data_;  //cd_info has the pointer of profile_data_

  for(int i=0; i<MAX_PROFILE_DATA; ++i) {
    profile_data_[i] = 0;
  }
  children_.clear();

//  cout<<"CDHandle Constructor"<<endl;
//  cout<<"level (map size) : "<< cd_info.size() << "\tsibling (map size) : "<<cd_info[level_].size() <<endl;

//  cout<<"level (map size) : "<< cd_info.size() << "\tsibling (map size) : "<<this->children_.size() <<endl;
//getchar();
}
CDHandle::~CDHandle()
{
  ///---cout<<"Delete CDHandle ID: "<<cd_id_<< endl;
  //delete this_cd_;
  ///---cout<<"Destroy this CDHandle of level "<<this->level_<<endl;

  //cd_info[this->level_].pop_back();     //  FIXME
//  cout<<"CDHandle Destroy call"<<endl;
  //getchar();
  if(parent_ != nullptr) {
    (parent_->children_).erase(sibling_id_);
  }
}

/** CD::Create()
 * List of profile for CD information
 *  (1) CD tree skeleton generation
 *      - The nodes and their relations (parent, sibling children, ...)
 */
CDHandle* CDHandle::Create(CDType cd_type, CDErrType* cd_err_t)
{
  if(this->is_destroyed_ != true) {
//    cout<<"\n\ncreate first time\n\n"<<endl;
    CDHandle* new_cd = new CDHandle(this, cd_type, this->this_cd_->level_); //Should be cdproxy
    return new_cd;
  } else {

//    cout<<"\n\ncreate second time\n\n"<<endl;

//    cout<<"Construct CDHandleCore: "<<this->this_cd_->cd_id_<< endl;
//    CDHandleCore* new_cd_core = this->this_cd_->Create(cd_type, cd_err_t);
    CDHandleCore* new_cd_core = new CDHandleCore(this->this_cd_, cd_type, this->this_cd_->level_);
//    this->this_cd_ = new_cd_core;

  for(auto it = this->children_.begin(); it != this->children_.end(); ++it) {
    if(it->second->this_cd_ == nullptr){
      it->second->this_cd_ = new_cd_core;
//      cout<<"--  "<<it->first<<"th new_cd_core was re_created.!!!\n"<<endl;
      return &*(it->second);
    }
  }
  cout<<"something is wrong in the CDHandle::Create()"<<endl;
  exit(-1);

//  this_cd_ = new CDHandleCore(parent->this_cd_, cd_type, level); //Should be cdproxy
//  this_cd_ = parent->this_cd_->Create(cd_type, cd_err_t, level);
//  sibling_id_ = parent_->children_.size();  // Getting sibling_id_ from parent
//children_id_;
//  if(parent_ != nullptr) {
//      (parent_->children_)[sibling_id_] = this;
//  }
//  cout<<parent_->children_.size()<<endl;
//  getchar();
//  uint64_t level_ = this_cd_->level_;


//    return this;
  }

//  this_cd_ = new CDHandleCore(parent->this_cd_, cd_type, level); //Should be cdproxy
}



/** CD::Destroy()
 *  Disconnect parent child relationship here if possible.
 */
CDErrType CDHandle::Destroy()
{
  ///cout<<"# of siblings: "<< this->parent_cd_->children_.size()<<endl;
  if(this == GetRoot()) { Finalize(); }
  if(this->parent_ != nullptr) { this->parent_->is_destroyed_ = true; }
//  cout<<"CDHandle Destroy call and CDHandleCore Destroy will be called.\t"<<this->this_cd_->cd_id_<<endl;
  this->this_cd_->Destroy();
  this->this_cd_ = nullptr;

  return kOK;
}

/** CDHandle::Begin()
 */
CDErrType CDHandle::Begin()
{
  /// Timer on
  //time(&(this->this_point_));
  this->this_point_ = rdtsc();
  return this_cd_->Begin();
}


/** CDHandle::Complete()
 */
CDErrType CDHandle::Complete()
{
  /// Timer off
  uint64_t tmp_point = rdtsc();
  this->that_point_ = tmp_point;

  /// Loop Count (# of seq. CDs) + 1
  (this->profile_data_)[LOOP_COUNT] += 1;

  /// Calcualate the execution time
  (this->profile_data_)[EXEC_CYCLE] += (this->that_point_) - (this->this_point_);

  return this_cd_->Complete();
}

CDErrType CDHandle::Preserve(void* data_p, uint64_t len_in_bytes, PreserveType prvTy, std::string ref_name)
{
  ///ptr_cd_->entry_directory_[ref_name] = new CDEntry(data_p, len_in_bytes, prvTy, ref_name);

  /// Preserve meta-data
  /// Accumulated volume of data to be preserved for Sequential CDs. 
  /// It will be averaged out with the number of seq. CDs.
  if(prvTy == kCopy){

    (this->profile_data_)[PRV_COPY_DATA] += len_in_bytes;

    if( (this->parent_ != nullptr) && check_overlap(this->parent_, ref_name) ){
       /// Sequential overlapped accumulated data. It will be calculated to OVERLAPPED_DATA/PRV_COPY_DATA
      /// overlapped data: overlapped data that is preserved only via copy method.
      (this->profile_data_)[OVERLAPPED_DATA] += len_in_bytes;
//      cout<<"something is weird  "<<"level is "<<this->this_cd_->level_ << "length : "<<len_in_bytes<<endl;
    }

  } else if(prvTy == kRef) {

    (this->profile_data_)[PRV_REF_DATA] += len_in_bytes;

  } else {
    
    cout<<"prvTy is not supported currently : "<< prvTy<<endl;      
    exit(-1);

  }
  ///---cout<<"being Preserved right before return"<<endl;
  return this_cd_->Preserve(data_p, len_in_bytes, prvTy, ref_name);
}

/*  CD::Detect()
 *  (1) Estimate the execution time of detection routine.  
 *  (2) 
 */
CDErrType CDHandle::Detect()
{
  return this_cd_->Detect();
}


void CDHandle::Print_Graph(){
  // Generate CD Tree Graph using Sight
//  if((this->profile_data_) != cd_info[this->this_cd_->level_][0]) { return; }  // Print when it is only children #0 

  CDHandle* cd_root = GetRoot();
  
  dbg <<   "<h2>CD Graph corresponding to profile output"<<
//          "cd_profile_"<< this->level_<<'_'<<profile_version<< ".conf"<<
          "cd_profile"<<
          "  [# of level = "<<cd_info.size()<<"]</h2>";
  graph cd_graph;
//if( this != cd_root ){
  attr choice("hide", NODE_VIZ);
  scope start_node( txt()<<"CD_"<< cd_root->level_ << '_' << cd_root->sibling_id_<<
      "\n"<< "Loop count: "<<(cd_root->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Exec cycle: "<<(cd_root->profile_data_)[EXEC_CYCLE]/(cd_root->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Preserved data: "<<(cd_root->profile_data_)[PRV_COPY_DATA]/(cd_root->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Overlapped data: "<<(cd_root->profile_data_)[OVERLAPPED_DATA]/(cd_root->profile_data_)[LOOP_COUNT],
                    attrEQ("hide", 1)  );

  cout<<  "LOOP COUNT: "<<(cd_root->profile_data_)[LOOP_COUNT]<<"\n"<<
          "EXEC CYCLE: "<<(cd_root->profile_data_)[EXEC_CYCLE]<<"\n"<<
          "PRESERVED DATA: "<<(cd_root->profile_data_)[PRV_COPY_DATA]<<"\n"<<
          "OVERLAPPED DATA: "<<(cd_root->profile_data_)[OVERLAPPED_DATA]<<"\n"<<endl;

  for(auto iter = (cd_root->children_).begin(); iter != (cd_root->children_).end(); ++iter)
  {
    attr choice("hide", NODE_VIZ);
    scope child_node(txt()<<"CD_"<< (iter->second)->level_ << '_' << (iter->second)->sibling_id_<< 
      "\n"<< "Loop count: "<<(iter->second->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Exec cycle: "<<(iter->second->profile_data_)[EXEC_CYCLE] / (iter->second->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Preserved data: "<<(iter->second->profile_data_)[PRV_COPY_DATA]/(iter->second->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Overlapped data: "<<(iter->second->profile_data_)[OVERLAPPED_DATA]/(iter->second->profile_data_)[LOOP_COUNT]      
      , attrEQ("hide", 1)    );

  cout<< "LOOP COUNT: "<<(iter->second->profile_data_)[LOOP_COUNT]<<"\n"<<
        "EXEC CYCLE: "<<(iter->second->profile_data_)[EXEC_CYCLE]<<"\n"<<
        "PRESERVED DATA: "<<(iter->second->profile_data_)[PRV_COPY_DATA]<<"\n"<<
        "OVERLAPPED DATA: "<<(iter->second->profile_data_)[OVERLAPPED_DATA]<<"\n"<<endl;

    
    cd_graph.addDirEdge( start_node.getAnchor(), child_node.getAnchor() );
    this->Print_Graph(iter->second, cd_graph, child_node);  // Recursive call start!
  }
/*  
} else {
  
  attr choice("hide", NODE_VIZ);
  scope empty_node(txt()<<"Empty", attrEQ("hide", 1) );
  dbg<<"This is the beginning point of the CD";
  
  {
  attr choice("hide", NODE_VIZ);
  scope start_node(txt()<<"CD_"<< cd_root->this_cd_->level_ << '_' << cd_root->sibling_id_<<  
      "\n"<< "Loop count: "<<(this->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Exec cycle: "<<(this->profile_data_)[EXEC_CYCLE]/(this->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Preserved data: "<<(this->profile_data_)[PRV_COPY_DATA]/(this->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Overlapped data: "<<(this->profile_data_)[OVERLAPPED_DATA]/(this->profile_data_)[LOOP_COUNT]      
      , attrEQ("hide", 1)  );

  dbg<<  "LOOP COUNT: "<<(this->profile_data_)[LOOP_COUNT]<<"\n"<<
          "EXEC CYCLE: "<<(this->profile_data_)[EXEC_CYCLE]<<"\n"<<
          "PRESERVED DATA: "<<(this->profile_data_)[PRV_COPY_DATA]<<"\n"<<
          "OVERLAPPED DATA: "<<(this->profile_data_)[OVERLAPPED_DATA]<<"\n"<<endl;

  cd_graph.addDirEdge(empty_node.getAnchor(), start_node.getAnchor());
  }

}
*/
}
void CDHandle::Print_Graph(CDHandle* current_cdh, graph& cd_graph, scope& cd_node){
  if ( !((current_cdh->children_).empty()) ) {
    for(auto iter = (current_cdh->children_).begin(); iter != (current_cdh->children_).end(); ++iter)
    {
      attr choice("hide", NODE_VIZ);
      scope child_node(
      txt()<<"CD_"<<
      (iter->second)->level_<<'_'<<(iter->second)->parent_->sibling_id_<<'_'<< (iter->second)->sibling_id_<<
      "\n"<< "Loop count: "<<(iter->second->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Exec cycle: "<<(iter->second->profile_data_)[EXEC_CYCLE]/(iter->second->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Preserved data: "<<(iter->second->profile_data_)[PRV_COPY_DATA]/(iter->second->profile_data_)[LOOP_COUNT]<<
      "\n"<< "Overlapped data: "<<(iter->second->profile_data_)[OVERLAPPED_DATA]/(iter->second->profile_data_)[LOOP_COUNT]      
      , attrEQ("hide", 1)  );
      /*
        dbg<< "LOOP COUNT: "<<(iter->second->profile_data_)[LOOP_COUNT]<<"\n"<<
              "EXEC CYCLE: "<<(iter->second->profile_data_)[EXEC_CYCLE]<<"\n"<<
              "PRESERVED DATA: "<<(iter->second->profile_data_)[PRV_COPY_DATA]<<"\n"<<
              "OVERLAPPED DATA: "<<(iter->second->profile_data_)[OVERLAPPED_DATA]<<"\n"<<endl;
      */
      // Linking the Graph
      cd_graph.addDirEdge( cd_node.getAnchor(), child_node.getAnchor() );

      this->Print_Graph(iter->second, cd_graph, child_node);  // Recursive call!!

    }
  // Termination condition
  } else {    
    
    ///---cout<<" being terminated from recurssion " <<endl;

  }

}

void CDHandle::Print_Profile(std::map<uint64_t, std::map<uint64_t,uint64_t*> >::iterator it, txt* cd_buf){
//  uint64_t total_profile[MAX_PROFILE_DATA] =  {0,};
  uint64_t level       =  it->first;
  uint64_t sibling_cnt = (it->second).size();
  //cout<<"------- sibling # at level "<< level << ": "<<sibling_cnt<<"-------------"<<endl;
  //cout<<"level is : "<<level<<endl;
//  cout<< total_profile
  if ( it != cd_info.end() ) {

    cd_buf[BODY]<<put_tabs(level)<<"GroupCD G"<<level<<"\n";
    cd_buf[BODY]<<put_tabs(level)<<"{\n";
    cd_buf[BODY]<<put_tabs(level)<<"\tnumber_of_elements= "<<sibling_cnt<<";\n";
    
    // jt is vector iterator
    // (*jt) is vector element ( std::array<uint64_t, MAX_PROFILE_DATA>* )
    for(auto jt = (it->second).begin(); jt != (it->second).end(); ++jt){
      /// Data size calcuation
      /// LOOP_COUNT calculation
      /// Getting per-CD information
      uint64_t* current_profile = jt->second;
      total_profile[LOOP_COUNT]     += current_profile[LOOP_COUNT]; 
      total_profile[EXEC_CYCLE]     += current_profile[EXEC_CYCLE];
      total_profile[PRV_COPY_DATA]  += current_profile[PRV_COPY_DATA];
      total_profile[PRV_REF_DATA]   += current_profile[PRV_REF_DATA];
      total_profile[OVERLAPPED_DATA]+= current_profile[OVERLAPPED_DATA];
       cout<<"loop: "        << current_profile[LOOP_COUNT]    << 
            "\texec_cycle: "<< current_profile[EXEC_CYCLE]    << 
            "\tprv: "       << current_profile[PRV_COPY_DATA] <<
            "\tref "        << current_profile[PRV_REF_DATA]  << 
            "\toverlap "    << current_profile[OVERLAPPED_DATA] << endl; 
    }

    /// Profile Formatting
    if(sibling_cnt != 0){
      cd_buf[BODY]<<put_tabs(level)<<"\tCD C" << level <<"\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t{\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\tloop\t\t\t\t\t\t= "              << total_profile[LOOP_COUNT]/sibling_cnt       <<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\tbandwidth\t\t\t\t= BW"       << level                                       <<";\n";          
      cd_buf[BODY]<<put_tabs(level)<<"\t\tinput_data_size\t= "     << total_profile[PRV_COPY_DATA]/sibling_cnt    <<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\tpreservation\t\t= P"     << level                                       <<";\n\n";

      cd_buf[PRV]<<"preservation P"           << level                                       <<"\n";
      cd_buf[PRV]<<"{\n";
      cd_buf[PRV]<<"\tinput_data_size\t\t\t\t\t= "      << total_profile[PRV_COPY_DATA]/sibling_cnt    <<";\n";
      cd_buf[PRV]<<"\tpreserved_via_ref_data\t= " << total_profile[PRV_REF_DATA]/sibling_cnt     <<";\n";
      cd_buf[PRV]<<"\toverlapped_data_size\t\t= "   << total_profile[OVERLAPPED_DATA]/sibling_cnt  <<";\n";
      cd_buf[PRV]<<"}\n\n";

      cd_buf[REC]<<"recovery R" << level  <<"\n";
      cd_buf[REC]<<"{\n";
      cd_buf[REC]<<"\tmax_try\t\t\t\t\t\t= "       << 1000    <<";\n";
      cd_buf[REC]<<"\trelative_overhead\t= " << total_profile[PRV_REF_DATA] / (total_profile[PRV_REF_DATA]+total_profile[PRV_COPY_DATA]) <<";\n";
      cd_buf[REC]<<"\t// you can mimic partial preservation overhead by adjusting this"  <<";\n";
      cd_buf[REC]<<"}\n\n";
    } else {  // not divid it by sibling_cnt
      cd_buf[BODY]<<put_tabs(level)<<"\tCD C" << level <<"\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t{\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\tloop\t\t\t\t\t\t= "              << total_profile[LOOP_COUNT]      <<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\tbandwidth\t\t\t\t= BW"       << level                          <<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\tinput_data_size\t= "     << total_profile[PRV_COPY_DATA]   <<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\tpreservation\t\t= P"     << level                          <<";\n\n";

      cd_buf[PRV]<<"preservation P"           << level                          <<"\n";
      cd_buf[PRV]<<"\t{\n";
      cd_buf[PRV]<<"\tinput_data_size\t\t\t\t\t= "      << total_profile[PRV_COPY_DATA]    <<";\n";
      cd_buf[PRV]<<"\tpreserved_via_ref_data\t= " << total_profile[PRV_REF_DATA]     <<";\n";
      cd_buf[PRV]<<"\toverlapped_data_size\t\t= "   << total_profile[OVERLAPPED_DATA]  <<";\n";
      cd_buf[PRV]<<"\t\tsize is wrong\t\t= "      << sibling_cnt                     <<";\n";
      cd_buf[PRV]<<"\t}\n\n";

      cd_buf[REC]<<"recovery R" << level  <<"\n";
      cd_buf[REC]<<"{\n";
      cd_buf[REC]<<"\tmax_try\t\t\t\t\t\t= "       << 1000    <<";\n";
      cd_buf[REC]<<"\trelative_overhead\t= " << total_profile[PRV_REF_DATA] / (total_profile[PRV_REF_DATA]+total_profile[PRV_COPY_DATA]) <<";\n";
      cd_buf[REC]<<"\t// you can mimic partial preservation overhead by adjusting this"  <<";\n";
      cd_buf[REC]<<"}\n\n";
    }
    ///---cout<<"--level: "<<level<<"--------cd_buf[BODY] printout-------------------\n"<<cd_buf[BODY]<<endl;

////detection part////////////////////////////////////////////////////////////////////////////
    if(this->profile_data_[SYSTEM_BIT_VECTOR] & 0x1 ){
      cd_buf[BODY]<<put_tabs(level)<<"\t\tdetection de"<<level<<"{\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t\terror_type\t\t\t= E"<<level<<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t\trecovery_method\t= R"<<level<<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t}\n\n";
    }
    if(this->profile_data_[SYSTEM_BIT_VECTOR] & 0x2 ){
      cd_buf[BODY]<<put_tabs(level)<<"\t\tdetection de"<<level<<"{\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t\terror_type\t\t\t= E"<<level<<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t\trecovery_method\t= R"<<level<<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t}\n\n";
    }

    //if(escalation)
    if(1){
      cd_buf[BODY]<<put_tabs(level)<<"\t\tdetection de"<<level<<"{\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t\terror_type\t\t\t= E"<<level<<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t\trecovery_method\t= R"<<level<<";\n";
      cd_buf[BODY]<<put_tabs(level)<<"\t\t}\n\n";
    }
//////////////////////////////////////////////////////////////////////////////////////////////////
    ++it;    
    
    if(it != cd_info.end() ) {
      
      this->Print_Profile(it, cd_buf);  /// Recursive call!!

    } else {  /// The last point of Recursion
      if(sibling_cnt != 0){
        cd_buf[BODY]<<put_tabs(level)<<"\t\tNonCD NCD"<<level<<"\n";
        cd_buf[BODY]<<put_tabs(level)<<"\t\t{\n";
        cd_buf[BODY]<<put_tabs(level)<<"\t\t\tlength = "<< total_profile[EXEC_CYCLE]/sibling_cnt <<";\n";
        cd_buf[BODY]<<put_tabs(level)<<"\t\t}\n";
      } else {
        cd_buf[BODY]<<put_tabs(level)<<"\tNonCD NCD"<<level<<"\n\t{\n\t\tlength="  << total_profile[EXEC_CYCLE]<<"there is something wrong with sibling_cnt" <<";\n\t}";
      }
    }

  } else {

    ///---cout<<" being terminated from recurssion " <<endl;

  }

  cd_buf[BODY]<<put_tabs(level)<<"\t}\n";  // end of level
  cd_buf[BODY]<<put_tabs(level)<<"}\n";

}



void CDHandle::Print_Profile(){
  /// Generate one file per one Profile
  /// There are some profile_version

  string filename(txt("./profile_data/cd_profile_")<< this->level_<<'_'<<profile_version++<< ".conf");
  FILE* file_p = fopen(filename.data(), "w");
  txt cd_buf[MAX_FORMAT];
  for(uint64_t i=0; i<MAX_FORMAT; ++i) {
    cd_buf[i].clear();
  }
  auto iter = cd_info.begin();
  CDHandle* cd_root = GetRoot();  
  uint64_t level = 0;
  uint64_t* current_profile = cd_root->profile_data_;

  total_profile[LOOP_COUNT]     = current_profile[LOOP_COUNT]; 
  total_profile[EXEC_CYCLE]     = current_profile[EXEC_CYCLE];
  total_profile[PRV_COPY_DATA]  = current_profile[PRV_COPY_DATA];
  total_profile[PRV_REF_DATA]   = current_profile[PRV_REF_DATA];
  total_profile[OVERLAPPED_DATA]= current_profile[OVERLAPPED_DATA];
///Buffering start/////////////////////////////////////////////////////////////////////////////

  cd_buf[BODY]<<"\tCD C" << level <<"\n";
  cd_buf[BODY]<<"\t{\n";
  cd_buf[BODY]<<"\t\tloop\t\t\t\t\t\t= "              << total_profile[LOOP_COUNT]     <<";\n";
  cd_buf[BODY]<<"\t\tbandwidth\t\t\t\t= BW"       << level <<";\n";          
  cd_buf[BODY]<<"\t\tinput_data_size\t= "     << total_profile[PRV_COPY_DATA] <<";\n";
  cd_buf[BODY]<<"\t\tpreservation\t\t= P"     << level <<";\n\n";
  
  cd_buf[PRV]<<"preservation P" << level  <<"\n";
  cd_buf[PRV]<<"{\n";
  cd_buf[PRV]<<"\tinput_data_size\t\t\t\t\t= "      << total_profile[PRV_COPY_DATA]    <<";\n";
  cd_buf[PRV]<<"\tpreserved_via_ref_data\t= " << total_profile[PRV_REF_DATA]     <<";\n";
  cd_buf[PRV]<<"\toverlapped_data_size\t\t= "   << total_profile[OVERLAPPED_DATA]  <<";\n";
  cd_buf[PRV]<<"}\n\n";

  cd_buf[REC]<<"recovery R" << level  <<"\n";
  cd_buf[REC]<<"{\n";
  cd_buf[REC]<<"\tmax_try\t\t\t\t\t\t= "       << 1000    <<";\n";
  cd_buf[REC]<<"\trelative_overhead\t= " << total_profile[PRV_REF_DATA] / (total_profile[PRV_REF_DATA]+total_profile[PRV_COPY_DATA]) <<";\n";
  cd_buf[REC]<<"\t// you can mimic partial preservation overhead by adjusting this"  <<";\n";
  cd_buf[REC]<<"}\n\n";
//////detection part////////////////////////////////////////////////////////////////////////////
  if(this->profile_data_[SYSTEM_BIT_VECTOR] & 0x1 ) {
    cd_buf[BODY]<<"\t\tdetection de"<<level<<"{\n";
    cd_buf[BODY]<<"\t\t\terror_type\t\t\t= E"   <<level<<";\n";
    cd_buf[BODY]<<"\t\t\trecovery_method\t= R"<<level<<";\n";
    cd_buf[BODY]<<"\t\t}\n\n";
  }
  if(this->profile_data_[SYSTEM_BIT_VECTOR] & 0x2 ) {
    cd_buf[BODY]<<"\t\tdetection de"<<level<<"{\n";
    cd_buf[BODY]<<"\t\t\terror_type\t\t\t= E"<<level<<";\n";
    cd_buf[BODY]<<"\t\t\trecovery_method\t= R"<<level<<";\n";
    cd_buf[BODY]<<"\t\t}\n\n";
  }
  //if(escalation)
  if(1) {
    cd_buf[BODY]<<"\t\tdetection de"<<level<<"{\n";
    cd_buf[BODY]<<"\t\t\terror_type\t\t\t= E"<<level<<";\n";
    cd_buf[BODY]<<"\t\t\trecovery_method\t= R"<<level<<";\n";
    cd_buf[BODY]<<"\t\t}\n\n";
  }
////////////////////////////////////////////////////////////////////////////////////////////////////

  this->Print_Profile(++(cd_info.begin()), cd_buf);  // Recursive call start!

  cd_buf[BODY]<<"\n\t};\n\n\n\n";  // end of GroupCD

  for(uint64_t i=0; i<MAX_FORMAT; ++i) {
    fwrite(cd_buf[i].data(), sizeof(char), cd_buf[i].size(), file_p);
    string newlines("\n\n");
    fwrite(newlines.data(), sizeof(char), newlines.size(), file_p);
  }

  fclose(file_p);
  
//  for(uint64_t i=0; i<2; ++i) {
//    cd_buf[i].clear();
//  }

}





//-- CD-related Global function definition ------------------------------------------
//   (1) CD::Init()
//   (2) CD::GetRoot()

/** CD::Init()
 *  (1) Initiate all the hassel of the Sight regarding profilation
 */
CDHandle* Init(CDErrType cd_err_t, bool collective)
{
  SightInit(txt()<<"CDs", txt()<<"dbg.CDs.visualizaion");

  /// Title
  dbg << "<h1>Containment Domains Profiling and Visualization</h1>" << endl;

  /// Explanation
  dbg << "Some explanation on CD runtime "<<endl<<endl;

  /// Register Root CD
  root = new CDHandle();

  return root;
}
/** CD::GetRoot()
 *  (1) Get root CDhandle pointer
 */
CDHandle* GetRoot()
{
  return root;
}

void Finalize(){
  //cout<<"Finalize call"<<endl;
  //getchar();
  for(auto it = cd_info.begin(); it != cd_info.end(); ++it) {
    //cout<<"level (map size) : "<< cd_info.size() << "\tsibling (map size) : "<<it->second.size() <<endl;
  }
  GetRoot()->Print_Profile();
  GetRoot()->Print_Graph();
}



//-- Profiler-related function definition -------------------------------------------
bool check_overlap(CDHandle* parent, std::string& ref_name){
  auto matched_it = (parent->this_cd_->ptr_cd_->entry_directory_).find(ref_name);
  
  if( matched_it == (parent->this_cd_->ptr_cd_->entry_directory_).end() )
    return false;
  else
    return true;
}

// Create the directory workDir/dirName and return the string that contains the absolute
// path of the created directory
string createDir(string workDir, string dirName) {
  ostringstream fullDirName; 
  fullDirName<<workDir<<"/"<<dirName;
  ostringstream cmd; 
  cmd << "mkdir -p "<<fullDirName.str();

  int ret = system(cmd.str().c_str());
  if(ret == -1) { 
//    cout << "ERROR creating directory \""<<workDir<<"/"<<dirName<<"\"!"; 
    exit(-1); 
  }
  return fullDirName.str();
}


// Open a freshly-allocated output stream to write a file with the given name and return a pointer to the object.
ofstream& createFile(string fName) {
  ofstream *f = new ofstream();
  try {
    f->open(fName.c_str(), std::ios::out);
  } 
  catch (ofstream::failure e) { 
//    cout << "createFile() ERROR opening file \""<<fName<<"\" for writing!"; 
    exit(-1); 
  }
  
  return *f;
}

// Returns a string that contains n put_tabs
string put_tabs(int tab_count)
{
  string adjusted_put_tabs;
  for(int i=0; i<tab_count; ++i) {
    adjusted_put_tabs += "\t\t";
  }

  return adjusted_put_tabs;
}

inline void end_of_session(FILE* fp) 
{
  string newlines("\n\n");
  fwrite(newlines.data(), sizeof(char), newlines.size(), fp);
}

};  // namespace cd end
