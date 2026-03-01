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
        case serviceRendering3dSyncDrawFrame:{
            driverJoltSync(driverJoltSyncStep, 0, 0, 0, 0);
            for(uint32_t i = 0; i < _serviceRendering3d.entitiyCount; i++){
                serviceRendering3dEntity* ent = &_serviceRendering3d.entities[i];
                if(ent->joltBodyId > 0){
                    driverJoltSync(driverJoltSyncGetBodyTransform, (uintptr_t)ent->joltBodyId, (uintptr_t)ent->renderInfo.trans.pos, (uintptr_t)ent->renderInfo.trans.rot, 0);
                }
            }
            driverBgfxSync(driverBgfxSyncRenderFrame, (uintptr_t)_serviceRendering3d.entities, (uintptr_t)_serviceRendering3d.entitiyCount, sizeof(serviceRendering3dEntity), offsetof(serviceRendering3dEntity, renderInfo));
            break;
        }
        case serviceRendering3dSyncCreateEntity:{
            if(!arg2 || !arg3){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            if(_serviceRendering3d.entitiyCount >= DRIVER_PHYSICS_BACKEND_JOLT_MAX_BODIES){ logError("Max entity count reached");
                result = retFail; goto syncExit;
            }
            serviceRendering3dEntity* ent = &_serviceRendering3d.entities[_serviceRendering3d.entitiyCount];
            serviceRendering3dRenderType renderType = (serviceRendering3dRenderType)arg1;
            float* startPos = (float*)arg2;
            float* startRot = (float*)arg3;
            driverBgfxMeshConfig meshConfig = {0};
            driverJoltBodyConfig bodyConfig = {0};
            switch(renderType){
#if APP_BLACKHOLE_SIMULATION
                case serviceRendering3dRenderTypeBlackhole:
                    meshConfig.meshType = driverBgfxMeshTypeSphere;
                    meshConfig.abgr = 0xFF00FF00;
                    meshConfig.segment = 5,
                    meshConfig.sphere.radius = 1.0f;
                    ent->renderInfo.trans.scale = 1.0f;
                    bodyConfig.bodyType = driverJoltBodyTypeSphere;
                    bodyConfig.radius = 1.0f;
                    bodyConfig.isDynamic = false;
                    break;
                case serviceRendering3dRenderTypeAccretionDisk:
                    meshConfig.meshType = driverBgfxMeshTypeDisk;
                    meshConfig.abgr = 0xFF0000FF;
                    meshConfig.segment = 64,
                    meshConfig.sphere.radius = 5.0f;
                    ent->renderInfo.trans.scale = 3.0f;
                    bodyConfig.bodyType = driverJoltBodyTypeDisk;
                    bodyConfig.radius = 5.0f;
                    bodyConfig.isDynamic = false;
                    break;
                case serviceRendering3dRenderTypeBackground:
                    break;
#endif
            }
            driverBgfxSync(driverBgfxSyncCreateMesh, (uintptr_t)&meshConfig, (uintptr_t)&ent->renderInfo, 0, 0);
            if(startPos){
                bodyConfig.position[0] = startPos[0];
                bodyConfig.position[1] = startPos[1];
                bodyConfig.position[2] = startPos[2];
                ent->renderInfo.trans.pos[0] = startPos[0];
                ent->renderInfo.trans.pos[1] = startPos[1];
                ent->renderInfo.trans.pos[2] = startPos[2];
            }
            if(startRot){
                bodyConfig.rotation[0] = startRot[0];
                bodyConfig.rotation[1] = startRot[1];
                bodyConfig.rotation[2] = startRot[2];
                bodyConfig.rotation[3] = startRot[3];
                ent->renderInfo.trans.rot[0] = startRot[0];
                ent->renderInfo.trans.rot[1] = startRot[1];
                ent->renderInfo.trans.rot[2] = startRot[2];
                ent->renderInfo.trans.rot[3] = startRot[3];
            }
            if(driverJoltSync(driverJoltSyncCreateBody, (uintptr_t)&bodyConfig, (uintptr_t)&ent->joltBodyId, 0, 0)){ logError("Jolt body creation failed");
                result = retFail; goto syncExit;
            }
            _serviceRendering3d.entitiyCount++;
            logDebug("Entity Created - ID: %d, Pos: %.1f, %.1f, %.1f", ent->joltBodyId, ent->renderInfo.trans.pos[0], ent->renderInfo.trans.pos[1], ent->renderInfo.trans.pos[2]);
            break;
        }
    }
syncExit:
    osalMutexUnlock(&_serviceRendering3d.objMutex);
    return result;
}
#endif
