#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-10
 ******************************************************************************/
#if defined (APP_SAMPLE)
    #include "app/sample/app.h"
#endif

typedef enum{
    // appMain
    appMainEventStart = objSyncBegin,
        appMainEventTimer,
    appMainEventEnd = 99,
    // appTest
    appTestEventStart = 100,
        appTestEventTimer,
    appTestEventEnd = 199,
} appEventList;

int appCommonClose(void);
int appCommonOpen(void);
