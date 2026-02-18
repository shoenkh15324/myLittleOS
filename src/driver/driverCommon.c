/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "driver/driverCommon.h"

int driverCommonClose(void){
    if(driverPlatformWin32Close()){ logError("driverPlatformWin32Close fail");
        return -1;
    }
    if(driverGfxOpenglClose()){ logError("driverGfxOpenglClose fail");
        return -1;
    }
    return 0;
}
int driverCommonOpen(void){ 
    if(driverPlatformWin32Open()){ logError("driverPlatformWin32Open fail");
        return -1;
    }
    if(driverGfxOpenglOpen()){ logError("driverGfxOpenglOpen fail");
        return -1;
    }
    return 0;
}
int driverCommonSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    switch(sync){
        case driverCommonSyncTimer: //logDebug("driverCommonSyncTimer");
            driverPlatformWin32Sync(driverPlatformWin32SyncTimer, 0, 0, 0, 0);
            break;
    }
    return 0;
}
