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
#include "cd_containers.hpp"
using namespace cd;


static int count = 1;

template <typename T>
void MutateVector(T &vec) {
  int i=0;
  int last = vec.size();
  for(; i<last; i++) {
    vec[i] += count;
  }
  auto last_elem = vec[last-1];
  last *= 2;
  for(; i<last; i++) {
    vec.push_back(last_elem + count + i);
  }

  count++;
}

//packer::MagicStore magic __attribute__((aligned(0x1000)));

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv) ;
  packer::CDPacker packer;

//  int myTaskID = 0;
//  int taskSize = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &cd::myTaskID);
  MPI_Comm_size(MPI_COMM_WORLD, &cd::totalTaskSize);
  uint64_t magic_offset = 0;//packer.data_->PadZeros(sizeof(packer::MagicStore));
  uint64_t orig_size = magic_offset;// + packer.data_->offset();//packer.data_->used(); // return

  

  int arrA[8] = {0, 1, 2, 3, 4, 5, 6, 7}; // reference
  CDVector<int> vecA({0, 1, 2, 3, 4, 5, 6, 7});
  CDVector<float> vecB({0.0, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7});
  for(int i=0; i<32; i++) {
    vecB[i] = 1.1*i;
  }

  printf("Original ptr: %p, vec size:%zu, vec cap:%zu, vec obj size:%zu\n", 
         vecA.data(), vecA.size(), vecA.capacity(), sizeof(vecA));
  vecA.PreserveObject(packer, "A");
  vecB.PreserveObject(packer, "B");
  if(myTaskID == 0) {
  vecA.Print("[Before]"); 
  vecB.Print("[Before]"); 
  }
  MutateVector(vecA);
  MutateVector(vecB);
  if(myTaskID == 0) {
  vecA.Print("[Mutated]"); 
  vecB.Print("[Mutated]"); 
  }
  vecA.Deserialize(packer, "A");
  vecB.Deserialize(packer, "B");
  if(myTaskID == 0) {
  vecA.Print("[Deserialize]"); 
  vecB.Print("[Deserialize]");
  }




      packer.data_->PadZeros(0); 
      uint64_t table_offset = packer.AppendTable();
      uint64_t total_size_   = packer.data_->used() - orig_size;
      uint64_t table_offset_ = table_offset - (magic_offset);// + packer.data_->offset());
      uint64_t table_type_   = packer::kCDEntry;
      packer.data_->Flush();
      packer::MagicStore &magic = packer.data_->GetMagicStore();
      magic.total_size_   = total_size_, 
      magic.table_offset_ = table_offset_;
      magic.entry_type_   = table_type_;
      magic.reserved2_    = 0x12345678;
      magic.reserved_     = 0x01234567;
//      uint64_t magic_offset_in_buf = magic_offset - packer.data_->offset();
//      packer.data_->Write((char *)&magic, sizeof(packer::MagicStore), 0);//magic_offset_in_buf);
      packer.data_->FlushMagic(&magic);
      
  MPI_Finalize();
  return 0;
}

