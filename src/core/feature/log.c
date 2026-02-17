/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-12
 ******************************************************************************/
#include "appCfgSelector.h"

#include "core/feature/log.h"
#if APP_LOG_ENABLE
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "core/feature/osal.h"

static char buffer[APP_LOG_BUFFER_SIZE];
static char message[APP_LOG_BUFFER_SIZE - 128];

static osalMutex _logMutex;

#if APP_LOG_BACKEND == SYSTEM_LOG_BACKEND_PRINTF
    #if APP_LOG_COLOR
        static void _logOutputWithColor(uint8_t level, const char* str){
            const char* color = "\033[0m"; // default color
            switch(level){
                case SYSTEM_LOG_LEVEL_ERROR: color = "\033[31m"; break; // Red
                case SYSTEM_LOG_LEVEL_WARN: color = "\033[33m"; break; // Yellow
                case SYSTEM_LOG_LEVEL_INFO: color = "\033[36m"; break; // Cyan
            }
            printf("%s%s\033[0m\n", color, str);
        }
    #else
        #define _logOutput(str) printf("%s\n", str)
    #endif
#elif APP_LOG_BACKEND == SYSTEM_LOG_BACKEND_PRINTK
    extern int printk(const char* fmt, ...);
    #if APP_LOG_COLOR
        #define _logOutputWithColor(level, str) printk("%s\n", str)
    #else
        #define _logOutput(str) printk("%s\n", str)
    #endif
#elif APP_LOG_BACKEND == SYSTEM_LOG_BACKEND_UART
    extern void uartWrite(const char* str);
    #if APP_LOG_COLOR
        #define _logOutputWithColor(level, str) uartWrite(str)
    #else
        #define _logOutput(str) uartWrite(str)
    #endif
#else
    #define _logOutput(str) ((void)0)
#endif

static const char* _logExtractFileName(const char* path){
    const char* file = strrchr(path, '/');
#ifdef _WIN32
    const char* backslash = strrchr(path, '\\');
    if (backslash && (!file || backslash > file))
        file = backslash;
#endif
    return file ? file + 1 : path;
}
static const char* _logLevelToString(uint8_t level){
    switch(level){
        case SYSTEM_LOG_LEVEL_ERROR: return "ERROR";
        case SYSTEM_LOG_LEVEL_WARN:  return "WARN";
        case SYSTEM_LOG_LEVEL_INFO:  return "INFO";
        case SYSTEM_LOG_LEVEL_DEBUG: return "DEBUG";
        default: return "NONE";
    }
}

void _logInternalArgs(int level,  const char* func, int line, const char* fmt, va_list args){
#if APP_LOG_ENABLE
    osalMutexLock(&_logMutex, -1);
    vsnprintf(message, sizeof(message), fmt, args);
    #if APP_LOG_TIMESTAMP
        char timestamp[16] = {0};
        osalGetDate(timestamp, sizeof(timestamp));
    #endif
    if(APP_LOG_LEVEL == SYSTEM_LOG_LEVEL_NONE){
        #if APP_LOG_TIMESTAMP
            snprintf(buffer, sizeof(buffer), "%s | %s:%d [%s] %s", timestamp, func, line, SYSTEM_LOG_PREFIX, message);
        #else
            snprintf(buffer, sizeof(buffer), "%s:%d [%s] %s", func, line, SYSTEM_LOG_PREFIX, message);
        #endif
    }else{
        #if APP_LOG_TIMESTAMP
            snprintf(buffer, sizeof(buffer), "%s | %s:%d [%s][%s] %s", timestamp, func, line, SYSTEM_LOG_PREFIX, _logLevelToString(level), message);
        #else
            snprintf(buffer, sizeof(buffer), "%s:%d [%s][%s] %s", func, line, SYSTEM_LOG_PREFIX, _logLevelToString(level), message);
        #endif
    }
    #if APP_LOG_MODE == SYSTEM_LOG_MODE_SYNC
        #if APP_LOG_COLOR
            _logOutputWithColor(level, buffer);
        #else
            _logOutput(buffer);
        #endif
    #else
        // TODO: log async
    #endif
    osalMutexUnlock(&_logMutex);
#else
    (void)level; (void)func; (void)line; (void)fmt; (void)args;
#endif
}
void _logInternal(int level, const char* func, int line, const char* fmt, ...){
#if APP_LOG_ENABLE
    va_list args;
    va_start(args, fmt);
    _logInternalArgs(level, func, line, fmt, args);
    va_end(args);
#else
    (void)level; (void)func; (void)line; (void)fmt;
#endif
}

int logOpen(void){
    if(osalMutexOpen(&_logMutex)){
        return retFail;
    }
    return retOk;
}
int logClose(void){
    if(osalMutexClose(&_logMutex)){
        return retFail;
    }
    return retOk;
}

#endif
