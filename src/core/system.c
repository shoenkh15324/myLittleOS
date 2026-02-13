/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-10
 ******************************************************************************/
#include "core/system.h"

int systemOpen(void){
    logInfo("%s Open / v%d.%d.%d / Author: %s", SYSTEM_NAME, SYSTEM_VERSION_MAJOR, SYSTEM_VERSION_MINOR, SYSTEM_VERSION_PATCH, SYSTEM_AUTHOR);
    return 0;
}
