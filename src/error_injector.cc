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
#include <cstdlib>
#include <iostream>
//#include <random>
#include "cd_def_internal.h"
#include "cd_global.h"

#define DEFAULT_ERROR_THRESHOLD 0.0

using namespace cd;
using namespace cd::interface;


// Uniform Random
long double UniformRandom::GenErrorVal(void) 
{
  return drand48();
}


void UniformRandom::TestErrorProb(int num_bucket) 
{
  const int nrolls = 10000;  // number for test
  const int nstars = 100;    // maximum number of stars to distribute
  int bucket[num_bucket];
  
  for (int i=0; i<num_bucket; ++i) {
    bucket[i] = 0;
  }

  for (int i=0; i<nrolls; ++i) {
    long double number = GenErrorVal();
    if(number < 0) std::cout << "negative : " << number << std::endl;
    if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
      ++bucket[static_cast<int>(number)];
  } 

  for (int i=0; i<num_bucket; ++i) {
    std::cout << i << "-" << (i+1) << ":\t";
    std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
  }

}

/*
// LogNromal
double LogNormal::GenErrorVal(void) 
{
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
  rand_generator_ = new UniformRandom();
  enabled_    = false;
  logfile_    = stdout;
  error_rate_  = DEFAULT_ERROR_THRESHOLD;
}

ErrorInjector::ErrorInjector(double error_rate, RandType rand_type, FILE *logfile) 
{
  rand_generator_ = new UniformRandom();
  enabled_    = false;
  logfile_    = stdout;
  error_rate_  = error_rate;
}

ErrorInjector::ErrorInjector(bool enabled, double error_rate, RandType rand_type, FILE *logfile) 
{
  rand_generator_ = new UniformRandom();
  enabled_    = enabled;
  logfile_    = stdout;
  error_rate_  = error_rate;
}

void ErrorInjector::Init(RandType rand_type, FILE *logfile)
{
  rand_generator_ = new UniformRandom();
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

long double ErrorInjector::GetErrorProb(double error_rate, double unit_time) 
{
  long double reliability = exp((-1.0)*(long double)error_rate*(long double)unit_time);
  CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "reliability %Le = 1.0 - exp(-%f*%f)   %Le\n", reliability, error_rate, unit_time);
  return reliability; 
}

uint64_t ErrorInjector::Inject(void) 
{
  return InjectError(error_rate_);
}

// It generate an uniform-random number between 0 and 1,
// and compare that with reliability (probability to survive from the error type)
// If the random number is greater than reliability,
// it injects error!
uint64_t ErrorInjector::InjectError(const long double &error_prob)
{
  long double rand_var = rand_generator_->GenErrorVal();
  int error = error_prob < rand_var;
  CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "Error %Le(prob to survive) < %Le(random var)   ERROR? %d\n", error_prob, rand_var, error);

  return error;
}

uint64_t SystemErrorInjector::Inject(void) 
{
  uint64_t error_vec = NO_ERROR_INJECTED;
  CD_CLOCK_T curr_clk = CD_CLOCK();
  double period = (double)(curr_clk - prev_clk_)/CLK_NORMALIZER;
  prev_clk_ = curr_clk;

  // [Kyushick]
  // Check if error happened at every CD level. 
  // At each CD level, there is a claim that 
  // the CD can cover for recovery against a certain type of error.
  // Check this from leaf to root CD. 
  // If there is error occurred at upper level,
  // that overwrites rollback_point.
  for(auto it=sc_.failure_rate_.rbegin(); it!=sc_.failure_rate_.rend(); ++it) {
    if( InjectError(GetErrorProb(it->second, period)) ) { 
      error_vec |= it->first;
//      printf("ERROR!!! %lx, curr: %lx\n", error_vec, it->first);
    }
    CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "Error rate %lu : %f (%lx) [period:%lf]\n", 
        it->first, it->second, error_vec, period);
  }
  return error_vec;
}

#if 0
MultiTypeErrorInjector::MultiTypeErrorInjector(const std::initializer_list<std::pair<uint32_t, double>> &err_type_list)
{
  // FIXME : make sure how order map structure is.
  for(auto it=err_type_list.begin(); it!=err_type_list.end(); ++it) {
    if(error_prob_bin_.size() != 0) {
      error_prob_bin_[it->first] = it->second;
    } else {
      error_prob_bin_[it->first] = it->second + error_prob_bin_.end()->second;
    }
  }
}


uint32_t MultiTypeErrorInjector::Inject(void) 
{
  double rand_var = rand_generator_->GenErrorVal();

 /*      a     b    c      no error
 *  +-------+----+----+------------------+
 *  0      .2   .3   .4                  1
 */
  for(auto it=error_prob_bin_.begin(); it!=error_prob_bin_.end(); ++it) {
    if(rand_var < it->second) {
      return it->first;
    }
  }
}
#endif

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
    CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "push back cd %u\n", *it);
    cd_to_fail_.push_back(*it);
  }
  for(auto it=task_list_to_fail.begin(); it!=task_list_to_fail.end(); ++it) {
    CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "push back task %u\n", *it);
    task_to_fail_.push_back(*it);
  }
  force_to_fail_ = true;
  CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "EIE CDErrorInjector created!\n");
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

uint64_t CDErrorInjector::Inject(void)
{
  if(enabled_ == false) return false;

  if( force_to_fail_ ) {
    CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "force_to_fail is turned on. cd fail list size : %zu, task fail list size : %zu\n", cd_to_fail_.size(), task_to_fail_.size());

    for(auto it=cd_to_fail_.begin(); it!=cd_to_fail_.end(); ++it) {
      CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "cd_to_fail : %u = %u\n", *it, rank_in_level_);
      if(*it == rank_in_level_) {
        CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "cd failed rank_in_level #%u\n", *it);
        return true;
      }
    }

    for(auto it=task_to_fail_.begin(); it!=task_to_fail_.end(); ++it) {
      CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "task_to_fail : %u = %u\n", *it, task_in_color_);
      if(*it == task_in_color_) {
        CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "task failed task_in_color #%u\n", *it);
        return true;
      }
    }

    CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "\n");
//      enabled_ = false; // Turn off error injection in the reexecution.
    return false; // if it reach this point. No tasks/CDs are registered to be failed.
                  // So, return false.
  }
  long double rand_var = rand_generator_->GenErrorVal();
  int error = error_rate_ < rand_var;
  CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "task #%d failed : %f(error_rate) < %Le(random var)\n", task_in_color_, error_rate_, rand_var);
  CD_DEBUG_COND(DEBUG_OFF_ERRORINJ, "EIE\n");

  enabled_ = false;
  return error;
}

/** @} */ // Ends error_injector


#endif
