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
 *  This functionality can be turned on/off by setting compile flag, `ENABLE_ERROR_INJECTOR`, as 1.
 *  It will be helpful to take a look at how to interact with CDs to inject errors through CD runtime.
 * - \ref sec_example_error_injection
 */
#include <cstdio>
#include <iostream>
#include <random>
#define DEFAULT_ERROR_THRESHOLD 0.0

class ErrorProb {
protected:
  double threshold_;
#ifndef GEN_TRUE_RANDOM
  std::default_random_engine generator_;
#else
  std::random_device generator_;
#endif
public:
  virtual bool GenErrorVal(void)=0;

  ErrorProb(void) 
    : threshold_(0.0) {}
  ErrorProb(double threshold) 
    : threshold_(threshold) {}
  virtual ~ErrorProb(void) {}
};

class UniformRandom : public ErrorProb {
protected:
  std::uniform_real_distribution<double> distribution_;
  double low_;
  double high_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Uniform Randomi, low_ : " << low_ <<", high : "<< high_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];
    
    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }

    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }

  UniformRandom(void) 
    : ErrorProb(DEFAULT_ERROR_THRESHOLD), distribution_(0.0, 1.0), low_(0.0), high_(1.0) {}
  UniformRandom(double low, double high) 
    : distribution_(low, high), low_(low), high_(high) {}
  virtual ~UniformRandom() {}
};

class LogNormal : public ErrorProb {
protected:
  std::lognormal_distribution<double> distribution_;
  double mean_;
  double std_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Log Normal, mean : " << mean_ << ", std : " << std_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }
  LogNormal(void) 
    : ErrorProb(DEFAULT_ERROR_THRESHOLD), distribution_(0.0, 1.0), mean_(0.0), std_(1.0) {}
  LogNormal(double mean, double std) 
    : distribution_(mean, std), mean_(mean), std_(std) {}
  virtual ~LogNormal() {}
};

class Exponential : public ErrorProb {
protected:
  std::exponential_distribution<double> distribution_;
  double lamda_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Exponential, lamda : " << lamda_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }
  Exponential(void) 
    : ErrorProb(DEFAULT_ERROR_THRESHOLD), distribution_(1.0), lamda_(1.0) {}

  Exponential(double lamda) 
    : distribution_(lamda), lamda_(lamda) {}

  virtual ~Exponential() {}
};

class Normal : public ErrorProb {
protected:
  std::normal_distribution<double> distribution_;
  double mean_;
  double std_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    if(error < 0) error *= -1;
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Normal mean : " << mean_ << ", std : " << std_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) number *= -1;
      if(number < 0) std::cout << "negative : " << number << std::endl;

      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }
  Normal(void) 
    : ErrorProb(DEFAULT_ERROR_THRESHOLD), distribution_(0.0, 1.0), mean_(0.0), std_(1.0) {}
  Normal(double mean, double std) 
    : distribution_(mean, std), mean_(mean), std_(std) {}
  virtual ~Normal() {}
};

class Poisson : public ErrorProb {
protected:
  std::poisson_distribution<int> distribution_;
  double mean_;
public:
  bool GenErrorVal(void) {
    int error = distribution_(generator_);
    return static_cast<double>(error) >= threshold_;
  }
  
  void TestErrorProb(int num_bucket=10) {
    std::cout << "Poisson mean : " << mean_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }

  Poisson(void) 
    : ErrorProb(DEFAULT_ERROR_THRESHOLD), distribution_(1.0), mean_(1.0) {}
  Poisson(double mean) 
    : distribution_(mean), mean_(mean) {}
  virtual ~Poisson() {}
};


enum RandType { kUniform = 0,
                kExponential,
                kLogNormal,
                kNormal,
                kPoisson
              };
static inline ErrorProb *
CreateErrorProb(RandType random_type)
{
  ErrorProb *random_number = NULL;
    switch(random_type) {
      case kUniform : 
        random_number = new UniformRandom();
        break; 
      case kExponential : 
        random_number = new Exponential();
        break; 
      case kLogNormal :
        random_number = new LogNormal();
        break; 
      case kNormal :
        random_number = new Normal();
        break; 
      case kPoisson :
        random_number = new Poisson();
        break; 
      default :
        random_number = new Exponential();
    }
  return random_number;
}




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
public:
//  enum RandType { kUniform = 0,
//                  kExponential,
//                  kLogNormal,
//                  kNormal,
//                  kPoisson
//                };

