/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#include "appCfgSelector.h"
#include "core/feature/buffer.h"
#include "core/feature/log.h"
#include "core/feature/osal.h"
#include <stdlib.h>

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
    return retOk;
}
int bufferCanPush(ringBuffer* pHandle, size_t dataSize){
    if(!pHandle || !pHandle->pBuf || dataSize == 0){ logError("Invaild Params"); return retInvalidParam; }
    size_t used = (pHandle->head >= pHandle->tail) ? (pHandle->head - pHandle->tail) : (pHandle->size - (pHandle->tail - pHandle->head));
    if((pHandle->size - used - 1) < dataSize){
        return retFail;
    }
    return retOk;
}
int bufferPush(ringBuffer* pHandle, uint8_t* data, size_t dataSize) {
    if(!pHandle || !pHandle->pBuf || !data || dataSize == 0){ logError("Invaild Params"); return retInvalidParam; }
    size_t head = pHandle->head;
    size_t tail = pHandle->tail;
    size_t used = (head >= tail) ? (head - tail) : (pHandle->size - (tail - head));
    size_t freeSpace = (pHandle->size > used) ? (pHandle->size - used - 1) : 0;
    if(dataSize > freeSpace){ logError("push fail: no space (need:%zu free:%zu)", dataSize, freeSpace);
        return retFail;
    }
    size_t spaceToEnd = pHandle->size - head;
    size_t firstChunk = (dataSize <= spaceToEnd) ? dataSize : spaceToEnd;
    size_t secondChunk = dataSize - firstChunk;
    memcpy(&pHandle->pBuf[head], data, firstChunk);
    if (secondChunk > 0) {
        memcpy(pHandle->pBuf, data + firstChunk, secondChunk);
    }
    osalMemoryBarrier();
    pHandle->head = (head + dataSize) % pHandle->size;
    return retOk;
}
size_t bufferPop(ringBuffer* pHandle, uint8_t* pBuf, size_t bufSize){
    if(!pHandle || !pBuf){ logError("Invaild Params"); return retInvalidParam; }
    size_t head = pHandle->head;
    size_t tail = pHandle->tail;
    osalMemoryBarrier();
    size_t available = (head >= tail) ? (head - tail) : (pHandle->size - (tail - head));
    if(available == 0){ // 데이터 없음 (Empty)
        return 0; 
    }
    size_t readSize = (bufSize < available) ? bufSize : available;
    size_t spaceToEnd = pHandle->size - tail;
    size_t firstChunk = (readSize <= spaceToEnd) ? readSize : spaceToEnd;
    size_t secondChunk = readSize - firstChunk;
    memcpy(pBuf, &pHandle->pBuf[tail], firstChunk);
    if(secondChunk > 0){
        memcpy(pBuf + firstChunk, pHandle->pBuf, secondChunk);
    }
    osalMemoryBarrier();
    pHandle->tail = (tail + readSize) % pHandle->size;
    return readSize;
}
