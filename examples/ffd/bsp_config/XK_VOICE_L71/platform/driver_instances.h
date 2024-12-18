// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef DRIVER_INSTANCES_H_
#define DRIVER_INSTANCES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rtos_intertile.h"
#include "rtos_qspi_flash.h"
#include "rtos_gpio.h"
#include "rtos_i2c_master.h"
#include "rtos_i2c_slave.h"
#include "rtos_i2s.h"
#include "rtos_uart_tx.h"

#ifdef __cplusplus
};
#endif

#include "rtos_mic_array.h"

#if appconfRECOVER_MCLK_I2S_APP_PLL
/* Config headers for sw_pll */
#include "sw_pll.h"
#include "fractions_1000ppm.h"
#include "register_setup_1000ppm.h"
#endif

/* Tile specifiers */
#define FLASH_TILE_NO      0
#define I2C_TILE_NO        0
#define I2C_CTRL_TILE_NO   I2C_TILE_NO
#define MICARRAY_TILE_NO   1
#define I2S_TILE_NO        1
#define UART_TILE_NO       0

/** TILE 0 Clock Blocks */
#define FLASH_CLKBLK  XS1_CLKBLK_1

/** TILE 1 Clock Blocks */
#define PDM_CLKBLK_1  XS1_CLKBLK_1
#define PDM_CLKBLK_2  XS1_CLKBLK_2
#define I2S_CLKBLK    XS1_CLKBLK_3
#define MCLK_CLKBLK   XS1_CLKBLK_4

/* Port definitions */
#define PORT_MCLK           PORT_MCLK_IN_OUT
#define PORT_SQI_CS         PORT_SQI_CS_0
#define PORT_SQI_SCLK       PORT_SQI_SCLK_0
#define PORT_SQI_SIO        PORT_SQI_SIO_0
#define PORT_I2S_DAC_DATA   I2S_DATA_IN
#define PORT_I2S_ADC_DATA   I2S_MIC_DATA

extern rtos_intertile_t *intertile_ctx;
extern rtos_intertile_t *intertile_ap_ctx;
extern rtos_qspi_flash_t *qspi_flash_ctx;
extern rtos_gpio_t *gpio_ctx_t0;
extern rtos_gpio_t *gpio_ctx_t1;
extern rtos_mic_array_t *mic_array_ctx;
extern rtos_i2c_master_t *i2c_master_ctx;
extern rtos_i2c_slave_t *i2c_slave_ctx;
extern rtos_i2s_t *i2s_ctx;
extern rtos_uart_tx_t *uart_tx_ctx;

#if appconfRECOVER_MCLK_I2S_APP_PLL
typedef struct {
    port_t p_mclk_count;                    // Used for keeping track of MCLK output for sw_pll
    port_t p_bclk_count;                    // Used for keeping track of BCLK input for sw_pll
    sw_pll_state_t *sw_pll;                 // Pointer to sw_pll state (if used)

}sw_pll_ctx_t;

static sw_pll_state_t sw_pll = {0};

extern sw_pll_ctx_t *sw_pll_ctx;
#endif

#endif /* DRIVER_INSTANCES_H_ */
