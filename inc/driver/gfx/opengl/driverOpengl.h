#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-18
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    driverGfxOpenglSyncXXX = objSyncBegin,
};
enum{
    driverGfxOpenglStateXXX = objStateBegin,
};

typedef struct driverGfxOpengl{
    objectState objState;
    osalMutex objMutex;
} driverGfxOpengl;

int driverGfxOpenglOpen(void);
int driverGfxOpenglClose(void);
int driverGfxOpenglSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
