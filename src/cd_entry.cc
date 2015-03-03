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
    dbg << "free preserved data\n" << endl; 
  }

  if( dst_data_.handle_type() == DataHandle::kOSFile )  {
    dbg << "erase file start!\n" << endl; //dbgBreak();
    dbg << "erase the file of preserved data\n" << endl; 
//    delete dst_data_.file_name(); 

    char cmd[30];
    sprintf(cmd, "rm %s", dst_data_.file_name_);
    int ret = system(cmd);

    if( ret == -1 ) {
      cerr << "Failed to create a directory for preservation data (SSD)" << endl;
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
    // FIXME: Jinsuk we might want our own memory manager 
    // since we don't want to call malloc everytime we want small amount of additional memory. 
    dst_data_.set_address_data(allocated_space);
//    dbg <<"..." << dst_data_.len()*sizeof(char) << endl;;
//    dbg << "allocated memory: " << allocated_space << endl;
//    dbg << "dst_data_: " << dst_data_.address_data() << endl;
//    dbgBreak();
  }
  memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len());

  dbg << "\tSaved Data "<<tag2str[entry_tag_]<<" : " << *(reinterpret_cast<int*>(dst_data_.address_data())) 
      << " at " << dst_data_.address_data() <<" (Memory)" << endl; 
//  dbg<<"memcpy "<< src_data_.len() <<" size data from "<< src_data_.address_data() << " to "<< dst_data_.address_data() <<endl<<endl;

  if(false) { // Is there a way to check if memcpy is done well?
    cerr << "Not enough memory." << endl;
    assert(0);
    return kOutOfMemory; 
  }


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

    return kOK;
  }
  else return kFileOpenError;
	
}




CDEntry::CDEntryErrT CDEntry::SavePFS( struct tsn_log_struct *log )
{
	//First we should check for PFS file => I think we have checked it before calling this function (not sure).
	//MPI_Status preserve_status;//This variable can be used to non-blocking writes to PFS. By checking this variable we can understand whether write has been finished or not.
  //MPI_File_get_position( ptr_cd_->PFS_d_, &(dst_data_.parallel_file_offset_));

  // Dynamic Chunk Interleave
  dst_data_.parallel_file_offset_ = ptr_cd_->Par_IO_Man->Write( src_data_.address_data(), src_data_.len() );

	return kOK;
}


// -----------------------------------------------------------------------------------------------

void CDEntry::RequestEntrySearch(void)
{
  dbg << "\nCDEntry::RequestEntrySearch\n" << endl;
  assert(0);
  // SetMailbox
  CDEventT entry_search = kEntrySearch;
  ptr_cd()->SetMailBox(entry_search);

//  MPI_Alloc_mem(src_data_.len(), MPI_INFO_NULL, &(dst_data_.address_data_));

  ENTRY_TAG_T entry_tag_to_search = dst_data_.ref_name_tag();
  ptr_cd()->entry_request_req_[entry_tag_to_search] = CommInfo();
  // Tag should contain CDID+entry_tag
  ENTRY_TAG_T sendBuf[2] = {entry_tag_to_search, static_cast<uint64_t>(myTaskID)};
  MPI_Isend(sendBuf, 
            2, 
            MPI_UNSIGNED_LONG_LONG,
            ptr_cd()->head(), 
            ptr_cd()->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG),
            ptr_cd()->color(),
            &(ptr_cd()->entry_request_req_[entry_tag_to_search].req_));

  // Receive the preserved data to restore the data in application memory
  MPI_Irecv(src_data_.address_data(), 
            src_data_.len(), 
            MPI_BYTE, 
            MPI_ANY_SOURCE, 
            ptr_cd()->GetCDID().GenMsgTag(entry_tag_to_search), 
            MPI_COMM_WORLD, 
            &(ptr_cd()->entry_recv_req_[entry_tag_to_search].req_));  
}





// Bring data handle from parents
CDEntry::CDEntryErrT CDEntry::Restore(void) {
  dbg<<"CDEntry::Restore(void), ref name: "    << dst_data_.ref_name() << ", at level: " << ptr_cd()->level() <<endl;

  DataHandle *buffer = NULL;

	if( dst_data_.handle_type() == DataHandle::kReference) {  
    // Restoration from reference
    dbg<< "GetBuffer, reference"<<endl; //dbgBreak();
    
    int found_level = 0;
	  CDEntry *entry = ptr_cd()->SearchEntry(dst_data_.ref_name_tag(), found_level);

    // Remote case
    if(entry == NULL) {
      return InternalRestore(buffer);
    } 
    else { // Local case
      buffer = &(entry->dst_data_);
  
      buffer->set_ref_offset(dst_data_.ref_offset() );   
  
      // length could be different in case we want to describe only part of the preserved entry.
      buffer->set_len(dst_data_.len());     

//      dbg<<"addr "<<entry->dst_data_.address_data()<<endl; 
//      dbg<<"addr "<<buffer->address_data()<<endl; 
//			dbg <<"[Buffer] ref name: "    << buffer->ref_name() 
//                <<", value: "<<*(reinterpret_cast<int*>(buffer->address_data())) << endl;

    }
	}
  else {  
    // Restoration from memory or file system. Just use the current dst_data_ for it.
    dbg<<"GetBuffer :: kCopy "<<endl; //dbgBreak();
    buffer = &dst_data_;
    dst_data_.address_data();
  }
  
  return InternalRestore(buffer);
}

CDEntry::CDEntryErrT CDEntry::InternalRestore(DataHandle *buffer)
{
  dbg<<"CDEntry::InternalRestore(void)"<<endl;

  // Handles preservation via reference for remote copy.
  if(buffer == NULL) {
#if _FIX
    // Add this entry for receiving preserved data remotely.
    ptr_cd()->entry_recv_req_[dst_data_.ref_name_tag()] = CommInfo(src_data_.address_data());
    RequestEntrySearch();
#endif
    return kEntrySearchRemote;
  }
	else if(buffer->handle_type() == DataHandle::kMemory)	{
	  //FIXME we need to distinguish whether this request is on Remote or local for both 
    // when using kOSFile or kMemory and do appropriate operations..

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
			cerr << "Not enough memory." << endl;
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
      if( buffer->ref_offset() != 0 ) 
        fseek(fp, buffer->ref_offset(), SEEK_SET); 
			fread(src_data_.address_data(), 1, src_data_.len(), fp);
			fclose(fp);
			return CDEntryErrT::kOK;
		}
		else 
      return CDEntryErrT::kFileOpenError;

	}


	return kOK;
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


