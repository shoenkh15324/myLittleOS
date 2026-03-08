/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "service/serviceCommon.h"

#if (APP_SERVICE_RENDERING == SERVICE_RENDERING_3D) && APP_DRIVER_GFX
#include <math.h>

#define PI 3.1415926535f
#define TO_RAD(deg) (deg * (PI / 180.0f))

static serviceRendering3d _serviceRendering3d = {
    .objState = objStateClosed,
};

static void _serviceRendering3dUpdateCamera(serviceRendering3dCamera* cam){
    //logDebug("MouseDx: %.2f, MouseDy: %.2f, Wheel: %.2f", cam->accumulatedMouseDx, cam->accumulatedMouseDy, cam->accumulatedMouseWheel);
    if(cam->accumulatedMouseDx || cam->accumulatedMouseDy){
        cam->yaw += cam->accumulatedMouseDx * APP_SERVICE_RENDERING_MOUSE_MOVE_SENSITIVITY * DRIVER_PHYSICS_BACKEND_JOLT_DELTA_TIME;
        cam->pitch += cam->accumulatedMouseDy * APP_SERVICE_RENDERING_MOUSE_MOVE_SENSITIVITY * DRIVER_PHYSICS_BACKEND_JOLT_DELTA_TIME;
        if(cam->pitch > 89.0f) cam->pitch = 89.0f;
        if(cam->pitch < -89.0f) cam->pitch = -89.0f;
    }
    if(cam->accumulatedMouseWheel != 0.0f){
        float wheelDelta = cam->accumulatedMouseWheel;
        cam->fov -= wheelDelta * APP_SERVICE_RENDERING_MOUSE_WHEEL_SENSITIVITY;
        if(cam->fov < 1.0f) cam->fov = 1.0f;
        if(cam->fov > 120.0f) cam->fov = 120.0f;
        cam->accumulatedMouseWheel = 0.0f;
    }
    float p = TO_RAD(cam->pitch), y = TO_RAD(cam->yaw);
    cam->front[0] = cosf(y) * cosf(p); cam->front[1] = sinf(p); cam->front[2] = sinf(y) * cosf(p);
    bgfxMathVec3Normalize((bgfxVec3*)cam->front);
    float target[3] = { cam->pos[0] + cam->front[0], (cam->pos[1] + cam->front[1]), (cam->pos[2] + cam->front[2]) };
    bgfxMatViewLookat((bgfxMat4*)cam->viewMtx, *(bgfxVec3*)cam->pos, *(bgfxVec3*)target, *(bgfxVec3*)cam->up);
    bgfxMathProjPerspective((bgfxMat4*)cam->projMtx, cam->fov, ((float)_serviceRendering3d.width / (float)_serviceRendering3d.height), 0.1f, 1000.0f);
    cam->accumulatedMouseDx = 0.0f;
    cam->accumulatedMouseDy = 0.0f;
}
static void _serviceRendering3dSubmitRenderItem(serviceRendering3dScene* scene, serviceRendering3dRenderType type) {
    for(uint32_t i = 0; i < scene->entityCount; i++) {
        if(scene->entities[i].type == type) {
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
            driverBgfxSync(driverBgfxSyncSubmitItem, (uintptr_t)&scene->entities[i].item, 0, 0, 0);
#endif
        }
    }
}
static int _serviceRendering3dSetEntityMaterial(serviceRendering3dRenderType type, void* item, float* startPos, float* startRot){
    if(!item || !startPos || !startRot){ logError("Invalid Params"); return retFail; }
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
    driverBgfxRenderItem* pItem = (driverBgfxRenderItem*)item; 
#endif
    switch(type){
#if APP_BLACKHOLE_SIMULATION
        case serviceRendering3dRenderTypeStarField:
            pItem->mesh.meshType = driverBgfxMeshTypeSphere;
            memcpy(pItem->transform.pos, startPos, sizeof(float) * 3);
            memcpy(pItem->transform.rot, startRot, sizeof(float) * 4);
            pItem->transform.scale = 100.0f;
            pItem->material.shaderType = driverBgfxShaderTypeDefault;
            pItem->material.shaderParams.param1 = 0.0f;
            break;
        case serviceRendering3dRenderTypeBlackhole:
            pItem->mesh.meshType = driverBgfxMeshTypeSphere;
            memcpy(pItem->transform.pos, startPos, sizeof(float) * 3);
            memcpy(pItem->transform.rot, startRot, sizeof(float) * 4);
            pItem->transform.scale = 1.0f;
            pItem->material.shaderType = driverBgfxShaderTypeDefault;
            pItem->material.shaderParams.param1 = 1.0f;
            pItem->material.shaderParams.param2 = 100.0f; // mass(?)
            break;
#endif
        default: logError("Unknown Type");
            return retFail;
    }
    return retOk;
}
static int _serviceRendering3dSetEntityBody(serviceRendering3dRenderType type, void* body, float* startPos, float* startRot){
#if APP_DRIVER_PHYSICS_BACKEND
        if(!body || !startPos || !startRot){ logError("Invalid Params"); return retFail; }
    #if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
        driverJoltBody* pBody = (driverJoltBody*)body;
    #endif
        switch(type){
    #if APP_BLACKHOLE_SIMULATION
            case serviceRendering3dRenderTypeStarField:
                pBody->bodyType = driverJoltBodyTypeNone;
                break;
            case serviceRendering3dRenderTypeBlackhole:
                pBody->bodyType = driverJoltBodyTypeSphere;
                pBody->radius = 5.0f;
                pBody->mass = 100.0f;
                memcpy(pBody->position, startPos, 3);
                memcpy(pBody->rotation, startRot, 4);
                pBody->isDynamic = true;
                break;
    #endif
            default: logError("Unknown Type");
                return retFail;
        }
#endif
    return retOk;
}
static int _serviceRendering3dApplyBodyTransform(serviceRendering3dEntity* entity, float* pos, float* rot, void* body){
#if APP_DRIVER_PHYSICS_BACKEND
        if(!entity || !pos || !rot || !body){ logError("Invalid Params"); return retFail; }
    #if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
        driverJoltBody* pBody = (driverJoltBody*)body;
    #endif
        if(pos){
            memcpy(pBody->position, pos, sizeof(float) * 3);
            memcpy(entity->item.transform.pos, pos, sizeof(float) * 3);
        }
        if(rot){
            memcpy(pBody->rotation, rot, sizeof(float) * 4);
            memcpy(entity->item.transform.rot, rot, sizeof(float) * 4);
        }
#endif
    return retOk;
}
static int _serviceRendering3dSetBackgroundPass(serviceRendering3dScene* scene){
    if(!scene){ logError("Invalid Params"); return retFail; }
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
    driverBgfxCameraState bgCam = {0};
    memset(bgCam.camPos, 0, sizeof(float) * 3);
    //memcpy(bgCam.camPos, scene->camera.pos, sizeof(float) * 3);
    memcpy(&bgCam.viewMtx, scene->camera.viewMtx, sizeof(float) * 16);
    bgCam.viewMtx[12] = 0.0f; bgCam.viewMtx[13] = 0.0f; bgCam.viewMtx[14] = 0.0f; 
    memcpy(&bgCam.projMtx, scene->camera.projMtx, sizeof(float) * 16); 
    //logDebug("BGFX BackgroundPass Camera Pos: x=%.2f y=%.2f z=%.2f", bgCam.camPos[0], bgCam.camPos[1], bgCam.camPos[2]);
    //logDebug("BGFX BackgroundPass ViewMtx[0]: %.3f %.3f %.3f %.3f", bgCam.viewMtx[0], bgCam.viewMtx[1], bgCam.viewMtx[2], bgCam.viewMtx[3]);
    driverBgfxSync(driverBgfxSyncSetPass, driverBgfxPassTypeBackground, 0, 0, 0);
    driverBgfxSync(driverBgfxSyncSetCameraState, (uintptr_t)&bgCam, 0, 0, 0);
#endif
    _serviceRendering3dSubmitRenderItem(scene, serviceRendering3dRenderTypeStarField);
    return retOk;
}
static int _serviceRendering3dSetObjectPass(serviceRendering3dScene* scene){
    if(!scene){ logError("Invalid Params"); return retFail; }
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
    driverBgfxCameraState objCam = {0};
    memcpy(objCam.projMtx, scene->camera.projMtx, sizeof(float) * 16);
    memcpy(objCam.viewMtx, scene->camera.viewMtx, sizeof(float) * 16);
    memcpy(objCam.camPos, scene->camera.pos, sizeof(float) * 3);
    objCam.camPos[3] = 1.0f;
    //logDebug("BGFX ObjectPass Camera Pos: x=%.2f y=%.2f z=%.2f", objCam.camPos[0], objCam.camPos[1], objCam.camPos[2]);
    //logDebug("BGFX ObjectPass ViewMtx[0]: %.3f %.3f %.3f %.3f", objCam.viewMtx[0], objCam.viewMtx[1], objCam.viewMtx[2], objCam.viewMtx[3]);
    driverBgfxSync(driverBgfxSyncSetPass, driverBgfxPassTypeObject, 0, 0, 0);
    driverBgfxSync(driverBgfxSyncSetCameraState, (uintptr_t)&objCam, 0, 0, 0);
#endif
    _serviceRendering3dSubmitRenderItem(scene, serviceRendering3dRenderTypeBlackhole);
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
    _serviceRendering3d.width = APP_WINDOW_WIDTH;
    _serviceRendering3d.height = APP_WINDOW_HEIGHT;
    float cameraStartPos[3] = {0.0f, 10.0f, 30.0f};
    float cameraStartUp[3] = {0.0f, 1.0f, 0.0f};
    memcpy(_serviceRendering3d.activeScene.camera.pos, cameraStartPos, sizeof(float) * 3);
    memcpy(_serviceRendering3d.activeScene.camera.up, cameraStartUp, sizeof(float) * 3);
    _serviceRendering3d.activeScene.camera.yaw = -90.0f;
    _serviceRendering3d.activeScene.camera.pitch = -15.0f;
    _serviceRendering3d.activeScene.camera.fov = 60.0f;
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
            serviceRendering3dScene* scene = &_serviceRendering3d.activeScene;
            serviceRendering3dSync(serviceRendering3dSyncUpdatePhysics, 0, 0, 0, 0);
            _serviceRendering3dUpdateCamera(&scene->camera);
            driverBgfxSync(driverBgfxSyncBeginFrame, 0, 0, 0, 0);
            // 백그라운드 패스
            _serviceRendering3dSetBackgroundPass(scene);
            // 오브젝트 패스
            _serviceRendering3dSetObjectPass(scene);
            driverBgfxSync(driverBgfxSyncEndFrame, 0, 0, 0, 0);
            break;
        }
        case serviceRendering3dSyncCreateEntity:{
            if(!arg2 || !arg3){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            serviceRendering3dScene* scene = &_serviceRendering3d.activeScene;
            if(scene->entityCount >= DRIVER_PHYSICS_BACKEND_JOLT_MAX_BODIES){ logError("Max entity count reached");
                result = retFail; goto syncExit;
            }
            serviceRendering3dEntity* entity = &scene->entities[scene->entityCount];
            memset(&entity->item, 0, sizeof(entity->item));
            _serviceRendering3dSetEntityMaterial((serviceRendering3dRenderType)arg1, &entity->item, (float*)arg2, (float*)arg3);
            if(driverBgfxSync(driverBgfxSyncCreateMesh, (uintptr_t)&entity->item, 0, 0, 0)){ logError("driverBgfxSyncCreateMesh failed");
                result = retFail; goto syncExit;
            }
#if APP_DRIVER_PHYSICS_BACKEND
    #if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
            driverJoltBody body = {0};
    #else
            int body = 0;
    #endif
            _serviceRendering3dSetEntityBody((serviceRendering3dRenderType)arg1, &body, (float*)arg2, (float*)arg3);
            _serviceRendering3dApplyBodyTransform(entity, (float*)arg2, (float*)arg3, &body);
            if(driverJoltSync(driverJoltSyncCreateBody, (uintptr_t)&body, (uintptr_t)&entity->joltBodyId, 0, 0)){ logError("driverJoltSyncCreateBody failed");
                result = retFail; goto syncExit;
            }
#endif
            entity->type = (serviceRendering3dRenderType)arg1;
            logDebug("Crate Entity / entityType: %d", (int)arg1);
            scene->entityCount++;
            break;
        }
        case serviceRendering3dSyncUpdateViewport:
            _serviceRendering3d.width = (uint16_t)arg1;
            _serviceRendering3d.height = (uint16_t)arg2;
#if APP_DRIVER_GFX == DRIVER_GFX_BGFX
            driverBgfxSync(driverBgfxSyncUpdateViewport, arg1, arg2, 0, 0);
#endif
            break;
        case serviceRendering3dSyncUpdateCamera:{
            serviceRendering3dScene* scene = &_serviceRendering3d.activeScene;
            scene->camera.accumulatedMouseDx += (float)(int16_t)arg1;
            scene->camera.accumulatedMouseDy += (float)(int16_t)arg2;
            break;
        }
        case serviceRendering3dSyncUpdateZoom:{
            serviceRendering3dScene* scene = &_serviceRendering3d.activeScene;
            scene->camera.accumulatedMouseWheel += (float)(int16_t)(uint16_t)arg1;
            break;
        }
        case serviceRendering3dSyncUpdatePhysics:{
            serviceRendering3dScene* scene = &_serviceRendering3d.activeScene;
#if APP_DRIVER_PHYSICS_BACKEND == DRIVER_PHYSICS_BACKEND_JOLT
            driverJoltSync(driverJoltSyncStep, 0, 0, 0, 0);
            for(uint32_t i = 0; i < scene->entityCount; i++){
                serviceRendering3dEntity* ent = &scene->entities[i];
                if(ent->joltBodyId > 0){
                    driverJoltSync(driverJoltSyncGetBodyTransform, (uintptr_t)ent->joltBodyId, (uintptr_t)ent->item.transform.pos, (uintptr_t)ent->item.transform.rot, 0);
                }
            }
#endif
            break;
        }
    }
syncExit:
    osalMutexUnlock(&_serviceRendering3d.objMutex);
    return result;
}
#endif
