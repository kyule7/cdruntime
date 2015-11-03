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
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCuDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCuDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCuDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include "error_injector.h"
#if CD_ERROR_INJECTION_ENABLED

#include <cstdio>
#include <iostream>
#include <random>
#include <cstdio>
#include "cd_def_internal.h"
#include "cd_global.h"

#define DEFAULT_ERROR_THRESHOLD 0.0

using namespace cd;
using namespace cd::interface;


// Uniform Random
double UniformRandom::GenErrorVal(void) 
{
  distribution_.reset();
  double rand_var = distribution_(generator_);
  std::cout << "rand var: " <<rand_var << std::endl;
  return rand_var;
}

/*
void UniformRandom::TestErrorProb(int num_bucket) 
{
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

// LogNromal
double LogNormal::GenErrorVal(void) 
{
  distribution_.reset();
  return distribution_(generator_);
}

void LogNormal::TestErrorProb(int num_bucket) 
{
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


// Exponenitial
double Exponential::GenErrorVal(void) 
{
  distribution_.reset();
  return distribution_(generator_);
}

void Exponential::TestErrorProb(int num_bucket) 
{
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


// Normal
double Normal::GenErrorVal(void) 
{
  distribution_.reset();
  return distribution_(generator_);
}

void Normal::TestErrorProb(int num_bucket) 
{
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


// Poisson
double Poisson::GenErrorVal(void) 
{
  return static_cast<double>(distribution_(generator_));
}

void Poisson::TestErrorProb(int num_bucket) 
{
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
*/

// ErrorInjector
ErrorInjector::ErrorInjector(void)
{
  rand_generator_ = //new UniformRandom();
  enabled_    = false;
  logfile_    = stdout;
  threshold_  = DEFAULT_ERROR_THRESHOLD;
}

ErrorInjector::ErrorInjector(double threshold, RandType rand_type, FILE *logfile) 
{
  rand_generator_ = CreateErrorProb(rand_type);
  enabled_    = false;
  logfile_    = stdout;
  threshold_  = threshold;
}

ErrorInjector::ErrorInjector(bool enabled, double threshold, RandType rand_type, FILE *logfile) 
{
  rand_generator_ = CreateErrorProb(rand_type);
  enabled_    = enabled;
  logfile_    = stdout;
  threshold_  = threshold;
}

void ErrorInjector::Init(RandType rand_type, FILE *logfile)
{
  if(rand_generator_ == NULL)
    rand_generator_ = CreateErrorProb(rand_type);
  enabled_    = false;
  logfile_    = stdout;
}

ErrorProb *ErrorInjector::CreateErrorProb(RandType rand_type)
{
  ErrorProb *random_number = NULL;
//    switch(rand_type) {
//      case kUniform : 
//        random_number = new UniformRandom();
//        break; 
//      case kExponential : 
//        random_number = new Exponential();
//        break; 
//      case kLogNormal :
//        random_number = new LogNormal();
//        break; 
//      case kNormal :
//        random_number = new Normal();
//        break; 
//      case kPoisson :
//        random_number = new Poisson();
//        break; 
//      default :
//        random_number = new Exponential();
//    }
  return random_number;
}



// CDErrorInjector
CDErrorInjector::CDErrorInjector(void) 
  : ErrorInjector(0.0, kUniform, stdout) 
{
  force_to_fail_ = false;
}

CDErrorInjector::CDErrorInjector(double error_rate, RandType rand_type, FILE *logfile) 
  : ErrorInjector(true, error_rate, rand_type, logfile) 
{
  force_to_fail_ = false;
}

CDErrorInjector::CDErrorInjector(std::initializer_list<uint32_t> cd_list_to_fail, 
                                 std::initializer_list<uint32_t> task_list_to_fail, 
                                 double error_rate) 
  : ErrorInjector(true, error_rate, kUniform, stdout) 
{
  for(auto it=cd_list_to_fail.begin(); it!=cd_list_to_fail.end(); ++it) {
    CD_DEBUG("push back cd %u\n", *it);
    cd_to_fail_.push_back(*it);
  }
  for(auto it=task_list_to_fail.begin(); it!=task_list_to_fail.end(); ++it) {
    CD_DEBUG("push back task %u\n", *it);
    task_to_fail_.push_back(*it);
  }
  force_to_fail_ = true;
  CD_DEBUG("EIE CDErrorInjector created!\n");
}

