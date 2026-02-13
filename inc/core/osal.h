#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app/appCommon.h"
// Time / Tick
int osalGetTimeMs(void);
int osalGetTick(void);
void osalDelayMs(int);
void osalDelayTick(int);
#if APP_OS == OS_LINUX
int64_t osalGetTimeUs(void);
int64_t osalGetTiemNs(void);
void osalDelayUs(int);
void osalGetDate(char*, size_t);
#endif
// Timer

// Memory
int osalMalloc(void**, size_t);
int osalFree(void*);
// Thread

// Mutex

// Semaphore

#ifdef __cplusplus
}
#endif
