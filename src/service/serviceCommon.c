/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "service/serviceCommon.h"

int serviceCommonClose(void){
#if APP_ENGINE_2D
    if(serviceRendering2dClose()){ logError("serviceRendering2dClose fail");
        return retFail;
    }
#else APP_EUCLID_ENGINE
    if(serviceRendering3dClose()){ logError("serviceRendering3dClose fail");
        return retFail;
    }
#endif
    return 0;
}
int serviceCommonOpen(void){
#if APP_ENGINE_2D
    if(serviceRendering2dOpen()){ logError("serviceRendering2dOpen fail");
        return retFail;
    }
#else APP_EUCLID_ENGINE
    if(serviceRendering3dOpen()){ logError("serviceRendering3dOpen fail");
        return retFail;
    }
#endif
    return 0;
}
int serviceCommonSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    switch(sync){
        case serviceCommonSyncTimer:
            break;
    }
    return 0;
}
