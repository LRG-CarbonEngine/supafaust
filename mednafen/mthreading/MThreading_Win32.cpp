/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* MThreading_Win32.cpp:
**  Copyright (C) 2014-2019 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 Condition variables here are implemented in a half-assed manner(thin wrapper around Win32 events).
 This implementation is prone to large numbers of spurious wakeups, and is not easily extended to allow for correctly waking up all waiting threads.
 We don't need that feature right now, and by the time we need it, maybe we can just use Vista+ condition variables and forget about XP ;).

 ...but at least the code is simple and should be much faster than SDL 1.2's condition variables for our purposes.
*/

#include <mednafen/mendafen_types.h>
#include <mednafen/MThreading.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <windows.h>
#include <io.h>
#include <process.h>    /* _beginthread, _endthread */
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>
namespace Mednafen
{
namespace MThreading
{

struct Thread
{
 HANDLE thr;
 int (*fn)(void *);
 void* data;
};

struct Cond
{
 HANDLE evt;
};

struct Sem
{
 HANDLE sem;
};

struct Mutex
{
 CRITICAL_SECTION cs;
};

static unsigned __stdcall ThreadPivot(void* data) __attribute__((force_align_arg_pointer));
static unsigned __stdcall ThreadPivot(void* data)
{
 Thread* t = (Thread*)data;

 return t->fn(t->data);
}

Thread* Thread_Create(int (*fn)(void *), void *data, const char* debug_name)
{
 static unsigned dummy;
 Thread* ret = NULL;

 if(!(ret = (Thread*)calloc(1, sizeof(Thread))))
 {
  return NULL;
 }

 ret->fn = fn;
 ret->data = data;

 if(!(ret->thr = (HANDLE)_beginthreadex(NULL, 0, ThreadPivot, ret, 0, &dummy)))
 {
  ErrnoHolder ene(errno);

  free(ret);

  return NULL;
 }

 return ret;
}

void Thread_Wait(Thread* thread, int* status)
{
 DWORD exc = -1;

 WaitForSingleObject(thread->thr, INFINITE);
 GetExitCodeThread(thread->thr, &exc);
 if(status != NULL)
  *status = exc;

 CloseHandle(thread->thr);

 free(thread);
}

uintptr_t Thread_ID(void)
{
 return GetCurrentThreadId();
}

//
//
//

Mutex *Mutex_Create(void)
{
 Mutex* ret;

 if(!(ret = (Mutex*)calloc(1, sizeof(Mutex))))
  return NULL;

 InitializeCriticalSection(&ret->cs);

 return ret;
}

void Mutex_Destroy(Mutex* mutex)
{
 DeleteCriticalSection(&mutex->cs);
 free(mutex);
}

bool Mutex_Lock(Mutex* mutex)
{
 EnterCriticalSection(&mutex->cs);

 return true;
}

bool Mutex_Unlock(Mutex* mutex)
{
 LeaveCriticalSection(&mutex->cs);

 return true;
}

//
//
//

Cond* Cond_Create(void)
{
	Cond* ret;

	if(!(ret = (Cond*)calloc(1, sizeof(Cond))))
		return NULL;
	if(!(ret->evt = CreateEvent(NULL, FALSE, FALSE, NULL)))
	{
		free(ret);
		return NULL;
	}

	return ret;
}

void Cond_Destroy(Cond* cond)
{
  CloseHandle(cond->evt);
 free(cond);
}

bool Cond_Signal(Cond* cond)
{
  return SetEvent(cond->evt) != 0;
}

bool Cond_Wait(Cond* cond, Mutex* mutex)
{
  LeaveCriticalSection(&mutex->cs);

  WaitForSingleObject(cond->evt, INFINITE);

  EnterCriticalSection(&mutex->cs);

  return true;
}

bool Cond_TimedWait(Cond* cond, Mutex* mutex, unsigned ms)
{
 bool ret = true;

  ResetEvent(cond->evt);	// Reset before unlocking mutex.
  LeaveCriticalSection(&mutex->cs);

  switch(WaitForSingleObject(cond->evt, ms))
  {
   case WAIT_OBJECT_0:
	ret = true;
	break;

   case WAIT_TIMEOUT:
	ret = false;
	break;

   default:
	ret = false;
	break;
  }

  EnterCriticalSection(&mutex->cs);

 return ret;
}

//
//
//

Sem* Sem_Create(void)
{
 Sem* ret;

 if(!(ret = (Sem*)calloc(1, sizeof(Sem))))
  return NULL;

 if(!(ret->sem = CreateSemaphore(NULL, 0, INT_MAX, NULL)))
 {
  free(ret);
  return NULL;
 }

 return ret;
}

void Sem_Destroy(Sem* sem)
{
 CloseHandle(sem->sem);
 sem->sem = NULL;
 free(sem);
}

bool Sem_Post(Sem* sem)
{
 return ReleaseSemaphore(sem->sem, 1, NULL) != 0;
}

bool Sem_Wait(Sem* sem)
{
 return WaitForSingleObject(sem->sem, INFINITE) == WAIT_OBJECT_0;
}

bool Sem_TimedWait(Sem* sem, unsigned ms)
{
 bool ret;

 switch(WaitForSingleObject(sem->sem, ms))
 {
   case WAIT_OBJECT_0:
	ret = true;
	break;

   case WAIT_TIMEOUT:
	ret = false;
	break;

   default:
	ret = false;
	break;
 }

 return ret;
}

uint64 Thread_SetAffinity(Thread* thread, uint64 mask)
{
 uint64 ret;

 if(mask > ~(DWORD_PTR)0)
 {
   return 0;
 }

 if(!(ret = SetThreadAffinityMask(thread ? thread->thr : GetCurrentThread(), mask)))
 {
  return 0;
 }

 return ret;
}

}
}
