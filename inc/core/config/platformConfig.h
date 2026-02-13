#pragma once
/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-12
 ******************************************************************************/
#include "systemConfig.h"

/* [MCU] */
#define MCU_NONE 0
#define MCU_STM32F103C8T6 1
#define MCU_STM32F411CEU6 2

/* [BOARD] */
#define BOARD_NONE 0

/* [OS] */
#define OS_NONE 0
#define OS_BAREMETAL 1
#define OS_FREERTOS 2
#define OS_ZEPHYR 3
#define OS_LINUX 4

/* [SDK] */
#define SDK_NONE 0
#define SDK_STM32_HAL 1
#define SDK_POSIX 2

/* [PERIPHERAL] */
// UART
#define UART_NONE 0
#define UART_STM32_HAL 1
// I2C
#define I2C_NONE 0
#define I2C_STM32_HAL 1
// SPI
#define SPI_NONE 0
#define SPI_STM32_HAL 1
