#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "core/system.h"
#include "core/feature/active.h"

typedef struct{
    activeObject actor;
} appMain;
typedef struct{
    activeObject actor;
} appTest;

int appClose(void);
int appOpen(void);
int appSync(uint16_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
