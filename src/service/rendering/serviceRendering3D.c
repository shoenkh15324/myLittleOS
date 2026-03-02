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
            driverJoltSync(driverJoltSyncStep, 0, 0, 0, 0);
            for(uint32_t i = 0; i < _serviceRendering3d.entitiyCount; i++){
                serviceRendering3dEntity* ent = &_serviceRendering3d.entities[i];
                if(ent->joltBodyId > 0){
                    driverJoltSync(driverJoltSyncGetBodyTransform, (uintptr_t)ent->joltBodyId, (uintptr_t)ent->renderInfo.trans.pos, (uintptr_t)ent->renderInfo.trans.rot, 0);
                }
            }
            _serviceRendering3dUpdateCamera();
            driverBgfxSceneContext scene = {
                .pItems = (uint8_t*)_serviceRendering3d.entities,
                .itemCount = _serviceRendering3d.entitiyCount,
                .itemStride = sizeof(serviceRendering3dEntity),
                .itemOffset = offsetof(serviceRendering3dEntity, renderInfo),
                .camPos[0] = _serviceRendering3d.camera.pos[0],
                .camPos[1] = _serviceRendering3d.camera.pos[1],
                .camPos[2] = _serviceRendering3d.camera.pos[2],
                .camFront[0] = _serviceRendering3d.camera.front[0],
                .camFront[1] = _serviceRendering3d.camera.front[1],
                .camFront[2] = _serviceRendering3d.camera.front[2],
                .fov = _serviceRendering3d.camera.fov,
            };
            driverBgfxSync(driverBgfxSyncRenderFrame, (uintptr_t)&scene, 0, 0, 0);
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
            memset(&ent->renderInfo, 0, sizeof(ent->renderInfo));
            ent->renderInfo.trans.scale = 1.0f; // 매우 중요!
            serviceRendering3dRenderType renderType = (serviceRendering3dRenderType)arg1;
            float* startPos = (float*)arg2;
            float* startRot = (float*)arg3;
            driverBgfxMeshConfig meshConfig = {0};
            driverJoltBodyConfig bodyConfig = {0};
            switch(renderType){
#if APP_BLACKHOLE_SIMULATION
                case serviceRendering3dRenderTypeBlackhole:
                    meshConfig.meshType = driverBgfxMeshTypeSphere;
                    meshConfig.segment = 5;
                    meshConfig.sphere.radius = bodyConfig.radius = 5.0f;
                    ent->renderInfo.trans.scale = 2.0f;
                    ent->renderInfo.material.baseColor[0] = 0.0f; // Black
                    ent->renderInfo.material.baseColor[1] = 0.0f;
                    ent->renderInfo.material.baseColor[2] = 0.0f;
                    ent->renderInfo.material.baseColor[3] = 1.0f;
                    ent->renderInfo.material.emission = 0.0f; // 빛 흡수
                    ent->renderInfo.material.opacity = 1.0f;
                    ent->renderInfo.material.depthWrite = true; // 제일 안쪽이라 뎁스 기록
                    bodyConfig.bodyType = driverJoltBodyTypeSphere;
                    break;
                case serviceRendering3dRenderTypeAccretionDisk:
                    meshConfig.meshType = driverBgfxMeshTypeDisk;
                    meshConfig.segment = 64;
                    meshConfig.disk.radius = bodyConfig.radius = 10.0f;
                    ent->renderInfo.trans.scale = 1.0f;
                    ent->renderInfo.material.baseColor[0] = 1.0f; // 강렬한 오렌지/백색
                    ent->renderInfo.material.baseColor[1] = 0.6f;
                    ent->renderInfo.material.baseColor[2] = 0.2f;
                    ent->renderInfo.material.baseColor[3] = 1.0f;
                    ent->renderInfo.material.emission = 10.0f; // 엄청나게 밝음
                    ent->renderInfo.material.opacity = 0.8f; // 약간의 투명감
                    ent->renderInfo.material.metallic = 0.5f; // 가스 밀도로 재해석 가능
                    ent->renderInfo.material.depthWrite = false; // 반투명 객체는 보통 false
                    bodyConfig.bodyType = driverJoltBodyTypeDisk;
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
        case serviceRendering3dSyncUpdateCamera:{
            _serviceRendering3d.camera.accumulatedMouseDx += (float)(int16_t)arg1;
            _serviceRendering3d.camera.accumulatedMouseDy += (float)(int16_t)arg2;
            break;
        }
        case serviceRendering3dSyncUpdateZoom:{
            _serviceRendering3d.camera.accumulatedMouseWheel += (float)(int16_t)(uint16_t)arg1;
            break;
        }
    }
syncExit:
    osalMutexUnlock(&_serviceRendering3d.objMutex);
    return result;
}
#endif
