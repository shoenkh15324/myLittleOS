/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "core/osal.h"
#include "core/system.h"
#if APP_OS == OS_LINUX
    #include <unistd.h>
    #include <sys/time.h>
#endif

// Time / Tick
void osalGetDate(char* buf, size_t bufSize){
    checkParamsVoid(buf, bufSize);
#if APP_OS == OS_LINUX
    struct timeval  time;
    gettimeofday(&time, NULL);
    struct tm* tmInfo = localtime(&time.tv_sec);
    snprintf(buf, bufSize, "%02d:%02d:%02d.%03ld", tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec, time.tv_usec  / 1000);
#endif
}
int osalGetTimeMs(void){
#if APP_OS == OS_LINUX
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (int)((time.tv_sec * 1000) + (time.tv_nsec / 1000000));
#else 
    return 0;
#endif
}
int64_t osalGetTimeUs(void){
#if APP_OS == OS_LINUX
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (int64_t)time.tv_sec * 1000000LL + (int64_t)(time.tv_nsec / 1000);
#else
    return 0;
#endif
}
int64_t osalGetTimeNs(void){
#if APP_OS == OS_LINUX
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * 1000000000L + time.tv_nsec;
#else
    return 0;
#endif
}
int osalGetTick(void){
    //
    return 0;
}
void osalDelayMs(int ms){
#if APP_OS == OS_LINUX
    usleep(ms * 1000);
#endif
}
void osalDelayUs(int us){
#if APP_OS == OS_LINUX // Note: In Linux user-space, microsecond delays are not precise due to scheduler and timer resolution.
    usleep(us);
#endif
}
void osalDelayTick(int tick){
    //
}
// Timer


// Memory
#if APP_MEM == SYSTEM_OSAL_STATIC_MEM
static alignas(max_align_t) uint8_t _memPool[APP_MEM_POOL_SIZE] = {0};
static bool _memBlockUsed[APP_MEM_BLOCK_COUNT] = {0}; // 0 = free, 1 = used
#endif
int osalMalloc(void** ptr, size_t size){
    checkParams(ptr, size);
#if APP_OS == OS_LINUX
    #if APP_MEM == SYSTEM_OSAL_DYNAMIC_MEM
        *ptr = malloc(size);
        if(*ptr == NULL){ logError("malloc fail");
            return retFail;
        }
    #elif APP_MEM == SYSTEM_OSAL_STATIC_MEM
        if(size > APP_MEM_BLOCK_SIZE){ logError("Requested size too large");
            return retFail;
        }
        for(int i = 0; i < APP_MEM_BLOCK_COUNT; i++){
            if(!_memBlockUsed[i]){
                _memBlockUsed[i] = true;
                *ptr = &_memPool[i * APP_MEM_BLOCK_SIZE];
                return retOk;
            }
        }
        logError("Insufficient memory pool size");
        return retFail;
    #endif
#else
    //
#endif
    return retOk;
}
int osalFree(void* ptr){
    checkParams(ptr);
#if APP_OS == OS_LINUX
    #if APP_MEM == SYSTEM_OSAL_DYNAMIC_MEM
        free(ptr);
    #elif APP_MEM == SYSTEM_OSAL_STATIC_MEM
        uintptr_t offset = (uint8_t*)ptr - _memPool;
        if((uint8_t*)ptr < _memPool || offset >= APP_MEM_POOL_SIZE){ logError("Out of memory pool range");
            return retFail;
        }
        if((offset % APP_MEM_BLOCK_SIZE) == 0){
            int index = offset / APP_MEM_BLOCK_SIZE;
            _memBlockUsed[index] = 0;
        }else{ logError("Invaild memory offset");
            return retFail;
        }
    #endif
#else
    //
#endif
    return retOk;
}

// Thread

// Mutex
int osalMutexOpen(osalMutex* this){
    checkParams(this);
#if APP_OS == OS_LINUX
    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr)){ logError("pthread_mutexattr_init fail");
        return retFail;
    }
    if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)){ logError("pthread_mutexattr_settype fail");
        pthread_mutexattr_destroy(&attr);
        return retFail;
    }
    if(pthread_mutex_init(&(this->mutex), &attr)){ logError("pthread_mutex_init fail");
        return retFail;
    }
    if(pthread_mutexattr_destroy(&attr)){ logError("pthread_mutexattr_destroy fail");
        return retFail;
    }
#endif
    return retOk;
}
int osalMutexClose(osalMutex* this){
    checkParams(this);
#if APP_OS == OS_LINUX
    if(pthread_mutex_destroy(&(this->mutex))){ logError("pthread_mutex_destroy fail");
        return retFail;
    }
#endif
    return retOk;
}
int osalMutexLock(osalMutex* this){
    checkParams(this);
#if APP_OS == OS_LINUX
    if(pthread_mutex_lock(&(this->mutex))){ logError("pthread_mutex_lock fail");
        return retFail;
    }
#endif
    return retOk;
}
int osalMutexUnlock(osalMutex* this){
    checkParams(this);
#if APP_OS == OS_LINUX
    if(pthread_mutex_unlock(&(this->mutex))){ logError("pthread_mutex_unlock fail");
        return retFail;
    }
#endif
    return retOk;
}

// Semaphore
int osalSemaphoreOpen(osalSemaphore* this, int count){
    checkParams(this, count);
#if APP_OS == OS_LINUX
    count = (count == -1) ? 32767 : count;
    if(sem_init(&(this->sema), 0, (unsigned int)count) != 0){ logError("sem_init fail");
        return retFail;
    }
#endif
    return retOk;
}
int osalSemaphoreClose(osalSemaphore* this){
    checkParams(this);
#if APP_OS == OS_LINUX
    if(sem_destroy(&(this->sema)) != 0){ logError("sem_destroy fail");
        return retFail;
    }
#endif
    return retOk;
}
int osalSemaphoreTake(osalSemaphore* this){
    checkParams(this);
#if APP_OS == OS_LINUX
    if(sem_wait(&(this->sema)) != 0){ logError("sem_wait fail");
        return retFail;
    }
#endif
    return retOk;
}
int osalSemaphoreGive(osalSemaphore* this){
    checkParams(this);
#if APP_OS == OS_LINUX
    if(sem_post(&(this->sema)) != 0){ logError("sem_post fail");
        return retFail;
    }
#endif
    return retOk;
}

// Etc
int osalIsInIsr(void){
#if APP_OS == OS_LINUX
    return 0; // User-space Linux has no direct ISR access.
#endif
}

#ifdef __cplusplus
}
#endif

