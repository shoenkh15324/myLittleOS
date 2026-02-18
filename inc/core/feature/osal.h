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
typedef struct osalTimer{
#if APP_OS == OS_LINUX
    int hTimer;
#elif APP_OS == OS_WIN32
    HANDLE hTimer;
#endif
    osalTimerCb timerCb;
    void* timerArg;
} osalTimer;
int osalTimerOpen(osalTimer*, osalTimerCb, void*, int);
int osalTimerClose(osalTimer*);

// Memory
#if defined(_MSC_VER)
    #include <intrin.h>
    #define osalMemoryBarrier() MemoryBarrier()
#elif defined(__GNUC__)
    #define osalMemoryBarrier() __sync_synchronize()
#else
    #define osalMemoryBarrier() ((void)0)
#endif

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
typedef struct osalThreadAttribute{
    const char* name;
    size_t statckSize;
    int priority;
} osalThreadAttribute;
typedef struct osalThread{
#if APP_OS == OS_LINUX
    pthread_t hThread;
#elif APP_OS == OS_WIN32
    HANDLE hThread;
    DWORD threadId;
#endif
    int isCreated;
} osalThread;
int osalThreadOpen(osalThread*, const osalThreadAttribute*, oslThreadCallback, void*);
int osalThreadSetPriority(osalThread*, osalThreadPriority);
int osalThreadJoin(osalThread*);
int osalThreadClose(osalThread*);
int osalThreadGetCurrent(osalThread*);

// Mutex
typedef struct osalMutex{
#if APP_OS == OS_LINUX
    pthread_mutex_t hMutex;
#elif APP_OS == OS_WIN32
    HANDLE hMutex;
#endif
} osalMutex;
int osalMutexOpen(osalMutex*);
int osalMutexClose(osalMutex*);
int osalMutexLock(osalMutex*, int);
int osalMutexUnlock(osalMutex*);

// Semaphore
typedef struct osalSemaphore{
#if APP_OS == OS_LINUX
    sem_t hSema;
#elif APP_OS == OS_WIN32
    HANDLE hSema;
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
typedef struct osalEpoll{
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
