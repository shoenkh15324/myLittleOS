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
    driverJoltSyncCreateBody,
};
enum{
    driverJoltStateXXX = objStateBegin,
};
typedef enum{
    driverJoltBodyTypeSphere,
    driverJoltBodyTypeDisk,
} driverJoltBodyType;

typedef struct{
    driverJoltBodyType bodyType;
    float position[3], rotation[4], radius, mass, friction, restitution;
    bool isDynamic;
} driverJoltBodyConfig;
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
