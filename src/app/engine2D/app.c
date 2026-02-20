/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "appCfgSelector.h"
#include "app/appCommon.h"
#include "core/physics/vector/vector2D.h"

static void _appMainTimerHandler(void*);
static void _appMainEventHandler(void*, void*, void*);
static void _appRenderEventHandler(void*, void*, void*);

static appMain _appMain = {
    .actor.isMainThread = true,
    .actor.objState = objStateClosed, 
    .actor.eventQueueSize = APP_MAIN_THREAD_EVENT_QUEUE_SIZE,
    .actor.appThreadAttr.name = "main",
    .actor.appThreadAttr.priority = osalThreadPriorityNormal,
    .actor.appThreadAttr.statckSize = APP_MAIN_THREAD_STACK_SIZE,
    .actor.appThreadHandler = _appMainEventHandler,
    .actor.appTimerHandler = _appMainTimerHandler,
    .actor.appEventIdxStart = appMainEventStart,
    .actor.appEventIdxEnd = appMainEventEnd,
    .actor.payloadBufferSize = APP_MAIN_THREAD_PAYLOAD_BUFFER_SIZE,
};
static appRender _appRender = {
    .actor.objState = objStateClosed, 
    .actor.eventQueueSize = APP_RENDER_THREAD_EVENT_QUEUE_SIZE,
    .actor.appThreadAttr.name = "render",
    .actor.appThreadAttr.priority = osalThreadPriorityIdle,
    .actor.appThreadAttr.statckSize = APP_RENDER_THREAD_STACK_SIZE,
    .actor.appThreadHandler = _appRenderEventHandler,
    .actor.appEventIdxStart = appRenderEventStart,
    .actor.appEventIdxEnd = appRenderEventEnd,
    .actor.payloadBufferSize = APP_RENDER_THREAD_PAYLOAD_BUFFER_SIZE,
};
static void _appMainTimerHandler(void* arg){ //logDebug("_appMainTimerHandler");
    activeObject* actor = (activeObject*)arg;
    if(actor->isMainThread){
        if(asyncPush(asyncTypeAsync, appMainEventTimer, 0, 0, 0, 0)){ logError("asyncPush fail"); }
        _appMain.actor.appTimerCount += APP_TIMER_INTERVAL;
        if(_appMain.actor.appTimerCount >= (1000 / APP_SERVICE_RENDERING_FPS)){
            if(asyncPush(asyncTypeAsync, appMainEventUpdateFrame, 0, 0, 0, 0)){ logError("asyncPush fail"); }
        }
    }
}
static int _appMainUpdateFrame(void){
    world2dStep(_appMain.world, ((float)APP_SERVICE_RENDERING_FPS / 1000.0f));
    if(serviceRenderingSync(serviceRenderingSyncSubmitWorld, (intptr_t)_appMain.world, 0, 0, 0)){ logError("serviceRenderingSyncSubmitWorld fail");
        return retFail;
    }
    if(serviceRenderingSync(serviceRenderingSyncSwapBuffer, 0, 0, 0, 0)){ logError("serviceRenderingSyncSwapBuffer fail");
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appRenderDrawFrame, 0, 0, 0, 0)){ logError("appRenderDrawFrame fail");
        return retFail;
    }
    return retOk;
}
static void _appMainEventHandler(void* arg1, void* arg2, void* arg3){
    activeObject* actor = (activeObject*)arg1;
    asyncPacket* pAsync = (asyncPacket*)arg2;
    uint8_t* pPayload = (uint8_t*)arg3;
    osalMutexLock(&actor->objMutex, -1);
    switch(pAsync->eventId){
        case appMainEventTimer: //logDebug("appMainEventTimer");
            driverCommonSync(driverCommonSyncTimer, 0, 0, 0, 0);
            serviceCommonSync(serviceCommonSyncTimer, 0, 0, 0, 0);
            break;
        // Physics
        case appMainEventCreateWorld:{ 
            _appMain.world = world2dCreate(pAsync->arg1, pAsync->arg2);
            if(!_appMain.world){ logError("world2dCreate fail"); goto appMainEventHandlerExit; }
            // red circle
            circle2d* shapeCircleRed = circle2dCreate(50.0f, NULL);
            body2d* bodyCircleRed = body2dCreate((vector2d){150.0f, 200.0f}, 1.0f, (shape2d*)shapeCircleRed, 0, 0xFFFF0000);
            body2dApplyImpulse(bodyCircleRed, (vector2d){10.0f, 0.0f});
            // blue circle
            circle2d* shapeCircleBlue = circle2dCreate(30.0f, NULL);
            body2d* bodyCircleBlue = body2dCreate((vector2d){450.0f, 200.0f}, 1.0f, (shape2d*)shapeCircleBlue, 0, 0xFF0000FF);
            body2dApplyImpulse(bodyCircleBlue, (vector2d){-10.0f, 0.0f});
            if(world2dAddBody(_appMain.world, bodyCircleRed)){ logError("world2dAddBody fail"); goto appMainEventHandlerExit; }
            if(world2dAddBody(_appMain.world, bodyCircleBlue)){ logError("world2dAddBody fail"); goto appMainEventHandlerExit; }
            break;
        }
        // Win32
        case appMainEventPlatformWin32CreateWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncCreateWindow, (uintptr_t)APP_WINDOW_NAME, APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, 0);
            break;
        case appMainEventPlatformWin32DestroyWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncDestroyWindow, 0, 0, 0, 0);
            break;
        case appMainEventPlatformWin32ResizeWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncResizeWindow, pAsync->arg1, pAsync->arg2, 0, 0);
            break;
        // 2D Engine
        case appMainEventUpdateFrame: logDebug("appMainEventUpdateFrame");
            if(_appMainUpdateFrame()){ logError("_appMainUpdateFrame fail"); goto appMainEventHandlerExit; }
            break;
    }
