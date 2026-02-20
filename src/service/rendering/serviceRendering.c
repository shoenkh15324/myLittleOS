/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-19
 ******************************************************************************/
#include "service/serviceCommon.h"
#if APP_SERVICE_RENDERING == SERVICE_RENDERING_ENABLE
#include "core/physics/world/world2D.h"
#include "core/physics/shape/circle2D.h"
#include "core/physics/shape/aabb2D.h"
#include <math.h>

static serviceRendering _serviceRendering = {
    .objState = objStateClosed,
};

static int _serviceRenderingRequestPrimitiveDrawing(gfxPrimitiveType type, const float* vertex, int vertexCount){
#if APP_DRIVER_GFX == DRIVER_GFX_OPENGL
    driverOpenglSync(driverOpenglSyncDrawPrimitive, type, (uintptr_t)vertex, vertexCount, 0);
#endif
    return retOk;
}
static void _serviceRenderingDrawCircle(const drawCircle2D* diagram){
    int segments = 36, vIdx = 0;
    float vertex[360 * 2];
    float cx = diagram->x, cy = diagram->y, r = diagram->radius;
    for (int i = 0; i < segments; i++) {
        float t1 = 2.0f * 3.14159f * (float)i / (float)segments;
        float t2 = 2.0f * 3.14159f * (float)(i + 1) / (float)segments;
        // 삼각형 1개 정의 (중심, 점1, 점2)
        vertex[vIdx++] = cx; vertex[vIdx++] = cy;
        vertex[vIdx++] = cx + cosf(t1) * r; vertex[vIdx++] = cy + sinf(t1) * r;
        vertex[vIdx++] = cx + cosf(t2) * r; vertex[vIdx++] = cy + sinf(t2) * r;
    }
    if(_serviceRenderingRequestPrimitiveDrawing(gfxPrimitiveTypeTriangles, (const float*)vertex, (vIdx / 2))){ logError("_serviceRenderingRequestPrimitiveDrawing fail"); }
}
static void _serviceRenderingDrawRect(const drawRect2D* diagram) {
    float w = diagram->width, h = diagram->height, x = diagram->x - (w * 0.5f), y = diagram->y - (h * 0.5f);
    float vertex[] = { 
        x, y, x + w, y + h, x, y + h, // 삼각형 1
        x, y, x + w, y, x + w, y + h  // 삼각형 2
    };
    if(_serviceRenderingRequestPrimitiveDrawing(gfxPrimitiveTypeTriangles, (const float*)vertex, 6)){ logError("_serviceRenderingRequestPrimitiveDrawing fail"); }
}
static void _serviceRenderingDrawLine(const drawLine2D* diagram) {
    float vertex[] = { diagram->x1, diagram->y1, diagram->x2, diagram->y2 };
    if(_serviceRenderingRequestPrimitiveDrawing(gfxPrimitiveTypeLines, (const float*)vertex, 2)){ logError("_serviceRenderingRequestPrimitiveDrawing fail"); }
}
static int _serviceRenderingDrawDiagram(void){
    renderCmd cmd;
    while(bufferPop(_serviceRendering.pReadBuf, (uint8_t*)&cmd, sizeof(cmd))){ 
        driverOpenglSync(driverOpenglSyncSetColor, (cmd.color >> 16) & 0xFF, (cmd.color >> 8) & 0xFF, cmd.color & 0xFF, 0);
        switch(cmd.type){
            case renderCmdTypeCircle:
                _serviceRenderingDrawCircle((const drawCircle2D*)&cmd.diagram);
                break;
            case renderCmdTypeRect:
                _serviceRenderingDrawRect((const drawRect2D*)&cmd.diagram);
                break;
            case renderCmdTypeLine:
                _serviceRenderingDrawLine((const drawLine2D*)&cmd.diagram);
                break;
        }
    }
    return retOk;
}
static int _serviceRenderingBeginFrame(void){
#if APP_DRIVER_GFX == DRIVER_GFX_OPENGL
    if(driverOpenglSync(driverOpenglSyncBeginFrame, 0, 0, 0, 0)){ logError("driverOpenglSyncBeginFrame fail"); return retFail; }
#endif
    return retOk;
}
static int _serviceRenderingEndFrame(void){
#if APP_DRIVER_GFX == DRIVER_GFX_OPENGL
    if(driverOpenglSync(driverOpenglSyncEndFrame, 0, 0, 0, 0)){ logError("driverOpenglSyncEndFrame fail"); return retFail; }
#endif
    return retOk;
}
static int _serviceRenderingDrawFrame(void){
    if(_serviceRenderingBeginFrame()){ logError("_serviceRenderingBeginFrame fail");
        return retFail;
    }
    if(_serviceRenderingDrawDiagram()){ logError("_serviceRenderingDrawDiagram fail");
        return retFail;
    }
    if(_serviceRenderingEndFrame()){ logError("_serviceRenderingEndFrame fail");
        return retFail;
    }
    return retOk;
}
static void _serviceRenderingSwapBuffer(void){
    ringBuffer* temp = _serviceRendering.pWriteBuf;
    _serviceRendering.pWriteBuf = _serviceRendering.pReadBuf;
    _serviceRendering.pReadBuf = temp;
    bufferClear(_serviceRendering.pWriteBuf);
}
static int _serviceRenderingPushRenderCmd(renderCmd* cmd, size_t size){
    if(!cmd){ logError("Invalid Params"); return retFail; }
    if(bufferPush(_serviceRendering.pWriteBuf, (uint8_t*)cmd, size)){ logError("bufferPush fail");
        return retFail;
    }
    return retOk;
}
static int _serviceRenderingSubmitWorld(world2d* world){
    if(!world || !world->bodies){ logError("Invalid Params"); return retFail; }
    for(size_t i = 0; i < world->bodyCount; i++){
        body2d* body = world->bodies[i];
        if(!body || !body->shape) continue;
        shape2d* shape = body->shape;
        renderCmd cmd = {0};
        cmd.color = body->color;
        cmd.layer = 0;
        switch(body->shape->type){
            case shape2dTypeCircle:{
                circle2d* circle = (circle2d*)body->shape;
                cmd.type = renderCmdTypeCircle;
                cmd.diagram.circle.x = body->position.x;
                cmd.diagram.circle.y = body->position.y;
                cmd.diagram.circle.radius = circle->radius;
                break;
            }
            case shape2dTypeAABB:{
                aabb2d* aabb = (aabb2d*)body->shape;
                cmd.type = renderCmdTypeRect;
                cmd.diagram.rect.x = body->position.x - (aabb->width / 2.0f);
                cmd.diagram.rect.y = body->position.y - (aabb->height / 2.0f);
                cmd.diagram.rect.width = aabb->width;
                cmd.diagram.rect.height = aabb->height;
                cmd.diagram.rect.rotation = 0.0f;
                break;
            }
        }
        if(_serviceRenderingPushRenderCmd(&cmd, sizeof(cmd))){ logError("_serviceRenderingPushRenderCmd fail");
            return retFail;
        }
    }
    return retOk;
}
int serviceRenderingClose(void){
    int result = retOk;
    if(_serviceRendering.objState >= objStateOpening){
        osalMutexLock(&_serviceRendering.objMutex, -1);
        _serviceRendering.objState = objStateClosing;
        //
        for(int i = 0; i < 2; i++){
            if(bufferClose(&_serviceRendering.renderQueue[i])){ logError("bufferClose fail");
                result = retFail; goto closeExit;
            }
        }
        _serviceRendering.pWriteBuf = _serviceRendering.pReadBuf = NULL;
        //
        _serviceRendering.objState = objStateClosed;
closeExit:
        osalMutexUnlock(&_serviceRendering.objMutex);
    }
    return result;
}
int serviceRenderingOpen(void){
    int result = retOk;
    osalMutexOpen(&_serviceRendering.objMutex);
    osalMutexLock(&_serviceRendering.objMutex, -1);
    _serviceRendering.objState = objStateOpening;
    //
    for(int i = 0; i < 2; i++){
        if(bufferOpen(&_serviceRendering.renderQueue[i], APP_SERVICE_RENDERING_RENDER_QUEUE_SIZE)){ logError("bufferOpen fail");
            result = retFail; goto openExit;
        }
    }
    _serviceRendering.pWriteBuf = &_serviceRendering.renderQueue[0];
    _serviceRendering.pReadBuf = &_serviceRendering.renderQueue[1];
    //
    _serviceRendering.objState = objStateOpened;
openExit:
    osalMutexUnlock(&_serviceRendering.objMutex);
    return result;
}
int serviceRenderingSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    if(_serviceRendering.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _serviceRendering.objState); return retFail; }
    int result = retOk;
    switch(sync){
        case serviceRenderingSyncDrawFrame: //logDebug("serviceRenderingSyncDrawFrame");
            if(_serviceRenderingDrawFrame()){ logError("_serviceRenderingDrawFrame fail");
                return retFail;
            }
            return retOk;
    }
    osalMutexLock(&_serviceRendering.objMutex, -1);
    switch(sync){
        case serviceRenderingSyncTimer:
            break;
        case serviceRenderingSyncInit:
            if(driverOpenglSync(driverOpenglSyncInit, 0, 0, 0, 0)){ logError("driverOpenglSyncInit fail");
                result = retFail; goto syncExit;
            }
            if(driverOpenglSync(driverOpenglSyncSetClearColor, 0, 0, 0, 0)){ logError("driverOpenglSyncSetClearColor fail");
                result = retFail; goto syncExit;
            }
            break;
        case serviceRenderingSyncDeinit:
            if(driverOpenglSync(driverOpenglSyncDeinit, 0, 0, 0, 0)){ logError("driverOpenglSyncDeinit fail");
                result = retFail; goto syncExit;
            }
            break;
        case serviceRenderingSyncSubmitWorld:
            if(!arg1){ logError("Invalid Params");
                result = retFail; goto syncExit;
            }
            if(_serviceRenderingSubmitWorld((world2d*)arg1)){ logError("_serviceRenderingSubmitWorld fail");
                result = retFail; goto syncExit;
            }
            break;
        case serviceRenderingSyncSwapBuffer:
            _serviceRenderingSwapBuffer();
            break;
    }
syncExit:
    osalMutexUnlock(&_serviceRendering.objMutex);
    return result;
}
#endif
