#ifndef _PACKER_H
#define _PACKER_H

#include "data_store.h"
#include "table_store.hpp"
#include "string.h"
namespace cd {

///////////////////////////////////////////////////////////////////////////////////
// 
// Packer can be reused for another environment such as CUDA or GasNET
// by inheriting Packer class and re-define Alloc/Realloc/Free/Read/Write methods.
//
// 1. Add
// 2. GetTotalData
//
// (ex) Packer packer(true, new MyTable, new MyCudaData); 
//     
template <typename EntryT> 
class Packer {
//  protected:
  public: 
    TableStore<EntryT> *table_;
    DataStore  *data_;
    uint64_t cur_pos_;
  public:
    Packer(uint64_t alloc=1, TableStore<EntryT> *table=NULL, DataStore *data=NULL) : cur_pos_(0) {
      if(table == NULL) {
        table_ = new TableStore<EntryT>(alloc);
      } else {
        table_ = table;
      }
      if(data == NULL) {
        data_ = new DataStore(alloc);
      } else {
        data_ = data;
      }
    }
    Packer(TableStore<EntryT> *table, DataStore *data=NULL) : cur_pos_(0) {
      if(table == NULL) {
        table_ = new TableStore<EntryT>(true);
      } else {
        table_ = table;
      }
      if(data == NULL) {
        data_ = new DataStore(true);
      } else {
        data_ = data;
      }
    }
    
    virtual ~Packer() { 
      delete table_;
      delete data_;
    }
    
    BaseTable *GetTable(void) { return table_; }

    ///@brief Add data to pack in packer data structure.
    uint64_t Add(char *app_data, uint64_t len, uint64_t id)
    {
      uint64_t offset = data_->Write(app_data, len);
      uint64_t ret = table_->Insert(EntryT(id, len, offset).SetSrc(app_data));
      return ret;
    }

    uint64_t Add(char *src, EntryT *entry)
    {
      uint64_t offset = data_->Write(src, entry->size());
      uint64_t ret = table_->Insert(entry->SetOffset(offset));
      return ret;
    }

    uint64_t Add(char *src, EntryT &&entry)
    {
      uint64_t offset = data_->Write(src, entry.size());
      uint64_t ret = table_->Insert(entry.SetOffset(offset));
      return ret;
    }
    
    EntryT *AddEntry(char *src, EntryT &&entry)
    {
      uint64_t offset = data_->Write(src, entry.size());
      EntryT *ret = table_->InsertEntry(entry.SetOffset(offset));
      return ret;
    }

    char *GetAt(uint64_t id, uint64_t &ret_size)
    {
      uint64_t ret_offset = 0;
      CDErrType err = table_->Find(id, ret_size, ret_offset);
      if(err == kNotFound) { assert(0); }
      char *dst = new char[ret_size];
      data_->Read(dst, ret_size, ret_offset);
      return dst;
    }

    char *GetAt(const EntryT &entry)
    {
      char *dst = new char[entry.size()];
      data_->Read(dst, entry.size(), entry.offset_);
      return dst;
    }

    ///@brief Get total size required for table (metadata) and data.
    char *GetTotalData(uint64_t &total_data_size)
    {
      const uint64_t tablesize = table_->usedsize();
      const uint64_t datasize = data_->tail();
      const uint64_t table_offset = datasize + sizeof(MagicStore);
      const uint64_t total_size = tablesize + table_offset;
      void *total_data = NULL;

      total_data_size = total_size;
      if(total_size != 0) {
        data_->Alloc(&total_data, datasize + tablesize);

        printf("total:%lu\n", total_size);
        printf("[%s] (%zu+%lu+%lu)%lu %lu %u\n", __func__, 
            sizeof(MagicStore), datasize, tablesize,
            total_size, table_offset, table_->type());

        // Update MagicStore
        MagicStore magic(total_size, table_offset, table_->type());
        data_->UpdateMagic(magic);
        
        printf("Creat MagicStore(%lu %lu %u)\n", total_size, table_offset, table_->type());
    
        char *target = (char *)total_data;
        // Update MagicStore
        *(reinterpret_cast<MagicStore *>(target-sizeof(MagicStore))) = magic;

        // Copy DataChunk
        data_->ReadAll(target);
        target += datasize;

        table_->Read(target, tablesize, 0);
      }

      uint32_t *test_ptr = (uint32_t *)total_data;
      printf("[%s] return:%p %u %u %u ###\n", __func__, total_data, *test_ptr, *(test_ptr+1), *(test_ptr+2));

      return (char *)total_data;
    }

