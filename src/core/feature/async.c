/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "core/feature/async.h"
#include "core/feature/log.h"

static asyncSubscriber _asyncSubscriber[APP_THREAD_MAX_COUNT];
static int _asyncSubCnt = 0;

static activeObject* _asyncLookupTarget(uint16_t eventId){
    for(int i = 0; i < _asyncSubCnt; i++){
        if((eventId >= _asyncSubscriber[i].startId) && (eventId <= _asyncSubscriber[i].endId)){
            return _asyncSubscriber[i].pActObj;
        }
    }
    return NULL;
}
int asyncSubscribe(activeObject* pActObj, uint16_t startId, uint16_t endId){
    checkParams(pActObj);
    if(_asyncSubCnt >= APP_THREAD_MAX_COUNT){ logError("_asyncSubCnt >= APP_THREAD_MAX_COUNT");
        return retFail;
    }
    _asyncSubscriber[_asyncSubCnt].pActObj = pActObj;
    _asyncSubscriber[_asyncSubCnt].startId = startId;
    _asyncSubscriber[_asyncSubCnt].endId = endId;
    _asyncSubCnt++;
    return retOk;
}
int asyncPush(asyncType type, uint16_t eventId, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    activeObject* pTarget = _asyncLookupTarget(eventId);
    if(!pTarget){ logError("Cannot find active object"); return retFail; }
    if(pTarget->objState < objStateOpened){ logError("objState(%d) < objStateOpened", pTarget->objState); }
    int result = retOk;
    osalMutexLock(&pTarget->objMutex, -1);
    switch(type){
        case asyncTypeAsync:{
            if(osalIsInIsr()){ logDebug("ISR Thread"); }
            asyncPacket async = {type, eventId, arg1, arg2, arg3, arg4, 0};
            if(bufferCanPush(&pTarget->eventQueue, sizeof(async))){ logError("bufferCanPush fail");
                result = retFail; goto Exit;
            }
            if(bufferPush(&pTarget->eventQueue, (uint8_t*)&async, sizeof(async))){ logError("bufferPush fail");
                result = retFail; goto Exit;
            }
            break;
        }
        case asyncTypeAsyncPayload:{ // arg1: payload, arg2: payloadSize
            if(osalIsInIsr()){ logError("ISR Thread");
                result = retFail; goto Exit;
            }
            asyncPacket async = {type, eventId, 0, 0, arg3, arg4, arg2};
            if(bufferCanPush(&pTarget->eventQueue, (sizeof(async) + arg2))){ logError("bufferCanPush fail");
                result = retFail; goto Exit;
            }
            if(bufferPush(&pTarget->eventQueue, (uint8_t*)&async, sizeof(async))){ logError("bufferPush fail");
                result = retFail; goto Exit;
            }
            if(bufferPush(&pTarget->eventQueue, (uint8_t*)arg1, (size_t)arg2)){ logError("bufferPush fail");
                result = retFail; goto Exit;
            }
            break;
        }
        case asyncTypeAwait:{ // TODO
            break;
        }
        case asyncTypeExpress:{ // TODO
            break;
        }
    }
Exit:
    osalMutexUnlock(&pTarget->objMutex);
    if(result == retOk){
        osalSemaphoreGive(&pTarget->objSema);
#if APP_OS == OS_LINUX
        osalEpollNotify(&pTarget->objEpoll);
#endif
    } 
    return result;
}
size_t asyncPop(activeObject* pTarget, asyncPacket* pOutPacket, uint8_t* payloadBuf){
    checkParams(pTarget, pOutPacket);
    osalMutexLock(&pTarget->objMutex, -1);
    size_t popResult = bufferPop(&pTarget->eventQueue, pOutPacket, sizeof(asyncPacket));
    if(popResult < 0){ logError("bufferPop fail");
        osalMutexUnlock(&pTarget->objMutex);
        return retFail;
    }
    if(pOutPacket->type == asyncTypeAsyncPayload){
        if(bufferPop(&pTarget->eventQueue, payloadBuf, pOutPacket->payloadSize)){ logError("payload bufferPop fail");
            osalMutexUnlock(&pTarget->objMutex);
            return retFail;
        }
    }
    osalMutexUnlock(&pTarget->objMutex);
    return popResult;
}

#ifdef __cplusplus
}
#endif
