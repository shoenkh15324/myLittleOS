/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#include "appCfgSelector.h"

#include "core/feature/buffer.h"
#include <stdlib.h>
#if APP_OS == OS_LINUX
    #include <stdatomic.h>
#endif
#include "core/feature/log.h"
#include "core/feature/osal.h"

static inline bool _bufferLock(ringBuffer* pHandle){
#if APP_BUFFER_LOCK == SYSTEM_BUFFER_LOCK_ENABLE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    if(pHandle->lock){ logError("Already locked");
        return retFail;
    }
    pHandle->lock = true;
#endif
    return retOk;
}
static inline void _bufferUnlock(ringBuffer* pHandle){
#if APP_BUFFER_LOCK == SYSTEM_BUFFER_LOCK_ENABLE
    if(!pHandle){ logError("Invaild Params"); return; }
    pHandle->lock = false;
#endif
}
int bufferOpen(ringBuffer* pHandle, size_t size){
    if(!pHandle || !size){ logError("Invaild Params"); return retInvalidParam; }
    if(bufferReset(pHandle)){ logError("bufferReset fail");
        return retFail;
    }
    if(osalMalloc((void**)&pHandle->pBuf, size)){ logError("malloc fail");
        return retFail;
    }
    pHandle->size = size;
    return retOk;
}
int bufferClose(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    if(pHandle->pBuf){
        if(osalFree(pHandle->pBuf)){ logError("osalFree fail");
            return retFail;
        }
        pHandle->size = 0;
    }
    bufferReset(pHandle);
    return retOk;
}
int bufferReset(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    pHandle->pBuf = NULL;
    pHandle->size = 0;
    pHandle->head = 0;
    pHandle->tail = 0;
    pHandle->usage = 0;
    pHandle->lock = false;
#if APP_BUFFER_STATISTICS
    pHandle->totalPopCount = 0;
    pHandle->totalPushCount = 0;
    pHandle->maxUsage = 0;
#endif
    return retOk;
}
int bufferCanPush(ringBuffer* pHandle, size_t dataSize){
#if APP_BUFFER_PUSH_OVERWRITE
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    return ((pHandle->size - pHandle->usage) > dataSize) ? retOk : retFail;
#else
    return retOk;
#endif
}
int bufferPush(ringBuffer* pHandle, uint8_t* data, size_t dataSize){
    if(!pHandle || !data){ logError("Invaild Params"); return retInvalidParam; }
    if(pHandle->pBuf == NULL){ logError("[SYSTEM] Invalid Params"); return retInvalidParam; }
    _bufferLock(pHandle);
    if(dataSize > pHandle->size){ logError("push fail: size(%zu) > cap(%zu)", dataSize, pHandle->size);
        _bufferUnlock(pHandle);
        return retFail;
    }
#if APP_BUFFER_PUSH_OVERWRITE
    if((pHandle->usage + dataSize) > pHandle->size){
        size_t overwriteSize = (pHandle->usage + dataSize) - pHandle->size;
        logWarn("overwrite %zu bytes", overwriteSize);
        pHandle->tail = (pHandle->tail + overwriteSize) % pHandle->size;
        pHandle->usage -= overwriteSize;
    #if APP_BUFFER_STATISTICS
        pHandle->discardCount++;
    #endif
    }
#else
    if((pHandle->usage + dataSize) > pHandle->size){ logError("push fail: no space (need:%zu free:%zu)", dataSize, pHandle->size - pHandle->usage);
        _bufferUnlock(pHandle);
        return retFail;
    }
#endif
    size_t spaceToEnd = pHandle->size - pHandle->head;
    size_t firstChunk = (dataSize <= spaceToEnd) ? dataSize : spaceToEnd;
    size_t secondChunk = dataSize - firstChunk;
    memcpy(&pHandle->pBuf[pHandle->head], data, firstChunk);
    if(secondChunk){ memcpy(pHandle->pBuf, data + firstChunk, secondChunk); }
    pHandle->head = (pHandle->head + dataSize) % pHandle->size;
    pHandle->usage += dataSize;
#if APP_BUFFER_STATISTICS
    pHandle->totalPushCount++;
    if(pHandle->usage > pHandle->maxUsage) pHandle->maxUsage = pHandle->usage;
    logDebug("push %zu bytes (usage: %zu / %zu)", dataSize, pHandle->usage, pHandle->size);
#endif
    _bufferUnlock(pHandle);
    return retOk;
}
size_t bufferPop(ringBuffer* pHandle, uint8_t* pBuf, size_t bufSize){
    if(!pHandle || !pBuf){ logError("Invaild Params"); return retInvalidParam; }
    _bufferLock(pHandle);
    if(pHandle->usage == 0){ //logDebug("pop 0 bytes (empty)");
        _bufferUnlock(pHandle);
        return retOk;
    }
    size_t readSize = (bufSize < pHandle->usage) ? bufSize : pHandle->usage;
    size_t spaceToEnd = pHandle->size - pHandle->tail;
    size_t firstChunk = (readSize <= spaceToEnd) ? readSize : spaceToEnd;
    size_t secondChunk = readSize - firstChunk;
    memcpy(pBuf, &pHandle->pBuf[pHandle->tail], firstChunk);
    if(secondChunk){ memcpy(pBuf + firstChunk, pHandle->pBuf, secondChunk); }
    pHandle->tail = (pHandle->tail + readSize) % pHandle->size;
    pHandle->usage -= readSize;
#if APP_BUFFER_STATISTICS
    pHandle->totalPopCount++;
    logDebug("pop %zu bytes (usage: %zu / %zu)", readSize, pHandle->usage, pHandle->size);
#endif
    _bufferUnlock(pHandle);
    return readSize;
}
#if APP_BUFFER_PEAK
size_t bufferPeek(ringBuffer* pHandle, uint8_t* pBuf, size_t bufSize){
    if(!pHandle || !pBuf){ logError("Invaild Params"); return retInvalidParam; }
    _bufferLock(pHandle);
    if(pHandle->usage == 0){ logDebug("peek 0 bytes (empty)");
        _bufferUnlock(pHandle);
        return 0;
    }
    size_t readSize = (bufSize < pHandle->usage) ? bufSize : pHandle->usage;
    size_t spaceToEnd = pHandle->size - pHandle->tail;
    size_t firstChunk = (readSize <= spaceToEnd) ? readSize : spaceToEnd;
    size_t secondChunk = readSize - firstChunk;
    memcpy(pBuf, &pHandle->pBuf[pHandle->tail], firstChunk);
    if(secondChunk){ memcpy(pBuf + firstChunk, pHandle->pBuf, secondChunk); }
    logDebug("peek %zu bytes (usage: %zu / %zu)", readSize, pHandle->usage, pHandle->size);
    _bufferUnlock(pHandle);
    return readSize;
}
#endif

size_t inline bufferGetTotalPushCount(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    return pHandle->totalPushCount;
}
size_t inline bufferGetTotalPopped(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    return pHandle->totalPopCount;
}
size_t inline bufferGetCurrentUsage(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    return pHandle->usage;
}
size_t inline bufferGetMaxUsage(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    return pHandle->maxUsage;
}
size_t inline bufferGetDiscardCount(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    return pHandle->discardCount;
}
size_t inline bufferGetPushFailCount(ringBuffer* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    return pHandle->pushFailCount;
}
