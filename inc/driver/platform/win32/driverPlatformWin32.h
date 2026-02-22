#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-18
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    driverPlatformWin32SyncTimer = objSyncBegin,
    driverPlatformWin32SyncCreateWindow,
    driverPlatformWin32SyncShowWindow,
    driverPlatformWin32SyncResizeWindow,
    driverPlatformWin32SyncDestroyWindow,
    driverPlatformWin32SyncGetNativeHandle,
    driverPlatformWin32SyncGetClientSize,
};
enum{
    driverPlatformWin32StateXXX = objStateBegin,
};

typedef struct driverPlatformWin32{
    objectState objState;
    osalMutex objMutex;
    HWND hwnd;
    HDC hdc;
    int width, height;
    volatile int running, msgNeedAsync;
} driverPlatformWin32;

int driverPlatformWin32Open(void);
int driverPlatformWin32Close(void);
int driverPlatformWin32Sync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
