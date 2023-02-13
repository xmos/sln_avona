// Copyright (c) 2022 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

/* STD headers */
#include <platform.h>
#include <xs1.h>
#include <xcore/hwtimer.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

/* App headers */
#include "app_conf.h"
#include "platform/driver_instances.h"
#include "intent_engine/intent_engine.h"
#include "ssd1306_rtos_support.h"

void intent_engine_proc_keyword_result(const char **text, int id)
{
    rtos_printf("KEYWORD: 0x%x, %s\n", id, (char*)*text);
#if appconfSSD1306_DISPLAY_ENABLED
    // some temporary fixes to the strings returned
    switch (id) {
        case 3:
            // fix capital "On"
            ssd1306_display_ascii_to_bitmap("Switch on the TV\0");
            break;
        case 15:
            // fix lower case "speed"
            // fix word wrapping
            ssd1306_display_ascii_to_bitmap("Speed up the   fan\0");
            break;
        case 16:
            // fix lower case "slow"
            ssd1306_display_ascii_to_bitmap("Slow down the fan\0");
            break;
        case 17:
            // fix lower case "set"
            // fix word wrapping
            ssd1306_display_ascii_to_bitmap("Set higher    temperature\0");
            break;
        case 18:
            // fix lower case "set"
            // fix word wrapping
            ssd1306_display_ascii_to_bitmap("Set lower     temperature\0");
            break;
        default:
            ssd1306_display_ascii_to_bitmap((char *)*text);
    }
#endif
#if appconfINTENT_I2C_OUTPUT_ENABLED
    i2c_res_t ret;
    uint32_t buf = id;
    size_t sent = 0;

    ret = rtos_i2c_master_write(
        i2c_master_ctx,
        appconfINTENT_I2C_OUTPUT_DEVICE_ADDR,
        (uint8_t*)&buf,
        sizeof(uint32_t),
        &sent,
        1
    );

    if (ret != I2C_ACK) {
        rtos_printf("I2C inference output was not acknowledged\n\tSent %d bytes\n", sent);
    }
#endif
#if appconfINTENT_UART_OUTPUT_ENABLED && (UART_TILE_NO == ASR_TILE_NO)
    uint32_t buf_uart = id;
    rtos_uart_tx_write(uart_tx_ctx, (uint8_t*)&buf_uart, sizeof(uint32_t));
#endif
}
