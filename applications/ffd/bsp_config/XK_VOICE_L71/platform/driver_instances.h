// Copyright (c) 2022 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef DRIVER_INSTANCES_H_
#define DRIVER_INSTANCES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rtos_intertile.h"
#include "rtos_qspi_flash.h"
#include "rtos_gpio.h"
#include "rtos_i2c_master.h"
#include "rtos_i2s.h"

#ifdef __cplusplus
};
#endif

#include "rtos_mic_array.h"

/* Tile specifiers */
#define FLASH_TILE_NO      0
#define MICARRAY_TILE_NO   1

/** TILE 0 Clock Blocks */
#define FLASH_CLKBLK  XS1_CLKBLK_1

/** TILE 1 Clock Blocks */
#define PDM_CLKBLK_1  XS1_CLKBLK_1
#define PDM_CLKBLK_2  XS1_CLKBLK_2
#define MCLK_CLKBLK   XS1_CLKBLK_4

/* Port definitions */
#define PORT_MCLK           PORT_MCLK_IN_OUT
#define PORT_SQI_CS         PORT_SQI_CS_0
#define PORT_SQI_SCLK       PORT_SQI_SCLK_0
#define PORT_SQI_SIO        PORT_SQI_SIO_0

extern rtos_intertile_t *intertile_ctx;
extern rtos_qspi_flash_t *qspi_flash_ctx;
extern rtos_gpio_t *gpio_ctx_t0;
extern rtos_gpio_t *gpio_ctx_t1;
extern rtos_mic_array_t *mic_array_ctx;

#endif /* DRIVER_INSTANCES_H_ */