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

#ifndef _ERROR_INJECTOR_H
#define _ERROR_INJECTOR_H
/**
 * @file error_injector.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * \brief Error injection interface. 
 *
 *  User can inject errors using this error injector interface. 
 * `ErrorInjector` class is the base class that users will inherit if they want to plug in their own error injector.
 *  This functionality can be turned on/off by setting `ENABLE_ERROR_INJECTOR` as 1 in Makefile.
 *  It will be helpful to take a look at how to interact with CDs to inject errors through CD runtime.
 * - \ref sec_example_error_injection
 */
#include "cd_features.h"

#if CD_ERROR_INJECTION_ENABLED

//#include <cstdio>
//#include <iostream>
//#include <cstdio>
#include <random>
#include <cmath>
//#include "cd_def_internal.h"
#include "cd_global.h"
#include "sys_err_t.h"

#define DEFAULT_ERROR_THRESHOLD 0.0
#define NO_ERROR_INJECTED 0
namespace cd {
  namespace interface {


enum RandType { kUniform = 0,
                kExponential,
                kLogNormal,
                kNormal,
                kPoisson
              };

class ErrorProb {
public:
  virtual long double GenErrorVal(void)=0;

  ErrorProb(void) {}
  virtual ~ErrorProb(void) {}
};


class UniformRandom : public ErrorProb {
public:
  long double GenErrorVal(void);
  void TestErrorProb(int num_bucket=10);
};
/*
class LogNormal : public ErrorProb {
protected:
  std::lognormal_distribution<float> distribution_;
  float mean_;
  float std_;
public:
  float GenErrorVal(void);

  void TestErrorProb(int num_bucket=10);

    LogNormal(void) 
    : distribution_(0.0, 1.0), mean_(0.0), std_(1.0) {}
  LogNormal(float mean, float std) 
    : distribution_(mean, std), mean_(mean), std_(std) {}
  virtual ~LogNormal() {}
};

class Exponential : public ErrorProb {
protected:
  std::exponential_distribution<float> distribution_;
  float lamda_;
public:
  float GenErrorVal(void);

  void TestErrorProb(int num_bucket=10);

  Exponential(void) 
    : distribution_(1.0), lamda_(1.0) {}

  Exponential(float lamda) 
    : distribution_(lamda), lamda_(lamda) {}

  virtual ~Exponential() {}
};

class Normal : public ErrorProb {
protected:
  std::normal_distribution<float> distribution_;
  float mean_;
  float std_;
public:
  float GenErrorVal(void);

  void TestErrorProb(int num_bucket=10);

  Normal(void) 
    : distribution_(0.0, 1.0), mean_(0.0), std_(1.0) {}
  Normal(float mean, float std) 
    : distribution_(mean, std), mean_(mean), std_(std) {}
  virtual ~Normal() {}
};

class Poisson : public ErrorProb {
protected:
  std::poisson_distribution<int> distribution_;
  float mean_;
public:
  float GenErrorVal(void);
  
  void TestErrorProb(int num_bucket=10);


  Poisson(void) 
    : distribution_(1.0), mean_(1.0) {}
  Poisson(float mean) 
    : distribution_(mean), mean_(mean) {}
  virtual ~Poisson() {}
};
*/



/**@addtogroup error_injector
 * @{
 */


/**@class ErrorInjector
 * @brief Base class to provide interface to inject error.
 * 
 * User will inherit this base class for every error injector 
 * whose error injection will be invoked internally in CD runtime.
 * - \ref sec_example_error_injection
 * @{
 */

class ErrorInjector {
protected:
  ErrorProb *rand_generator_;
  bool enabled_;
  FILE *logfile_;
  float error_rate_;

public:

  ErrorInjector(void);
  ErrorInjector(float error_rate, RandType random_type=kUniform, FILE *logfile=stdout);
  ErrorInjector(bool enabled, float error_rate, RandType random_type=kUniform, FILE *logfile=stdout);
  virtual ~ErrorInjector(void) {}

  void Init(RandType random_type, FILE *logfile=stdout);
  
