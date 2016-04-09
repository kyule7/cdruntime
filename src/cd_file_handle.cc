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
#include "cd_config.h"
#include "cd_file_handle.h"
#include "cd_def_debug.h"
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#if _PRV_FILE_ERASED
#include <unistd.h>
#endif
using namespace std;
using namespace cd;
using namespace cd::internal;

std::string CDFileHandle::global_prv_path_ = CD_DEFAULT_PRV_GLOBAL_BASEPATH;
std::string CDFileHandle::local_prv_path_  = CD_DEFAULT_PRV_LOCAL_BASEPATH;

/// CDFileHandle ///////////////////////////////////
CDFileHandle::CDFileHandle(void) 
{
  opened_ = false;
  fpos_generator_ = 0;
}

CDFileHandle::CDFileHandle(const PrvMediumT& prv_medium, 
                           const std::string &filename) 
{
  opened_ = false;
  fpos_generator_ = 0;
  SetFilePath(prv_medium, filename);
}

void CDFileHandle::Initialize(const PrvMediumT& prv_medium, 
                              const std::string &filename) 
{
  opened_ = false;
  fpos_generator_ = 0;
  SetFilePath(prv_medium, filename);
}

void CDFileHandle::OpenFilePath(void)     
{ 
  // opend_ == true means that the corresponding filepath already exists!
  assert(opened_ == false);
  file_desc_ = mkstemp(unique_filename_);
  if(file_desc_ != -1){
    opened_ = true;
    CD_DEBUG("[CD_Init] this is the temporary path created for run: %s\n", GetFilePath());
  }
  else {
    ERROR_MESSAGE("Failed to generate an unique filepath.\n");
  }

  fp_ = fdopen(file_desc_, "w");
}

CDFileHandle::~CDFileHandle() 
{
  Close();
}

void CDFileHandle::Close()
{
  if(opened_) {
//    fclose(fp_);
#if _PRV_FILE_ERASED
//    unlink(unique_filename_);
#endif
   }
}

char *CDFileHandle::GetFilePath(void)
{ 
  if(!opened_) {
    OpenFilePath();
  }
  return unique_filename_;
}

void CDFileHandle::SetFilePath(const PrvMediumT& prv_medium, /*const std::string &basepath,*/ const std::string &filename)
{
  if(prv_medium != kDRAM) {
    string fullpath;
    if(prv_medium == kHDD) {
      fullpath = local_prv_path_ + filename;
    }
    else if(prv_medium == kSSD) {
      fullpath = local_prv_path_ + filename;
    }
    else if(prv_medium == kPFS) { 
      fullpath = global_prv_path_ + filename;
    }
    else {
      ERROR_MESSAGE("Unsupported medium : %d\n", prv_medium);
    }

    strcpy(unique_filename_, fullpath.c_str());

  }
  else {
    CD_DEBUG("DRAM does not need to set filepath : %d\n", prv_medium);
  }
}

///// FilePath ///////////////////////////////////
//FilePath::FilePath(void)
//{ 
//  basepath_ = global_prv_path_;
//  filename_ = CD_DEFAULT_PRV_FILENAME;
//}
//
//FilePath::FilePath(const std::string &basepath, const std::string &filename) 
//{
//  SetFilePath(basepath, filename);
//}
//
//void FilePath::SetFilePath(const std::string &basepath, const std::string &filename)
//{
//  basepath_ = basepath;
//  filename_ = filename;
//}
//
//std::string FilePath::GetFilePath(void) 
//{ return basepath_+filename_; }
  
