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

#ifndef _PACKER_H
#define _PACKER_H

#include "cd_global.h"

/*

   We have two main section. One section is Table section, it is kind of header where it locates where the data is, and shows the length of data element.

   [ENTIRE_SIZE][TABLE][DATA]
   [ID][POS][LEN]
   And the table look like the following.

 */

// Packer packer;
// packer.Add(ID_NAME, 100, name_ptr);
// packer.Add(ID_NAME, 100, name_ptr);
// packer.Add(ID_NAME, 100, name_ptr);
// char *serialized_data = packer.GetTotalData( total_data_size);
// packer.Add(ID_SOMETHING, 10, something_ptr);
// char *serialized_data = packer.GetTotalData( total_data_size);
class cd::Packer  
{
  public:

    enum PackerErrT {kOK =0, kMallocFailed, kReallocFailed};

    Packer();
    Packer(unsigned long table_grow_unit, unsigned long data_grow_unit);
    virtual ~Packer();

    virtual unsigned int Add(unsigned long id, unsigned long length, const void *position);
    virtual char *GetTotalData(unsigned long &total_data_size);



    virtual void SetBufferGrow( unsigned long table_grow_unit, unsigned long data_grow_unit);



  private:
    unsigned int ClearData();
    unsigned int AddData(unsigned long id, unsigned long length, unsigned long position, char *ptr_data);
    unsigned int CheckAlloc();
    unsigned int CheckRealloc(unsigned long length);
    void WriteWord(char *dst_buffer,unsigned long value);
    void WriteData(char *ptr_data, unsigned long length, unsigned long position);



  private:

    unsigned long table_grow_unit_;  // Growth unit is used when we need to increase the memory allocation size. This scheme is needed because we don't know how much will be needed at the beginning.  
    unsigned long data_grow_unit_;   // This is the same as above but it is for the data section.  (Two section exist) 

    unsigned long table_size_;   //Current table size.
    unsigned long data_size_; 
    unsigned long used_table_size_; 
    unsigned long used_data_size_; 

    unsigned long alloc_flag_;

    char *ptr_table_;
    char *ptr_data_;
};
#endif
