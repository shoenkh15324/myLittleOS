#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "driver/driverCommon.h"
#include "serviceDefs.h"

#if APP_SERVICE_XXX == SERVICE_SAMPLE
    #include "service/sample/serviceSample.h"
#endif

enum{
    serviceCommonSyncTimer = 0,
};

int serviceCommonClose(void);
int serviceCommonOpen(void);
int serviceCommonSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
