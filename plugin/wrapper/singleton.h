#include <sys/time.h>
#include "logging.h"

namespace logger {

struct Singleton { 
    struct timeval start_, 
                   end_,
                   begin_;
  public:
    Singleton(void); 
    ~Singleton(void);
    void Initialize(void);
    void BeginClk(enum FTID ftid);
    void EndClk(enum FTID ftid);
};

extern Singleton singleton;

}
