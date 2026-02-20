/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "service/serviceCommon.h"

#if APP_SERVICE_RENDERING == SERVICE_RENDERING_3D

static serviceRendering3d _serviceRendering3d = {
    .objState = objStateClosed,
};

int serviceRendering3dClose(void){
    int result = retOk;
    if(_serviceRendering3d.objState >= objStateOpening){
        osalMutexLock(&_serviceRendering3d.objMutex, -1);
        _serviceRendering3d.objState = objStateClosing;
        //
        _serviceRendering3d.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_serviceRendering3d.objMutex);
    }
    return result;
}
int serviceRendering3dOpen(void){
    int result = retOk;
    osalMutexOpen(&_serviceRendering3d.objMutex);
    osalMutexLock(&_serviceRendering3d.objMutex, -1);
    _serviceRendering3d.objState = objStateOpening;
    //
    _serviceRendering3d.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_serviceRendering3d.objMutex);
    return result;
}
int serviceRendering3dSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_serviceRendering3d.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _serviceRendering3d.objState); return retFail; }
    int result = retOk;
    osalMutexLock(&_serviceRendering3d.objMutex, -1);
    switch(sync){
        //
    }
syncExit:
    osalMutexUnlock(&_serviceRendering3d.objMutex);
    return result;
}
#endif