appMainEventHandlerExit:
    osalMutexUnlock(&actor->objMutex);
}
static void _appRenderEventHandler(void* arg1, void* arg2, void* arg3){
    activeObject* actor = (activeObject*)arg1;
    asyncPacket* pAsync = (asyncPacket*)arg2;
    uint8_t* pPayload = (uint8_t*)arg3;
    osalMutexLock(&actor->objMutex, -1);
    switch(pAsync->eventId){
        case appRenderEventTimer: //logDebug("appRenderEventTimer");
            driverCommonSync(driverCommonSyncTimer, 0, 0, 0, 0);
            serviceCommonSync(serviceCommonSyncTimer, 0, 0, 0, 0);
            break;
        // Rendering Service
        case appRenderServiceRenderingInit:
            serviceRenderingSync(serviceRenderingSyncInit, 0, 0, 0, 0);
            break;
        case appRenderServiceRenderingDeinit:
            serviceRenderingSync(serviceRenderingSyncDeinit, 0, 0, 0, 0);
            break;
        case appRenderDrawFrame:
            serviceRenderingSync(serviceRenderingSyncDrawFrame, 0, 0, 0, 0);
            break;
        // OpenGL
        case appRenderEventOpenglSyncUpdateViewport:
            driverOpenglSync(driverOpenglSyncUpdateViewport, pAsync->arg1, pAsync->arg2, 0, 0);
            break;
    }
appRenderEventHandlerExit:
    osalMutexUnlock(&actor->objMutex);
}
int appClose(void){
    if(activeClose(&_appMain.actor)){ logError("activeClose fail");
        return retFail;
    }
    if(activeClose(&_appRender.actor)){ logError("activeClose fail");
        return retFail;
    }
    return retOk;
}
int appOpen(void){
    if(activeOpen(&_appMain.actor)){ logError("activeOpen fail / %s", _appMain.actor.appThreadAttr.name);
        return retFail;
    }
    if(activeOpen(&_appRender.actor)){ logError("activeOpen fail / %s", _appRender.actor.appThreadAttr.name);
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appMainEventPlatformWin32CreateWindow, 0, 0, 0, 0)){logError("appMainEventPlatformWin32CreateWindow fail");
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appMainEventCreateWorld, 5, 10, 0, 0)){ logError("appMainEventCreateWorld fail");
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appRenderServiceRenderingInit, 0, 0, 0, 0)){ logError("appRenderServiceRenderingInit fail");
        return retFail;
    }
appOpenExit:
    return retOk;
}
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t){
    return retOk;
}
