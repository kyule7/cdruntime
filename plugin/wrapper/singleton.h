#ifndef _SINGLETON_H
#define _SINGLETON_H
#include <sys/time.h>
#include "logging.h"
//#include "libc_wrapper.h"
namespace logger {

struct Singleton { 
    struct timeval start_, 
                   end_,
                   begin_;
  public:
    Singleton(void); 
    ~Singleton(void);
    void Initialize(void);
    void BeginClk(int ftid);
    void EndClk(int ftid);
};

extern Singleton singleton;

}

#endif
