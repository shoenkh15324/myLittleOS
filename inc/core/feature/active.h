#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-15
 ******************************************************************************/

#include "core/feature/async.h"
#include "core/sysDefs.h"
#include "core/feature/buffer.h"
#include "core/feature/osal.h"

typedef struct {
    objectState objState;
    // Synchronization
    osalMutex objMutex;
    osalSemaphore objSema;
#if APP_OS == OS_LINUX
    osalEpoll objEpoll;
#endif
    // Event Queue
    ringBuffer eventQueue;
    size_t eventQueueSize;
    // Thread
    osalThread appThread;
    osalThreadAttribute appThreadAttr;
    void (*appThreadHandler)(void*, void*, void*);
    // Timer
    osalTimer appTimer;
    void (*appTimerHandler)(void*);
    uint64_t appTimerCount;
    // Etc
    bool isMainThread;
    uint8_t *pPayloadBuffer;
    uint16_t appEventIdxStart, appEventIdxEnd;
    size_t payloadBufferSize;
    void (*appOnOpenHandler)(void*);
} activeObject;

int activeOpen(activeObject*);
int activeClose(activeObject*);
