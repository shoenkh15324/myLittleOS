#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    serviceSampleSyncXXX = objSyncBegin,
};
enum{
    serviceSampleStateXXX = objStateBegin,
};

typedef struct{
    objectState objState;
    osalMutex objMutex;
} serviceSample;

int serviceSampleOpen(void);
int serviceSampleClose(void);
int serviceSampleSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
