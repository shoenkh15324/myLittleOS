#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-16
 ******************************************************************************/
/******************************************************************************
 *  Central App Configuration Selector
 * 
 *  This header is the single point for app configuration.
 *  The appropriate app config is selected based on APP_TARGET definition
 *  passed from CMake or compiler -D flag.
 * 
 *  Usage:
 *    - With CMake: cmake -DAPP_TARGET=sample ..
 *    - Direct:      gcc -DAPP_SAMPLE=1 ...
 *    - Default:     Falls back to APP_SAMPLE if nothing defined
 ******************************************************************************/

#if defined(APP_SAMPLE)
    #include "app/sample/appConfig.h"
#elif defined(APP_ENGINE_2D)
    #include "app/engine2D/appConfig.h"
#elif defined(APP_EUCLID_ENGINE)
    #include "app/euclidEngine/appConfig.h"
#else
    #include "app/sample/appConfig.h"
#endif
