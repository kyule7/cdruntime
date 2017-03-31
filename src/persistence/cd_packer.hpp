#include "packer.hpp"

namespace packer {
//  Preservation
//    1. Single entry
//    2. 
// Restoration
//    1. Prefetch
class CDPacker : public Packer<CDEntry> {
  public:
    CDPacker(TableStore<CDEntry> *table=NULL, DataStore *data=NULL) 
      : Packer<CDEntry>(table, data) {}
    CDPacker(bool alloc, TableStore<CDEntry> *table, DataStore *data=NULL) 
      : Packer<CDEntry>(alloc, table, data) {}
    virtual ~CDPacker() {}
    char *Restore(uint64_t tag, char *dst=NULL) 
    {
      void *ret = dst;
      MYDBG("tag:%lu\n", tag);
      // 1. Find entry in table store
      // 2. Copy data from data store to src
      CDEntry *pentry = reinterpret_cast<CDEntry *>(table_->Find(tag));
      if(pentry == NULL) {
        for(int i=0; i<table_->used(); i++) {
          printf("table:%lu\n", (*table_)[i].id_);
        }
      }
      PACKER_ASSERT(pentry != NULL);
      CDEntry entry = *pentry;
      dst = (dst == NULL)? entry.src_ : dst;
      PACKER_ASSERT(dst == entry.src_);

#if 1
      if( entry.size_.Check(Attr::knested) == false) {
        data_->GetData(dst, entry.size(), entry.offset_);
      } else {
        
//        uint64_t produced = data_->Fetch(entry.size(), entry.offset_);
//        printf("\n\n TEST $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n"); //getchar();
        uint64_t table_offset = (uint64_t)entry.src_;
        uint64_t data_size   = table_offset - entry.offset_; //entry.offset_ - table_offset;
        uint64_t table_size  = entry.size() - data_size;
//        printf("Check entry for packed obj:totsize:%lu offset:%lu tableoffset:%lu tablesize:%lu\n", 
//           entry.size(), entry.offset_, table_offset, table_size); //getchar();
        data_->Fetch(table_size, table_offset);
        CDEntry *pEntry = reinterpret_cast<CDEntry *>(
            data_->GetPtr(table_offset + data_->head()));

        // Test
//        if(0)
        {
          printf("\n\n $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n");
          uint64_t *ptest = reinterpret_cast<uint64_t *>(pEntry);
          for(uint64_t i=0; i<table_size/sizeof(CDEntry); i++) {
            printf("%6lu %6lu %6lu %6lu\n", *ptest, *(ptest+1), *(ptest+2), *(ptest+3));
            ptest += 4;
          }
          printf("\n\n $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n\n\n");
          //getchar();
        }
        // This will just read from cache (memory)
        uint64_t num_entries = table_size/sizeof(CDEntry);
//        printf("# of entries:%lu, head:%lu, pos_buf:%lu\n", num_entries, data_->head(), pEntry->offset_); //getchar();
        for(uint32_t i=0; i<num_entries; i++, pEntry++) {
          data_->GetData(pEntry->src_, pEntry->size(), pEntry->offset_);
        }

//        PACKER_ASSERT_STR(align_up(table_size) == consumed, 
//            "tablesize:%lu==%lu(consumed)\n", table_size, consumed);

      }
#else
      if( entry.size_.Check(Attr::knested) == false ) {
        data_->Read(dst, entry.size(), entry.offset_);
      } else {
        // Table format
        // [ID] [SIZE]  [OFFSET] [SRC]
        //  ID  totsize  offset  tableoffset
        posix_memalign(&ret, CHUNK_ALIGNMENT, entry.size());
        printf("Read:%lu, %lu\n", entry.size(), entry.offset_);
        data_->Read((char *)ret, entry.size(), entry.offset_);
        MagicStore *magic = reinterpret_cast<MagicStore *>(ret);
        //uint64_t *packed_data = reinterpret_cast<uint64_t *>((char *)ret + sizeof(MagicStore));
        int *packed_data = reinterpret_cast<int *>(ret);
        printf("#### check read magicstore###\n");
        for(int i=0; i<128/16; i++) {
          for(int j=0; j<16; j++) {
            printf("%4d ", *(packed_data + i*16 + j));
          }
          printf("\n");
        } 
        printf("#### check read magicstore###\n");
        printf("Read id:%lu, attr:%lx chunk: %lu at %lu, "
               "Read MagicStore: size:%lu, tableoffset:%lu, entry_type:%u\n", 
                entry.id_, entry.attr(), entry.size(), entry.offset(), 
                magic->total_size_, magic->table_offset_, magic->entry_type_);
        getchar();
//        PACKER_ASSERT(magic->table_offset_ == (uint64_t)entry.src_);
        PACKER_ASSERT_STR(magic->total_size_ == entry.size(), 
            "magic size:%lu==%lu\n", magic->total_size_, entry.size());
        MYDBG("Read MagicStore: size:%lu, tableoffset:%lu, entry_type:%u\n", 
              magic->total_size_, magic->table_offset_, magic->entry_type_);
      }
#endif
      return (char *)ret;
    }
};


}
