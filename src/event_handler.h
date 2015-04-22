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

#ifndef _EVENT_HANDLER_H 
#define _EVENT_HANDLER_H

/**
 * @file event_handler.h
 * @author Kyushick Lee
 * @date March 2015
 *
 * @brief Containment Domains API v0.2 (C++)
 */

#include "cd_global.h"
#include "cd_def_internal.h" 
#include "cd.h"

using namespace cd;

namespace cd {
  namespace internal {

class EventHandler {
public:
  EventHandler() {}
  virtual ~EventHandler(void) {}
  virtual void HandleEvent(void) {dbg << "EventHandler::HandleEvent is called" << endl;}
};

class CDEventHandler : public EventHandler {
protected:
  CD *ptr_cd_;
public:
  CDEventHandler(CD *ptr_cd) 
    : ptr_cd_(ptr_cd) {}
  virtual ~CDEventHandler(void){ dbg << "CDEventHandler is destroyed" << endl; }
  virtual void HandleEvent(void) {dbg << "CDEventHandler::HandleEvent is called" << endl;}
};

class HeadCDEventHandler : public EventHandler {
protected:
  HeadCD *ptr_cd_;
public:
  HeadCDEventHandler(HeadCD *ptr_cd) 
    : ptr_cd_(ptr_cd) {}
  virtual ~HeadCDEventHandler(void){ dbg << "HeadCDEventHandler is destroyed" << endl; }
  virtual void HandleEvent(void) {dbg << "HeadCDEventHandler::HandleEvent is destroyed" << endl;}
};

// ------------------------------------------------------------------------

class HandleAllPause : public CDEventHandler {
public:
  HandleAllPause(CD *ptr_cd) 
    : CDEventHandler(ptr_cd) {}
  ~HandleAllPause(){}
  virtual void HandleEvent(void);
};
class HandleAllReexecute : public CDEventHandler {
public:
  HandleAllReexecute(CD *ptr_cd) 
    : CDEventHandler(ptr_cd) {}
  ~HandleAllReexecute(){}
  virtual void HandleEvent(void);
};
class HandleEntrySend : public CDEventHandler {
public:
  HandleEntrySend(CD *ptr_cd) 
    : CDEventHandler(ptr_cd) {}
  ~HandleEntrySend(){}
  virtual void HandleEvent(void);
};



class HandleErrorOccurred : public HeadCDEventHandler {
  int task_id_;
public:
  HandleErrorOccurred(HeadCD *ptr_cd, int task_id=0) 
    : HeadCDEventHandler(ptr_cd), task_id_(task_id) {}
  ~HandleErrorOccurred() { dbg << "HandleErrorOccurred is destroyed" << endl; }
  virtual void HandleEvent(void);
};

class HandleEntrySearch : public HeadCDEventHandler {
  int task_id_;
public:
  HandleEntrySearch(HeadCD *ptr_cd, int task_id) 
    : HeadCDEventHandler(ptr_cd) {
    task_id_ = task_id;
  }
  ~HandleEntrySearch() {}
  virtual void HandleEvent(void);
};

class HandleAllResume : public HeadCDEventHandler {
public:
  HandleAllResume(HeadCD *ptr_cd) 
    : HeadCDEventHandler(ptr_cd) {}
  ~HandleAllResume() {}
  virtual void HandleEvent(void);
};


  } // namespace internal ends
} // namespace cd ends
#endif
