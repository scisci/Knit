/*
 *  Threads.h
 *  videopicmatic
 *
 *  Created by Daniel Riley on 1/13/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef D2BD_THREADS_H
#define D2BD_THREADS_H


#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

//#define TARGET_USES_PTHREADS 1
#if (defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(TARGET_OS_IPHONE) || defined(TARGET_LINUX) || defined(TARGET_UNIX))
  #define TARGET_USES_PTHREADS
#endif


#if defined( TARGET_WIN32 )
#include <windows.h>
#include <process.h>
#elif defined( TARGET_USES_PTHREADS )
#include <pthread.h>
#endif // TARGET_WIN32

// Operating system dependent thread functionality.
#ifndef MUTEX_INITIALIZE
#if defined( TARGET_WIN32 )
#define MUTEX_INITIALIZE(A)   InitializeCriticalSection(A)
#define MUTEX_DESTROY(A)      DeleteCriticalSection(A)
#define COND_INITIALIZE(A)    InitializeConditionVariable(A)
#define COND_DESTROY(A)       
#define MUTEX_LOCK(A)         EnterCriticalSection(A)
#define MUTEX_UNLOCK(A)       LeaveCriticalSection(A)
#define COND_WAIT(C,M)        SleepConditionVariableCS((C),(M),INFINITE)
#define COND_TIMED_WAIT(C,M,AT) SleepConditionVariableCS((C),(M),(AT).tv_sec * 1000L + (AT).tv_usec / 1000L)
#define COND_SIGNAL(C)        WakeConditionVariable(C)
#define COND_BROADCAST(C)     WakeAllConditionVariable(C)

#elif defined( TARGET_USES_PTHREADS )
// pthread API
#define MUTEX_INITIALIZE(A)   pthread_mutex_init(A, NULL)
#define MUTEX_DESTROY(A)      pthread_mutex_destroy(A)
#define COND_INITIALIZE(A)    pthread_cond_init(A,NULL)
#define COND_DESTROY(A)       pthread_cond_destroy(A)
#define MUTEX_LOCK(A)         pthread_mutex_lock(A)
#define MUTEX_UNLOCK(A)       pthread_mutex_unlock(A)
#define COND_WAIT(C,M)        pthread_cond_wait((C),(M))
#define COND_TIMED_WAIT(C,M,AT) pthread_cond_timedwait((C),(M),(AT))
#define COND_SIGNAL(C)        pthread_cond_signal(C)
#define COND_BROADCAST(C)     pthread_cond_broadcast(C)
#else
#define MUTEX_INITIALIZE(A) abs(*A) // dummy definitions
#define MUTEX_DESTROY(A)    abs(*A) // dummy definitions
#endif
#endif //ifndef MUTEX_INITIALIZE(A)

namespace d2bd
{

#if defined( TARGET_WIN32 )
#define THREAD_TYPE __stdcall
typedef unsigned long ThreadHandle;
typedef unsigned ThreadReturn;
typedef unsigned (__stdcall *ThreadFunction)(void *);
typedef CRITICAL_SECTION MutexHandle;
typedef CONDITION_VARIABLE ConditionHandle

#elif defined( TARGET_USES_PTHREADS )
// Using pthread library for various flavors of unix.
#define THREAD_TYPE
typedef pthread_t ThreadHandle;
typedef void * ThreadReturn;
typedef void * (*ThreadFunction)(void *);
typedef pthread_mutex_t MutexHandle;
typedef pthread_cond_t ConditionHandle;

#else // Setup for "dummy" behavior

typedef int ThreadHandle;
typedef int MutexHandle;

#endif


class Thread
{
public:
  
  static int yield();
  
  //! Default constructor.
  Thread();
  
  //! The class destructor does not attempt to cancel or join a thread.
  ~Thread();
  
  //! Begin execution of the thread \e routine.  Upon success, true is returned.
  /*!
   A data pointer can be supplied to the thread routine via the
   optional \e ptr argument.  If the thread cannot be created, the
   return value is false.
   */
  bool start( ThreadFunction routine, void * ptr = NULL );
  
  //! Signal cancellation of a thread routine, returning \e true on success.
  /*!
   This function only signals thread cancellation.  It does not
   wait to verify actual routine termination.  A \e true return value
   only signifies that the cancellation signal was properly executed,
   not thread cancellation.  A thread routine may need to make use of
   the testCancel() function to specify a cancellation point.
   */
  bool cancel(void);
  
  //! Block the calling routine indefinitely until the thread terminates.
  /*!
   This function suspends execution of the calling routine until the thread has terminated.  It will return immediately if the thread was already terminated.  A \e true return value signifies successful termination.  A \e false return value indicates a problem with the wait call.
   */
  bool wait(void);
  
  //! Create a cancellation point within a thread routine.
  /*!
   This function call checks for thread cancellation, allowing the
   thread to be terminated if a cancellation request was previously
   signaled.
   */
  void testCancel(void);
  
protected:
  
  ThreadHandle _thread;
  
};
  
} // namespace d2bd


#endif // D2BD_THREADS_H

