/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-20
 ******************************************************************************/
#include "appCfgSelector.h"
#include "app/appCommon.h"

static void _appMainTimerHandler(void*);
static void _appMainEventHandler(void*, void*, void*);

static appMain _appMain = {
    .actor.isMainThread = true,
    .actor.objState = objStateClosed, 
    .actor.eventQueueSize = APP_MAIN_THREAD_EVENT_QUEUE_SIZE,
    .actor.appThreadAttr.name = "mario_main",
    .actor.appThreadAttr.priority = osalThreadPriorityNormal,
    .actor.appThreadAttr.statckSize = APP_MAIN_THREAD_STACK_SIZE,
    .actor.appThreadHandler = _appMainEventHandler,
    .actor.appTimerHandler = _appMainTimerHandler,
    .actor.appEventIdxStart = appMainEventStart,
    .actor.appEventIdxEnd = appMainEventEnd,
    .actor.payloadBufferSize = APP_MAIN_THREAD_PAYLOAD_BUFFER_SIZE,
};

static void _appMainTimerHandler(void* arg){ //logDebug("_appMainTimerHandler");
    activeObject* actor = (activeObject*)arg;
    if(actor->isMainThread){
        if(asyncPush(asyncTypeAsync, appMainEventTimer, 0, 0, 0, 0)){ logError("asyncPush fail"); }
    }
}
static void _appMainEventHandler(void* arg1, void* arg2, void* arg3){
    activeObject* actor = (activeObject*)arg1;
    asyncPacket* pAsync = (asyncPacket*)arg2;
    uint8_t* pPayload = (uint8_t*)arg3;
    osalMutexLock(&actor->objMutex, -1);
    switch(pAsync->eventId){
        case appMainEventTimer:{ //logDebug("appMainEventTimer");
            driverCommonSync(driverCommonSyncTimer, 0, 0, 0, 0);
            serviceCommonSync(serviceCommonSyncTimer, 0, 0, 0, 0);
            // rendering loop
            serviceRendering3dSync(serviceRendering3dSyncDrawFrame, 0 ,0, 0, 0);
            break;
        }
        // Win32
        case appMainEventPlatformWin32CreateWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncCreateWindow, (uintptr_t)APP_WINDOW_NAME, APP_WINDOW_WIDTH, APP_WINDOW_HEIGHT, 0);
            break;
        case appMainEventPlatformWin32DestroyWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncDestroyWindow, 0, 0, 0, 0);
            break;
        case appMainEventPlatformWin32ShowWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncShowWindow, 0, 0, 0, 0);
            break;
        case appMainEventPlatformWin32ResizeWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncResizeWindow, pAsync->arg1, pAsync->arg2, 0, 0);
            break;
        case appMainEventPlatformWin32MouseMove:
            driverPlatformWin32Sync(driverPlatformWin32SyncMouseMove, pAsync->arg1, pAsync->arg2, 0, 0);
            break;
        case appMainEventPlatformWin32MouseWheel:
            driverPlatformWin32Sync(driverPlatformWin32SyncMouseWheel, pAsync->arg1, 0, 0, 0);
            break;
        // Bgfx
        case appMainEventBgfxInit:
            driverBgfxSync(driverBgfxSyncInit, 0, 0, 0, 0);
            break;
        case appMainEventBgfxUpdateViewport:
            serviceRendering3dSync(serviceRendering3dSyncUpdateViewport, pAsync->arg1, pAsync->arg2, 0, 0);
            break;
        // Rendering Service
        case appMainEventServiceRenderingCreateEntity:
            serviceRendering3dSync(serviceRendering3dSyncCreateEntity, pAsync->arg1, pAsync->arg2, pAsync->arg3, 0);
            break;
        case appMainEventServiceRenderingUpdateCamera:
            serviceRendering3dSync(serviceRendering3dSyncUpdateCamera, pAsync->arg1, pAsync->arg2, 0, 0);
            break;
        case appMainEventServiceRenderingUpdateZoom:
            serviceRendering3dSync(serviceRendering3dSyncUpdateZoom, pAsync->arg1, 0, 0, 0);
            break;
    }
appMainEventHandlerExit:
    osalMutexUnlock(&actor->objMutex);
}
int appClose(void){
    if(activeClose(&_appMain.actor)){ logError("activeClose fail");
        return retFail;
    }
    return retOk;
}
int appOpen(void){
    if(activeOpen(&_appMain.actor)){ logError("activeOpen fail / %s", _appMain.actor.appThreadAttr.name);
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appMainEventPlatformWin32CreateWindow, 0, 0, 0, 0)){logError("appMainEventPlatformWin32CreateWindow fail");
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appMainEventBgfxInit, 0, 0, 0, 0)){logError("appMainEventBgfxInit fail");
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appMainEventPlatformWin32ShowWindow, 0, 0, 0, 0)){logError("appMainEventPlatformWin32ShowWindow fail");
        return retFail;
    }
#if 1
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if(asyncPush(asyncTypeAsync, appMainEventServiceRenderingCreateEntity, serviceRendering3dRenderTypeStarField, position, rotation, 0)){logError("appMainEventPlatformWin32ShowWindow fail");
        return retFail;
    }
    if(asyncPush(asyncTypeAsync, appMainEventServiceRenderingCreateEntity, serviceRendering3dRenderTypeBlackhole, position, rotation, 0)){logError("appMainEventPlatformWin32ShowWindow fail");
        return retFail;
    }
#endif
appOpenExit:
    return retOk;
}
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t){
    return retOk;
}
