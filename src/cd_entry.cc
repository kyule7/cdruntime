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

CDEntry::CDEntryErrT CDEntry::Delete(void)
{
  // Do something

  if( dst_data_.address_data() != NULL ) {
    DATA_FREE( dst_data_.address_data() );
    printf("free preserved data\n"); 
//    getchar();
  }

  if( dst_data_.handle_type() == DataHandle::kOSFile )  {
    printf("erase file start!\n"); getchar();
//    delete dst_data_.file_name(); 
    char cmd[30];
    sprintf(cmd, "rm %s", dst_data_.file_name_);
    int ret = system(cmd);
    printf("erase the file of preserved data\n"); 
    if( ret == -1 ) {
      ERROR_MESSAGE("Failed to create a directory for preservation data (SSD)");
      assert(0);
    }
  }

	return CDEntry::CDEntryErrT::kOK;
}



CDEntry::CDEntryErrT CDEntry::SaveMem(void)
{
//  std::cout<< "SaveMem(void) this: " << this<<std::endl;
  if(dst_data_.address_data() == NULL) {
    void *allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));
    // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 
    dst_data_.set_address_data(allocated_space);
//    printf("....%d\n", dst_data_.len()*sizeof(char));
//    printf("allocated memory: %x\n", allocated_space);
//    printf("dst_data_: %x\n", dst_data_.address_data());
//    getchar();
  }
//  else {
//    printf("This one shouldn't be called!!!\n\n");
//    assert(0);
    memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len());

    std::cout << "\tSaved Data "<<tag2str[entry_tag_]<<" : " << *(reinterpret_cast<int*>(dst_data_.address_data())) << " at " << dst_data_.address_data() <<" (Memory)" << std::endl; //getchar();

//    std::cout<<"memcpy "<< src_data_.len() <<" size data from "<< src_data_.address_data() << " to "<< dst_data_.address_data() <<std::endl<<std::endl;

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

    printf("we write a file \n");
    getchar();

    return kOK;
  }
  else return kFileOpenError;



//
// 	if(!isOpen) {
//		int ret;
//    printf("inside saveMem\n");
//    getchar();
//		ret = tsn_log_create_file(dst_data_.file_name_.c_str());
//	  assert(ret>=0);	
//		ret = tsn_log_open_file(dst_data_.file_name_.c_str(), TSN_LOG_READWRITE, log);
//	  assert(ret>=0);	
//	}
//  assert(log);
//  assert(log->log_ops);
//	uint64_t bytes = log->log_ops->l_write(log, src_data_.address_data(), src_data_.len(), &lsn);
//	std::cout<<lsn.lsn<<std::endl;
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
      std::cout<<"memcpy "<< src_data_.len() <<" size data from "<< src_data_.address_data() << " to "<< dst_data_.address_data() <<std::endl<<std::endl;
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

//	std::cout <<"ref name: "    << dst_data_.ref_name() 
//            << ", at level: " << ptr_cd()->GetCDID().level()<<std::endl;



	if( dst_data_.handle_type() == DataHandle::kReference) {  // Restoration from reference
    std::cout<< "GetBuffer, reference"<<std::endl; //getchar();
    buffer = &real_dst_data;
    
    // FIXME: for now let's just search immediate parent only.  Let's extend this to more general way.
//		cd::CDHandle* parent_cd = GetParentCD(ptr_cd_->GetCDID());

		cd::CDHandle* parent_cd = CDPath::GetParentCD();
    CDEntry* entry_tmp = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());

    std::cout<<"parent name: "<<parent_cd->GetName()<<std::endl;
    if(entry_tmp != NULL) { 
      std::cout << "parent dst addr : " << entry_tmp->dst_data_.address_data()
                << ", parent entry name : " << entry_tmp->dst_data_.ref_name()<<std::endl;
    } else {
      std::cout<<"there is no reference in parent level"<<std::endl;
    }
//		if( ptr_cd_ == 0 ) { ERROR_MESSAGE("Pointer to CD object is not set."); assert(0); }

