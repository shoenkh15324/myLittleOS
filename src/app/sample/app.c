/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "app/appCommon.h"

static void _appMainTimerHandler(activeObject* actor);
static void _appMainEventHandler(activeObject* actor, asyncPacket* pAsync, uint8_t* pPayload);
static void _appTestEventHandler(activeObject* actor, asyncPacket* pAsync, uint8_t* pPayload);

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
static appTest _appTest = {
    .actor.objState = objStateClosed, 
    .actor.eventQueueSize = APP_TEST_THREAD_EVENT_QUEUE_SIZE,
    .actor.appThreadAttr.name = "test",
    .actor.appThreadAttr.priority = osalThreadPriorityIdle,
    .actor.appThreadAttr.statckSize = APP_TEST_THREAD_STACK_SIZE,
    .actor.appThreadHandler = _appTestEventHandler,
    .actor.appEventIdxStart = appTestEventStart,
    .actor.appEventIdxEnd = appTestEventEnd,
    .actor.payloadBufferSize = APP_TEST_THREAD_PAYLOAD_BUFFER_SIZE,
};
static void _appMainTimerHandler(activeObject* actor){ //logDebug("_appMainTimerHandler");
    if(actor->isMainThread){
        if(asyncPush(asyncTypeAsync, appMainEventTimer, NULL, NULL, NULL, NULL)){ logError("asyncPush fail"); }
        actor->appTimerCount += APP_TIMER_INTERVAL;
        if(actor->appTimerCount >= 2000){
            if(asyncPush(asyncTypeAsync, appTestEventTimer, NULL, NULL, NULL, NULL)){ logError("asyncPush fail"); }
            actor->appTimerCount = 0;
        }
    }
}
static void _appMainEventHandler(activeObject* actor, asyncPacket* pAsync, uint8_t* pPayload){
    osalMutexLock(&actor->objMutex, -1);
    switch(pAsync->eventId){
        case appMainEventTimer: logDebug("appMainEventTimer");
            break;
    }
    osalMutexUnlock(&actor->objMutex);
}
static void _appTestEventHandler(activeObject* actor, asyncPacket* pAsync, uint8_t* pPayload){
    osalMutexLock(&actor->objMutex, -1);
    switch(pAsync->eventId){
        case appTestEventTimer: logDebug("appTestEventTimer");
            break;
    }
    osalMutexUnlock(&actor->objMutex);
}
int appClose(void){
    if(activeClose(&_appMain.actor)){ logError("activeClose fail");
        return retFail;
    }
    if(activeClose(&_appTest.actor)){ logError("activeClose fail");
        return retFail;
    }
    return retOk;
}
int appOpen(void){
    if(activeOpen(&_appMain.actor)){ logError("activeOpen fail");
        return retFail;
    }
    if(activeOpen(&_appTest.actor)){ logError("activeOpen fail");
        return retFail;
    }
appOpenExit:
    return retOk;
}
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t){
    return retOk;
}
