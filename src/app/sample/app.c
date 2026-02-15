/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "app/appCommon.h"

static appMain _appMain = {
    .actObj.objState = objStateClosed, 
    .appThreadAttr.name = "main",
    .appThreadAttr.priority = osalThreadPriorityIdle,
    .appThreadAttr.statckSize = APP_THREAD_STACK_SIZE,
};

static void _appEventHandler(asyncPacket* pAsync, uint8_t* pPayload){
    osalMutexLock(&_appMain.actObj.objMutex, -1);
    switch(pAsync->eventId){
        case appMainEventTimer: logDebug("appMainEventTimer");
            break;
    }
appEventHandlerExit:
    osalMutexUnlock(&_appMain.actObj.objMutex);
}
static void _appTimerHandler(void* arg){ //logDebug("_appTimerHandler");
    if(asyncPush(asyncTypeAsync, appMainEventTimer, NULL, NULL, NULL, NULL)){
        logError("asyncPush fail");
    }
}
static void _appThreadHandler(void){
    if(_appMain.actObj.objState < objStateOpened){ logError("objState(%d) < objStateOpened", _appMain.actObj.objState); return; }
    asyncPacket async;
    for(;;){
#if APP_OS == OS_LINUX
        int fd; uint64_t expired;
        if(!osalEpollWait(&_appMain.actObj.objEpoll, &fd, -1)){
            if(fd == _appMain.actObj.objEpoll.eventFd){ //logDebug("eventFd");
                read(fd, &expired, sizeof(expired));
                while(asyncPop(&_appMain.actObj, &async, &_appMain.payloadBuf)){ //logDebug("_appEventHandler");
                    _appEventHandler(&async, &_appMain.payloadBuf);
                }
            }else if(fd == _appMain.appTimer.timerFd){ //logDebug("timerFd");
                read(fd, &expired, sizeof(expired));
                if(_appMain.appTimer.timerCb){ _appMain.appTimer.timerCb(_appMain.appTimer.timerArg); }
            }
        }
#else
        if(osalSemaphoreTake(&_appMain.actObj.objSema, -1)){
            while(!asyncPop(&_appMain.actObj, &async, &_appMain.payloadBuf)){
                _appEventHandler(&async, &_appMain.payloadBuf);
            }
        }
#endif
    }
}
int appClose(void){
    int result = retOk;
    if(_appMain.actObj.objState >= objStateOpening){
        osalMutexLock(&_appMain.actObj.objMutex, -1);
        _appMain.actObj.objState = objStateClosing;
        osalMutexUnlock(&_appMain.actObj.objMutex);
#if APP_OS == OS_LINUX
        osalEpollNotify(&_appMain.actObj.objEpoll);
#endif
        if(osalThreadClose(&_appMain.appThread)){ logError("osalThreadClose fail");
            return retFail;
        }
        if(osalTimerClose(&_appMain.appTimer)){ logError("osalTimerClose fail");
            return retFail;
        }
        if(osalEpollClose(&_appMain.actObj.objEpoll)){ logError("osalEpollClose fail");
            return retFail;
        }
        _appMain.actObj.objState = objStateClosed;
        osalMutexClose(&_appMain.actObj.objMutex); 
    }
    return result;
}
int appOpen(void){
    int result = retOk;
    osalMutexOpen(&_appMain.actObj.objMutex);
    osalSemaphoreOpen(&_appMain.actObj.objSema, -1);
#if APP_OS == OS_LINUX
    osalEpollOpen(&_appMain.actObj.objEpoll);
#endif
    osalMutexLock(&_appMain.actObj.objMutex, -1);
    _appMain.actObj.objState = objStateOpening;
    //
    if(bufferOpen(&_appMain.actObj.eventQueue, APP_THREAD_EVENT_QUEUE_SIZE)){ logError("bufferOpen fail");
        result = retFail; goto appOpenExit;
    }
    if(asyncSubscribe(&_appMain.actObj, appMainEventStart, appMainEventEnd)){ logError("asyncSubscribe fail");
        result = retFail; goto appOpenExit;
    }
    if(osalTimerOpen(&_appMain.appTimer, _appTimerHandler, APP_TIMER_INTERVAL)){ logError("osalTimerOpen fail");
        result = retFail; goto appOpenExit;
    }
#if APP_OS == OS_LINUX
    if(osalEpollAddFd(&_appMain.actObj.objEpoll, _appMain.appTimer.timerFd, osalEpollEventFlagIn)){ logError("osalEpollAddFd(timer) fail"); 
        result = retFail; goto appOpenExit;
    }
#endif
    if(osalThreadOpen(&_appMain.appThread, &_appMain.appThreadAttr, _appThreadHandler, NULL)){ logError("osalThreadOpen fail");
        result = retFail; goto appOpenExit;
    }
    //
    _appMain.actObj.objState = objStateOpened;
appOpenExit:
    osalMutexUnlock(&_appMain.actObj.objMutex);
    return result;
}
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t){
    return retOk;
}
