#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app/appCommon.h"

#if APP_OS == OS_LINUX
    #include <pthread.h>
    #include <semaphore.h>
#endif

// Time / Tick
int osalGetTimeMs(void);
int osalGetTick(void);
void osalDelayMs(int);
void osalDelayTick(int);
#if APP_OS == OS_LINUX
int64_t osalGetTimeUs(void);
int64_t osalGetTimeNs(void);
void osalDelayUs(int);
void osalGetDate(char*, size_t);
#endif

// Timer


// Memory
int osalMalloc(void**, size_t);
int osalFree(void*);

// Thread


// Mutex
typedef struct{
#if APP_OS == OS_LINUX
    pthread_mutex_t mutex;
#endif
} osalMutex;
int osalMutexOpen(osalMutex*);
int osalMutexClose(osalMutex*);
int osalMutexLock(osalMutex*);
int osalMutexUnlock(osalMutex*);

// Semaphore
typedef struct{
#if APP_OS == OS_LINUX
    sem_t sema;
#endif
} osalSemaphore;
int osalSemaphoreOpen(osalSemaphore*, int);
int osalSemaphoreClose(osalSemaphore*);
int osalSemaphoreTake(osalSemaphore*);
int osalSemaphoreGive(osalSemaphore*);

// Etc
int osalIsInIsr(void);

#ifdef __cplusplus
}
#endif
