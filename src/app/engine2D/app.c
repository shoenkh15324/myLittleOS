/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "appCfgSelector.h"
#include "app/appCommon.h"

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
    }
}
static void _appMainEventHandler(void* arg1, void* arg2, void* arg3){
    activeObject* actor = (activeObject*)arg1;
    asyncPacket* pAsync = (asyncPacket*)arg2;
    uint8_t* pPayload = (uint8_t*)arg3;
    osalMutexLock(&actor->objMutex, -1);
    switch(pAsync->eventId){
        case appMainEventTimer: logDebug("appMainEventTimer");
            driverCommonSync(driverCommonSyncTimer, 0, 0, 0, 0);
            serviceCommonSync(serviceCommonSyncTimer, 0, 0, 0, 0);
            break;
        // Win32
        case appMainEventPlatformWin32CreateWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncCreateWindow, pAsync->arg1, pAsync->arg2, pAsync->arg3, 0);
            break;
        case appMainEventPlatformWin32DestroyWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncDestroyWindow, 0, 0, 0, 0);
            break;
        case appMainEventPlatformWin32ResizeWindow:
            driverPlatformWin32Sync(driverPlatformWin32SyncResizeWindow, pAsync->arg1, pAsync->arg2, 0, 0);
            break;
    }
    osalMutexUnlock(&actor->objMutex);
}
static void _appRenderEventHandler(void* arg1, void* arg2, void* arg3){
    activeObject* actor = (activeObject*)arg1;
    asyncPacket* pAsync = (asyncPacket*)arg2;
    uint8_t* pPayload = (uint8_t*)arg3;
    osalMutexLock(&actor->objMutex, -1);
    switch(pAsync->eventId){
        case appRenderEventTimer: //logDebug("appRenderEventTimer");
            break;
    }
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
    if(asyncPush(asyncTypeAsync, appMainEventPlatformWin32CreateWindow, (uintptr_t)"engin2D", 700, 500, 0)){logError("appMainEventPlatformWin32CreateWindow fail");
        return retFail;
    }
appOpenExit:
    return retOk;
}
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t){
    return retOk;
}
