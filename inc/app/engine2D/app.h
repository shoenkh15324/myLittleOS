#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "core/system.h"
#include "core/feature/active.h"
#include "core/physics/world/world2D.h"

typedef enum{
    appMainStateXXX = objStateBegin,
} appMainState;
typedef enum{
    appTestStateXXX = objStateBegin,
} appRenderState;

typedef struct appMain{
    activeObject actor;
    world2d* world;
} appMain;
typedef struct appRender{
    activeObject actor;
} appRender;

int appClose(void);
int appOpen(void);
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
