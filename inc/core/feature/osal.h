#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#include "core/systemDefs.h"
#if APP_OS == OS_LINUX
    #include <pthread.h>
    #include <semaphore.h>
    #include <sys/epoll.h>
    #include <time.h>
#elif APP_OS == OS_WIN32
    #include <windows.h>
#endif

// Time / Tick
int osalGetTimeMs(void);
int64_t osalGetTimeUs(void);
int64_t osalGetTimeNs(void);
int osalGetTick(void);
void osalGetDate(char*, size_t);
void osalSleepMs(uint32_t);
void osalSleepUs(uint32_t);

// Timer
typedef void (*osalTimerCb)(void*);
typedef struct{
#if APP_OS == OS_LINUX
    int timerFd;
#elif APP_OS == OS_WIN32
    HANDLE timerHandle;
#endif
    osalTimerCb timerCb;
    void* timerArg;
} osalTimer;
int osalTimerOpen(osalTimer*, osalTimerCb, void*, int);
int osalTimerClose(osalTimer*);

// Memory
int osalMalloc(void**, size_t);
int osalFree(void*);

// Thread
typedef void (*oslThreadCallback)(void *);
typedef enum{
    osalThreadPriorityIdle = 0,
    osalThreadPriorityLow,
    osalThreadPriorityBelowNormal,
    osalThreadPriorityNormal,
    osalThreadPriorityAboveNormal,
    osalThreadPriorityHigh,
    osalThreadPriorityRealtime,
} osalThreadPriority;
typedef struct{
    const char* name;
    size_t statckSize;
    int priority;
} osalThreadAttribute;
typedef struct{
#if APP_OS == OS_LINUX
    pthread_t thread;
#elif APP_OS == OS_WIN32
    HANDLE threadHandle;
    DWORD threadId;
#endif
    int isCreated;
} osalThread;
int osalThreadOpen(osalThread*, const osalThreadAttribute*, oslThreadCallback, void*);
int osalThreadSetPriority(osalThread*, osalThreadPriority);
int osalThreadJoin(osalThread*);
int osalThreadClose(osalThread*);

// Mutex
typedef struct{
#if APP_OS == OS_LINUX
    pthread_mutex_t mutex;
#elif APP_OS == OS_WIN32
    HANDLE mutexHandle;
#endif
} osalMutex;
int osalMutexOpen(osalMutex*);
int osalMutexClose(osalMutex*);
int osalMutexLock(osalMutex*, int);
int osalMutexUnlock(osalMutex*);

// Semaphore
typedef struct{
#if APP_OS == OS_LINUX
    sem_t sema;
#elif APP_OS == OS_WIN32
    HANDLE semaHandle;
#endif
} osalSemaphore;
int osalSemaphoreOpen(osalSemaphore*, int);
int osalSemaphoreClose(osalSemaphore*);
int osalSemaphoreTake(osalSemaphore*, int);
int osalSemaphoreGive(osalSemaphore*);

// Epoll (only Linux)
#if APP_OS == OS_LINUX
enum{
    osalEpollEventFlagIn = EPOLLIN,
    osalEpollEventFlagOut = EPOLLOUT,
    osalEpollEventFlagError = EPOLLERR,
};
#endif
typedef struct{
    int epollFd;
    int eventFd;
} osalEpoll;
int osalEpollOpen(osalEpoll*);
int osalEpollClose(osalEpoll*);
int osalEpollAddFd(osalEpoll*, int, uint32_t);
int osalEpollDeleteFd(osalEpoll*, int);
int osalEpollWait(osalEpoll*, int*, int);
int osalEpollNotify(osalEpoll*); 
// Etc
int osalIsInIsr(void);
