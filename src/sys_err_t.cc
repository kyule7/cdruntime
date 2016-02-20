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
using namespace cd;

SystemConfig cd::system_config;

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


void SystemConfig::LoadSystemConfig(char *configFile)
{
  if(configFile == NULL) {
#ifdef ERROR_TYPE_0
  system_config[ERROR_TYPE_0]  = ERROR_RATE_TYPE_0;
#endif
#ifdef ERROR_TYPE_1
  system_config[ERROR_TYPE_1]  = ERROR_RATE_TYPE_1;
#endif
#ifdef ERROR_TYPE_2
  system_config[ERROR_TYPE_2]  = ERROR_RATE_TYPE_2;
#endif
#ifdef ERROR_TYPE_3
  system_config[ERROR_TYPE_3]  = ERROR_RATE_TYPE_3;
#endif
#ifdef ERROR_TYPE_4
  system_config[ERROR_TYPE_4]  = ERROR_RATE_TYPE_4;
#endif
#ifdef ERROR_TYPE_5
  system_config[ERROR_TYPE_5]  = ERROR_RATE_TYPE_5;
#endif
#ifdef ERROR_TYPE_6
  system_config[ERROR_TYPE_6]  = ERROR_RATE_TYPE_6;
#endif
#ifdef ERROR_TYPE_7
  system_config[ERROR_TYPE_7]  = ERROR_RATE_TYPE_7;
#endif
#ifdef ERROR_TYPE_8
  system_config[ERROR_TYPE_8]  = ERROR_RATE_TYPE_8;
#endif
#ifdef ERROR_TYPE_9
  system_config[ERROR_TYPE_9]  = ERROR_RATE_TYPE_9;
#endif
  }
  else {

    // Read and parse system info, pass it to CD runtime

  }
}
