/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-15
 ******************************************************************************/
#include "appCfgSelector.h"
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
            }else if((actor->isMainThread) && (fd == actor->appTimer.hTimer)){ //logDebug("hTimer");
                read(fd, &expired, sizeof(expired));
                if(actor->appTimerHandler) actor->appTimerHandler(actor);
            }
        }
#elif APP_OS == OS_WIN32
        HANDLE handles[8];
        int handleIdx = 0;
        handles[handleIdx++] = actor->objSema.hSema;
        if(actor->isMainThread && actor->appTimer.hTimer){ handles[handleIdx++] = actor->appTimer.hTimer; }
        DWORD triggeredIdx = MsgWaitForMultipleObjects(handleIdx, handles, FALSE, INFINITE, QS_ALLINPUT);
        if((triggeredIdx >= WAIT_OBJECT_0) && (triggeredIdx < (WAIT_OBJECT_0 + handleIdx))){ //logDebug("triggeredIdx");
            int event = triggeredIdx - WAIT_OBJECT_0;
            if(event == 0){ //logDebug("semaphore event"); // semaphore event 
                while(asyncPop(actor, &async, actor->pPayloadBuffer)){
                    if(actor->appThreadHandler) actor->appThreadHandler(actor, &async, actor->pPayloadBuffer);
                }
            }else if((event == 1) && actor->appTimer.hTimer){ //logDebug("timer event"); // timer event
                if(actor->appTimerHandler) actor->appTimerHandler(actor);
            }
        }
        if(actor->isMainThread){
            MSG msg;
            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
                TranslateMessage(&msg);
                DispatchMessage(&msg);
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
    for(int i = 0; i < (APP_THREAD_MAX_COUNT + 1); i++){
        if(bufferOpen(&pHandle->eventQueue[i], pHandle->eventQueueSize)){ logError("bufferOpen fail");
            result = retFail; goto appOpenExit;
        }
    }
    if(osalMalloc((void**)&pHandle->pPayloadBuffer, pHandle->payloadBufferSize)){ logError("osalMalloc fail");
        result = retFail; goto appOpenExit;
    }
    if(asyncSubscribe(pHandle, pHandle->appEventIdxStart, pHandle->appEventIdxEnd)){ logError("asyncSubscribe fail");
        result = retFail; goto appOpenExit;
    }
    if(pHandle->isMainThread){
        if(osalTimerOpen(&pHandle->appTimer, pHandle->appTimerHandler, pHandle, APP_TIMER_INTERVAL)){ logError("osalTimerOpen fail");
            result = retFail; goto appOpenExit;
        }
#if APP_OS == OS_LINUX
        if(osalEpollAddFd(&pHandle->objEpoll, pHandle->appTimer.hTimer, osalEpollEventFlagIn)){ logError("osalEpollAddFd(timer) fail"); 
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
    return result;
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
        for(int i = 0; i < APP_THREAD_MAX_COUNT; i++){
            if(bufferClose(&pHandle->eventQueue[i])){ logError("bufferClose fail"); 
                return retFail; 
            }
        }
        if(osalFree(pHandle->pPayloadBuffer)){ logError("osalFree fail"); 
            return retFail;
        }
        if(osalThreadClose(&pHandle->appThread)){ logError("osalThreadClose fail");
            return retFail;
        }
        if(osalTimerClose(&pHandle->appTimer)){ logError("osalTimerClose fail");
            return retFail;
        }
#if APP_OS == OS_LINUX
        if(osalEpollClose(&pHandle->objEpoll)){ logError("osalEpollClose fail");
            return retFail;
        }
#endif
        pHandle->objState = objStateClosed;
        osalMutexClose(&pHandle->objMutex); 
    }
    return result;
}