protected:
  ErrorProb *rand_generator_;
  bool enabled_;
  FILE *logfile_;

public:

  ErrorInjector(void) {
    rand_generator_ = new UniformRandom();
    enabled_    = false;
    logfile_    = stdout;
  }

  ErrorInjector(RandType random_type=kUniform, FILE *logfile=stdout) {
    rand_generator_ = CreateErrorProb(random_type);
    enabled_    = false;
    logfile_    = stdout;
  }

  virtual ~ErrorInjector(void) {}

  void Init(RandType random_type, FILE *logfile=stdout) {
    if(rand_generator_ == NULL)
      rand_generator_ = CreateErrorProb(random_type);
    enabled_    = false;
    logfile_    = stdout;
  }
  
  void Enable(void)  { enabled_=true; }
  void Disable(void) { enabled_=false; }
  void SetLogfile(FILE *logfile) { logfile_ = logfile; }
  virtual void Inject(void)=0;
  virtual bool InjectAndTest(void)=0;
      
};

/** @} */ // Ends error_injector

class MemoryErrorInjector : public ErrorInjector {

  void    *data_;
  uint64_t size_;

public:
  MemoryErrorInjector(void *data=NULL, uint64_t size=0)
    : ErrorInjector(kUniform, stdout), data_(data), size_(size) {}

  MemoryErrorInjector(void *data, uint64_t size, RandType random_type, FILE *logfile)
    : ErrorInjector(random_type, logfile), data_(data), size_(size) {}
  virtual ~MemoryErrorInjector() {}

  virtual int64_t SetRange(uint64_t range)=0;
  virtual void PushRange(void *data, uint64_t ndata, uint64_t sdata, char *desc)=0;

};

class CDErrorInjector : public ErrorInjector {
  double error_rate_;

  uint32_t level_;          
  uint32_t rank_in_level_;
  uint32_t num_cds_in_level_;
  uint32_t task_in_color_;
  uint32_t task_size_;
  uint32_t total_task_size_;

public:
  CDErrorInjector(void) 
    : ErrorInjector(kUniform, stdout), error_rate_(0.0) {}

  CDErrorInjector(double error_rate, RandType rand_type=kUniform, FILE *logfile=stdout) 
    : ErrorInjector(rand_type, logfile), error_rate_(error_rate) {}

  CDErrorInjector(uint32_t level, uint32_t rank_in_level, uint32_t num_cds_in_level,
                  uint32_t task_in_color, uint32_t task_size, uint32_t total_task_size,
                  double error_rate, RandType rand_type=kUniform, FILE *logfile=stdout) 
    : ErrorInjector(rand_type, logfile), error_rate_(error_rate) {
    level_ = level;
    rank_in_level_ = rank_in_level_;
    num_cds_in_level_ = num_cds_in_level;
    task_in_color_ = task_in_color;
    task_size_ = task_in_color;
    total_task_size_ = total_task_size;  
  
  }

  virtual bool InjectAndTest(void)
  {
//    // Generate random value
//    // Compare with threashold
//    // Depending on compile-time flag indicating statically forcing a task failed,
//    // Check compile-time flag, and if it is set, set the threshold to 1.
//    // Every random variable will be less that 1, so it will be always failed.
//    double random_num = GenRandomNumber();
//    if(INJECTED_LEVEL_0 == 
//    switch(level_) {
//      case 0 :
//      case 1 :
//        
//    rand_generator_->GenErrorVal()
    return true;
  }

  virtual ~CDErrorInjector(void) {}
};

/** @} */ // Ends error_injector


class NodeFailureInjector : public ErrorInjector {

  double error_rate_;

public:
  NodeFailureInjector(void) 
    : ErrorInjector(kUniform, stdout), error_rate_(0.0) {}

  NodeFailureInjector(double error_rate, RandType rand_type=kUniform, FILE *logfile=stdout) 
    : ErrorInjector(rand_type, logfile), error_rate_(error_rate) {}

  virtual bool InjectAndTest(void)=0;
  virtual ~NodeFailureInjector(void) {}
};



#endif

