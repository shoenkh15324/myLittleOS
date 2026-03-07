/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "service/serviceCommon.h"

#if APP_SERVICE_RENDERING == SERVICE_RENDERING_3D
#include <math.h>

#define PI 3.1415926535f
#define TO_RAD(deg) (deg * (PI / 180.0f))

static serviceRendering3d _serviceRendering3d = {
    .objState = objStateClosed,
};

static void _serviceRendering3dUpdateCamera(void){
    if(_serviceRendering3d.camera.accumulatedMouseDx || _serviceRendering3d.camera.accumulatedMouseDy){
        _serviceRendering3d.camera.yaw += _serviceRendering3d.camera.accumulatedMouseDx * APP_SERVICE_RENDERING_MOUSE_MOVE_SENSITIVITY * DRIVER_PHYSICS_BACKEND_JOLT_DELTA_TIME;
        _serviceRendering3d.camera.pitch += _serviceRendering3d.camera.accumulatedMouseDy * APP_SERVICE_RENDERING_MOUSE_MOVE_SENSITIVITY * DRIVER_PHYSICS_BACKEND_JOLT_DELTA_TIME;
        if(_serviceRendering3d.camera.pitch > 89.0f) _serviceRendering3d.camera.pitch = 89.0f;
        if(_serviceRendering3d.camera.pitch < -89.0f) _serviceRendering3d.camera.pitch = -89.0f;
        float p = TO_RAD(_serviceRendering3d.camera.pitch);
        float y = TO_RAD(_serviceRendering3d.camera.yaw);
        _serviceRendering3d.camera.front[0] = cosf(y) * cosf(p);
        _serviceRendering3d.camera.front[1] = sinf(p);
        _serviceRendering3d.camera.front[2] = sinf(y) * cosf(p);
        _serviceRendering3d.camera.accumulatedMouseDx = 0.0f;
        _serviceRendering3d.camera.accumulatedMouseDy = 0.0f;
    }
    if(_serviceRendering3d.camera.accumulatedMouseWheel != 0.0f){
        float wheelDelta = _serviceRendering3d.camera.accumulatedMouseWheel;
        _serviceRendering3d.camera.fov -= wheelDelta * APP_SERVICE_RENDERING_MOUSE_WHEEL_SENSITIVITY;
        if(_serviceRendering3d.camera.fov < 1.0f) _serviceRendering3d.camera.fov = 1.0f;
        if(_serviceRendering3d.camera.fov > 120.0f) _serviceRendering3d.camera.fov = 120.0f;
        _serviceRendering3d.camera.accumulatedMouseWheel = 0.0f;
    }
}
static void _serviceRendering3dSubmitRenderItem(serviceRendering3dRenderType type) {
    for(uint32_t i = 0; i < _serviceRendering3d.entitiyCount; i++) {
        if(_serviceRendering3d.entities[i].type == type) {
            driverBgfxSync(driverBgfxSyncSubmitItem, (uintptr_t)&_serviceRendering3d.entities[i].item, 0, 0, 0);
        }
    }
}
static int _serviceRendering3dSetEntityConfig(serviceRendering3dRenderType type, driverBgfxMaterial* material, driverJoltBody* body, driverBgfxRenderItem* item){
    if(!material || !body || !item){ logError("Invalid Params"); return retFail; }
    switch(type){
#if APP_BLACKHOLE_SIMULATION
        case serviceRendering3dRenderTypeStarField:
            break;
        case serviceRendering3dRenderTypeBlackhole:
            break;
#endif
        default: logError("Unknown Type");
            return retFail;
    }
    return retOk;
}
static int _serviceRendering3dApplyTransform(serviceRendering3dEntity* entity, float* pos, float* rot, driverJoltBody* body){
    if(!entity || !pos || !rot ||!body){ logError("Invalid Params"); return retFail; }
    if(pos){
        memcpy(body->position, pos, sizeof(float) * 3);
        memcpy(entity->item.transform.pos, pos, sizeof(float) * 3);
    }
    if(rot){
        memcpy(body->rotation, rot, sizeof(float) * 4);
        memcpy(entity->item.transform.rot, rot, sizeof(float) * 4);
    }
    return retOk;
}
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
    // camera setting
    _serviceRendering3d.camera.pos[0] = 0.0f;
    _serviceRendering3d.camera.pos[1] = 10.0f;
    _serviceRendering3d.camera.pos[2] = 30.0f;
    _serviceRendering3d.camera.up[0] = 0.0f;
    _serviceRendering3d.camera.up[1] = 1.0f;
    _serviceRendering3d.camera.up[2] = 0.0f;
    _serviceRendering3d.camera.yaw = -90.0f;
    _serviceRendering3d.camera.pitch = -15.0f;
    _serviceRendering3d.camera.fov = 60.0f;
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
            serviceRendering3dSync(serviceRendering3dSyncUpdatePhysics, 0, 0, 0, 0);
            _serviceRendering3dUpdateCamera();
            driverBgfxSync(driverBgfxSyncBeginFrame, (uintptr_t)&_serviceRendering3d.camera, 0, 0, 0);
            // 백그라운드 패스
            driverBgfxSync(driverBgfxSyncSetPass, driverBgfxPassTypeBackground, 0, 0, 0);
            _serviceRendering3dSubmitRenderItem(serviceRendering3dRenderTypeStarField);
            // 오브젝트 패스
            driverBgfxSync(driverBgfxSyncSetPass, driverBgfxPassTypeObject, 0, 0, 0);
            _serviceRendering3dSubmitRenderItem(serviceRendering3dRenderTypeBlackhole);
            driverBgfxSync(driverBgfxSyncEndFrame, 0, 0, 0, 0);
            break;
        }
        case serviceRendering3dSyncCreateEntity:{
            if(!arg2 || !arg3){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            if(_serviceRendering3d.entitiyCount >= DRIVER_PHYSICS_BACKEND_JOLT_MAX_BODIES){ logError("Max entity count reached");
                result = retFail; goto syncExit;
            }
            serviceRendering3dEntity* entity = &_serviceRendering3d.entities[_serviceRendering3d.entitiyCount];
            memset(&entity->item, 0, sizeof(entity->item));
            driverBgfxMaterial material = {0}; driverJoltBody body = {0};
            _serviceRendering3dSetEntityConfig((serviceRendering3dRenderType)arg1, &material, &body, &entity->item);
            _serviceRendering3dApplyTransform(&entity->item, (float*)arg2, (float*)arg3, &body);
            if(driverBgfxSync(driverBgfxSyncCreateMesh, (uintptr_t)&entity->item, 0, 0, 0)){ logError("driverBgfxSyncCreateMesh failed");
                result = retFail; goto syncExit;
            }
            if(driverJoltSync(driverJoltSyncCreateBody, (uintptr_t)&body, (uintptr_t)&entity->joltBodyId, 0, 0)){ logError("driverJoltSyncCreateBody failed");
                result = retFail; goto syncExit;
            }
            _serviceRendering3d.entitiyCount++;
            break;
        }
        case serviceRendering3dSyncUpdateCamera:{
            _serviceRendering3d.camera.accumulatedMouseDx += (float)(int16_t)arg1;
            _serviceRendering3d.camera.accumulatedMouseDy += (float)(int16_t)arg2;
            break;
        }
        case serviceRendering3dSyncUpdateZoom:{
            _serviceRendering3d.camera.accumulatedMouseWheel += (float)(int16_t)(uint16_t)arg1;
            break;
        }
        case serviceRendering3dSyncUpdatePhysics:
            driverJoltSync(driverJoltSyncStep, 0, 0, 0, 0);
            for(uint32_t i = 0; i < _serviceRendering3d.entitiyCount; i++){
                serviceRendering3dEntity* ent = &_serviceRendering3d.entities[i];
                if(ent->joltBodyId > 0){
                    driverJoltSync(driverJoltSyncGetBodyTransform, (uintptr_t)ent->joltBodyId, (uintptr_t)ent->item.transform.pos, (uintptr_t)ent->item.transform.rot, 0);
                }
            }
            break;
    }
syncExit:
    osalMutexUnlock(&_serviceRendering3d.objMutex);
    return result;
}
#endif
