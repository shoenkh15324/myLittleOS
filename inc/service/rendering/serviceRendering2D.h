#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-19
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    serviceRendering2dSyncTimer = objSyncBegin,
    serviceRendering2dSyncInit,
    serviceRendering2dSyncDeinit,
    serviceRendering2dSyncSubmitWorld,
    serviceRendering2dSyncSwapBuffer,
    serviceRendering2dSyncDrawFrame,
};
enum{
    serviceRendering2dStateXXX = objStateBegin,
};

typedef struct serviceRendering2d{
    objectState objState;
    osalMutex objMutex;
    ringBuffer renderQueue[2];
    ringBuffer *pWriteBuf, *pReadBuf;
} serviceRendering2d;

int serviceRendering2dOpen(void);
int serviceRendering2dClose(void);
int serviceRendering2dSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
