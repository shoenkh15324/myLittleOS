#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-10
 ******************************************************************************/
#include "core/systemDefs.h"
#include "service/serviceCommon.h"

#if defined (APP_SAMPLE)
    #include "app/sample/app.h"
#elif defined (APP_ENGINE_2D)
    #include "app/engine2D/app.h"
#elif defined (APP_EUCLID_ENGINE)
    #include "app/euclidEngine/app.h"
#endif

int appCommonClose(void);
int appCommonOpen(void);
