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

#include "cd_global.h"
#include "transaction.h"
#define CDLog tsn_log_struct

using namespace cd;

namespace cd {

class Path {
public:
	std::string _path_SSD;
	std::string _path_HDD;
public:
	Path() {
		_path_SSD = "./SSD";
		_path_HDD = "./HDD";
	}
	Path(std::string ssd, std::string hdd) {
		_path_SSD = ssd;
		_path_HDD = hdd;
	}
	void SetSSDPath(std::string path_SSD) { _path_SSD = path_SSD; }
	void SetHDDPath(std::string path_HDD) { _path_HDD = path_HDD; }
	std::string GetSSDPath(void)          { return _path_SSD;     }
	std::string GetHDDPath(void)          { return _path_HDD;     }
	Path& operator=(const Path& that) {
		_path_SSD = that._path_SSD;
		_path_HDD = that._path_HDD;
		return *this;
	}
};

class CDLogHandle {
public:
  CDLog HDDlog;
  CDLog SSDlog;
  bool _OpenHDD, _OpenSSD;
  Path path;
public:
  CDLogHandle() : _OpenHDD(false), _OpenSSD(false), path() {}
  ~CDLogHandle() {}
  void InitOpenHDD() { _OpenHDD = false; }
  void InitOpenSSD() { _OpenSSD = false; }
  bool isOpenHDD()   { return _OpenHDD;  }
  bool isOpenSSD()   { return _OpenSSD;  }
  void OpenHDD();     
  void OpenSSD();     
};

} // namespace cd ends
#endif
