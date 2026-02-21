#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
#include "core/feature/osal.h"

enum{
    serviceRendering3dSyncDrawFrame = objSyncBegin,
};
enum{
    serviceRendering3dStateXXX = objStateBegin,
};

typedef struct serviceRendering3d{
    objectState objState;
    osalMutex objMutex;
} serviceRendering3d;

int serviceRendering3dOpen(void);
int serviceRendering3dClose(void);
int serviceRendering3dSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
