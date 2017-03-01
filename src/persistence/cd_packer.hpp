#include "packer.hpp"

namespace cd {
//  Preservation
//    1. Single entry
//    2. 
// Restoration
//    1. Prefetch
class CDPacker : public Packer<CDEntry> {
  public:
    CDErrType Restore(uint64_t tag, char *dst=NULL) 
    {
      MYDBG("tag:%lu\n", tag);
      // 1. Find entry in table store
      // 2. Copy data from data store to src
      CDEntry entry = *reinterpret_cast<CDEntry *>(table_->Find(tag));
      dst = (dst == NULL)? entry.src_ : dst;
      data_->Read(dst, entry.size(), entry.offset_);
      return kOK;
    }
};


}
