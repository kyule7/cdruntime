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


#ifndef _CD_EVENT_H
#define _CD_EVENT_H
/**
 * @file cd_event.h
 * @author Jinsuk Chung, Kyushick Lee
 * @date March 2015
 *
 */
#define SIZE_EVENT_ARRAY 64  // allocate 32bit * 64 memory. If we need more we allocate additional 32bit * 64 
// TODO This part should be carefully designed for better efficiency
#include "cd_global.h"
#include "cd_def_internal.h"
#include <stdint.h>



namespace cd {

/**@class CDEvent 
 * @brief An object that provides an event identifier to a non-blocking CD runtime call
 *
 * This is basically just an internal event handle that the user can wait on
 * when trying to make a non-blocking call.
 *
 * Events are automatically chained (see CDHandle::Preserve()),
 *
 * \note We do not rely on C++11 async/futures here because the API is
 * meant to be somewhat portable to other languages.
 */ 
class CDEvent
{
  protected:
    uint32_t *event_;
    uint32_t current_pos_;
    uint32_t len_event_;

  public:
    CDEvent();
    ~CDEvent();
    // Using bitset perhaps if we would like to use this to represent the case where multiple async function needs to be completed before we do something? 
    // Or is this something that we do not want anyways? Assuming we use this, would it cause too much contention? Perhaps, no?

    void InitEvent();
    uint32_t AddEvent();
    void ResetEvent(uint32_t pos);
    void SetEvent(uint32_t pos);
    void DestroyEvent();

public:
/** \addtogroup cd_event_funcs
 * @{
 */

/**@brief Blocking call waiting on the event to complete
 *
 * Once CDEvent::Wait() returns, the event is empty (uninitialized), as if it
 * has been explicitly CDEvent::Reset().
 *
 * @return kOK on success and kError if the event timed out
 *
 */
    CDErrT Wait();

/**@brief Non-blocking call to test whether the event completed
 *
 * @return `true` if complete and `false` is any chained events not
 * yet done.
 *
 */
    bool Test();
/** @} */ // End cd_event_funcs group ===========================================================================
};


} // namespace cd ends
#endif 
