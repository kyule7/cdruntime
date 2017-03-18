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

#include "sys_err_t.h"
#include "system_config.h"
#include "cd_global.h"
#include "phase_tree.h"
#include <yaml.h>
#include <string.h>
using namespace cd;
#define INDENT_SIZE "    "
SystemConfig cd::config;
//PhaseTree tuned::phaseTree;
//PhaseMapType cd::phaseMap;
//PhasePathType cd::phasePath;
//PhasePathType tuned::phasePath;
//uint32_t cd::PhaseNode::phase_gen = 0;

int64_t errortype = -1;
int64_t interval = -1;
int64_t level = -1;
int64_t phase = -1;
int seq_cnt = 0;
char prv;
std::string label;

uint64_t SoftMemErrInfo::get_pa_start(void)     { return pa_start_; }
uint64_t SoftMemErrInfo::get_va_start(void)     { return va_start_; }
uint64_t SoftMemErrInfo::get_length(void)       { return length_; }
char    *SoftMemErrInfo::get_data(void)         { return data_; }
uint64_t SoftMemErrInfo::get_syndrome_len(void) { return syndrome_len_; }
char    *SoftMemErrInfo::get_syndrome(void)     { return syndrome_; }


std::vector<uint64_t> DegradedMemErrInfo::get_pa_starts(void) { return pa_starts_; }
std::vector<uint64_t> DegradedMemErrInfo::get_va_starts(void) { return va_starts_; }
std::vector<uint64_t> DegradedMemErrInfo::get_lengths(void)   { return lengths_; }



uint32_t DeclareErrName(const char* name_string)
{
  // STUB
  return kOK;
}

CDErrT UndeclareErrName(uint32_t error_name_id)
{
  // STUB
  return kOK;
}

uint32_t DeclareErrLoc(const char *name_string)
{
  // STUB
  return kOK;
}

CDErrT UndeclareErrLoc(uint32_t error_name_id)
{
  // STUB
  return kOK;
}


void ConfigEntry::Print(int64_t level, int64_t phase) 
{ 
  printf("[%ld/%ld] interval:%ld, type:%lX\n", 
            level, phase, interval_, failure_type_); 
}

static inline
void AddIndent(int cnt)
{
  for(int i=0; i<cnt; i++) {
    printf(INDENT_SIZE);
  }
}


void SystemConfig::UpdateSwitchParams(char *str)
{
  char *prv_str = str;
//  printf("1:%s\n", str);
  str = strtok(prv_str, "_");
  if(str == NULL) { printf("Param format is wrong\n"); /*assert(0);*/ }
  str = strtok(NULL, "_");
//  printf("1:%s\n", str);
  level = atol(str);
  str = strtok(NULL, "_");
//  printf("2:%s\n", str);
  if(str == NULL) { printf("Param format is wrong\n"); /*assert(0);*/ }
  phase = atol(str);
//  printf("Check: %ld, %ld\n", level, phase);  getchar();
}

