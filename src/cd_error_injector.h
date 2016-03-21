#ifndef _ERROR_INJECTOR_H
#define _ERROR_INJECTOR_H

#include "error_injector.h"
#include "sys_err_t.h"

class SystemErrorInjector : public ErrorInjector {
  SystemConfig &system_config;
  CD_CLOCK_T prev_clk_;

  MultiErrorInjector(SystemConfig &sc)
  {
    system_config = sc;
    prev_clk_ = CD_CLOCK();
  }

  int Inject(void) {
    for(auto it!=system_config.begin(); it!=system_config.end(); ++it) {
      InjectError();      

    }
    CDHandle *cdh = cdh_;
    while(cdh != NULL) {
      cdh->error_injector_->Inject(cdh->failure_rate());
      cdh = GetParentCD(cdh->level());
    }
  }

  // Specialize error prob for specific CD granularity or task set
  // It is only valid at the current CD level where it is called
  void TestTargetToFail(std::initializer_list<uint32_t> cd_list_to_fail, 
                        std::initializer_list<uint32_t> task_list_to_fail, 
                        float error_rate)
  {
    err_desc = new CDErrorDesc(cd_list_to_fail, task_list_to_fail, error_rate);
  }
};



/*
class ErrorInjector {
public:
  enum RandType { kUniform = 0,
                  kExponential,
                  kLogNormal,
                  kNormal,
                  kPoisson
                };

private:
  ErrorProb *temp_error_;
  ErrorProb *spatial_error_;
  ErrorProb *node_failure_error_;
public:
  ErrorInjector(void) {
    temp_error_    = new Exponential();
    spatial_error_ = new UniformRandom();
    node_failure_error_ = new UniformRandom();
  }

  ErrorInjector(RandType temp_error_type=kUniform, 
                RandType spatial_error_type=kExponential,
                RandType node_failure_error_type=kUniform) 
{
    switch(temp_error_type) {
      case kUniform : 
        temp_error_ = new UniformRandom();
        break; 
      case kExponential : 
        temp_error_ = new Exponential();
        break; 
      case kLogNormal :
        temp_error_ = new LogNorm();
        break; 
      case kNormal :
        temp_error_ = new Normal();
        break; 
      case kPoisson :
        temp_error_ = new Poisson();
        break; 
      default :
        temp_error_ = new Exponential();
    }
 
    switch(spatial_error_type) {
      case kUniform : 
        spatial_error_ = new UniformRandom();
        break; 
      case kExponential : 
        spatial_error_ = new Exponential();
        break; 
      case kLogNormal :
        spatial_error_ = new LogNorm();
        break; 
      case kNormal :
        spatial_error_ = new Normal();
        break; 
      case kPoisson :
        spatial_error_ = new Poisson();
        break; 
      default :
        spatial_error_ = new UniformRandom();
    }
    switch(node_failure_error_type) {
      case kUniform : 
        node_failure_error_ = new UniformRandom();
        break; 
      case kExponential : 
        node_failure_error_ = new Exponential();
        break; 
      case kLogNormal :
        node_failure_error_ = new LogNorm();
        break; 
      case kNormal :
        node_failure_error_ = new Normal();
        break; 
      case kPoisson :
        node_failure_error_ = new Poisson();
        break; 
      default :
        node_failure_error_ = new UniformRandom();
    }
  }

  ~ErrorInjector(void) {
    delete temp_error_;
    delete spatial_error_;
  }

  void FlipBit(void *pA, uint64_t size) {
    char *pB = static_cast<char *>(pA);
    uint64_t error_val_mask = error_prob_.GenErrorVal() * (sizeof(uint64_t) * size + 1);
    uint64_t error_loc = error_prob_.GenErrorVal() * (sizeof(uint64_t) * size + 1);
    pB[error_loc] ^= error_val_mask;  // If the position in error mask is 1, flip it.
  }

  void FlipByte(void *pA, uint64_t size) {
    char *pB = static_cast<char *>(pA);
    char error_val_mask = error_prob_.GenErrorVal() * (sizeof(char) * size + 1);
    uint64_t error_loc = error_prob_.GenErrorVal() * (sizeof(uint64_t) * size + 1);
    pB[error_loc] ^= error_val_mask;  // If the position in error mask is 1, flip it.
  }

  void Flip4Bytes(void *pA, uint64_t size) {
    uint32_t *pB = static_cast<uint32_t *>(pA);
    uint32_t error_val_mask = error_prob_.GenErrorVal() * (sizeof(uint32_t) * size + 1);
    uint64_t error_loc = error_prob_.GenErrorVal() * (sizeof(uint64_t) * size + 1);
    pB[error_loc] ^= error_val_mask;  // If the position in error mask is 1, flip it.
  }

  void Flip8Bytes(void *pA, uint64_t size) {
    char *pB = static_cast<char *>(pA);
    uint64_t error_val_mask = error_prob_.GenErrorVal() * (sizeof(uint64_t) * size + 1);
    uint64_t error_loc = error_prob_.GenErrorVal() * (sizeof(uint64_t) * size + 1);
    pB[error_loc] ^= error_val_mask;  // If the position in error mask is 1, flip it.
  }

}
*/

#endif
