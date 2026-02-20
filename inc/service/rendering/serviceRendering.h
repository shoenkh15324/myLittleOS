#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-19
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    serviceRenderingSyncTimer = objSyncBegin,
    serviceRenderingSyncInit,
    serviceRenderingSyncDeinit,
    serviceRenderingSyncSubmitWorld,
    serviceRenderingSyncSwapBuffer,
    serviceRenderingSyncDrawFrame,
};
enum{
    serviceRenderingStateXXX = objStateBegin,
};

typedef struct serviceRendering{
    objectState objState;
    osalMutex objMutex;
    ringBuffer renderQueue[2];
    ringBuffer *pWriteBuf, *pReadBuf;
} serviceRendering;

int serviceRenderingOpen(void);
int serviceRenderingClose(void);
int serviceRenderingSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
