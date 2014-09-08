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

#ifndef _CD_ENTRY_H 
#define _CD_ENTRY_H
#include "cd_global.h"
#include "cd.h"
#include "serializable.h"
#include "data_handle.h"
#include "util.h"
#include <stdint.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "unixlog.h"

class cd::CDEntry : public cd::Serializable
{
  private:
    DataHandle src_data_;
    DataHandle dst_data_;
    CDID cd_id_;
    std::string name_; // need a unique name to do via reference, this variable can be empty string when this is not needed 
    cd::CD *ptr_cd_;
    // perhaps here we accept Handles.... 
    /*  CDEntry(const void *source_data, uint64_t len)
        {
        src_data_ = new DataHandleDRAM; 

        src_data_.data_address =    
        }*/
  public:
//    enum CDEntryErrT {kOK=0, kOutOfMemory, kFileOpenError};
    
    CDEntry(){}
    CDEntry(const DataHandle src_data, const DataHandle dst_data, const char *my_name=0) 
    {
      src_data_ = src_data;
      dst_data_ = dst_data;
      if( my_name != 0 ) 
        name_ = my_name;
    }

    ~CDEntry()
    {

      //FIXME we need to delete allocated (preserved) space at some point, should we do it when entry is destroyed?  when is appropriate for now let's do it now.
      if( dst_data_.address_data() != NULL )
        DATA_FREE( dst_data_.address_data() ); 
      if( dst_data_.file_name() != NULL )
      {
        delete dst_data_.file_name(); 
      }

    }
    cd::CD *ptr_cd()
    {
      return ptr_cd_; 
    }
    void set_my_cd(cd::CD *ptr_cd)
    {
      //FIXME: currently it is assumed that my cd is always in the same memory space.
      //In the future we might need a distributed CD structure. For example right now we have one list who handles all the CDEntries for that CD. But this list might need to be distributed if that CD spans among multiple threads. If we have that data structure the issue here will be solved anyways. 
      ptr_cd_ = ptr_cd;
    }

		CDErrT Delete(void);

//GONG
#if _WORK
		private:
		struct tsn_lsn_struct lsn, durable_lsn;

		public:
		std::string my_name(){return name_;}
		CDErrT SaveMem()
		{
      if(dst_data_.address_data() == NULL)
      {
        void *allocated_space;
        allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));  // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 
        dst_data_.set_address_data(allocated_space);
      }


      if(dst_data_.address_data() != NULL) 
      {
        memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len()); 
      }
      else 
      {
        return kOutOfMemory;
        //  ERROR_MESSAGE("Not enough memory.");
      }		
      return kOK;
		}

		CDErrT SaveFile(std::string base_, bool open, struct tsn_log_struct *log)
    {
      if( dst_data_.file_name() == NULL )
      {
        char *cd_file = new char[MAX_FILE_PATH];
        cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file, base_); 
// assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
//        cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); 
        dst_data_.set_file_name(cd_file);
      }
		 	if(!open){
				int ret;
        printf("inside saveMem\n");
        getchar();
				ret = tsn_log_create_file(dst_data_.file_name());
			  assert(ret>=0);	
				ret = tsn_log_open_file(dst_data_.file_name(), TSN_LOG_READWRITE, log);
			  assert(ret>=0);	
			}

			uint64_t bytes = log->log_ops->l_write(log, src_data_.address_data(), src_data_.len(), &lsn);
			std::cout<<lsn.lsn<<std::endl;
			assert(bytes == src_data_.len());
//			int err = log->log_ops->l_flush(log, &lsn, 1, &durable_lsn);
//			assert(err);

			return kOK;
			
		}

		void CloseFile(struct tsn_log_struct *log)
    {
	 		int ret;
			ret = tsn_log_close_file(log);
			assert(ret>=0);
			ret = tsn_log_destroy_file(dst_data_.file_name());	
		}

#else
    CDErrT Save()
    {
      //Direction is from src_data_ to dst_data_

      //FIXME Jinsuk: Just for now we assume everything works on memory
      // What we need to do is that distinguish the medium type and then do operations appropriately

      //FIXME we need to distinguish whether this request is on Remote or local for both when using kOSFile or kMemory and do appropriate operations..
      //FIXME we need to distinguish whether this is to a reference....
      if(dst_data_.handle_type() == DataHandle::kMemory)
      {
        if(dst_data_.address_data() == NULL)
        {

          void *allocated_space;
          allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));  // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 


          dst_data_.set_address_data(allocated_space);

        }


        if(dst_data_.address_data() != NULL) 
        {
          memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len()); 
        }
        else 
        {
          return kOutOfMemory;
          //  ERROR_MESSAGE("Not enough memory.");
        }
      }
      else if( dst_data_.handle_type() == DataHandle::kOSFile) 
      {
        if( dst_data_.file_name() == NULL )
        {
          char *cd_file = new char[MAX_FILE_PATH];
          cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
          dst_data_.set_file_name(cd_file);
        }

        //FIXME	Jinsuk: we need to collect file writes and write as a chunk. We don't want to have too many files per one CD. So perhaps one file per chunk is okay.

        FILE *fp;
        //char filepath[1024]; 
        //Util::GetCDFilePath(cd_id_, src_data.address_data(), filepath); // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
        fp = fopen(dst_data_.file_name(), "w");
        if( fp!= NULL )
        {
          fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);
          fclose(fp);
          return kOK;
        }
        else return kFileOpenError;
      }


      return kOK;

    }
#endif

    //FIXME We need another Restore function that would accept offset and length, so basically it will use dst_data_ as base and then offset and length is used to restore only partial of the original. This functionality is required for via reference to work. Basically when via reference is doing restoration, it will first try to find the entry it self by going up and look into parent's directories. After finding one, it will retrive the dst_data_ and then copy from them.


//GONG
#if _WORK
    CDErrT Restore(bool open, struct tsn_log_struct *log);
#else
    CDErrT Restore();
#endif
    bool isViaReference()
    {
      return (dst_data_.handle_type() == DataHandle::kReference);

    }

    cd::CDPreserveT preserve_type_; // already determined according to class 



    virtual void * Serialize(uint64_t *len_in_bytes)
    {
      //STUB
      return 0;  
    }
    virtual void Deserialize(void * object) 
    {
      //STUB
      return;
    }


  private:

};

#endif

