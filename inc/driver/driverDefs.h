#pragma once
/******************************************************************************
 *  Author : Minkyu Kim
 *  Created: 2026-02-12
 ******************************************************************************/

typedef enum{
    gfxPrimitiveTypePoints = 0,
    gfxPrimitiveTypeLines,
    gfxPrimitiveTypeTriangles,
} gfxPrimitiveType;

/* [UART] */
#define DRIVER_UART_NONE 0
#define DRIVER_UART_STM32_HAL 1

/* [I2C] */
#define DRIVER_I2C_NONE 0
#define DRIVER_I2C_STM32_HAL 1

/* [SPI] */
#define DRIVER_SPI_NONE 0
#define DRIVER_SPI_STM32_HAL 1

/* [PLATFOMR] */
#define DRIVER_PLATFORM_LINUX 1
#define DRIVER_PLATFORM_WIN32 2

/* [GFX] */
#define DRIVER_GFX_OPENGL 1
#define DRIVER_GFX_BGFX 2

/* [PHYSICS BACKEND] */
#define DRIVER_PHYSICS_BACKEND_JOLT 1

