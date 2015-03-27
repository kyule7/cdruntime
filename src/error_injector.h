#ifndef _ERROR_INJECTOR_H
#define _ERROR_INJECTOR_H

#include <cstdio>

class ErrorInjector {

protected:

  bool enabled_;
  FILE *logfile_;

public:

  ErrorInjector(void) {
    enabled_    = false;
    logfile_    = stdout;
  }

  ErrorInjector(FILE *logfile=stdout) {
    enabled_    = false;
    logfile_    = stdout;
  }

  virtual ~ErrorInjector(void) {}

  void Init(FILE *logfile=stdout) {
    enabled_    = false;
    logfile_    = stdout;
  }
  
  void Enable(void)  { enabled_=true; }
  void Disable(void) { enabled_=false; }
  void SetLogfile(FILE *logfile) { logfile_ = logfile; }
  virtual void Inject(void)=0;
      
};

class MemoryErrorInjector : public ErrorInjector {

  void    *data_;
  uint64_t size_;

public:
  MemoryErrorInjector(void *data, uint64_t size)
    : data_(data), size_(size), ErrorInjector(stdout) {}

  MemoryErrorInjector(void *data, uint64_t size, FILE *logfile)
    : data_(data), size_(size), ErrorInjector(logfile) {}
  virtual ~MemoryErrorInjector() {}

  int64_t SetRange(uint64_t range)=0;
  void PushRange(void *data, uint64_t ndata, uint64_t sdata, char *desc)=0;

};

class NodeFailureInjector : public ErrorInjector {

  double error_rate_;

public:
  NodeFailureInjector(void) 
    : error_rate_(0.0), ErrorInjector(stdout) {}

  NodeFailureInjector(double error_rate, FILE *logfile=stdout) 
    : error_rate_(error_rate), ErrorInjector(logfile) {}

  virtual ~NodeFailureInjector(void) {}
};

#endif

