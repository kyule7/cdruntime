#ifndef _BUFFER_CONSUMER_INTERFACE_H
#define _BUFFER_CONSUMER_INTERFACE_H
#include <pthread.h>

namespace cd {

extern pthread_cond_t  full; 
extern pthread_cond_t  empty;
extern pthread_mutex_t mutex;

inline void BufferLock(void)
{ pthread_mutex_lock(&mutex); }

inline void BufferUnlock(void)
{ pthread_mutex_unlock(&mutex); }

}
#endif
