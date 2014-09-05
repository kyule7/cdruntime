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


#include "cd_event.h"
#include <unistd.h>


using namespace cd;


CDEvent::CDEvent() : event_(0) , current_pos_(0) 
{
  InitEvent();
}

CDEvent::~CDEvent()
{
  DestroyEvent();
}

void CDEvent::InitEvent()
{
  if( event_ == 0 )
  {
    event_ = new uint32_t[SIZE_EVENT_ARRAY];
  }
}
void CDEvent::DestroyEvent()
{
  if( event_ != 0) 
  {
    delete [] event_;
  }
}
uint32_t CDEvent::AddEvent()
{
  uint32_t ret_prepare_pos = current_pos_;	
  uint32_t converted_pos = current_pos_ / sizeof(uint32_t);
  if( len_event_ < converted_pos) 
  {
    //FIXME need to allocate additional memory and then probably memcpy ? the original one? let's check this out.	


  }

  SetEvent(current_pos_);
  current_pos_++;
  return ret_prepare_pos;

}
void CDEvent::ResetEvent(uint32_t pos)
{
  //x& = ~(1u <<3);
  // check if we need larger array.
  uint32_t converted_pos = pos / sizeof(uint32_t);
  uint32_t offset = pos % sizeof(uint32_t);

  event_[converted_pos] &= ~(1u << offset);

}
void CDEvent::SetEvent(uint32_t pos)
{
  // x|= (1u << 3);  // 4th bit set
  uint32_t converted_pos = pos / sizeof(uint32_t);
  uint32_t offset = pos % sizeof(uint32_t);


  event_[converted_pos] |= (1u << offset);
}

bool CDEvent::Test()
{
  if( current_pos_ == 0 ) return false;
  uint32_t end= current_pos_/ sizeof(uint32_t);
  for( uint32_t i = 0 ; i < end ; i++ )
  {
    if( event_[i] != 0 ) 
    {
      return false;
    }

  }
  return true;
}


CDErrT CDEvent::Wait()
{
  while(!Test())
  {
    usleep(0);
  }
  return kOK;
}
