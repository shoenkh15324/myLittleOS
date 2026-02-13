#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-10
 ******************************************************************************/

// C Standard Library
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
// System Core Feature
#include "config/platformConfig.h"
#include "osal.h"
#include "core/log.h"
#include "core/buffer.h"

typedef enum{
    retOk = 0,
    retFail,
    retTimeout,
    retInvalidParam,
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

typedef struct{
    uint16_t eventId;
    void *payload;
    size_t payloadSize;
} asyncPacket;

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
int systemOpen(void);
