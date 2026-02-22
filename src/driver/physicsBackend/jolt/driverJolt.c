/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-21
 ******************************************************************************/
#include "driver/driverCommon.h"

#if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT

static driverJolt _driverJolt = {
    .objState = objStateClosed,
};

int driverJoltClose(void){
    int result = retOk;
    if(_driverJolt.objState >= objStateOpening){
        osalMutexLock(&_driverJolt.objMutex, -1);
        _driverJolt.objState = objStateClosing;
        //
        if(joltDeinit(_driverJolt.joltCtx)){ logError("joltInit fail");
            result = retFail; goto closeExit;
        }
        //
        _driverJolt.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_driverJolt.objMutex);
    }
    return result;
}
int driverJoltOpen(void){
    int result = retOk;
    osalMutexOpen(&_driverJolt.objMutex);
    osalMutexLock(&_driverJolt.objMutex, -1);
    _driverJolt.objState = objStateOpening;
    //
    _driverJolt.joltCtx = joltInit();
    if(!_driverJolt.joltCtx){ logError("joltInit fail");
        result = retFail; goto openExit;
    }
    //
    _driverJolt.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_driverJolt.objMutex);
    _driverJolt.bodyIds[_driverJolt.bodyIdIdx++] = joltCreateFloor(_driverJolt.joltCtx, 0.0f, -1.0f, 0.0f, 100.0f, 1.0f, 100.0f);
    _driverJolt.bodyIds[_driverJolt.bodyIdIdx++] = joltCreateSphere(_driverJolt.joltCtx, 0.0f, 2.0f, 0.0f, 0.5f, true);
    return result;
}
int driverJoltSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_driverJolt.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _driverJolt.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_driverJolt.objMutex, -1);
    switch(sync){
        case driverJoltSyncStep:
            joltStep(_driverJolt.joltCtx, DRIVER_PHYSICS_BACKEND_JOLT_DELTA_TIME, DRIVER_PHYSICS_BACKEND_JOLT_COLLISTION_STEP);
            joltPrintBodyPosition(_driverJolt.joltCtx, _driverJolt.bodyIds[1]);
            break;
        case driverJoltSyncGetBodyTransform:
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            joltGetBodyTransform(_driverJolt.joltCtx, _driverJolt.bodyIds[1], (float*)arg1, (float*)arg2);
            break;
    }
syncExit:
    osalMutexUnlock(&_driverJolt.objMutex);
    return result;
}
#endif
