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


#ifndef _CD_LOG_HANDLE_H
#define _CD_LOG_HANDLE_H

/**
 * @file cd_log_handle.h
 * @author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * @date March 2015
 *
 * @brief Containment Domains API v0.2 (C++)
 */

#include "cd_global.h"
#include "cd_def_internal.h"
//#include "transaction.h"
//#define CDLog tsn_log_struct

using namespace cd;

namespace cd {

class CDLogHandle;

class Path {
  friend class cd::CD;
  friend class cd::CDLogHandle;
private:
	std::string filepath_;
	Path() : filepath_(CD_FILEPATH_DEFAULT) {}
	Path(const std::string &filepath) : filepath_(filepath) {}
	Path(const char *filepath) : filepath_(filepath) {}
	void SetFilePath(std::string filepath) { filepath_ = filepath; }

	std::string GetFilePath(void) { return filepath_; }

	Path &operator=(const Path &that) {
    filepath_ = that.filepath_;
		return *this;
	}

	Path &operator=(const char *filepath) {
    filepath_ = filepath;
		return *this;
	}
};



class CDLogHandle {
  friend class cd::CD;
  friend class cd::HeadCD;

//  CDLog file_log_;
  bool opened_;
  Path path_;

public:
  CDLogHandle(void) : opened_(false), path_(CD_FILEPATH_DEFAULT) {}
  CDLogHandle(const PrvMediumT& prv_medium) : opened_(false)
  {
    switch(prv_medium) {
      case kDRAM :
        path_ = CD_FILEPATH_INVALID;
        break;
      case kPFS :
        path_ = CD_FILEPATH_PFS;
        break;
      case kHDD :
        path_ = CD_FILEPATH_HDD;
        break;
      case kSSD :
        path_ = CD_FILEPATH_SSD;
        break;
      default :
        path_ = CD_FILEPATH_DEFAULT;
    }   
  }
  ~CDLogHandle() {}

	std::string GetFilePath(void);
  void InitOpenFile() { opened_ = false; }
  bool IsOpen()   { return opened_;  }
  void OpenFilePath(void);     
};



} // namespace cd ends
#endif