CDErrorInjector::CDErrorInjector(std::initializer_list<uint32_t> cd_list_to_fail, 
                                 std::initializer_list<uint32_t> task_list_to_fail, 
                                 double error_rate, RandType rand_type, FILE *logfile) 
  : ErrorInjector(true, error_rate, rand_type, logfile) 
{
  for(auto it=cd_list_to_fail.begin(); it!=cd_list_to_fail.end(); ++it) {
    cd_to_fail_.push_back(*it);
  }
  for(auto it=task_list_to_fail.begin(); it!=task_list_to_fail.end(); ++it) {
    task_to_fail_.push_back(*it);
  }
  force_to_fail_ = true;
}

CDErrorInjector::CDErrorInjector(uint32_t cd_to_fail, uint32_t task_to_fail, 
                double error_rate, RandType rand_type, FILE *logfile) 
  : ErrorInjector(true, error_rate, rand_type, logfile) 
{
  cd_to_fail_.push_back(cd_to_fail);
  task_to_fail_.push_back(task_to_fail); 
  force_to_fail_ = true;
}


CDErrorInjector::CDErrorInjector(uint32_t cd_to_fail, 
                                 uint32_t task_to_fail, 
                                 uint32_t rank_in_level, 
                                 uint32_t task_in_color,
                                 double error_rate, 
                                 RandType rand_type, 
                                 FILE *logfile) 
  : ErrorInjector(error_rate, rand_type, logfile) 
{
  cd_to_fail_.push_back(cd_to_fail);
  task_to_fail_.push_back(task_to_fail); 
  rank_in_level_ = rank_in_level;
  task_in_color_ = task_in_color;
  force_to_fail_ = true;
}

CDErrorInjector::CDErrorInjector(std::initializer_list<uint32_t> cd_list_to_fail, std::initializer_list<uint32_t> task_list_to_fail,
                uint32_t rank_in_level, uint32_t task_in_color,
                double error_rate, RandType rand_type, FILE *logfile) 
  : ErrorInjector(error_rate, rand_type, logfile) 
{
  for(auto it=cd_list_to_fail.begin(); it!=cd_list_to_fail.end(); ++it) {
    cd_to_fail_.push_back(*it);
  }
  for(auto it=task_list_to_fail.begin(); it!=task_list_to_fail.end(); ++it) {
    task_to_fail_.push_back(*it);
  }

  rank_in_level_ = rank_in_level;
  task_in_color_ = task_in_color;
  force_to_fail_ = true;
}

void CDErrorInjector::RegisterTarget(uint32_t rank_in_level, uint32_t task_in_color)
{
  rank_in_level_ = rank_in_level;
  task_in_color_ = task_in_color;
}

bool CDErrorInjector::InjectAndTest()
{
  if(enabled_ == false) return false;

  if( force_to_fail_ ) {
    CD_DEBUG("force_to_fail is turned on. cd fail list size : %zu, task fail list size : %zu\n", cd_to_fail_.size(), task_to_fail_.size());

    for(auto it=cd_to_fail_.begin(); it!=cd_to_fail_.end(); ++it) {
      CD_DEBUG("cd_to_fail : %u = %u\n", *it, rank_in_level_);
      if(*it == rank_in_level_) {
        CD_DEBUG("cd failed rank_in_level #%u\n", *it);
        return true;
      }
    }

    for(auto it=task_to_fail_.begin(); it!=task_to_fail_.end(); ++it) {
      CD_DEBUG("task_to_fail : %u = %u\n", *it, task_in_color_);
      if(*it == task_in_color_) {
        CD_DEBUG("task failed task_in_color #%u\n", *it);
        return true;
      }
    }

    CD_DEBUG("\n");
//      enabled_ = false; // Turn off error injection in the reexecution.
    return false; // if it reach this point. No tasks/CDs are registered to be failed.
                  // So, return false.
  }
  double rand_var = rand_generator_->GenErrorVal();
  bool error = threshold_ < rand_var;
  CD_DEBUG("task #%d failed : %f(threshold) < %f(random var)\n", task_in_color_, threshold_, rand_var);
  CD_DEBUG("EIE\n");

  enabled_ = false;
  return error;
}







/** @} */ // Ends error_injector


#endif
