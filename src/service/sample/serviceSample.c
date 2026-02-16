/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "service/serviceCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#if APP_DRIVER_XXX == DRIVER_SAMPLE

static serviceSample _serviceSample = {
    .objState = objStateClosed,
};

int serviceSampleOpen(void){
    int result = retOk;
    osalMutexOpen(&_serviceSample.objMutex);
    osalMutexLock(&_serviceSample.objMutex, -1);
    _serviceSample.objState = objStateOpening;
    //
    _serviceSample.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_serviceSample.objMutex);
    return result;
}
int serviceSampleClose(void){
    int result = retOk;
    if(_serviceSample.objState >= objStateOpening){
        osalMutexLock(&_serviceSample.objMutex, -1);
        _serviceSample.objState = objStateClosing;
        //
        _serviceSample.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_serviceSample.objMutex);
    }
    return result;
}
int serviceSampleSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_serviceSample.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _serviceSample.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_serviceSample.objMutex, -1);
    switch(sync){
        //
    }
syncExit:
    osalMutexUnlock(&_serviceSample.objMutex);
    return result;
}
#endif

#ifdef __cplusplus
}
#endif
