#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include "core/sysDefs.h"

typedef enum{
    asyncTypeAsync = 0,
    asyncTypeAsyncPayload,
    asyncTypeAwait,
    asyncTypeExpress,
} asyncType;

typedef struct{
    asyncType type;
    uint16_t eventId;
    uintptr_t arg1, arg2, arg3, arg4;
    size_t payloadSize;
} __attribute__((packed)) asyncPacket;

typedef struct{
    void* pActObj;
    uint16_t startId, endId;
} asyncSubscriber;

int asyncSubscribe(void*, uint16_t, uint16_t);
int asyncPush(asyncType, uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
size_t asyncPop(void*, asyncPacket*, uint8_t*);

#ifdef __cplusplus
}
#endif
