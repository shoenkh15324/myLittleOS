/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "driver/driverCommon.h"

int driverCommonClose(void){
#if APP_ENGINE_2D
    if(driverOpenglClose()){ logError("driverOpenglClose fail"); return -1; }
    if(driverPlatformWin32Close()){ logError("driverPlatformWin32Close fail"); return -1; }
#elif APP_EUCLID_ENGINE
    if(driverJoltClose()){ logError("driverJoltClose fail"); return -1; }
    if(driverBgfxClose()){ logError("driverBgfxClose fail"); return -1; }
    if(driverPlatformWin32Close()){ logError("driverPlatformWin32Close fail"); return -1; }
#endif
    return 0;
}
int driverCommonOpen(void){
#if APP_ENGINE_2D
    if(driverPlatformWin32Open()){ logError("driverPlatformWin32Open fail"); return -1; }
    if(driverOpenglOpen()){ logError("driverOpenglOpen fail"); return -1; }
#elif APP_EUCLID_ENGINE
    if(driverPlatformWin32Open()){ logError("driverPlatformWin32Open fail"); return -1; }
    if(driverJoltOpen()){ logError("driverJoltOpen fail"); return -1; }
    if(driverBgfxOpen()){ logError("driverBgfxOpen fail"); return -1; }
#endif
    return 0;
}
int driverCommonSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    switch(sync){
        case driverCommonSyncTimer: //logDebug("driverCommonSyncTimer");
            driverPlatformWin32Sync(driverPlatformWin32SyncTimer, 0, 0, 0, 0);
#if APP_ENGINE_2D
            driverOpenglSync(driverOpenglSyncTimer, 0, 0, 0, 0);
#endif
            break;
    }
    return 0;
}
