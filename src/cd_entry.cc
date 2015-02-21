/*
Copyright 2014, The University of Texas at Austin 
All rights reserved.

THIS FILE IS PART OF THE CONTAINMENT DOMAINS RUNTIME LIBRARY

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include "cd_entry.h"
#include "cd_path.h"

#include <cassert>
using namespace cd;
using namespace std;

CDEntry::CDEntryErrT CDEntry::Delete(void)
{
  // Do something

  if( dst_data_.address_data() != NULL ) {
    DATA_FREE( dst_data_.address_data() );
//    dbg << "free preserved data\n" << endl; 
//    dbgBreak();
  }

  if( dst_data_.handle_type() == DataHandle::kOSFile )  {
    dbg << "erase file start!\n" << endl; //dbgBreak();
//    delete dst_data_.file_name(); 
    char cmd[30];
    sprintf(cmd, "rm %s", dst_data_.file_name_);
    int ret = system(cmd);
    dbg << "erase the file of preserved data\n" << endl; 
    if( ret == -1 ) {
      ERROR_MESSAGE("Failed to create a directory for preservation data (SSD)");
      assert(0);
    }
  }

	return CDEntry::CDEntryErrT::kOK;
}



CDEntry::CDEntryErrT CDEntry::SaveMem(void)
{
//  dbg<< "SaveMem(void) this: " << this<<endl;
  if(dst_data_.address_data() == NULL) {
    void *allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));
    // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 
    dst_data_.set_address_data(allocated_space);
//    dbg <<"..." << dst_data_.len()*sizeof(char) << endl;;
//    dbg << "allocated memory: " << allocated_space << endl;
//    dbg << "dst_data_: " << dst_data_.address_data() << endl;
//    dbgBreak();
  }
//  else {
//    dbg << "This one shouldn't be called!!!\n\n" << endl;
//    assert(0);
    memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len());

    dbg << "\tSaved Data "<<tag2str[entry_tag_]<<" : " << *(reinterpret_cast<int*>(dst_data_.address_data())) << " at " << dst_data_.address_data() <<" (Memory)" << endl; //dbgBreak();

//    dbg<<"memcpy "<< src_data_.len() <<" size data from "<< src_data_.address_data() << " to "<< dst_data_.address_data() <<endl<<endl;

    if(false) { // Is there a way to check if memcpy is done well?
      ERROR_MESSAGE("Not enough memory.");
      assert(0);
      return kOutOfMemory; 
    }

//  }

  return kOK;
}

CDEntry::CDEntryErrT CDEntry::SaveFile(std::string base_, bool isOpen, struct tsn_log_struct *log)
{
  // Get file name to write if it is currently NULL
  char* str; 
  if( !strcmp(dst_data_.file_name_, INIT_FILE_PATH) ) {
    // assume cd_id_ is unique (sequential cds will have different cd_id) 
    // and address data is also unique by natural
//      char *cd_file = new char[MAX_FILE_PATH];
//      cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file, base_); 
    const char* data_name;
    str = new char[30];
    if(entry_tag_ == 0) {
      sprintf(str, "%d", rand());
      data_name = str;
    }
    else  {
      data_name = tag2str[entry_tag_].c_str();
    }

    const char *file_name = Util::GetUniqueCDFileName(ptr_cd_->GetCDID(), base_.c_str(), data_name).c_str();
    strcpy(dst_data_.file_name_, file_name); 
  }

  FILE *fp = fopen(dst_data_.file_name_, "w");
  if( fp!= NULL ) {
    fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);
    fclose(fp);

    dbg << "we write a file \n" << endl;
    //dbgBreak();

    return kOK;
  }
  else return kFileOpenError;



//
// 	if(!isOpen) {
//		int ret;
//    dbg << "inside saveMem\n" << endl;
//    dbgBreak();
//		ret = tsn_log_create_file(dst_data_.file_name_.c_str());
//	  assert(ret>=0);	
//		ret = tsn_log_open_file(dst_data_.file_name_.c_str(), TSN_LOG_READWRITE, log);
//	  assert(ret>=0);	
//	}
//  assert(log);
//  assert(log->log_ops);
//	uint64_t bytes = log->log_ops->l_write(log, src_data_.address_data(), src_data_.len(), &lsn);
//	dbg<<lsn.lsn<<endl;
//	assert(bytes == src_data_.len());
//		int err = log->log_ops->l_flush(log, &lsn, 1, &durable_lsn);
//		assert(err);

//	return kOK;
	
}

CDEntry::CDEntryErrT CDEntry::Save(void)
{
  //Direction is from src_data_ to dst_data_

  //FIXME Jinsuk: Just for now we assume everything works on memory
  // What we need to do is that distinguish the medium type and then do operations appropriately

  //FIXME we need to distinguish whether this request is on Remote or local for both when using kOSFile or kMemory and do appropriate operations..
  //FIXME we need to distinguish whether this is to a reference....
  if( dst_data_.handle_type() == DataHandle::kMemory ) {
    if(dst_data_.address_data() == NULL) {

      void *allocated_space;

      // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 
      allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));  

      dst_data_.set_address_data(allocated_space);

    }

    if(dst_data_.address_data() != NULL) {
      memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len()); 
      dbg<<"memcpy "<< src_data_.len() <<" size data from "<< src_data_.address_data() << " to "<< dst_data_.address_data() <<endl<<endl;
    }
    else {
      return kOutOfMemory;
      //  ERROR_MESSAGE("Not enough memory.");
    }

  }
  else if( dst_data_.handle_type() == DataHandle::kOSFile ) {
    if( !strcmp(dst_data_.file_name_, INIT_FILE_PATH) ) {
//      char *cd_file = new char[MAX_FILE_PATH];
//      cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); 




      // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
//      dst_data_.file_name_ = Util::GetUniqueCDFileName(ptr_cd_->GetCDID(), "kOSFILE");
    }

    // FIXME	Jinsuk: we need to collect file writes and write as a chunk. 
    // We don't want to have too many files per one CD. So perhaps one file per chunk is okay.

    FILE *fp;
    //char filepath[1024]; 
    //Util::GetCDFilePath(cd_id_, src_data.address_data(), filepath); 
    // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
    fp = fopen(dst_data_.file_name_, "w");
    if( fp!= NULL ) {
      fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);
      fclose(fp);
      return kOK;
    }
    else return kFileOpenError;
  }


  return kOK;

}


void CDEntry::CloseFile(struct tsn_log_struct *log)
{
	int ret;
	ret = tsn_log_close_file(log);
	assert(ret>=0);
	ret = tsn_log_destroy_file(dst_data_.file_name_);	
}

DataHandle* CDEntry::GetBuffer() {
  DataHandle* buffer=0;
  DataHandle real_dst_data;

//	dbg <<"ref name: "    << dst_data_.ref_name() 
//            << ", at level: " << ptr_cd()->GetCDID().level()<<endl;



	if( dst_data_.handle_type() == DataHandle::kReference) {  // Restoration from reference
    dbg<< "GetBuffer, reference"<<endl; //dbgBreak();
    buffer = &real_dst_data;
    
    // FIXME: for now let's just search immediate parent only.  Let's extend this to more general way.
//		cd::CDHandle* parent_cd = GetParentCD(ptr_cd_->GetCDID());

		cd::CDHandle* parent_cd = CDPath::GetParentCD();
    CDEntry* entry_tmp = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());

    dbg<<"parent name: "<<parent_cd->GetName()<<endl;
    if(entry_tmp != NULL) { 
      dbg << "parent dst addr : " << entry_tmp->dst_data_.address_data()
                << ", parent entry name : " << entry_tmp->dst_data_.ref_name()<<endl;
    } else {
      dbg<<"there is no reference in parent level"<<endl;
    }
//		if( ptr_cd_ == 0 ) { ERROR_MESSAGE("Pointer to CD object is not set."); assert(0); }

//		CDEntry* entry = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());
//		dbg <<"ref name: "    << entry->dst_data_.ref_name() 
//              << ", at level: " << entry->ptr_cd()->GetCDID().level()<<endl;
//		if( entry != 0 ) {

      //Here, we search Parent's entry and Parent's Parent's entry and so on.
      //if ref_name does not exit, we believe it's original. 
      //Otherwise, there is original copy somewhere else, maybe grand parent has it. 

      CDEntry* entry = NULL;
			while( parent_cd != NULL ) {
		    entry = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());
				dbg <<"current entry name : "<< entry->name() << " with ref name : "    << entry->dst_data_.ref_name() 
                  << ", at level: " << entry->ptr_cd()->GetCDID().level()<<endl;

        if(entry != NULL) {
          dbg<<"I got my reference here!!"<<endl;
          break;
        }
        else {
				  parent_cd = CDPath::GetParentCD(ptr_cd()->GetCDID().level());
          dbg<< "Gotta go to upper level! -> " << parent_cd->GetName() << " at level "<< parent_cd->ptr_cd()->GetCDID().level() << endl;
        }
			} 

      dbg<<"here?? 2"<<endl;
       
			//lsn = entry->lsn;
			
      DataHandle& tmp = entry->dst_data_; 
      buffer = &tmp;

//      dbg<<"addr "<<entry->dst_data_.address_data()<<endl; 
//      dbg<<"addr "<<buffer->address_data()<<endl; 
//			dbg <<"[Buffer] ref name: "    << buffer->ref_name() 
//                <<", value: "<<*(reinterpret_cast<int*>(buffer->address_data())) << endl;
      buffer->set_ref_offset(dst_data_.ref_offset() );   
      // length could be different in case we want to describe only part of the preserved entry.
      buffer->set_len(dst_data_.len());     
//		}
//    else {  // no entry!!
//      ERROR_MESSAGE("Failed to retrieve a CDEntry");
//    }
		//FIXME: Assume the destination (it is a source in restoration context) is local. 
    // Basically parent's entry's destination is also local. So that we can acheive this by just copying memory or reading file   

	}
  else {  // Restoration from memory or file system. Just use the current dst_data_ for it.
    dbg<<"GetBuffer :: kCopy "<<endl; //dbgBreak();
    buffer = &dst_data_;
    dst_data_.address_data();
  }
  return buffer;
}

CDEntry::CDEntryErrT CDEntry::Restore(void)
{
  dbg<<"CDEntry::Restore(void)"<<endl;
  // Populate buffer
  DataHandle* buffer = GetBuffer();
  if(buffer == 0) { ERROR_MESSAGE("GetBuffer failed.\n"); assert(0); }

	//FIXME we need to distinguish whether this request is on Remote or local for both 
  // when using kOSFile or kMemory and do appropriate operations..
	if(buffer->handle_type() == DataHandle::kMemory)	{
    dbg<<"CDEntry::Restore -> kMemory"<<endl;
		if(src_data_.address_data() != NULL) {
      dbg << src_data_.len() << " - " << buffer->len() << ", offset : "<<buffer->ref_offset() << endl;
      dbg << src_data_.address_data() << " - " << buffer->address_data() << endl;
      assert( src_data_.len() == buffer->len() );
//      dbg << "sizeof src "<< sizeof(src_data_.address_data()) << ", sizeof dst " <<sizeof(buffer->address_data())<<endl;
			memcpy(src_data_.address_data(), (char *)buffer->address_data()+(buffer->ref_offset()), buffer->len()); 
      dbg<<"memcpy succeeds"<<endl; //dbgBreak();

      dbg<<"memcpy "<< dst_data_.len() <<" size data from "<< dst_data_.address_data() << " to "<< src_data_.address_data() <<endl<<endl;
		}
		else {
			ERROR_MESSAGE("Not enough memory.");
		}

	}
	else if( buffer->handle_type() == DataHandle::kOSFile)	{
    dbg<<"CDEntry::Restore -> kOSFile"<<endl;
		//FIXME we need to collect file writes and write as a chunk. We don't want to have too many files per one CD.   

		FILE *fp;
		//  Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); 
    // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
		const char* cd_file = buffer->file_name_;
		fp = fopen(cd_file, "r");
		if( fp!= NULL )	{
      if( buffer->ref_offset() != 0 ) fseek(fp, buffer->ref_offset(), SEEK_SET); 
			fread(src_data_.address_data(), 1, src_data_.len(), fp);
			fclose(fp);
			return CDEntryErrT::kOK;
		}
		else return CDEntryErrT::kFileOpenError;

	}


	return kOK;
	//Direction is from dst_data_ to src_data_

}













//GONG
CDEntry::CDEntryErrT CDEntry::Restore(bool open, struct tsn_log_struct *log)
{
  // Populate buffer
  DataHandle* buffer = GetBuffer();
  if(buffer == 0) { ERROR_MESSAGE("GetBuffer failed.\n"); assert(0); }
	//FIXME we need to distinguish whether this request is on Remote or local for both when using kOSFile or kMemory and do appropriate operations..
	if(buffer->handle_type() == DataHandle::kMemory) {
    assert(buffer!=NULL);
    dbg<<buffer<<" " <<buffer->address_data() << " "<< buffer->ref_offset() << " "<< buffer->len()<<endl;
		if(src_data_.address_data() != NULL) {
			memcpy(src_data_.address_data(), 
            (char*)buffer->address_data() + (buffer->ref_offset()), buffer->len()); 

      dbg<<"memcpy "<< dst_data_.len() <<" size data from "<< dst_data_.address_data() << " to "<< src_data_.address_data() <<endl<<endl;
		}
		else {
			ERROR_MESSAGE("Not enough memory.");
		}

	}
	else if( buffer->handle_type() == DataHandle::kOSFile) {
// FIXME: GONG
//    dbg<<"kOSFile file_name() : "<<buffer->file_name()<<" offset: "<<buffer->ref_offset()<<endl;					
//		if(!open){
//			int ret;
//			ret = tsn_log_create_file(buffer->file_name());
//			assert(ret>=0);	
//			ret = tsn_log_open_file(buffer->file_name(), TSN_LOG_READWRITE, log);
//			assert(ret>=0);	
//		}
//    assert(log);
//    assert(log->log_ops);
//		uint64_t bytes = log->log_ops->l_read(log, &lsn, src_data_.address_data(), src_data_.len());
//		dbg<<lsn.lsn<<" "<<bytes<<" "<<open<<endl;
//		assert(bytes == src_data_.len());



		//FIXME we need to collect file writes and write as a chunk. We don't want to have too many files per one CD.   

		FILE *fp;
//		char *cd_file; 
		//  Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
		const char* cd_file = buffer->file_name_;
		fp = fopen(cd_file, "r");
		if( fp!= NULL ) {
      if( buffer->ref_offset() != 0 )
        fseek(fp, buffer->ref_offset(), SEEK_SET); 
			fread(src_data_.address_data(), 1, src_data_.len(), fp);
			fclose(fp);
			return kOK;
		}
		else return kFileOpenError;

	}


	return CDEntry::CDEntryErrT::kOK;
	//Direction is from dst_data_ to src_data_

}


std::ostream& cd::operator<<(std::ostream& str, const CDEntry& cd_entry)
{
  return str << "\n== CD Entry Information ================"
             << "\nSource      :\t" << cd_entry.src_data_  
             << "\nDestination :\t" << cd_entry.dst_data_
             << "\nEntry Tag   :\t" << cd_entry.entry_tag_
             << "\nEntry Name  :\t" << tag2str[cd_entry.entry_tag_]
             << "\n========================================\n";
}

/*
void* CDEntry::Serialize(uint32_t* len_in_bytes)
{
//  Packer entry_packer;
//  entry_packer.Add(id, sizeof(DataHandle), &src_data_);
  return 0;  
}
void CDEntry::Deserialize(void* object) 
{
  //STUB
  return;
}
*/



