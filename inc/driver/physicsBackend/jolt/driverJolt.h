#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "core/feature/osal.h"
#include "driverJoltBridge.h"

enum{
    driverJoltSyncStep = objSyncBegin,
    driverJoltSyncGetBodyTransform,
};
enum{
    driverJoltStateXXX = objStateBegin,
};

typedef struct driverJolt{
    objectState objState;
    osalMutex objMutex;
    joltContext* joltCtx;
    unsigned int bodyIds[DRIVER_PHYSICS_BACKEND_JOLT_MAX_BODIES];
    unsigned int bodyIdIdx;
} driverJolt;

int driverJoltOpen(void);
int driverJoltClose(void);
int driverJoltSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
