#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-12
 ******************************************************************************/
#include "core/systemDefs.h"
#include "driver/driverDefs.h"
#include "service/serviceDefs.h"

#if defined (APP_ENGINE_2D)

typedef enum{
    // appMain
    appMainEventStart = objSyncBegin,
        appMainEventTimer,
        // Win32
        appMainEventPlatformWin32CreateWindow,
        appMainEventPlatformWin32DestroyWindow,
        appMainEventPlatformWin32ResizeWindow,
    appMainEventEnd = 99,
    // appTest
    appRenderEventStart = 100,
        appRenderEventTimer,
    appRenderEventEnd = 199,
} appEventList;

/* [INFO]*/
#define APP_NAME "Engine 2D"
#define APP_AUTHOR "Minkyu Kim"
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0

/* [PLATFORM] */
#define APP_MCU MCU_NONE
#define APP_OS OS_WIN32
#define APP_SDK SDK_WINDOW
#define APP_BOARD BOARD_NONE
#if (APP_BOARD == BOARD_NONE) && (APP_OS == OS_LINUX)
    #define APP_DRIVER_XXX 0
#elif (APP_BOARD == BOARD_NONE) && (APP_OS == OS_WIN32)
    #define APP_DRIVER_PLATFORM DRIVER_PLATFORM_WIN32
    #if APP_DRIVER_PLATFORM == DRIVER_PLATFORM_WIN32
        #define DRIVER_PLATFORM_WIN32_WINDOW_CLASS_NAME L"engine2dWindowClass"
        #define DRIVER_PLATFORM_WIN32_WM_USER_RESIZE (WM_USER + 1)
    #endif
    #define APP_DRIVER_GFX DRIVER_GFX_OPENGL
#elif APP_BOARD
    #define APP_DRIVER_XXX 0
#endif

/* [SERVICE] */
#define APP_SERVICE_XXX 0

/* [LOG] */
#define APP_LOG_ENABLE SYSTEM_LOG_ENABLE
#if APP_LOG_ENABLE
    #define APP_LOG_BUFFER_SIZE 1024
    #define APP_LOG_BACKEND SYSTEM_LOG_BACKEND_PRINTF
    #define APP_LOG_MODE SYSTEM_LOG_MODE_SYNC
    #define APP_LOG_LEVEL SYSTEM_LOG_LEVEL_DEBUG
    #define APP_LOG_COLOR SYSTEM_LOG_COLOR_ENABLE
    #define APP_LOG_TIMESTAMP SYSTEM_LOG_TIMESTAMP_ENABLE
#endif

/* [OSAL] */
// THREAD
#define APP_THREAD SYSTEM_OSAL_THREAD_ENABLE
#if APP_THREAD
    #define APP_THREAD_MAX_COUNT 2
    #if APP_THREAD_MAX_COUNT >= 1
        #define APP_MAIN_THREAD_STACK_SIZE (1024 * 1024)
        #define APP_MAIN_THREAD_EVENT_QUEUE_SIZE (128 * 1024)
        #define APP_MAIN_THREAD_PAYLOAD_BUFFER_SIZE (1024)
        #if APP_THREAD_MAX_COUNT >= 2
            #define APP_RENDER_THREAD_STACK_SIZE (512 * 1024)
            #define APP_RENDER_THREAD_EVENT_QUEUE_SIZE (128 * 1024)
            #define APP_RENDER_THREAD_PAYLOAD_BUFFER_SIZE 1024
            #if APP_LOG_MODE == SYSTEM_LOG_MODE_ASYNC
                #define APP_LOG_THREAD_STACK_SIZE (1 * 1024)
            #endif
    #endif
    #endif
#endif
// TIMER
#define APP_TIMER SYSTEM_OSAL_TIMER_ENABLE
#if APP_TIMER
    #define APP_TIMER_INTERVAL 10
#endif
// MEMORY
#if APP_THREAD && (APP_THREAD_MAX_COUNT > 1)
    #define APP_MEM SYSTEM_OSAL_DYNAMIC_MEM
#else
    #define APP_MEM SYSTEM_OSAL_STATIC_MEM
#endif
#if APP_MEM == SYSTEM_OSAL_STATIC_MEM
    #define APP_MEM_POOL_SIZE (1024 * 1024)
    #define APP_MEM_BLOCK_SIZE 1024
    #define APP_MEM_BLOCK_COUNT (APP_MEM_POOL_SIZE / APP_MEM_BLOCK_SIZE)
    #define APP_MEM_ALIGNMENT 8
#endif
// MUTEX
#define APP_MUTEX SYSTEM_OSAL_MUTEX_ENABLE
#if APP_MUTEX == SYSTEM_OSAL_MUTEX_ENABLE
    #define APP_MUTEX_TYPE_RECURSIVE SYSTEM_OSAL_MUTEX_TYPE_RECURSIVE
    #define APP_MUTEX_LOCK_TIMEOUT_MS 0
#endif
// EPOLL
#if APP_OS == OS_LINUX
    #define APP_EPOLL SYSTEM_OSAL_EPOLL_ENABLE
#endif
// SEMAPHORE
#if APP_OS != OS_LINUX
    #define APP_SEMAPHORE SYSTEM_OSAL_SEMAPHORE_ENABLE
#endif
#if APP_SEMAPHORE
    #define APP_SEMAPHORE_TYPE SYSTEM_OSAL_SEMAPHORE_TYPE_COUNTING
    #define APP_SEMAPHORE_MAX_COUNT 2024
    #define APP_SEMAPHORE_TIMEOUT_MS 0
#endif


#endif
