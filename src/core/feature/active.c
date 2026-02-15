/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-15
 ******************************************************************************/

#include "core/feature/active.h"
#include "core/feature/log.h"

static void _actorThreadHandler(void* arg){
    activeObject* actor = (activeObject*)arg;
    asyncPacket async;
    if(actor->objState < objStateOpened){ logError("objState(%d) < objStateOpened", actor->objState); return; }
    for(;;){
        #if APP_OS == OS_LINUX
        int fd; uint64_t expired;
        if(!osalEpollWait(&actor->objEpoll, &fd, -1)){
            if(fd == actor->objEpoll.eventFd){ //logDebug("eventFd");
                read(fd, &expired, sizeof(expired));
                while(asyncPop(actor, &async, (uint8_t*)actor->pPayloadBuffer)){ //logDebug("_appMainEventHandler");
                    if(actor->appThreadHandler){ actor->appThreadHandler(actor, &async, actor->pPayloadBuffer); }
                }
            }else if((actor->isMainThread) && (fd == actor->appTimer.timerFd)){ //logDebug("timerFd");
                 read(fd, &expired, sizeof(expired));
                if(actor->appTimerHandler) actor->appTimerHandler(actor);
            }
        }
#else
        if(osalSemaphoreTake(&actor->objSema, -1)){
            while(!asyncPop(actor, &async, actor->pPayloadBuffer)){
                if(actor->appThreadHandler)
                    actor->appThreadHandler(actor, &async, actor->pPayloadBuffer);
            }
        }
#endif
    }
}
int activeOpen(activeObject* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    int result = retOk;
    osalMutexOpen(&pHandle->objMutex);
    osalSemaphoreOpen(&pHandle->objSema, -1);
#if APP_OS == OS_LINUX
    osalEpollOpen(&pHandle->objEpoll);
#endif
    osalMutexLock(&pHandle->objMutex, -1);
    pHandle->objState = objStateOpening;
    //
    if(bufferOpen(&pHandle->eventQueue, pHandle->eventQueueSize)){ logError("bufferOpen fail");
        result = retFail; goto appOpenExit;
    }
    if(osalMalloc(&pHandle->pPayloadBuffer, pHandle->payloadBufferSize)){ logError("osalMalloc fail");
        result = retFail; goto appOpenExit;
    }
    if(asyncSubscribe(pHandle, pHandle->appEventIdxStart, pHandle->appEventIdxEnd)){ logError("asyncSubscribe fail");
        result = retFail; goto appOpenExit;
    }
    if(pHandle->isMainThread){
        if(osalTimerOpen(&pHandle->appTimer, pHandle->appTimerHandler, APP_TIMER_INTERVAL)){ logError("osalTimerOpen fail");
            result = retFail; goto appOpenExit;
        }
#if APP_OS == OS_LINUX
        if(osalEpollAddFd(&pHandle->objEpoll, pHandle->appTimer.timerFd, osalEpollEventFlagIn)){ logError("osalEpollAddFd(timer) fail"); 
            result = retFail; goto appOpenExit;
        }
#endif
    }
    pHandle->objState = objStateOpened;
    if(osalThreadOpen(&pHandle->appThread, &pHandle->appThreadAttr, _actorThreadHandler, pHandle)){ logError("osalThreadOpen fail");
        result = retFail; goto appOpenExit;
    }
    if(pHandle->appOnOpenHandler) pHandle->appOnOpenHandler(pHandle);
    //
appOpenExit:
    osalMutexUnlock(&pHandle->objMutex);
    return retOk;
}
int activeClose(activeObject* pHandle){
    if(!pHandle){ logError("Invaild Params"); return retInvalidParam; }
    int result = retOk;
    if(pHandle->objState >= objStateOpening){
        osalMutexLock(&pHandle->objMutex, -1);
        pHandle->objState = objStateClosing;
        osalMutexUnlock(&pHandle->objMutex);
#if APP_OS == OS_LINUX
        osalEpollNotify(&pHandle->objEpoll);
#endif
        if(pHandle->pPayloadBuffer){
            osalFree(pHandle->pPayloadBuffer);
            pHandle->pPayloadBuffer = NULL;
        }
        if(osalThreadClose(&pHandle->appThread)){ logError("osalThreadClose fail");
            return retFail;
        }
        if(osalTimerClose(&pHandle->appTimer)){ logError("osalTimerClose fail");
            return retFail;
        }
        if(osalEpollClose(&pHandle->objEpoll)){ logError("osalEpollClose fail");
            return retFail;
        }
        pHandle->objState = objStateClosed;
        osalMutexClose(&pHandle->objMutex); 
    }
    return result;
}

