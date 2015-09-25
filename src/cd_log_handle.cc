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
#include <stdio.h>
#include <stdlib.h>
using namespace cd;



void CDLogHandle::OpenFilePath(void)     
{ 
  char cmd[ 256 ];
  sprintf(cmd, "mkdir -p %s", path_.GetFilePath().c_str());
  int ret = system(cmd);
  if( ret == -1 ) {
    ERROR_MESSAGE("Failed to create a directory for preservation data.");
  }
  else {
    opened_ = true;  
  }
  
}

void CDLogHandle::CloseAndDeletePath( void )
{
  char cmd[ 256 ];
  sprintf(cmd, "rm -r -f %s", path_.GetFilePath().c_str());
  int ret = system(cmd);
  if( ret == -1 ) 
  {
    ERROR_MESSAGE("Failed to remove a directory for preservation data.");
  }
  else 
  {
    opened_ = false;  
  }
}

std::string CDLogHandle::GetFilePath(void)
{ 
  if(!opened_) {
    OpenFilePath();
  }
  return path_.GetFilePath();
}

void CDLogHandle::SetFilePath(const PrvMediumT& prv_medium, const std::string& unique_filepath = "")
{
  unique_basepath_ = unique_filepath;
#if 1
  if(prv_medium != kDRAM) {
    char *filepath_env = getenv("CD_PRV_BASEPATH");
    std::string basepath;
    if(filepath_env != NULL) {
      basepath = filepath_env;
    }
    else {
      basepath = CD_DEFAULT_PRV_FILEPATH;
    }

    if(prv_medium == kHDD) { 
      path_ = basepath + "/HDD/" + unique_basepath_ + "/";
    }
    else if(prv_medium == kSSD) {
      path_ = basepath + "/SSD/" + unique_basepath_ + "/";
    }
    else if(prv_medium == kPFS) { 
      path_ = basepath + "/PFS/" + unique_basepath_ + "/";
    }
    else {
      ERROR_MESSAGE("Unsupported medium : %d\n", prv_medium);
    }
  }
  else {
    ERROR_MESSAGE("DRAM does not need to set filepath : %d\n", prv_medium);
  }
#else
  switch(prv_medium) 
  {
    case kDRAM :
    {
      path_ = CD_FILEPATH_INVALID;
      break;
    }
    case kPFS :
    {
      if ( getenv( "CD_PRV_BASEPATH" ) )
      {
        std::string path_base( getenv( "CD_PRV_BASEPATH" ) );
        path_base = path_base + "/PFS/" + unique_basepath_ + "/";
        path_.filepath_.clear();
        path_.filepath_ = path_base;
      }
      else
        path_ = CD_FILEPATH_PFS;

      break;  
    }
    case kHDD :
    {
      if ( getenv( "CD_PRV_BASEPATH" ) )
      {
        std::string path_base( getenv( "CD_PRV_BASEPATH" ) );
        path_base = path_base + "/HDD/" + unique_basepath_ + "/";
        path_.filepath_.clear();
        path_.filepath_ = path_base;
      }
      else
        path_ = CD_FILEPATH_HDD;

      break;
    }
    case kSSD :
    {
      if ( getenv( "CD_PRV_BASEPATH" ) )
      {
        std::string path_base( getenv( "CD_PRV_BASEPATH" ) );
        path_base = path_base + "/SSD/" + unique_basepath_ + "/";
        path_.filepath_.clear();
        path_.filepath_ = path_base;
      }
      else
        path_ = CD_FILEPATH_SSD;

      break;
    }
    default :
    {
      path_ = CD_FILEPATH_DEFAULT;
    }
  }  
  std::cout << "ALI : " << path_.filepath_ << std::endl;
#endif
 
}
