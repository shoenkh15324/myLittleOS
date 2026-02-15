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

#define checkParams(...) \
    do { \
        const uintptr_t _args[] = { (uintptr_t)(__VA_ARGS__) }; \
        const int _num_args = sizeof(_args) / sizeof(_args[0]); \
        for (int _i = 0; _i < _num_args; _i++) { \
            if (_args[_i] == 0) { \
                logError("[SYSTEM] Invaild Params"); \
                return retInvalidParam; \
            } \
        } \
    } while(0)
#define checkParamsVoid(...) \
    do { \
        const uintptr_t _args[] = { (uintptr_t)(__VA_ARGS__) }; \
        for (size_t _i = 0; _i < sizeof(_args)/sizeof(_args[0]); _i++) { \
            if (_args[_i] == 0) { \
                logError("[SYSTEM] Invaild Params"); \
                return; \
            } \
        } \
    } while(0)
