/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "driver/driverCommon.h"

#if APP_DRIVER_XXX == DRIVER_SAMPLE

static driverSample _driverSample = {
    .objState = objStateClosed,
};

int driverSampleOpen(void){
    int result = retOk;
    osalMutexOpen(&_driverSample.objMutex);
    osalMutexLock(&_driverSample.objMutex, -1);
    _driverSample.objState = objStateOpening;
    //
    _driverSample.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_driverSample.objMutex);
    return result;
}
int driverSampleClose(void){
    int result = retOk;
    if(_driverSample.objState >= objStateOpening){
        osalMutexLock(&_driverSample.objMutex, -1);
        _driverSample.objState = objStateClosing;
        //
        _driverSample.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_driverSample.objMutex);
    }
    return result;
}
int driverSampleSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_driverSample.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _driverSample.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_driverSample.objMutex, -1);
    switch(sync){
        //
    }
syncExit:
    osalMutexUnlock(&_driverSample.objMutex);
    return result;
}
#endif
