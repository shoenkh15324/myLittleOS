#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
// C Standard Library
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined (APP_SAMPLE)
    #include "app/sample/appConfig.h"
#endif

#if APP_OS == OS_LINUX
    #define _GNU_SOURCE
#endif

typedef enum{
    retOk = 0,
    retFail = -1,
    retTimeout = -2,
    retInvalidParam = -3,
    retEmptyBuffer = -4,
} returnCode;

typedef enum{
    objStateClosed = 0,
    objStateClosing,
    objStateOpening,
    objStateOpened,
    objStateBegin,
} objectState;
typedef enum{
    objSyncBegin = 0,
} objectSync;

