/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "core/feature/osal.h"
#include <time.h>
#if APP_OS == OS_LINUX
    #include <unistd.h>
    #include <sys/time.h>
    #include <errno.h>
    #include <sys/eventfd.h>
    #include <sys/timerfd.h>
#endif
#include "core/feature/log.h"

// Time / Tick
static void _osalGetAbsTime(struct timespec* pTimeSpec, int timeoutMs) {
    clock_gettime(CLOCK_MONOTONIC, pTimeSpec);
    pTimeSpec->tv_sec += (timeoutMs / 1000);
    pTimeSpec->tv_nsec += (long)(timeoutMs % 1000) * 1000000L;

    if (pTimeSpec->tv_nsec >= 1000000000L) {
        pTimeSpec->tv_sec += 1;
        pTimeSpec->tv_nsec -= 1000000000L;
    }
}
void osalGetDate(char* pBuf, size_t bufSize){
    checkParamsVoid(pBuf, bufSize);
#if APP_OS == OS_LINUX
    struct timeval  time;
    gettimeofday(&time, NULL);
    struct tm* tmInfo = localtime(&time.tv_sec);
    snprintf(pBuf, bufSize, "%02d:%02d:%02d.%03ld", tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec, time.tv_usec  / 1000);
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
void osalSleepMs(int ms){
#if APP_OS == OS_LINUX
    usleep(ms * 1000);
#endif
}
void osalSleepUs(int us){
#if APP_OS == OS_LINUX // Note: In Linux user-space, microsecond delays are not precise due to scheduler and timer resolution.
    usleep(us);
#endif
}

// Timer
int osalTimerOpen(osalTimer* pHandle, osalTimerCb expiredCallback, int periodMs){
#if APP_TIMER == SYSTEM_OSAL_TIMER_ENABLE
    checkParams(pHandle, expiredCallback, periodMs);
    #if (APP_OS == OS_LINUX) && APP_EPOLL
        pHandle->timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if(pHandle->timerFd == -1){ logError("timerfd_create fail");
            return retFail;
        }
        pHandle->timerCb = expiredCallback;
        pHandle->timerArg = pHandle;
        struct itimerspec its;
        its.it_value.tv_sec = periodMs / 1000;
        its.it_value.tv_nsec = (periodMs % 1000) * 1000000;
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
        if(timerfd_settime(pHandle->timerFd, 0, &its, NULL) == -1){ logError("timerfd_settime fail");
            close(pHandle->timerFd);
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalTimerClose(osalTimer* pHandle){
#if APP_TIMER == SYSTEM_OSAL_TIMER_ENABLE
    checkParams(pHandle);
    #if (APP_OS == OS_LINUX) && APP_EPOLL
        if (pHandle->timerFd >= 0) {
            struct itimerspec its = {0};
            timerfd_settime(pHandle->timerFd, 0, &its, NULL);
            close(pHandle->timerFd);
            pHandle->timerFd = -1;
        }
    #endif
#endif
    return retOk;
}

// Memory
#if APP_MEM == SYSTEM_OSAL_STATIC_MEM
static alignas(APP_MEM_ALIGNMENT) uint8_t _memPool[APP_MEM_POOL_SIZE] = {0};
static bool _memBlockUsed[APP_MEM_BLOCK_COUNT] = {0}; // 0 = free, 1 = used
#endif
int osalMalloc(void** pHandle, size_t size){
    checkParams(pHandle, size);
#if APP_OS == OS_LINUX
    #if APP_MEM == SYSTEM_OSAL_DYNAMIC_MEM
        *pHandle = malloc(size);
        if(*pHandle == NULL){ logError("malloc fail");
            return retFail;
        }
    #elif APP_MEM == SYSTEM_OSAL_STATIC_MEM
        if(size > APP_MEM_BLOCK_SIZE){ logError("Requested size too large (%zu > %d)", size, APP_MEM_BLOCK_SIZE);
            return retFail;
        }
        for(int i = 0; i < APP_MEM_BLOCK_COUNT; i++){
            if(!_memBlockUsed[i]){
                _memBlockUsed[i] = true;
                *pHandle = &_memPool[i * APP_MEM_BLOCK_SIZE];
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
int osalFree(void* pHandle){
    checkParams(pHandle);
#if APP_OS == OS_LINUX
    #if APP_MEM == SYSTEM_OSAL_DYNAMIC_MEM
        free(pHandle);
    #elif APP_MEM == SYSTEM_OSAL_STATIC_MEM
        uint8_t* ptr = (uint8_t*)pHandle;
        ptrdiff_t offset = ptr - _memPool;
        if(ptr < _memPool || offset >= APP_MEM_POOL_SIZE) { logError("Out of memory pool range");
            return retFail;
        }
        if(!(offset % APP_MEM_BLOCK_SIZE)){
            int index = (int)(offset / APP_MEM_BLOCK_SIZE);
            _memBlockUsed[index] = false;
        }else{ logError("Invalid memory offset (misaligned or corrupted)");
            return retFail;
        }
    #endif
#else
    //
#endif
    return retOk;
}

// Thread
int osalThreadOpen(osalThread* pHandle, const osalThreadAttribute* attr, oslThreadEntry threadEntryCb, void* userArg){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    checkParams(pHandle, attr, threadEntryCb);
    #if APP_OS == OS_LINUX
        pthread_attr_t threadAttrLinux;
        if(pthread_attr_init(&threadAttrLinux)){ logError("pthread_attr_init fail");
            return retFail;
        }
        if(attr->statckSize > 0){
            if(pthread_attr_setstacksize(&threadAttrLinux, attr->statckSize)){ logError("pthread_attr_setstacksize fail");
                pthread_attr_destroy(&threadAttrLinux);
                return retFail; 
            }
        }
        if(pthread_create(&(pHandle->thread), &threadAttrLinux, (void *(*)(void *))threadEntryCb, userArg)){ logError("pthread_create fail");
            pthread_attr_destroy(&threadAttrLinux);
            return retFail;
        }
        pHandle->isCreated = 1;
        pthread_attr_destroy(&threadAttrLinux);
    #endif
#endif
    return retOk;
}
int osalThreadSetPriority(osalThread* pHandle, osalThreadPriority priority){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        int policy = SCHED_OTHER;
        struct sched_param param;
        if(priority >= osalThreadPriorityHigh){
            policy = SCHED_FIFO;
            param.sched_priority = 10;
        }else{
            policy = SCHED_OTHER;
            param.sched_priority = 0;
        }
        if(pthread_setschedparam(pHandle->thread, policy, &param)){ logError("pthread_setschedparam fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalThreadJoin(osalThread* pHandle){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        if(pHandle->isCreated){
            if(pthread_join(pHandle->thread, NULL)){ logError("pthread_join fail");
                return retFail;
            }
        }
        pHandle->isCreated = 0;
    #endif
#endif
    return retOk;
}
int osalThreadClose(osalThread* pHandle){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        if(pHandle->isCreated) pthread_detach(pHandle->thread);
        pHandle->thread = 0;
        pHandle->isCreated = 0;
    #endif
#endif
    return retOk;
}

// Mutex
int osalMutexOpen(osalMutex* pHandle){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        pthread_mutexattr_t attr;
        if(pthread_mutexattr_init(&attr)){ logError("pthread_mutexattr_init fail");
            return retFail;
        }
        int mutexType = PTHREAD_MUTEX_NORMAL;
        #if APP_MUTEX_TYPE_RECURSIVE == SYSTEM_OSAL_MUTEX_TYPE_RECURSIVE
            mutexType = PTHREAD_MUTEX_RECURSIVE;
        #endif
        if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)){ logError("pthread_mutexattr_settype fail");
            pthread_mutexattr_destroy(&attr);
            return retFail;
        }
        if(pthread_mutex_init(&(pHandle->mutex), &attr)){ logError("pthread_mutex_init fail");
            return retFail;
        }
        if(pthread_mutexattr_destroy(&attr)){ logError("pthread_mutexattr_destroy fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalMutexClose(osalMutex* pHandle){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        if(pthread_mutex_destroy(&(pHandle->mutex))){ logError("pthread_mutex_destroy fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalMutexLock(osalMutex* pHandle, int timeoutMs){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        int res = 0;
        if(timeoutMs < 0){
            res = pthread_mutex_lock(&pHandle->mutex);
        }else if (timeoutMs == 0){
            res = pthread_mutex_trylock(&pHandle->mutex);
        }else{
            struct timespec absTime;
            _osalGetAbsTime(&absTime, timeoutMs);
            res = pthread_mutex_timedlock(&pHandle->mutex, &absTime);
        }
        if(res != 0){
            if(res == ETIMEDOUT){ logError("Mutex Lock: Timeout (%dms)", timeoutMs);
                return retTimeout;
            }
            logError("Mutex Lock: Error (%d)", res);
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalMutexUnlock(osalMutex* pHandle){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        if(pthread_mutex_unlock(&(pHandle->mutex))){ logError("pthread_mutex_unlock fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}

// Semaphore
int osalSemaphoreOpen(osalSemaphore* pHandle, int count){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    checkParams(pHandle, count);
    unsigned int initCount = 0;
    #if APP_SEMAPHORE_TYPE == SYSTEM_OSAL_SEMAPHORE_TYPE_BINARY
        initCount = (count > 0) ? 1 : 0;
    #else
        initCount = (count == -1) ? APP_SEMAPHORE_MAX_COUNT : (unsigned int)count;
    #endif
    #if APP_OS == OS_LINUX
        if(sem_init(&(pHandle->sema), 0, initCount) != 0) {
            logError("sem_init fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalSemaphoreClose(osalSemaphore* pHandle){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        if(sem_destroy(&(pHandle->sema)) != 0){ logError("sem_destroy fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalSemaphoreTake(osalSemaphore* pHandle, int timeoutMs){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        int res = 0;
        if(timeoutMs < 0){
            res = sem_wait(&pHandle->sema);
        }else if (timeoutMs == 0){
            res = sem_trywait(&pHandle->sema);
        }else{
            struct timespec absTime;
            _osalGetAbsTime(&absTime, timeoutMs);
            res = sem_timedwait(&pHandle->sema, &absTime);
        }
        if(res != 0){
            if(errno == ETIMEDOUT){ logError("Semaphore Take: Timeout (%dms)", timeoutMs);
                return retTimeout;
            }
            logError("Semaphore Take: Fail (errno=%d)", errno);
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalSemaphoreGive(osalSemaphore* pHandle){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    checkParams(pHandle);
    #if APP_OS == OS_LINUX
        if(sem_post(&(pHandle->sema)) != 0){ logError("sem_post fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}

// Epoll
int osalEpollOpen(osalEpoll* pHandle){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    checkParams(pHandle);
    pHandle->epollFd = epoll_create1(0);
    if(pHandle->epollFd == -1){ logError("epoll_create1 fail");
        return retFail;
    }
    pHandle->eventFd = eventfd(0, EFD_NONBLOCK);
    if(pHandle->eventFd == -1){ logError("eventfd fail");
        close(pHandle->epollFd);
        return retFail;
    }
    osalEpollAddFd(pHandle, pHandle->eventFd, EPOLLIN);
#endif
    return retOk;
}
int osalEpollClose(osalEpoll* pHandle){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    checkParams(pHandle);
    close(pHandle->eventFd);
    close(pHandle->epollFd);
#endif
    return retOk;
}
int osalEpollAddFd(osalEpoll* pHandle, int fd, uint32_t events){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    checkParams(pHandle, events);
    if(fd < 0) return retInvalidParam;
    struct epoll_event epollEvent;
    memset(&epollEvent, 0, sizeof(epollEvent));
    epollEvent.events = events;
    epollEvent.data.fd = fd;
    return epoll_ctl(pHandle->epollFd, EPOLL_CTL_ADD, fd, &epollEvent) ? retFail : retOk;
#else
    return retOk;
#endif
}
int osalEpollDeleteFd(osalEpoll* pHandle, int fd){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    checkParams(pHandle);
    if(fd < 0) return retInvalidParam;
    return epoll_ctl(pHandle->epollFd, EPOLL_CTL_DEL, fd, NULL) ? retFail : retOk;
#else
    return retOk;
#endif
}
int osalEpollWait(osalEpoll* pHandle, int* triggeredFd, int timeoutMs){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    checkParams(pHandle, triggeredFd, timeoutMs);
    struct epoll_event event;
    int fd = epoll_wait(pHandle->epollFd, &event, 1, timeoutMs);
    if(fd < 0){ logError("epoll_wait fail");
        return retFail;
    }else if(fd == 0){
        return retTimeout;
    }
    *triggeredFd = event.data.fd;
#endif
    return retOk;
}
int osalEpollNotify(osalEpoll* pHandle){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    checkParams(pHandle);
    uint64_t val = 1;
    return (write(pHandle->eventFd, &val, sizeof(val)) == sizeof(uint64_t)) ? retOk : retFail;
#else
    return retOk;
#endif
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

