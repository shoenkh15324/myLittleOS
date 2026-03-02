#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    serviceRendering3dSyncDrawFrame = objSyncBegin,
    serviceRendering3dSyncCreateEntity,
    serviceRendering3dSyncUpdateCamera,
    serviceRendering3dSyncUpdateZoom,
};
enum{
    serviceRendering3dStateXXX = objStateBegin,
};
typedef enum{
#if APP_BLACKHOLE_SIMULATION
    serviceRendering3dRenderTypeBlackhole,
    serviceRendering3dRenderTypeAccretionDisk,
    serviceRendering3dRenderTypeBackground,
#endif
    serviceRendering3dRenderTypeEnd,
} serviceRendering3dRenderType;

typedef struct serviceRendering3dCamera{
    float pos[3], front[3], up[3], yaw, pitch, fov;
    float accumulatedMouseDx, accumulatedMouseDy, accumulatedMouseWheel;
} serviceRendering3dCamera;
typedef struct serviceRendering3dEntity{
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
    driverBgfxRenderItem renderInfo;
#endif
#if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
    uint32_t joltBodyId;
#endif
} serviceRendering3dEntity;
typedef struct serviceRendering3d{
    objectState objState;
    osalMutex objMutex;
    serviceRendering3dEntity entities[DRIVER_PHYSICS_BACKEND_JOLT_MAX_BODIES];
    uint32_t entitiyCount;
    serviceRendering3dCamera camera;
} serviceRendering3d;

int serviceRendering3dOpen(void);
int serviceRendering3dClose(void);
int serviceRendering3dSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