  void Enable(void)  { enabled_=true; }
  void Disable(void) { enabled_=false; }
  void SetLogfile(FILE *logfile) { logfile_ = logfile; }
  virtual uint64_t Inject(void);
protected:
  uint64_t InjectError(const long double &error_rate);
  long double GetErrorProb(float error_rate, float unit_time);
  inline ErrorProb *CreateErrorProb(RandType random_type);
};

/** @} */ // Ends error_injector

//class MultiTypeErrorInjector : public ErrorInjector {
//  std::map<uint32_t, float> error_prob_bin_;
//  MultiTypeErrorInjector(const std::initializer_list<std::pair<uint32_t, float>> &err_type_list);
//  virtual uint32_t Inject(void);
//
//};



class MemoryErrorInjector : public ErrorInjector {

  void    *data_;
  uint64_t size_;

public:
  MemoryErrorInjector(void)
    : ErrorInjector(0.0, kUniform, stdout), data_(NULL), size_(0) {}

  MemoryErrorInjector(void *data, uint64_t size)
    : ErrorInjector(0.0, kUniform, stdout), data_(data), size_(size) {}

  MemoryErrorInjector(void *data, uint64_t size, float error_rate, RandType random_type=kUniform, FILE *logfile=stdout)
    : ErrorInjector(error_rate, random_type, logfile), data_(data), size_(size) {}
  virtual ~MemoryErrorInjector() {}

//  virtual void Inject(void) {}
  virtual uint64_t Inject(void) { return NO_ERROR_INJECTED; }
  virtual int64_t SetRange(uint64_t range) {return 0;}
  virtual void PushRange(void *data, uint64_t ndata, uint64_t sdata, const char *desc){}
  virtual void Init(float soft_error_rate=0.0, float hard_error_rate=0.0){}

};

class SystemErrorInjector : public ErrorInjector {
  SystemConfig &sc_;
  clock_t prev_clk_;
public:
  SystemErrorInjector(SystemConfig &sc)
    : sc_(sc)
  {
    prev_clk_ = clock();
  }

  virtual uint64_t Inject(void);
};

class CDErrorInjector : public ErrorInjector {
  friend class cd::CDHandle;

  std::vector<uint32_t> cd_to_fail_;
  std::vector<uint32_t> task_to_fail_;
  uint32_t rank_in_level_;
  uint32_t task_in_color_;
  bool force_to_fail_;
public:
  CDErrorInjector(void);

  CDErrorInjector(float error_rate, RandType rand_type=kUniform, FILE *logfile=stdout);

  CDErrorInjector(uint32_t cd_to_fail, uint32_t task_to_fail, 
                  float error_rate, RandType rand_type=kUniform, FILE *logfile=stdout);

  CDErrorInjector(uint32_t cd_to_fail, uint32_t task_to_fail, uint32_t rank_in_level, uint32_t task_in_color,
                  float error_rate, RandType rand_type=kUniform, FILE *logfile=stdout);

  CDErrorInjector(std::initializer_list<uint32_t> cd_list_to_fail, 
                  std::initializer_list<uint32_t> task_list_to_fail, 
                  float error_rate);

  CDErrorInjector(std::initializer_list<uint32_t> cd_list_to_fail, 
                  std::initializer_list<uint32_t> task_list_to_fail, 
                  float error_rate, RandType rand_type, FILE *logfile);

  CDErrorInjector(std::initializer_list<uint32_t> cd_list_to_fail, 
                  std::initializer_list<uint32_t> task_list_to_fail,
                  uint32_t rank_in_level, uint32_t task_in_color,
                  float error_rate, RandType rand_type=kUniform, FILE *logfile=stdout);

  virtual ~CDErrorInjector(void) {}

  void RegisterTarget(uint32_t rank_in_level, uint32_t task_in_color);

  virtual uint64_t Inject(void);
};



class NodeFailureInjector : public ErrorInjector {
public:
  NodeFailureInjector(void) 
    : ErrorInjector(0.0, kUniform, stdout) {}

  NodeFailureInjector(float error_rate, RandType rand_type=kUniform, FILE *logfile=stdout) 
    : ErrorInjector(error_rate, rand_type, logfile) {}

  virtual uint64_t Inject(void) { return NO_ERROR_INJECTED; }
  virtual ~NodeFailureInjector(void) {}
};



/** @} */ // Ends error_injector

} // namespace interface ends
} // namespace cd ends

#endif
#endif
