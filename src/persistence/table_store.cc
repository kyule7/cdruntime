#include "table_store.hpp"
namespace packer {
BaseTable *GetTable(uint32_t entry_type, char *ptr_entry, uint32_t len_in_byte) 
{
  MYDBG("[%s] %u %p %u\n", __func__, entry_type, ptr_entry, len_in_byte);
  BaseTable *ret = NULL;
  switch(entry_type) { 
    case kBaseEntry: {
      ret = new TableStore<BaseEntry>(reinterpret_cast<BaseEntry *>(ptr_entry), len_in_byte);
      break;
    }
    case kCDEntry: {
      ret = new TableStore<CDEntry>(reinterpret_cast<CDEntry *>(ptr_entry), len_in_byte);
      break;
    }
    default:
                   {}
  }
  printf("[%s] %u %p %u ret:%p\n", __func__, entry_type, ptr_entry, len_in_byte, ret);
  return ret;
}
const char *DefaultHash(uint64_t id) { return ""; }
}
