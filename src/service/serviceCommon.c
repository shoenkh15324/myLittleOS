/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-14
 ******************************************************************************/
#include "service/serviceCommon.h"

int serviceCommonClose(void){
    return 0;
}
int serviceCommonOpen(void){
    return 0;
}
int serviceCommonSync(uint16_t sync, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4){
    switch(sync){
        case serviceCommonSyncTimer:
            break;
    }
    return 0;
}
