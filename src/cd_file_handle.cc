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
#include "cd_log_handle.h"
#include "cd_def_internal.h"
#include "cd_config.h"
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#if _PRV_FILE_NOT_ERASED
#include <unistd.h>
#endif
using namespace std;
using namespace cd;
using namespace cd::internal;

std::string FilePath::prv_basePath_ = CD_DEFAULT_PRV_BASEPATH;

/// CDFileHandle ///////////////////////////////////
CDFileHandle::CDFileHandle(void) 
{
  opened_ = false;
  fpos_generator_ = 0;
}

CDFileHandle::CDFileHandle(const PrvMediumT& prv_medium, 
                           const std::string &basepath, 
                           const std::string &filename) 
{
  opened_ = false;
  fpos_generator_ = 0;
  if(prv_medium != kDRAM)
    filePath_.SetFilePath(basepath, filename);
}

void CDFileHandle::OpenFilePath(void)     
{ 
  // opend_ == true means that the corresponding filepath already exists!
  // Therefore, if it is not opened, we need to check if it exists.
  struct stat sb;
  const char* basepath = filePath_.basepath_.c_str();
  if(stat(basepath, &sb) == 0 && S_ISDIR(sb.st_mode)) {
    printf("Prv Path exists!\n");
  }
  else {
    char cmd[256];
    sprintf(cmd, "mkdir -p %s", basepath);
    printf("preservation file path size : %d\n", (int)sizeof(cmd));
    if( system(cmd) == -1 )
      ERROR_MESSAGE("ERROR: Failed to create directory for debug info. %s\n", cmd);
  }
  strcpy(unique_filename_, filePath_.GetFilePath().c_str());
  file_desc_ = mkstemp(unique_filename_);
  fp_ = fdopen(file_desc_, "w");
#if _PRV_FILE_NOT_ERASED
  unlink(unique_filename_);
  printf("unlink : %s\n", unique_filename_);
#endif
  printf("[CD_Init] this is the temporary path created for run: %s\n", unique_filename_);
  if(file_desc_ != -1){
    opened_ = true;
    CD_DEBUG("[CD_Init] this is the temporary path created for run: %s\n", filePath_.GetFilePath().c_str());
  }
  else {
    ERROR_MESSAGE("Failed to generate an unique filepath.\n");
  }

}
CDFileHandle::~CDFileHandle()
{
  if(opened_)
    fclose(fp_);
}

void CDFileHandle::CloseAndDeletePath( void )
{
  char cmd[ 256 ];
  sprintf(cmd, "rm -rf %s", filePath_.GetFilePath().c_str());
  if( system(cmd) == -1 ) { 
    ERROR_MESSAGE("Failed to remove a directory for preservation data.");
  }
  else 
    opened_ = false;  
}

std::string CDFileHandle::GetBasePath(void)
{
  return filePath_.basepath_;
}

char *CDFileHandle::GetFilePath(void)
{ 
  if(!opened_) {
    OpenFilePath();
  }
  return unique_filename_;
}

void CDFileHandle::SetFilePath(const PrvMediumT& prv_medium, const std::string &basepath, const std::string &filename)
{
  if(prv_medium != kDRAM) {
    if(prv_medium == kHDD) { 
      filePath_.SetFilePath(basepath + string(CD_FILEPATH_HDD), filename);
    }
    else if(prv_medium == kSSD) {
      filePath_.SetFilePath(basepath + string(CD_FILEPATH_HDD), filename);
    }
    else if(prv_medium == kPFS) { 
      filePath_.SetFilePath(basepath + string(CD_FILEPATH_HDD), filename);
    }
    else {
      ERROR_MESSAGE("Unsupported medium : %d\n", prv_medium);
    }
  }
  else {
    ERROR_MESSAGE("DRAM does not need to set filepath : %d\n", prv_medium);
  }
}

/// FilePath ///////////////////////////////////
FilePath::FilePath(void)
{ 
  basepath_ = CD_DEFAULT_PRV_BASEPATH;
  filename_ = CD_DEFAULT_PRV_FILENAME;
}

FilePath::FilePath(const std::string &basepath, const std::string &filename) 
{
  SetFilePath(basepath, filename);
}

void FilePath::SetFilePath(const std::string &basepath, const std::string &filename)
{
  basepath_ = basepath;
  filename_ = filename;
}

std::string FilePath::GetFilePath(void) 
{ return basepath_+filename_; }
  
