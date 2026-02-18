/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-18
 ******************************************************************************/
#include "driver/driverCommon.h"

#if APP_DRIVER_GFX == DRIVER_GFX_OPENGL

static driverGfxOpengl _driverGfxOpengl = {
    .objState = objStateClosed,
};

int driverGfxOpenglClose(void){
    int result = retOk;
    if(_driverGfxOpengl.objState >= objStateOpening){
        osalMutexLock(&_driverGfxOpengl.objMutex, -1);
        _driverGfxOpengl.objState = objStateClosing;
        //
        _driverGfxOpengl.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_driverGfxOpengl.objMutex);
    }
    return result;
}
int driverGfxOpenglOpen(void){
    int result = retOk;
    osalMutexOpen(&_driverGfxOpengl.objMutex);
    osalMutexLock(&_driverGfxOpengl.objMutex, -1);
    _driverGfxOpengl.objState = objStateOpening;
    //
    _driverGfxOpengl.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_driverGfxOpengl.objMutex);
    return result;
}
int driverGfxOpenglSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_driverGfxOpengl.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _driverGfxOpengl.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_driverGfxOpengl.objMutex, -1);
    switch(sync){
        //
    }
syncExit:
    osalMutexUnlock(&_driverGfxOpengl.objMutex);
    return result;
}
#endif
