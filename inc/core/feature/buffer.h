#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-12
 ******************************************************************************/
#include "core/systemDefs.h"

typedef struct ringBuffer{
    uint8_t* pBuf;
    volatile size_t size, head, tail;
} ringBuffer;

int bufferOpen(ringBuffer*, size_t);
int bufferClose(ringBuffer*);
int bufferReset(ringBuffer*);
int bufferCanPush(ringBuffer*, size_t);
int bufferPush(ringBuffer*, uint8_t*, size_t);
size_t bufferPop(ringBuffer*, uint8_t*, size_t);
