/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-10
 ******************************************************************************/
#include "core/system.h"
#include "core/config/platformConfig.h"
#include "app/appCommon.h"

int systemClose(void){
    return 0;
}
int systemOpen(void){
    printf("%s %s / %s Open / v%d.%d.%d / Author: %s\n", __DATE__, __TIME__, SYSTEM_NAME, SYSTEM_VERSION_MAJOR, SYSTEM_VERSION_MINOR, SYSTEM_VERSION_PATCH, SYSTEM_AUTHOR);
    logOpen();
    driverCommonOpen();
    serviceCommonOpen();
    appCommonOpen();
    while(1){ osalSleepMs(1000); }
    return 0;
}
