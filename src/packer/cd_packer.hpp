#include "packer.hpp"

namespace cd {
//  Preservation
//    1. Single entry
//    2. 
// Restoration
//    1. Prefetch
class CDPacker : public Packer<CDEntry> {
  public:
    void Restore(uint64_t tag) 
    {
      MYDBG("tag:%lu\n", tag);
      // 1. Find entry in table store
      // 2. Copy data from data store to src
      CDEntry entry = *reinterpret_cast<CDEntry *>(table_->Find(tag));
      data_->Read(entry.src_, entry.size(), entry.offset_);
    }
};


}
