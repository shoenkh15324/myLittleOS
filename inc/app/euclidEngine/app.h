#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "core/system.h"
#include "core/feature/active.h"

typedef enum{
    appMainStateXXX = objStateBegin,
} appMainState;

typedef struct appMain{
    activeObject actor;
} appMain;

int appClose(void);
int appOpen(void);
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