std::string trim(const std::string& str,
                 const std::string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

void SystemConfig::ParseParam(char *key)
{
  if(key[0] == 'C' && key[1] == 'D' && prv != 'l') {
    prv = key[0]; 
    AddIndent(seq_cnt); printf("%s\n", key);

    tuned::phaseTree.current_ = new PhaseNode(tuned::phaseTree.current_);
    if(tuned::phaseTree.root_ == NULL) 
      tuned::phaseTree.root_ = tuned::phaseTree.current_;

    UpdateSwitchParams(key);
    tuned::phaseTree.current_->level_ = level;
    tuned::phaseTree.current_->phase_ = phase;
  } else if(strcmp(key, "label") == 0) {
    prv = key[0]; 
    AddIndent(seq_cnt); printf("%s: ", key);
  } else if(strcmp(key, "interval") == 0) {
    prv = key[0]; 
    AddIndent(seq_cnt); printf("%s: ", key);
  } else if(strcmp(key, "errortype") == 0) { 
    prv = key[0]; 
    AddIndent(seq_cnt); printf("%s: ", key);
  } else if(key[0] == 'F') { 
    prv = key[0]; 
    errortype = atol(key+1);
    AddIndent(seq_cnt); printf("%s: ", key);
  } else { // value
    if(prv == 'F') {
      config.failure_rate_[errortype] = atof(key);
    } else if(prv == 'l') {
      label = key;
      tuned::phaseTree.current_->label_ = label;
      printf("%s ", label.c_str());
    } else if(prv == 'i') {
      interval = atol(key);
      tuned::phaseTree.current_->interval_ = interval;
      printf("[%p,%p] %ld ", tuned::phaseTree.root_, tuned::phaseTree.current_, interval); 
    } else if(prv == 'e') {
      errortype = strtol(key, NULL, 16);
      tuned::phaseTree.current_->errortype_ = errortype;
      config.mapping_[level][phase].failure_type_ = errortype;
      printf("0x%lX ", errortype); 
    }
    printf("%s\n", key);
  }
} 

/*
void PhaseNode::Print(void) 
{
  AddIndent(level_);
  printf("CD_%u_%u\n", level_, phase_);
  AddIndent(level_);
  printf("{\n");
  AddIndent(level_+1); printf("label:%s\n", label_.c_str());
  AddIndent(level_+1); printf("interval:%ld\n", interval_);
  AddIndent(level_+1); printf("errortype:0x%lX\n", errortype_);
  for(auto it=children_.begin(); it!=children_.end(); ++it) {
    (*it)->Print();
  }
  AddIndent(level_);
  printf("}\n");
}

std::string PhaseNode::GetPhasePath(void)
{
  return (parent_->GetPhasePath() + std::string("_") + label_);
}

// Create or Get phase depending on label
// Update phaseTree.current_, and returns phase ID number.
// If there is no phase for the unique label, create a new PhaseNode
// cd_name_.phase_ = cd::phaseTree->target_->GetPhaseNode();
uint32_t PhaseNode::GetPhaseNode(uint32_t level, const string &label)
{
  printf("## %s ## lv:%u, label:%s\n", __func__, level, label.c_str());
  uint32_t phase = INVALID_NUM;
  std::string phase_path = GetPhasePath();
  auto it = cd::phasePath.find(phase_path);

  // If there is no label before, it is a new phase!
  if(it == phaseMap.end()) {
    cd::phaseTree.current_ = new PhaseNode(cd::phaseTree.current_, level, label);
    uint32_t phase        = cd::phaseTree.current_->phase_;
    phaseMap[phase_path]  = phase;
    phaseNodeCache[phase] = cd::phaseTree.current_;
    //phaseMap[level][label] = cd::phaseTree.current_->phase_;
    printf("New Phase! %u %s\n", phase, label.c_str());
  } else {
    cd::phaseTree.current_ = phaseNodeCache[it->second];
    phase = it->second;
    printf("Old Phase! %u %s\n", phase, label.c_str()); //getchar();
  }
  return phase;
}
*/

void SystemConfig::LoadConfig(const char *config)
{
  FILE *fh = fopen(config, "r");
  yaml_parser_t parser;
  yaml_token_t  token;   /* new variable */
  yaml_event_t  event;   /* New variable */
  if(!yaml_parser_initialize(&parser)) assert(0);
  if(fh == NULL) assert(0);

  yaml_parser_set_input_file(&parser, fh);

//  PhaseTree tuned::phaseTree;
  do {
    if (!yaml_parser_parse(&parser, &event)) {
       printf("Parser error %d\n", parser.error);
       exit(EXIT_FAILURE);
    }
    switch(event.type)
    { 
      case YAML_NO_EVENT: puts("No event!"); break;
      /* Stream start/end */
      case YAML_STREAM_START_EVENT: puts("STREAM START"); break;
      case YAML_STREAM_END_EVENT:   puts("STREAM END");   break;
      /* Block delimeters */
      case YAML_DOCUMENT_START_EVENT: {

//                                      tuned::phaseTree.current_ = new PhaseNode(tuned::phaseTree.current_);
//                                      tuned::phaseTree.root_ = tuned::phaseTree.current_;
                                      puts("[[[[[[[[[[[[[[[[[[["); 
                                      break;
                                      }
      case YAML_DOCUMENT_END_EVENT:   puts("]]]]]]]]]]]]]]]]]]]");   break;
      case YAML_SEQUENCE_START_EVENT: {
//                                        tuned::phaseTree.current_ = new PhaseNode(tuned::phaseTree.current_);
//                                        if(tuned::phaseTree.root_ == NULL) 
//                                          tuned::phaseTree.root_ = tuned::phaseTree.current_;
//                                        else 
//                                        if(tuned::phaseTree.root_ != NULL) 
                                          AddIndent(seq_cnt++); puts("{"); 
//                                        seq_cnt++;
                                        break;
                                      }
      case YAML_SEQUENCE_END_EVENT:   {
                                        if(tuned::phaseTree.current_ != NULL)
                                          tuned::phaseTree.current_ = tuned::phaseTree.current_->parent_;
                                        
//                                        seq_cnt--;
//                                      if(tuned::phaseTree.current_ != tuned::phaseTree.root_){
                                        AddIndent(--seq_cnt); puts("}"); 
//                                      }
                                        break;
                                      }
      case YAML_MAPPING_START_EVENT:  {
                                        //printf("map begin\n"); 
                                        break;
                                      }
      case YAML_MAPPING_END_EVENT:    {
                                        //printf("map end\n"); 
                                        break;
                                      }
      /* Data */
      case YAML_ALIAS_EVENT:  printf("Got alias (anchor %s)\n", event.data.alias.anchor); break;
      case YAML_SCALAR_EVENT: {
        char *key = (char *)event.data.scalar.value;
        ParseParam(key);
        break;
      }
    }
    if(event.type != YAML_STREAM_END_EVENT)
      yaml_event_delete(&event);
  } while(event.type != YAML_STREAM_END_EVENT);
  yaml_event_delete(&event);
  yaml_parser_delete(&parser);
  fclose(fh);
  tuned::phaseTree.Print();
  
}














//void SystemConfig::UpdateSwitchParams(char *str)
//{
//  char *prv_str = str+1;
//  str = strtok(prv_str, "_");
//  if(str == NULL) { printf("Param format is wrong\n"); /*assert(0);*/ }
//  level = atol(str);
//  str = strtok(NULL, "_");
//  if(str == NULL) { printf("Param format is wrong\n"); /*assert(0);*/ }
//  phase = atol(str);
//  printf("Check: %ld, %ld\n", level, phase);  getchar();
//}
//
//
//void SystemConfig::ParseCDHierarchy(const char *key, int seq_cnt) 
//{
//  if(key[0] == 'C' && key[1] == 'D') { 
//    AddIndent(seq_cnt); printf("%s\n", key);
//  } else if(strcmp(key, "loop") == 0) { 
//    AddIndent(seq_cnt); printf("%s: ", key);
//  } else if(strcmp(key, "time") == 0) { 
//    AddIndent(seq_cnt); printf("%s: ", key);
//  } else if(strcmp(key, "failure_rate") == 0) { 
//    AddIndent(seq_cnt); printf("%s: ", key);
//  } else { // value
//    printf("%s\n", key);
//  }
//}
//
//void SystemConfig::ParseParam(const char *key) 
//{
//  static char prv;
//  char *str = const_cast<char *>(key);
//  if(key[0] == 'S') {
//    prv = key[0];
//    UpdateSwitchParams(str);
//  } else if(key[0] == 'F') {
//    prv = key[0];
//    failure_type = atoi(str+1);
//    printf("failure_type:%lx\n", failure_type);
//  } else if(key[0] == 'P') {
//    printf("key0:%s\n", key);
//  } else { // value
//    if(prv == 'S') {
//      char *prv_str = str;
//      str = strtok(prv_str, ",");
//      if(str != NULL) {
//        interval = atol(str);
//        str = strtok(NULL, ",");
//        if(str == NULL) { printf("Param format is wrong\n"); assert(0); }
//        printf("type:%s\n", str);
//        failure_type = strtol(str, NULL, 16);
//        config.mapping_[level][phase].interval_     = interval;
//        config.mapping_[level][phase].failure_type_ = failure_type;
//        config.mapping_[level][phase].Print(level, phase);
//      }
//    } else if(prv == 'F') {
//      config.failure_rate_[failure_type] = atof(str);
//    } else {
//      printf("key:%s\n", key);
//      assert(0);
//    }
//  }
//}
//
//void SystemConfig::LoadConfig(const char *config)
//{
//  FILE *fh = fopen(config, "r");
//  yaml_parser_t parser;
//  yaml_token_t  token;   
//  yaml_event_t  event;   
//
//  if(!yaml_parser_initialize(&parser)) assert(0);
//  if(fh == NULL) assert(0);
//
//  yaml_parser_set_input_file(&parser, fh);
//
//  do {
//    if (!yaml_parser_parse(&parser, &event)) {
//       printf("Parser error %d\n", parser.error);
//       exit(EXIT_FAILURE);
//    }
//    switch(event.type)
//    { 
//      case YAML_NO_EVENT:             printf("No event!"); break;
//      // Stream start/end
//      case YAML_STREAM_START_EVENT:   printf("STREAM START\n"); break;
//      case YAML_STREAM_END_EVENT:     printf("\nSTREAM END");   break;
//      // Block delimeters
//      case YAML_DOCUMENT_START_EVENT: printf("Start Configuration\n");   break;
//      case YAML_DOCUMENT_END_EVENT:   printf("\nEnd Configuration");     break;
//      case YAML_SEQUENCE_START_EVENT: {
//                                        //printf("seq start\n");/*AddIndent(seq_cnt++); printf("{");*/ 
//                                        break;
//                                      }
//      case YAML_SEQUENCE_END_EVENT:   {
//                                        //printf("seq end\n");/*AddIndent(--seq_cnt); printf("}");*/ 
//                                        break;
//                                      }
//      case YAML_MAPPING_START_EVENT:  {/*printf("map start\n");  */    break;
//                                      }
//      case YAML_MAPPING_END_EVENT:    {/*printf("map end\n");    */    break;
//                                      }
//      // Data
//      case YAML_ALIAS_EVENT:          break; 
//      case YAML_SCALAR_EVENT: 
//      { 
//        char key[32];
//        strcpy(key, (char *)event.data.scalar.value);
//        ParseParam(key);
//        break;
//      }
//    }
//    if(event.type != YAML_STREAM_END_EVENT)
//      yaml_event_delete(&event);
//  } while(event.type != YAML_STREAM_END_EVENT);
//
//  yaml_event_delete(&event);
//  yaml_parser_delete(&parser);
//  fclose(fh);
//}


//void LoadSystemConfig(char *configFile)
//{
//  if(configFile == NULL) {
//#ifdef ERROR_TYPE_0
//  system_config[ERROR_TYPE_0]  = ERROR_RATE_TYPE_0;
//#endif
//#ifdef ERROR_TYPE_1
//  system_config[ERROR_TYPE_1]  = ERROR_RATE_TYPE_1;
//#endif
//#ifdef ERROR_TYPE_2
//  system_config[ERROR_TYPE_2]  = ERROR_RATE_TYPE_2;
//#endif
//#ifdef ERROR_TYPE_3
//  system_config[ERROR_TYPE_3]  = ERROR_RATE_TYPE_3;
//#endif
//#ifdef ERROR_TYPE_4
//  system_config[ERROR_TYPE_4]  = ERROR_RATE_TYPE_4;
//#endif
//#ifdef ERROR_TYPE_5
//  system_config[ERROR_TYPE_5]  = ERROR_RATE_TYPE_5;
//#endif
//#ifdef ERROR_TYPE_6
//  system_config[ERROR_TYPE_6]  = ERROR_RATE_TYPE_6;
//#endif
//#ifdef ERROR_TYPE_7
//  system_config[ERROR_TYPE_7]  = ERROR_RATE_TYPE_7;
//#endif
//#ifdef ERROR_TYPE_8
//  system_config[ERROR_TYPE_8]  = ERROR_RATE_TYPE_8;
//#endif
//#ifdef ERROR_TYPE_9
//  system_config[ERROR_TYPE_9]  = ERROR_RATE_TYPE_9;
//#endif
//  }
//  else {
//
//    // Read and parse system info, pass it to CD runtime
//
//  }
//}
