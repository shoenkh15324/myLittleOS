#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "core/sysDefs.h"
#if APP_OS == OS_LINUX
    #include <pthread.h>
    #include <semaphore.h>
    //#include <signal.h>
    #include <time.h>
    #include <sys/epoll.h>
#endif

// Time / Tick
int osalGetTimeMs(void);
int osalGetTick(void);
void osalSleepMs(int);
#if APP_OS == OS_LINUX
int64_t osalGetTimeUs(void);
int64_t osalGetTimeNs(void);
void osalSleepUs(int);
void osalGetDate(char*, size_t);
#endif

// Timer
typedef void (*osalTimerCb)(void*);
typedef struct{
#if (APP_OS == OS_LINUX) && APP_EPOLL
    int timerFd;
    osalTimerCb timerCb;
    void* timerArg;
#endif
} osalTimer;
int osalTimerOpen(osalTimer*, osalTimerCb, int);
int osalTimerClose(osalTimer*);

// Memory
int osalMalloc(void**, size_t);
int osalFree(void*);

// Thread
typedef void (*oslThreadEntry)(void *);
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
    int isCreated;
#endif
} osalThread;
int osalThreadOpen(osalThread*, const osalThreadAttribute*, oslThreadEntry, void*);
int osalThreadSetPriority(osalThread*, osalThreadPriority);
int osalThreadJoin(osalThread*);
int osalThreadClose(osalThread*);

// Mutex
typedef struct{
#if APP_OS == OS_LINUX
    pthread_mutex_t mutex;
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
#endif
} osalSemaphore;
int osalSemaphoreOpen(osalSemaphore*, int);
int osalSemaphoreClose(osalSemaphore*);
int osalSemaphoreTake(osalSemaphore*, int);
int osalSemaphoreGive(osalSemaphore*);

// Epoll (only Linux)
enum{
    osalEpollEventFlagIn = EPOLLIN,
    osalEpollEventFlagOut = EPOLLOUT,
    osalEpollEventFlagError = EPOLLERR,
};
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

#ifdef __cplusplus
}
#endif
