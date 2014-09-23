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

#ifndef _SYS_ERR_T_H 
#define _SYS_ERR_T_H

#include <iostream>

namespace cd {

class SysErrInfo {
protected:
  uint64_t info_;
public:
  SysErrInfo() {}
  virtual ~SysErrInfo() {}
};

class MemErrInfo : public SysErrInfo {
protected:
  uint64_t pa_start_;
  uint64_t va_start_;
  uint64_t length_;
public:
  MemErrInfo() {}
  virtual ~MemErrInfo() {}
  virtual void get_pa_start();
  virtual void get_va_start();
  virtual void get_length();
};

class SoftMemErrInfo : public MemErrInfo {
protected:
  char* data_;
  uint64_t syndrome_len_;
  char* syndrome_;
public:
  SoftMemErrInfo() {}
  ~SoftMemErrInfo() {}
  void get_data();
  void get_syndrome_len();
  void get_syndrome();
};

class DegradedMemErrInfo : public MemErrInfo {
public:
  DegradedMemErrInfo() {}
  ~DegradedMemErrInfo() {}
};



// data structure for specifying errors and failure 
// should come up with a reasonable way to allow some extensible class hierarchy for various error types.
// currently, it is more like a bit vector
class SysErrT {
public:
/*
Type for specifying system errors and failure names.

This type represents the interface between the user and the system with respect to errors and failures. 
The intent is for these error/failure names to be fairly comprehensive with respect to system-based issues, 
while still providing general/abstract enough names to be useful to the application programmer. 
The use categories/names are meant to capture recovery strategies 
that might be different based on the error/failure and be comprehensive in that regard. 
The DeclareErrName() method may be used to create a programmer-defined error 
that may be associated with a programmer-provided detection method.

We considered doing an extensible class hierarchy like GVR, but ended up with a hybrid type system. 
There are predefined bit vector constants for error/failure names and machine location names. 
These may be extended by application programmers for specialized detectors. 
These types are meant to capture abstract general classes of errors that may be treated differently by recovery functions 
and therefore benefit from easy-to-access and well-defined names. 
Additional error/failure-specific information will be represented by the SysErrInfo interface class hierarchy, 
which may be extended by the programmer at compiler time. 
Thus, each error/failure is a combination of SysErrNameT, SysErrLocT, and SysErrInfo.
*/
//  SysErrNameT  sys_err_name_;
//  SysErrLocT   sys_err_loc_;
  SysErrInfo*  error_info_;

  SysErrT() {}
  ~SysErrT() {}
  char* printSysErrInfo() { return NULL; }
};

//std::ostream& operator<<(const std::ostream& str, const SysErrT& sys_err)
//{
//  return str << sys_err.printSysErrInfo();
//}



// Create a new error/failure type name

//uint64_t DeclareErrLoc(const char* name_string)
//{
//  // STUB
//  return 0;
//}
//
//uint64_t DeclareErrName(const char* name_string)
//{
//  // STUB
//  return 0;
//}
//
//CDErrT UndeclareErrLoc(uint64_t error_name_id)
//{
//  // STUB
//  return kOK;
//}
//
//
//CDErrT UndeclareErrName(uint64_t error_name_id)
//{
//  // STUB
//  return kOK;
//}



} // namespace cd ends


#endif
