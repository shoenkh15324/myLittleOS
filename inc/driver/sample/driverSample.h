#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    driverSampleSyncXXX = objSyncBegin,
};
enum{
    driverSampleStateXXX = objStateBegin,
};

typedef struct{
    objectState objState;
    osalMutex objMutex;
} driverSample;

int driverSampleOpen(void);
int driverSampleClose(void);
int driverSampleSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
