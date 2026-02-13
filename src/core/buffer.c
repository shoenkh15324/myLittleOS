/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
 #include "core/buffer.h"
#if APP_OS == OS_LINUX
    #include <stdatomic.h>
#endif

static inline void _bufferLock(ringBuffer* this){
    checkParamsVoid(this);
#if APP_OS == OS_LINUX
    while(atomic_flag_test_and_set(&this->lock)){} // busy-wait
#else
    this->lock = true;
#endif    
}
static inline void _bufferUnlock(ringBuffer* this){
    checkParamsVoid(this);
#if APP_OS == OS_LINUX
    atomic_flag_clear(&this->lock);
#else
    this->lock = false;
#endif  
}
int bufferOpen(ringBuffer* this, size_t size){
    checkParams(this, size);
    if(bufferReset(this)){ logError("bufferReset fail");
        return retFail;
    }
    if(osalMalloc(&this->buf, size)){ logError("malloc fail");
        return retFail;
    }
    this->size = size;
    return retOk;
}
int bufferClose(ringBuffer* this){
    checkParams(this);
    if(this->buf){
        if(osalFree(this->buf)){ logError("osalFree fail");
            return retFail;
        }
        this->size = 0;
    }
    bufferReset(this);
    return retOk;
}
int bufferReset(ringBuffer* this){
    checkParams(this);
    this->buf = NULL;
    this->size = 0;
    this->head = 0;
    this->tail = 0;
    this->usage = 0;
    this->lock = false;
#if APP_BUFFER_STATISTICS
    this->totalPopCount = 0;
    this->totalPushCount = 0;
    this->maxUsage = 0;
#endif
    return retOk;
}
int inline bufferIsFull(ringBuffer* this){
    checkParams(this);
    return this->usage >= this->size;
}
int bufferPush(ringBuffer* this, uint8_t* data, size_t dataSize){
    checkParams(this, this->buf, data, dataSize);
    _bufferLock(this);
    if((this->usage + dataSize) > this ->size){ 
#if APP_BUFFER_PUSH_OVERWRIT
        if (dataSize > this->size) {
            data = &data[dataSize - this->size];
            dataSize = this->size;
        }
        size_t overflow = (this->usage + dataSize) - this->size;
        this->tail = (this->tail + overflow) % this->size;
        this->usage -= overflow;
    #if APP_BUFFER_STATISTICS
        this->discardCount++;
    #endif
#else
        logError("Insufficient buffer size");
#if APP_BUFFER_STATISTICS
        this->pushFailCount++;
#endif
        _bufferUnlock(this);
        return retFail;
#endif
    }
    size_t firstChunk = this->size - this->head;
    if(firstChunk > dataSize) firstChunk = dataSize;
    memcpy(&this->buf[this->head], data, firstChunk);
    size_t secondChunk = dataSize - firstChunk;
    if(secondChunk > 0){
        memcpy(this->buf, &data[firstChunk], secondChunk);
    }
    this->head = (this->head + dataSize) % this->size;
    this->usage += dataSize;
#if APP_BUFFER_STATISTICS
    this->totalPushCount++;
    if(this->usage > this->maxUsage) this->maxUsage = this->usage;
#endif
    _bufferUnlock(this);
    return retOk;
}
size_t bufferPop(ringBuffer* this, uint8_t* buf, size_t bufSize){
    checkParams(this, this->buf, buf, bufSize);
    _bufferLock(this);
    if(this->usage == 0){
        _bufferUnlock(this);
        return 0;
    }
    size_t readSize = bufSize < this->usage ? bufSize : this->usage;
    size_t firstChunk = this->size - this->tail;
    if(firstChunk > readSize) firstChunk = readSize;
    memcpy(buf, &this->buf[this->tail], firstChunk);
    size_t secondChunk = readSize - firstChunk;
    if(secondChunk > 0){
        memcpy(&buf[firstChunk], this->buf, secondChunk);
    }
    this->tail = (this->tail + readSize) % this->size;
    this->usage -= readSize;
#if APP_BUFFER_STATISTICS
    this->totalPopCount++;
#endif
    _bufferUnlock(this);
    return readSize;
}
#if APP_BUFFER_PEAK
size_t bufferPeek(ringBuffer* this, uint8_t* buf, size_t bufSize){
    checkParams(this->buf, this, buf, bufSize);
    _bufferLock(this);
    if(this->usage == 0){
        _bufferUnlock(this);
        return 0;
    }
    size_t readSize = bufSize < this->usage ? bufSize : this->usage;
    size_t firstChunk = this->size - this->tail;
    if(firstChunk > readSize) firstChunk = readSize;
    memcpy(buf, &this->buf[this->tail], firstChunk);
    size_t secondChunk = readSize - firstChunk;
    if(secondChunk > 0){
        memcpy(&buf[firstChunk], this->buf, secondChunk);
    }
    _bufferUnlock(this);
    return (int)readSize; // 실제 읽은 바이트 수
}
#endif
#if APP_BUFFER_STATISTICS
size_t inline bufferGetTotalPushCount(ringBuffer* this){
    checkParams(this);
    return this->totalPushCount;
}
size_t inline bufferGetTotalPopped(ringBuffer* this){
    checkParams(this);
    return this->totalPopCount;
}
size_t inline bufferGetCurrentUsage(ringBuffer* this){
    checkParams(this);
    return this->usage;
}
size_t inline bufferGetMaxUsage(ringBuffer* this){
    checkParams(this);
    return this->maxUsage;
}
size_t inline bufferGetDiscardCount(ringBuffer* this){
    checkParams(this);
    return this->discardCount;
}
size_t inline bufferGetPushFailCount(ringBuffer* this){
    checkParams(this);
    return this->pushFailCount;
}
#endif

#ifdef __cplusplus
}
#endif
