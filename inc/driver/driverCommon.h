#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "appCfgSelector.h"
#include "core/systemDefs.h"
#include "driverDefs.h"
#include "core/feature/log.h"
#include "core/feature/buffer.h"
#include "core/feature/async.h"
#include "core/feature/osal.h"

#if APP_DRIVER_PLATFORM == DRIVER_PLATFORM_WIN32
    #include "driver/platform/win32/driverPlatformWin32.h"
#endif
#if APP_DRIVER_GFX == DRIVER_GFX_OPENGL
    #include "driver/gfx/opengl/driverOpengl.h"
#endif

enum{
    driverCommonSyncTimer = 0,
};

int driverCommonClose(void);
int driverCommonOpen(void);
int driverCommonSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
