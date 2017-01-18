/*
 *  Threads.cpp
 *  videopicmatic
 *
 *  Created by Daniel Riley on 1/13/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "d2bd/Threads.h"

int d2bd::Thread::yield()
{
  return sched_yield();
//#else
  //return -1;
}

d2bd::Thread::Thread()
{
  _thread = 0;
}

d2bd::Thread::~Thread()
{}

bool d2bd::Thread::start( ThreadFunction routine, void * ptr )
{
  if ( _thread ) 
  {
    //errorString_ << "Thread:: a thread is already running!";
    //handleError( StkError::WARNING );
    return false;
  }
  
#if defined( TARGET_USES_PTHREADS )
  
  if ( pthread_create(&_thread, NULL, *routine, ptr) == 0 )
    return true;
  
#elif defined( TARGET_WIN32 )
  
  unsigned thread_id;
  _thread = _beginthreadex(NULL, 0, routine, ptr, 0, &thread_id);
  if ( _thread ) return true;
  
#endif
  return false;
}

bool d2bd::Thread::cancel()
{
#if defined( TARGET_USES_PTHREADS )
  
  if ( pthread_cancel(_thread) == 0 ) {
    return true;
  }
  
#elif defined( TARGET_WIN32 )
  
  TerminateThread((HANDLE)_thread, 0);
  return true;
  
#endif
  return false;
}

bool d2bd::Thread::wait()
{
#if defined( TARGET_USES_PTHREADS )
  
  if ( pthread_join(_thread, NULL) == 0 ) {
    _thread = 0;
    return true;
  }
  
#elif defined( TARGET_WIN32 )
  
  long retval = WaitForSingleObject( (HANDLE)_thread, INFINITE );
  if ( retval == WAIT_OBJECT_0 ) {
    CloseHandle( (HANDLE)_thread );
    _thread = 0;
    return true;
  }
  
#endif
  return false;
}

void d2bd::Thread::testCancel(void)
{
#if defined( TARGET_USES_PTHREADS )
  
  pthread_testcancel();
  
#endif
}