//		CDEntry* entry = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());
//		std::cout <<"ref name: "    << entry->dst_data_.ref_name() 
//              << ", at level: " << entry->ptr_cd()->GetCDID().level()<<std::endl;
//		if( entry != 0 ) {

      //Here, we search Parent's entry and Parent's Parent's entry and so on.
      //if ref_name does not exit, we believe it's original. 
      //Otherwise, there is original copy somewhere else, maybe grand parent has it. 

      CDEntry* entry = NULL;
			while( parent_cd != NULL ) {
		    entry = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());
				std::cout <<"current entry name : "<< entry->name() << " with ref name : "    << entry->dst_data_.ref_name() 
                  << ", at level: " << entry->ptr_cd()->GetCDID().level()<<std::endl;

        if(entry != NULL) {
          std::cout<<"I got my reference here!!"<<std::endl;
          break;
        }
        else {
				  parent_cd = CDPath::GetParentCD(ptr_cd()->GetCDID().level());
          std::cout<< "Gotta go to upper level! -> " << parent_cd->GetName() << " at level "<< parent_cd->ptr_cd()->GetCDID().level() << std::endl;
        }
			} 

      std::cout<<"here?? 2"<<std::endl;
       
			//lsn = entry->lsn;
			
      DataHandle& tmp = entry->dst_data_; 
      buffer = &tmp;

//      std::cout<<"addr "<<entry->dst_data_.address_data()<<std::endl; 
//      std::cout<<"addr "<<buffer->address_data()<<std::endl; 
//			std::cout <<"[Buffer] ref name: "    << buffer->ref_name() 
//                <<", value: "<<*(reinterpret_cast<int*>(buffer->address_data())) << std::endl;
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
    std::cout<<"GetBuffer :: kCopy "<<std::endl; //getchar();
    buffer = &dst_data_;
    dst_data_.address_data();
  }
  return buffer;
}

CDEntry::CDEntryErrT CDEntry::Restore(void)
{
  std::cout<<"CDEntry::Restore(void)"<<std::endl;
  // Populate buffer
  DataHandle* buffer = GetBuffer();
  if(buffer == 0) { ERROR_MESSAGE("GetBuffer failed.\n"); assert(0); }

	//FIXME we need to distinguish whether this request is on Remote or local for both 
  // when using kOSFile or kMemory and do appropriate operations..
	if(buffer->handle_type() == DataHandle::kMemory)	{
    std::cout<<"CDEntry::Restore -> kMemory"<<std::endl;
		if(src_data_.address_data() != NULL) {
      std::cout << src_data_.len() << " - " << buffer->len() << ", offset : "<<buffer->ref_offset() << std::endl;
      std::cout << src_data_.address_data() << " - " << buffer->address_data() << std::endl;
      assert( src_data_.len() == buffer->len() );
//      std::cout << "sizeof src "<< sizeof(src_data_.address_data()) << ", sizeof dst " <<sizeof(buffer->address_data())<<std::endl;
			memcpy(src_data_.address_data(), (char *)buffer->address_data()+(buffer->ref_offset()), buffer->len()); 
      std::cout<<"memcpy succeeds"<<std::endl; //getchar();

      std::cout<<"memcpy "<< dst_data_.len() <<" size data from "<< dst_data_.address_data() << " to "<< src_data_.address_data() <<std::endl<<std::endl;
		}
		else {
			ERROR_MESSAGE("Not enough memory.");
		}

	}
	else if( buffer->handle_type() == DataHandle::kOSFile)	{
    std::cout<<"CDEntry::Restore -> kOSFile"<<std::endl;
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
    std::cout<<buffer<<" " <<buffer->address_data() << " "<< buffer->ref_offset() << " "<< buffer->len()<<std::endl;
		if(src_data_.address_data() != NULL) {
			memcpy(src_data_.address_data(), 
            (char*)buffer->address_data() + (buffer->ref_offset()), buffer->len()); 

      std::cout<<"memcpy "<< dst_data_.len() <<" size data from "<< dst_data_.address_data() << " to "<< src_data_.address_data() <<std::endl<<std::endl;
		}
		else {
			ERROR_MESSAGE("Not enough memory.");
		}

	}
	else if( buffer->handle_type() == DataHandle::kOSFile) {
// FIXME: GONG
//    std::cout<<"kOSFile file_name() : "<<buffer->file_name()<<" offset: "<<buffer->ref_offset()<<std::endl;					
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
//		std::cout<<lsn.lsn<<" "<<bytes<<" "<<open<<std::endl;
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



