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
int64_t osalGetTiemNs(void){
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
    static uint8_t _memPool[APP_MEM_POOL_SIZE] = {0};
    static uint8_t _memBlockUsed[APP_MEM_BLOCK_COUNT] = {0}; // 0 = free, 1 = used
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
            if (_memBlockUsed[i] == 0) {
                _memBlockUsed[i] = 1;
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
        int index = offset / APP_MEM_BLOCK_SIZE;
        _memBlockUsed[index] = 0;
    #endif
#else
    //
#endif
    return retOk;
}
// Thread

// Mutex

// Semaphore

#ifdef __cplusplus
}
#endif

