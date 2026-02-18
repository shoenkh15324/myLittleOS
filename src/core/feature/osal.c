/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#include "appCfgSelector.h"
#include "core/feature/osal.h"
#include <time.h>
#if APP_OS == OS_LINUX
    #include <unistd.h>
    #include <sys/time.h>
    #include <errno.h>
    #include <sys/eventfd.h>
    #include <sys/timerfd.h>
#elif APP_OS == OS_WIN32
#endif
#include "core/feature/log.h"

// Time / Tick
#if APP_OS == OS_WIN32
static int64_t _osalQueryPerformanceNs(void){
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if(!initialized){
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (int64_t)((counter.QuadPart * 1000000000LL) / freq.QuadPart);
}
#endif
void osalGetDate(char* pBuf, size_t bufSize){
    if(!pBuf || !bufSize){ logError("Invaild Params"); return; }
#if APP_OS == OS_LINUX
    struct timeval  time;
    gettimeofday(&time, NULL);
    struct tm* tmInfo = localtime(&time.tv_sec);
    snprintf(pBuf, bufSize, "%02d:%02d:%02d.%03ld", tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec, time.tv_usec / 1000);
#elif APP_OS == OS_WIN32
    SYSTEMTIME time;
    GetLocalTime(&time);
    snprintf(pBuf, bufSize, "%02d:%02d:%02d.%03d", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#endif
}
int osalGetTimeMs(void){
#if APP_OS == OS_LINUX
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (int)((time.tv_sec * 1000) + (time.tv_nsec / 1000000));
#elif APP_OS == OS_WIN32
    return (int)(_osalQueryPerformanceNs() / 1000000LL);
#else 
    return 0;
#endif
}
int64_t osalGetTimeUs(void){
#if APP_OS == OS_LINUX
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (int64_t)time.tv_sec * 1000000LL + (int64_t)(time.tv_nsec / 1000);
#elif APP_OS == OS_WIN32
    return _osalQueryPerformanceNs() / 1000LL;
#else
    return 0;
#endif
}
int64_t osalGetTimeNs(void){
#if APP_OS == OS_LINUX
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * 1000000000L + time.tv_nsec;
#elif APP_OS == OS_WIN32
    return _osalQueryPerformanceNs();
#else
    return 0;
#endif
}
int osalGetTick(void){
#if APP_OS == OS_WIN32
    return GetTickCount();
#else
    return 0;
#endif
}
void osalSleepMs(uint32_t ms){
#if APP_OS == OS_LINUX
    usleep(ms * 1000);
#elif APP_OS == OS_WIN32
    Sleep(ms);
#endif
}
void osalSleepUs(uint32_t us){
#if APP_OS == OS_LINUX // Note: In Linux user-space, microsecond delays are not precise due to scheduler and timer resolution.
    usleep(us);
#elif APP_OS == OS_WIN32
    if(us >= 1000){
        Sleep(us / 1000);
    }else{ // 1ms 이하 정밀 sleep은 busy wait
        int64_t start = _osalQueryPerformanceNs();
        int64_t waitNs = (int64_t)us * 1000LL;
        while ((_osalQueryPerformanceNs() - start) < waitNs) { YieldProcessor(); }
    }
#endif
}

// Timer
int osalTimerOpen(osalTimer* pHandle, osalTimerCb expiredCallback, void* arg, int periodMs){
#if APP_TIMER == SYSTEM_OSAL_TIMER_ENABLE
    if(!pHandle || !expiredCallback || !periodMs){ logError("Invaild Params"); return retInvalidParam; }
    #if (APP_OS == OS_LINUX) && APP_EPOLL
        pHandle->hTimer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if(pHandle->hTimer == -1){ logError("timerfd_create fail");
            return retFail;
        }
        pHandle->timerCb = expiredCallback;
        pHandle->timerArg = arg;
        struct itimerspec its;
        its.it_value.tv_sec = periodMs / 1000;
        its.it_value.tv_nsec = (periodMs % 1000) * 1000000;
        its.it_interval = its.it_value;
        if(timerfd_settime(pHandle->hTimer, 0, &its, NULL) == -1){ logError("timerfd_settime fail");
            close(pHandle->hTimer);
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        pHandle->hTimer = CreateWaitableTimer(NULL, false, NULL);
        if(!pHandle->hTimer){ logError("CreateWaitableTimer fail");
            return retFail;
        }
        pHandle->timerCb = expiredCallback;
        pHandle->timerArg = arg;
        LARGE_INTEGER dueTime;
        dueTime.QuadPart = -(LONGLONG)periodMs * 10000;
        if(!SetWaitableTimer(pHandle->hTimer, &dueTime, periodMs, NULL, NULL, false)){ logError("SetWaitableTimer fail");
            CloseHandle(pHandle->hTimer);
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalTimerClose(osalTimer* pHandle){
#if APP_TIMER == SYSTEM_OSAL_TIMER_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if (APP_OS == OS_LINUX) && APP_EPOLL
        if (pHandle->hTimer >= 0) {
            struct itimerspec its = {0};
            timerfd_settime(pHandle->hTimer, 0, &its, NULL);
            close(pHandle->hTimer);
            pHandle->hTimer = -1;
        }
    #elif APP_OS == OS_WIN32
        if(pHandle->hTimer){
            CancelWaitableTimer(pHandle->hTimer);
            CloseHandle(pHandle->hTimer);
            pHandle->hTimer = NULL;
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
    if(!pHandle || !size){ logError("Invaild Params"); return retInvalidParam; }
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
    return retOk;
}
int osalFree(void* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
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
    return retOk;
}

// Thread
#if APP_OS == OS_WIN32
typedef struct{
    oslThreadCallback userCallback;
    void* userArg;
} osalThreadWin32Arg;
static DWORD WINAPI _win32ThreadWrapper(LPVOID arg){
    osalThreadWin32Arg* win32Arg = (osalThreadWin32Arg*)arg;
    if(win32Arg && win32Arg->userCallback){
        win32Arg->userCallback(win32Arg->userArg);
    }
    osalFree(win32Arg);
    return 0;
}
#endif
int osalThreadOpen(osalThread* pHandle, const osalThreadAttribute* attr, oslThreadCallback threadEntryCb, void* userArg){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    if(!pHandle || !attr || !threadEntryCb){ logError("Invaild Params"); return retInvalidParam; }
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
        if(pthread_create(&(pHandle->hThread), &threadAttrLinux, (void *(*)(void *))threadEntryCb, userArg)){ logError("pthread_create fail");
            pthread_attr_destroy(&threadAttrLinux);
            return retFail;
        }
        pHandle->isCreated = 1;
        pthread_attr_destroy(&threadAttrLinux);
    #elif APP_OS == OS_WIN32
        osalThreadWin32Arg* win32Arg = NULL;
        if(osalMalloc((void**)&win32Arg, sizeof(osalThreadWin32Arg))){ logError("osalMalloc fail");
            return retFail;
        }
        win32Arg->userCallback = threadEntryCb;
        win32Arg->userArg = userArg;
        pHandle->hThread = CreateThread(NULL, attr->statckSize, _win32ThreadWrapper, win32Arg, 0, &pHandle->threadId);
        if(!pHandle->hThread){ logError("CreateThread fail");
            osalFree(win32Arg);
            return retFail;
        }
        pHandle->isCreated = 1;
    #endif
#endif
    return retOk;
}
int osalThreadSetPriority(osalThread* pHandle, osalThreadPriority priority){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
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
        if(pthread_setschedparam(pHandle->hThread, policy, &param)){ logError("pthread_setschedparam fail");
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        int winPriority = THREAD_PRIORITY_NORMAL;
        switch(priority){
            case osalThreadPriorityIdle:        winPriority = THREAD_PRIORITY_IDLE; break;
            case osalThreadPriorityLow:         winPriority = THREAD_PRIORITY_LOWEST; break;
            case osalThreadPriorityBelowNormal: winPriority = THREAD_PRIORITY_BELOW_NORMAL; break;
            case osalThreadPriorityNormal:      winPriority = THREAD_PRIORITY_NORMAL; break;
            case osalThreadPriorityAboveNormal: winPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
            case osalThreadPriorityHigh:        winPriority = THREAD_PRIORITY_HIGHEST; break;
            case osalThreadPriorityRealtime:    winPriority = THREAD_PRIORITY_TIME_CRITICAL; break;
        }
        if(!SetThreadPriority(pHandle->hThread, winPriority)){ logError("SetThreadPriority fail");
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalThreadJoin(osalThread* pHandle){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        if(pHandle->isCreated){
            if(pthread_join(pHandle->hThread, NULL)){ logError("pthread_join fail");
                return retFail;
            }
        }
        pHandle->isCreated = 0;
    #elif APP_OS == OS_WIN32
        if(pHandle->isCreated){
            if(WaitForSingleObject(pHandle->hThread, INFINITE) != WAIT_OBJECT_0){ logError("WaitForSingleObject fail");
                return retFail;
            }
        }
        CloseHandle(pHandle->hThread);
        pHandle->hThread = NULL;
        pHandle->threadId = 0;
        pHandle->isCreated = 0;
    #endif
#endif
    return retOk;
}
int osalThreadClose(osalThread* pHandle){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        if(pHandle->isCreated) pthread_detach(pHandle->hThread);
        pHandle->hThread = 0;
        pHandle->isCreated = 0;
    #elif APP_OS == OS_WIN32
        if(pHandle->isCreated && pHandle->hThread){
            CloseHandle(pHandle->hThread);
            pHandle->hThread = NULL;
            pHandle->threadId = 0;
            pHandle->isCreated = 0;
        }
    #endif
#endif
    return retOk;
}
int osalThreadGetCurrent(osalThread* pHandle){
#if APP_THREAD == SYSTEM_OSAL_THREAD_ENABLE
    #if APP_OS == OS_LINUX
        pHandle->hThread = pthread_self();
    #elif APP_OS == OS_WIN32
        pHandle->hThread = GetCurrentThread();
    #endif
#endif
    return retOk;
}

// Mutex
int osalMutexOpen(osalMutex* pHandle){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
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
        if(pthread_mutex_init(&(pHandle->hMutex), &attr)){ logError("pthread_mutex_init fail");
            return retFail;
        }
        if(pthread_mutexattr_destroy(&attr)){ logError("pthread_mutexattr_destroy fail");
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        pHandle->hMutex = CreateMutex(NULL, FALSE, NULL);
        if(pHandle->hMutex == NULL){ logError("CreateMutex fail"); 
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalMutexClose(osalMutex* pHandle){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        if(pthread_mutex_destroy(&(pHandle->hMutex))){ logError("pthread_mutex_destroy fail");
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        if(pHandle->hMutex){ CloseHandle(pHandle->hMutex); }
        pHandle->hMutex = NULL;
    #endif
#endif
    return retOk;
}
int osalMutexLock(osalMutex* pHandle, int timeoutMs){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        int res = 0;
        if(timeoutMs < 0){
            res = pthread_mutex_lock(&pHandle->hMutex);
        }else if (timeoutMs == 0){
            res = pthread_mutex_trylock(&pHandle->hMutex);
        }else{
            struct timespec absTime;
            clock_gettime(CLOCK_MONOTONIC, &absTime);
            absTime.tv_sec  += timeoutMs / 1000;
            absTime.tv_nsec += (timeoutMs % 1000) * 1000000L;
            if(absTime.tv_nsec >= 1000000000L){
                absTime.tv_sec += 1;
                absTime.tv_nsec -= 1000000000L;
            }
            res = pthread_mutex_timedlock(&pHandle->hMutex, &absTime);
        }
        if(res != 0){
            if(res == ETIMEDOUT){ logError("Mutex Lock: Timeout (%dms)", timeoutMs);
                return retTimeout;
            }
            logError("Mutex Lock: Error (%d)", res);
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        DWORD waitTime = (timeoutMs < 0) ? INFINITE : (DWORD)timeoutMs;
        DWORD res = WaitForSingleObject(pHandle->hMutex, waitTime);
        if(res == WAIT_TIMEOUT) return retTimeout;
        if(res != WAIT_OBJECT_0){ logError("WaitForSingleObject fail"); 
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalMutexUnlock(osalMutex* pHandle){
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        if(pthread_mutex_unlock(&(pHandle->hMutex))){ logError("pthread_mutex_unlock fail");
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        if(!ReleaseMutex(pHandle->hMutex)){ logError("ReleaseMutex fail"); 
            return retFail; 
        }
    #endif
#endif
    return retOk;
}

// Semaphore
int osalSemaphoreOpen(osalSemaphore* pHandle, int count){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    if(!pHandle){ logError("Invalid Params"); return retInvalidParam; }
    int maxCount = (count <= 0) ? APP_SEMAPHORE_MAX_COUNT : count; 
    #if APP_SEMAPHORE_TYPE == SYSTEM_OSAL_SEMAPHORE_TYPE_BINARY
        maxCount = 1;
    #endif
    #if APP_OS == OS_LINUX
        if(sem_init(&(pHandle->hSema), 0, maxCount) != 0) {
            logError("sem_init fail");
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        pHandle->hSema = CreateSemaphore(NULL, 0, maxCount, NULL);
        if(pHandle->hSema == NULL){ logError("CreateSemaphore fail / GetLastError=%lu", GetLastError());
            return retFail;
        }
    #endif
#endif
    return retOk;
}
int osalSemaphoreClose(osalSemaphore* pHandle){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        if(sem_destroy(&(pHandle->hSema)) != 0){ logError("sem_destroy fail");
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        if(pHandle->hSema){ CloseHandle(pHandle->hSema); }
        pHandle->hSema = NULL;
    #endif
#endif
    return retOk;
}
int osalSemaphoreTake(osalSemaphore* pHandle, int timeoutMs){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        int res = 0;
        if(timeoutMs < 0){
            res = sem_wait(&pHandle->hSema);
        }else if (timeoutMs == 0){
            res = sem_trywait(&pHandle->hSema);
        }else{
            struct timespec absTime;
            clock_gettime(CLOCK_MONOTONIC, &absTime);
            absTime.tv_sec  += timeoutMs / 1000;
            absTime.tv_nsec += (timeoutMs % 1000) * 1000000L;
            if(absTime.tv_nsec >= 1000000000L){
                absTime.tv_sec += 1;
                absTime.tv_nsec -= 1000000000L;
            }
            res = sem_timedwait(&pHandle->hSema, &absTime);
        }
        if(res != 0){
            if(errno == ETIMEDOUT){ logError("Semaphore Take: Timeout (%dms)", timeoutMs);
                return retTimeout;
            }
            logError("Semaphore Take: Fail (errno=%d)", errno);
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        DWORD waitTime = (timeoutMs < 0) ? INFINITE : (DWORD)timeoutMs;
        DWORD res = WaitForSingleObject(pHandle->hSema, waitTime);
        if(res == WAIT_TIMEOUT) return retTimeout;
        if(res != WAIT_OBJECT_0){ logError("WaitForSingleObject fail"); 
            return retFail; 
        }
    #endif
#endif
    return retOk;
}
int osalSemaphoreGive(osalSemaphore* pHandle){
#if APP_SEMAPHORE == SYSTEM_OSAL_SEMAPHORE_ENABLE
    if(!pHandle || !pHandle->hSema){ logError("Invaild Params"); return retInvalidParam; }
    #if APP_OS == OS_LINUX
        if(sem_post(&(pHandle->hSema)) != 0){ logError("sem_post fail");
            return retFail;
        }
    #elif APP_OS == OS_WIN32
        if(!ReleaseSemaphore(pHandle->hSema, 1, NULL)){
            DWORD err = GetLastError();
            logError("ReleaseSemaphore fail / hSema=%p / GetLastError=%lu", pHandle->hSema, err);
            return retFail;
        }
    #endif
#endif
    return retOk;
}

// Epoll
int osalEpollOpen(osalEpoll* pHandle){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
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
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    close(pHandle->eventFd);
    close(pHandle->epollFd);
#endif
    return retOk;
}
int osalEpollAddFd(osalEpoll* pHandle, int fd, uint32_t events){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
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
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    if(fd < 0) return retInvalidParam;
    return epoll_ctl(pHandle->epollFd, EPOLL_CTL_DEL, fd, NULL) ? retFail : retOk;
#else
    return retOk;
#endif
}
int osalEpollWait(osalEpoll* pHandle, int* triggeredFd, int timeoutMs){
#if APP_EPOLL == SYSTEM_OSAL_EPOLL_ENABLE
    if(!pHandle || !triggeredFd){ logError("Invaild Params"); return retInvalidParam; }
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
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    uint64_t val = 1;
    return (write(pHandle->eventFd, &val, sizeof(val)) == sizeof(uint64_t)) ? retOk : retFail;
#else
    return retOk;
#endif
} 

// Etc
int osalIsInIsr(void){
#if (APP_OS == OS_LINUX) || (APP_OS == OS_WIN32)
    return 0; // User-space Linux has no direct ISR access.
#endif
}
