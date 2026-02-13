#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-13
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "core/system.h"

typedef struct{
    uint16_t eventId;
    void *payload;
    size_t payloadSize;
} asyncPacket;



#ifdef __cplusplus
}
#endif
