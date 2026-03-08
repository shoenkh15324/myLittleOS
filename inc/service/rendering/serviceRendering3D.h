#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    serviceRendering3dSyncDrawFrame = objSyncBegin,
    serviceRendering3dSyncCreateEntity,
    serviceRendering3dSyncUpdateViewport,
    serviceRendering3dSyncUpdateCamera,
    serviceRendering3dSyncUpdateZoom,
    serviceRendering3dSyncUpdatePhysics,
};
enum{
    serviceRendering3dStateXXX = objStateBegin,
};
typedef enum{
#if APP_BLACKHOLE_SIMULATION
    serviceRendering3dRenderTypeStarField,
    serviceRendering3dRenderTypeBlackhole,
#endif
    serviceRendering3dRenderTypeEnd,
} serviceRendering3dRenderType;

typedef struct serviceRendering3dCamera{
    float pos[4], front[3], up[3], yaw, pitch, fov;
    float accumulatedMouseDx, accumulatedMouseDy, accumulatedMouseWheel;
    float viewMtx[16], projMtx[16];
} serviceRendering3dCamera;
typedef struct serviceRendering3dEntity{
    serviceRendering3dRenderType type;
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
    driverBgfxRenderItem item;
#endif
#if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
    uint32_t joltBodyId;
#endif
} serviceRendering3dEntity;
typedef struct serviceRendering3dScene{
    uint32_t entityCount;
    serviceRendering3dEntity entities[DRIVER_PHYSICS_BACKEND_JOLT_MAX_BODIES];
    serviceRendering3dCamera camera;
} serviceRendering3dScene;
typedef struct serviceRendering3d{
    objectState objState;
    osalMutex objMutex;
    uint16_t width, height;
    serviceRendering3dScene activeScene;
} serviceRendering3d;

int serviceRendering3dOpen(void);
int serviceRendering3dClose(void);
int serviceRendering3dSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
