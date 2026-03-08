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
    joltSetWorldGravity(_driverJolt.joltCtx, 0.0f, 0.0f, 0.0f);
    //
    _driverJolt.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_driverJolt.objMutex);
    return result;
}
int driverJoltSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_driverJolt.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _driverJolt.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_driverJolt.objMutex, -1);
    switch(sync){
        case driverJoltSyncStep:
            joltStep(_driverJolt.joltCtx, DRIVER_PHYSICS_BACKEND_JOLT_DELTA_TIME, DRIVER_PHYSICS_BACKEND_JOLT_COLLISTION_STEP);
            break;
        case driverJoltSyncGetBodyTransform:
            if(!arg1 || !arg2 || !arg3){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            joltGetBodyTransform(_driverJolt.joltCtx, (unsigned int)arg1, (float*)arg2, (float*)arg3);
            break;
        case driverJoltSyncCreateBody:{
            if(!arg1 || !arg2){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            driverJoltBody* bodyConfig = (driverJoltBody*)arg1;
            uint32_t bodyId = 0;
            switch(bodyConfig->bodyType){
                case driverJoltBodyTypeNone:
                    goto syncExit;
                case driverJoltBodyTypeSphere:
                    bodyId = joltCreateSphere(_driverJolt.joltCtx, bodyConfig->position[0], bodyConfig->position[1], bodyConfig->position[2], bodyConfig->radius, bodyConfig->isDynamic);
                    break;
                default: logError("Unknown body type: %d", bodyConfig->bodyType);
                    result = retFail;
                    goto syncExit;
            }
            if(bodyId <= 0){ logError("body create fail");
                result = retFail; goto syncExit;
            }
            *(uint32_t*)arg2 = bodyId;
            break;
        }
    }
syncExit:
    osalMutexUnlock(&_driverJolt.objMutex);
    return result;
}
#endif
