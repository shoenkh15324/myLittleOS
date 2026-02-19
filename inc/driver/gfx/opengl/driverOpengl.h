#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-18
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    driverOpenglSyncTimer = objSyncBegin,
    driverOpenglSyncInit,
    driverOpenglSyncDeinit,
    driverOpenglSyncBeginFrame,
    driverOpenglSyncEndFrame,
    driverOpenglSyncDrawPrimitive,
    driverOpenglSyncClear,
    driverOpenglSyncSetClearColor,
    driverOpenglSyncSetColor,
    driverOpenglSyncUpdateViewport,
};
enum{
    driverOpenglStateXXX = objStateBegin,
};

typedef struct driverOpengl{
    objectState objState;
    osalMutex objMutex;
#if APP_OS == OS_WIN32
    HWND hwnd;
    HDC hdc;
    HGLRC glrc;
#endif
    volatile float currColor[3];
} driverOpengl;

int driverOpenglOpen(void);
int driverOpenglClose(void);
int driverOpenglSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
