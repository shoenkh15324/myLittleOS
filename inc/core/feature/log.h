#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-10
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "core/sysDefs.h"

#if APP_LOG_ENABLE
    #if APP_LOG_LEVEL >= SYSTEM_LOG_LEVEL_ERROR
        #define logError(fmt, ...) _logInternal(SYSTEM_LOG_LEVEL_ERROR, __func__, __LINE__, fmt, ##__VA_ARGS__)
    #else
        #define logError(fmt, ...) ((void)0)
    #endif
    #if APP_LOG_LEVEL >= SYSTEM_LOG_LEVEL_WARN
        #define logWarn(fmt, ...) _logInternal(SYSTEM_LOG_LEVEL_WARN, __func__, __LINE__, fmt, ##__VA_ARGS__)
    #else
        #define logWarn(fmt, ...) ((void)0)
    #endif
    #if APP_LOG_LEVEL >= SYSTEM_LOG_LEVEL_INFO
        #define logInfo(fmt, ...) _logInternal(SYSTEM_LOG_LEVEL_INFO, __func__, __LINE__, fmt, ##__VA_ARGS__)
    #else
        #define logInfo(fmt, ...) ((void)0)
    #endif
    #if APP_LOG_LEVEL >= SYSTEM_LOG_LEVEL_DEBUG
        #define logDebug(fmt, ...) _logInternal(SYSTEM_LOG_LEVEL_DEBUG, __func__, __LINE__, fmt, ##__VA_ARGS__)
    #else
        #define logDebug(fmt, ...) ((void)0)
    #endif
#else
    #define log(...) ((void)0)
    #define logError(...) ((void)0)
    #define logWarn(...)  ((void)0)
    #define logInfo(...)  ((void)0)
    #define logDebug(...) ((void)0)
#endif

int logOpen(void);
int logClose(void);

#ifdef __cplusplus
}
#endif