    // Flush data store to file and append table store, then update magic store
    void WriteToFile(void) 
    {
      // Update MagicStore
      const uint64_t tablesize = table_->usedsize();
      const uint64_t table_offset = data_->used() + sizeof(MagicStore);
      const uint64_t total_size = tablesize + table_offset;
      MagicStore magic(total_size, table_offset, table_->type());
      data_->UpdateMagic(magic);
      
      // FIXME: Insert entry for the previous table chunk
      cur_pos_ = table_->Advance(data_->used());
//       table_->Insert(entry.SetOffset(offset));
      uint64_t table_offset_check = data_->Flush(table_->GetPtr(), table_->tablesize());
      ASSERT(table_offset == table_offset_check);
    }

    CDErrType Clear(bool reuse)
    {
      CDErrType err = table_->FreeData(reuse);
      if(err != kOK) {
        ERROR_MESSAGE("Packer::Clear Table : err:%u\n", err);
      }
      err = data_->FreeData(reuse);
      if(err != kOK) {
        ERROR_MESSAGE("Packer::Clear Data : err:%u\n", err);
      }
      return err;
    }

    ///@brief Grow buffer size internally used in packer.
    void SetBufferGrow(uint64_t data_grow_unit, uint64_t table_grow_unit=0)
    {
      if(data_grow_unit  != 0) data_->SetGrowUnit(data_grow_unit);
      if(table_grow_unit != 0) table_->SetGrowUnit(table_grow_unit);
    }

    void Copy(const Packer& that) {
      that.table_->Copy(table_);
      that.data_->Copy(data_);
    }

    // Soon to be deprecated
//    uint64_t Add(uint64_t id, uint64_t length, const void *position)
//    { return Add(position, length, id); }
};




// The role of Unpacker is to get a proper data from serialized data+metadata by packer.
// Unpacker takes a pointer for serialized data and ID.
// Unpacker searches for the ID in the serialized packer, 
// then return the proper region of data.
// it returns just pointer, therefore user needs to copy 
// to the app data for restoration from Unpacker.
// Ex 1) Find data with ID
// Unpacker unpacker;
// void *from_packer = unpacker.GetAt(serialized, id, returned_size);
// memcpy(orig_data, from_packer, returned_size);
//
// Or user may be able to use in the following way.
// void *from_packer = unpacker.GetAt(serialized, id, orig_data, returned_size);
//
// Ex 2) Find data from the first in order
// Unpacker unpacker;
// void *from_packer = unpacker.GetNext(serialized, returned_id, returned_size);
//
// ** Assumption **
// At the point when unpacker is used, 
// the destination ptr from chunk is already determined.
// 
// ** Operation **
// Unpacker do not perform actual copies. 
// The means of copy may vary depending on cases. (ex. CUDA, MPI, file)
// Actually copy must be done by some other handler using unpacker interface.
// Unpacker just provide the offset and the size in data chunk! (or entry pointer)
//
//  int ret = unpacker.GetAt(chunk, id, ret_size, ret_offset);
//  switch(ret) {
//    kDRAM:
//      memcpy(dst, chunk + ret_offset, ret_size);
//      break;
//    kOSFile:
//      read(dst, ret_offset, ret_size, fp);
//      break;
//    kRemote:
//      MPI_Get();
//      break;
//    default:
//  }
//
class Unpacker {
  protected:
    MagicStore magic_;
    uint64_t cur_pos_;
    BaseTable *table_;
  public:
    // If chunk is in the file indicated by fp
//    Unpacker(FILE *fp) : cur_pos_(0) {
//      // Update magic_ and table_
//      if(fp != NULL)
//        table_ = ReadFromFile(fp);
//    }
    
    Unpacker(void) : cur_pos_(0), table_(NULL) {}
    // If just pointer for chunk in memory is known
    Unpacker(char *chunk) : cur_pos_(0) {
      // Update magic_ and table_
      if(chunk != NULL) 
        table_ = ReadFromMemory(chunk);
      printf("Unpacker is created\n");
    }

    // If table to unpack is known, just use the table
    Unpacker(BaseTable *table) : cur_pos_(0), table_(table) {}

//    BaseTable *ReadFromFile(FILE *fp) 
//    {
//      fread(&magic_, sizeof(MagicStore), 1, fp);
//      const uint32_t table_size = magic_.total_size_ - magic_.table_offset_;
//      char *ptable = (char *)malloc(table_size);
//      fseek(fp, magic_.table_offset_, SEEK_SET);
//      fread(ptable, sizeof(char), table_size, fp);
//      return GetTable(magic_.entry_type_, ptable, table_size);
//    }
    virtual ~Unpacker(void) {}

