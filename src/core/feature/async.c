/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#include "appCfgSelector.h"
#include "core/feature/async.h"
#include "core/feature/active.h"
#include "core/feature/log.h"

static asyncSubscriber _asyncSubscriber[APP_THREAD_MAX_COUNT];
static int _asyncSubCnt = 0;

static int _asyncLookupSenderIdx(void){
    osalThread currThread;
    if(osalThreadGetCurrent(&currThread)){
        return -1;
    }
    for(int i = 0; i < _asyncSubCnt; i++){
        if(_asyncSubscriber[i].pActObj->appThread.hThread == currThread.hThread){
            return i;
        }
    }
    return APP_THREAD_MAX_COUNT;
}
static void* _asyncLookupTarget(uint16_t eventId){
    for(int i = 0; i < _asyncSubCnt; i++){
        if((eventId >= _asyncSubscriber[i].startId) && (eventId <= _asyncSubscriber[i].endId)){
            return _asyncSubscriber[i].pActObj;
        }
    }
    return NULL;
}
int asyncSubscribe(struct activeObject* pActObj, uint16_t startId, uint16_t endId){
    if(!pActObj){ logError("Invaild Params"); return retInvalidParam; }
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
    int senderIdx = osalIsInIsr() ? APP_THREAD_MAX_COUNT : _asyncLookupSenderIdx();
    activeObject* pTarget = (activeObject*)_asyncLookupTarget(eventId);
    if(!pTarget){ logError("Cannot find active object"); return retFail; }
    if(pTarget->objState < objStateOpened){ logError("objState(%d) < objStateOpened", pTarget->objState); }
    //logDebug("asyncPush / [Target:%s] Event:0x%04X, Type:%d, SenderIdx:%d", pTarget->appThreadAttr.name ? pTarget->appThreadAttr.name : "Unknown", eventId, type, senderIdx);
    switch(type){
        case asyncTypeAsync:{
            if(osalIsInIsr()){ logDebug("ISR Thread"); }
            asyncPacket async = {type, eventId, arg1, arg2, arg3, arg4, 0};
            if(bufferCanPush(&pTarget->eventQueue[senderIdx], sizeof(async))){ logError("bufferCanPush fail");
                return retFail;
            }
            if(bufferPush(&pTarget->eventQueue[senderIdx], (uint8_t*)&async, sizeof(async))){ logError("bufferPush fail");
                return retFail;
            }
            break;
        }
        case asyncTypeAsyncPayload:{ // arg1: payload, arg2: payloadSize
            if(osalIsInIsr()){ logError("ISR Thread"); return retFail; }
            asyncPacket async = {type, eventId, 0, 0, arg3, arg4, arg2};
            if(bufferCanPush(&pTarget->eventQueue[senderIdx], (sizeof(async) + arg2))){ logError("bufferCanPush fail");
                return retFail;
            }
            if(bufferPush(&pTarget->eventQueue[senderIdx], (uint8_t*)&async, sizeof(async))){ logError("bufferPush fail");
                return retFail;
            }
            if(bufferPush(&pTarget->eventQueue[senderIdx], (uint8_t*)arg1, (size_t)arg2)){ logError("bufferPush fail");
                return retFail;
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
    osalSemaphoreGive(&pTarget->objSema);
#if APP_OS == OS_LINUX
    osalEpollNotify(&pTarget->objEpoll);
#elif APP_OS == OS_WIN32
    // Windows에서는 Semaphore만 Give하면 _actorThreadHandler가 깨어남. 별도 notify 필요 없음
#endif
    return retOk;
}
size_t asyncPop(struct activeObject* pTarget, asyncPacket* pOutPacket, uint8_t* payloadBuf){
    if(!pTarget || !pOutPacket || !payloadBuf){ logError("Invaild Params"); return retInvalidParam; }
    for(int i = 0; i < (APP_THREAD_MAX_COUNT + 1); i++){
        size_t popResult = bufferPop(&pTarget->eventQueue[i], (uint8_t*)pOutPacket, sizeof(asyncPacket));
        if(popResult < 0){ logError("bufferPop fail");
            return retFail;
        }
        //logDebug("asyncPop / [From SenderIdx:%d] Event:0x%04X, Target:%s", i, pOutPacket->eventId, pTarget->appThreadAttr.name);
        if(pOutPacket->type == asyncTypeAsyncPayload){
            if(bufferPop(&pTarget->eventQueue[i], payloadBuf, pOutPacket->payloadSize) < 0){ logError("payload bufferPop fail");
                return retFail;
            }
        }
        return popResult;
    }
    return 0;
}
