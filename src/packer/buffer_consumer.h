#ifndef _BUFFER_CONSUMER_H
#define _BUFFER_CONSUMER_H

#include <vector>
#include <cstdint>
#include "buffer_consumer_interface.h"

namespace cd {

class DataStore;
// Singleton
class BufferConsumer {
    static BufferConsumer *buffer_consumer_;
    std::vector<DataStore *> buf_list_;
    BufferConsumer(void) {}
    ~BufferConsumer(void);
    void Init(void);
  public:
    static void *ConsumeBuffer(void *parm); 
    static BufferConsumer *Get(void);
    void Delete(void);
  public:
    void InsertBuffer(DataStore *ds);
    void RemoveBuffer(DataStore *ds);
    DataStore *GetBuffer(void); 
//    void WriteAll(DataStore *buffer);
//    char *ReadAll(uint64_t len);
};

static inline BufferConsumer *GetBufferConsumer(void) { return BufferConsumer::Get(); }


} // namespace cd ends

#endif