    BaseTable *ReadFromMemory(char *chunk, bool alloc=true)
    {
      memcpy(&magic_, chunk, sizeof(MagicStore));
      const uint32_t table_size = magic_.total_size_ - magic_.table_offset_;
      printf("[%s]type:%u, chunk:%p, offset:%lu, tablesize:%u\n", __func__,
              magic_.entry_type_, chunk, magic_.table_offset_, table_size);

      uint32_t *test_ptr = (uint32_t *)(chunk + magic_.table_offset_);
      //printf("return:%p %lu %lu %lu\n", test_ptr, *test_ptr, *(test_ptr+1), *(test_ptr+2));
      for(uint64_t i=0; i<10; i++, test_ptr+=4) {
        
        printf("test_ptr return:%p %u %u %u\n", test_ptr, *test_ptr, *(test_ptr+1), *(test_ptr+2));
      }
      
      //getchar();

      char *ptable = NULL;
      if(alloc) {
        ptable = (char *)malloc(table_size);
        memcpy(ptable, chunk + magic_.table_offset_, table_size);
      } else {
        ptable = chunk + magic_.table_offset_;
      }
      return GetTable(magic_.entry_type_, ptable, table_size);
    }
    
    char *GetAt(char *ptr_data, uint64_t id, uint64_t &return_size)
    {
      if(table_ != NULL) {
        uint64_t data_offset = 0;
        CDErrType ret = table_->Find(id, return_size, data_offset);
        ptr_data += data_offset + sizeof(MagicStore);
        if(ret == kNotFound)
          ERROR_MESSAGE("Could not find entry %lu\n", id);
      } else {
        ptr_data = NULL;
        ERROR_MESSAGE("Unpacking from invalid table\n");
      }
      return ptr_data;
    }

    char *GetNext(char *ptr_data, uint64_t &ret_id, uint64_t &ret_size, uint64_t &ret_offset)
    {
//      uint64_t *ptr = (uint64_t *)table_->GetPtr() + cur_pos_;//Entry(ret_id, ret_size, ret_offset);
      assert(table_ != NULL);
      CDErrType ret = table_->GetAt(cur_pos_++, ret_id, ret_size, ret_offset);
//      printf("[%s before] %lu %lu %lu\n", __func__, ret_id, ret_size, ret_offset);
//      ret_id = *ptr;
//      ret_size = *(ptr+1);
//      ret_offset = *(ptr+2);
//      printf("[%s after] %lx %lx %lx\n", __func__, ret_id, ret_size, ret_offset);
      // FIXME
//      cur_pos_ += sizeof(CDEntry)/sizeof(uint64_t);
      if(ret == kOK)
        return (char *)(ptr_data + (sizeof(MagicStore) + ret_offset));
      else if(ret == kOutOfTable)
        return NULL;
      else {
        ERROR_MESSAGE("Undefined return from TableStore::GetAt (%d)\n", ret);
      }
    }

};

//#define FILE_BUFFER_SIZE 67108864 // 64MiB
//#define FILE_BUFFER_SIZE 4096 
#define FILE_BUFFER_SIZE 256 
class FileUnpacker : public Unpacker {
  public:
    char *buffer_;
    FILE *fp_;
    uint64_t used_;
    uint64_t buffer_size_;
    FileUnpacker(FILE *fp) : Unpacker() {
      fp_ = fp;
      if(fp != NULL) { 
        AllocateBuffer(); 
        table_ = ReadFromFile(fp); 
      }
    }
    ~FileUnpacker(void) { free(buffer_); }
  private:
    void AllocateBuffer(void) 
    {
      buffer_size_ = FILE_BUFFER_SIZE;
      buffer_ = (char *)malloc(buffer_size_);
      used_ = 0;
    }

    BaseTable *ReadFromFile(FILE *fp) 
    {
      fread(&magic_, sizeof(MagicStore), 1, fp);
      const uint32_t table_size = magic_.total_size_ - magic_.table_offset_;
      char *ptable = (char *)malloc(table_size);
      fseek(fp, magic_.table_offset_, SEEK_SET);
      fread(ptable, sizeof(char), table_size, fp);
      return GetTable(magic_.entry_type_, ptable, table_size);
    }
  public:
    char *GetNext(uint64_t &ret_id, uint64_t &ret_size, uint64_t &ret_offset)
    {
      uint64_t datasize = magic_.table_offset_ - sizeof(MagicStore);
      uint64_t available = buffer_size_ - used_;
      uint64_t fetchsize = BUFFER_INSUFFICIENT;
      if(datasize <= available) { // cache mode
        fetchsize = datasize;
      } 
      else { // buffer mode
        while(1) {
          available = buffer_size_ - used_;
          fetchsize = table_->GetChunkToFetch(available, cur_pos_);
          if(fetchsize == BUFFER_INSUFFICIENT) {
            buffer_size_ <<= 1;
            char *new_buffer = (char *)malloc(buffer_size_);
            memcpy(new_buffer, buffer_, used_);
            free(buffer_);
            buffer_ = new_buffer;
          } else {
            break;
          }
        }
      }
      ReadFile(buffer_, fetchsize, used_);
      char *data_begin = buffer_ + used_;
      used_ += fetchsize;
      return data_begin;
    }

