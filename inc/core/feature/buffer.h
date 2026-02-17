#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-12
 ******************************************************************************/
#include "core/systemDefs.h"

typedef struct{
    uint8_t* pBuf;
    size_t size, head, tail, usage;
    bool lock;
    size_t totalPushCount, totalPopCount, maxUsage, discardCount, pushFailCount, errorCount;
} ringBuffer;

int bufferOpen(ringBuffer*, size_t);
int bufferClose(ringBuffer*);
int bufferReset(ringBuffer*);
int bufferCanPush(ringBuffer*, size_t);
int bufferPush(ringBuffer*, uint8_t*, size_t);
size_t bufferPop(ringBuffer*, uint8_t*, size_t);
size_t bufferPeek(ringBuffer*, uint8_t*, size_t);
size_t bufferGetTotalPushCount(ringBuffer*);
size_t bufferGetTotalPopped(ringBuffer*);
size_t bufferGetCurrentUsage(ringBuffer*);
size_t bufferGetMaxUsage(ringBuffer*);
size_t bufferGetDiscardCount(ringBuffer*);
size_t bufferGetPushFailCount(ringBuffer*);