    uint64_t ReadFile(char *buffer, uint64_t size, uint64_t offset)
    {
      return fread(buffer + offset, sizeof(char), size, fp_);
    }
};


} // namespace cd ends

#if 0

//      table_type_ = magic_.entry_type_;
//      uint64_t table_offset  = magic_.table_offset();
//      uint64_t totsize = magic_.total_size();
//      uint64_t table_size = totsize - table_offset;
//      uint32_t entry_size = entry_size[table_type_];
//      assert(entry_size);
//      assert(table_size % entry_size == 0);
//      table_ = (void *)malloc(table_size);
//      fread(table_, sizeof(entry_size), table_size/entry_size, fp);
    void *GetAt(char *chunk, uint64_t id, uint64_t &return_size)
    {
      char *data_ptr = chunk + sizeof(MagicStore);
      char *table_ptr = chunk + magic_.table_offset();
      switch(magic_.entry_type_) { 
        case kBaseEntry: {
                           BaseEntry *target = Foo<BaseEntry>(table, id);
                           break;
                         }
        case kCDEntry: {
                         CDEntry *target = Foo<CDEntry>(table, id);
                         break;
                       }
        defalt:
      }

//      if(magic_.entry_type_ == kBaseEntry) {
//        TableStore<CDEntry> table(false, table_, NULL);
//        BaseEntry *target = table.Find(id);
//        if(target != NULL) {
//          return_size = target->size();
//          offset = target->offset();
//        }
//      } else if(table_type_ == kCDEntry) {
//
//      } else {
//        ERROR_MESSAGE("Undefined entry type: %u\n", table_type_);
//      }
        
    }

    template <typename EntryT>
    EntryT Foo(uint64_t id, char *data_ptr, EntryT *entry_ptr, uint64_t &returned_size) {
      // Do not allocate, but just read from entry_ptr (beginning of table)
      // At the point when we invoke this, we know the entry type
      TableStore<EntryT> table(false, table_, NULL);
      EntryT *target = table.Find(id);
      if(target != NULL) {
        returned_size = target->size();
        offset = target->offset();
      }
      return (data_ptr + offset);
    } 
#else
//    void *GetAt(char *chunk, uint64_t id, uint64_t &return_size)
//    {
//      char *ptr_table = chunk + magic_.table_offset();
//      uint32_t entry_size = entry_size[magic_.table_type()];
//      uint32_t table_size = magic_.table_size();
//      BaseEntry entry = GetEntryFromTable(ptr_table, table_size, entry_size, id);
//      return ReadFromDataStore(entry);
//    }
//
//    virtual void ReadFromDataStore(BaseEntry entry) {
//
//    }
//
//    BaseEntry GetEntryFromTable(char *ptr_table, uint32_t table_size, uint32_t entry_size, uint64_t id)
//    {
//      uint32_t cur_pos = 0;
//      while(cur_pos < table_size) {
//        // The rule for entry is that the first element in object layout is always ID.
//        if( *(reinterpret_cast<uint64_t *>(ptr_table)) == id ) {
//          MYDBG("%lu == %lu\n", *(reinterpret_cast<uint64_t *>(ptr_table)), id);
//          break;
//        } else {
//          ptr_table += entry_size;
//        }
//      }
//      // size is the second element
//      ptr_table += sizeof(uint64_t);
//      uint64_t returned_size = *reinterpret_cast<uint64_t *>(ptr_table);
//      // offset is the third element
//      ptr_table += sizeof(uint64_t);
//      uint64_t returned_offset = *reinterpret_cast<uint64_t *>(ptr_table);
//      return BaseEntry(id, returned_size, returned_offset);
//    }
//    void *GetAt(char *chunk, uint64_t id, uint64_t &return_size)
//    {
//      
//    }
//
//    void *GetNext(char *chunk, uint64_t &return_id, uint64_t &return_size)
//    {
//      
//    }
#endif
#endif
