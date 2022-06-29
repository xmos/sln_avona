// Copyright (c) 2022 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef APP_CONF_H_
#define APP_CONF_H_

/* Intertile port settings */
#define appconfUSB_AUDIO_PORT                     0
#define appconfGPIO_T0_RPC_PORT                   1
#define appconfGPIO_T1_RPC_PORT                   2
#define appconfINTENT_MODEL_RUNNER_SAMPLES_PORT   3
#define appconfI2C_MASTER_RPC_PORT                4

#define appconfWANSON_READY_SYNC_PORT             16

/* Application tile specifiers */
#include "platform/driver_instances.h"
#define AUDIO_PIPELINE_TILE_NO  MICARRAY_TILE_NO
#define INFERENCE_TILE_NO       FLASH_TILE_NO

/* Audio Pipeline Configuration */
#define appconfAUDIO_CLOCK_FREQUENCY            MIC_ARRAY_CONFIG_MCLK_FREQ
#define appconfPDM_CLOCK_FREQUENCY              MIC_ARRAY_CONFIG_PDM_FREQ
#define appconfAUDIO_PIPELINE_SAMPLE_RATE       16000
#define appconfAUDIO_PIPELINE_CHANNELS          MIC_ARRAY_CONFIG_MIC_COUNT
/* If in channel sample format, appconfAUDIO_PIPELINE_FRAME_ADVANCE == MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME*/
#define appconfAUDIO_PIPELINE_FRAME_ADVANCE     MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME

/* Intent Engine Configuration */
#define appconfINFERENCE_FRAME_BUFFER_MULT      8       /* total buffer size is this value * MIC_ARRAY_CONFIG_SAMPLES_PER_FRAME */
#define appconfINFERENCE_FRAMES_PER_INFERENCE   240
#define appconfINFERENCE_RESET_DELAY_MS         5000

#ifndef appconfINFERENCE_ENABLED
#define appconfINFERENCE_ENABLED   1
#endif

#ifndef appconfINFERENCE_RAW_OUTPUT
#define appconfINFERENCE_RAW_OUTPUT   0
#endif

#ifndef appconfINFERENCE_I2C_OUTPUT_ENABLED
#define appconfINFERENCE_I2C_OUTPUT_ENABLED   0
#endif

#ifndef appconfINFERENCE_I2C_OUTPUT_DEVICE_ADDR
#define appconfINFERENCE_I2C_OUTPUT_DEVICE_ADDR 0x01
#endif

#ifndef appconfINFERENCE_USB_OUTPUT_ENABLED
#define appconfINFERENCE_USB_OUTPUT_ENABLED   0
#endif

#ifndef appconfSSD1306_DISPLAY_ENABLED
#define appconfSSD1306_DISPLAY_ENABLED   1
#endif

#ifndef appconfI2S_ENABLED
#define appconfI2S_ENABLED   0
#endif

#ifndef appconfAUDIO_PIPELINE_SKIP_IC_AND_VAD
#define appconfAUDIO_PIPELINE_SKIP_IC_AND_VAD   0
#endif

#ifndef appconfAUDIO_PIPELINE_SKIP_NS
#define appconfAUDIO_PIPELINE_SKIP_NS   0
#endif

#ifndef appconfAUDIO_PIPELINE_SKIP_AGC
#define appconfAUDIO_PIPELINE_SKIP_AGC   0
#endif

#ifndef appconfI2S_AUDIO_SAMPLE_RATE
#define appconfI2S_AUDIO_SAMPLE_RATE appconfAUDIO_PIPELINE_SAMPLE_RATE
#endif

#ifndef appconfUSB_ENABLED
#define appconfUSB_ENABLED   0
#endif

#ifndef appconfUSB_AUDIO_SAMPLE_RATE
#define appconfUSB_AUDIO_SAMPLE_RATE appconfAUDIO_PIPELINE_SAMPLE_RATE
#endif

#ifndef appconfUSB_AUDIO_ENABLED
#define appconfUSB_AUDIO_ENABLED 0
#endif

#define appconfUSB_AUDIO_RELEASE   0
#define appconfUSB_AUDIO_TESTING   1
#ifndef appconfUSB_AUDIO_MODE
#define appconfUSB_AUDIO_MODE      appconfUSB_AUDIO_RELEASE
#endif

#define appconfMIC_SRC_MICS        0
#define appconfMIC_SRC_USB         1
#ifndef appconfMIC_SRC_DEFAULT
#define appconfMIC_SRC_DEFAULT     appconfMIC_SRC_MICS
#endif


/* I/O and interrupt cores for Tile 0 */
/* Note, USB and SPI are mutually exclusive */
#define appconfXUD_IO_CORE                      1 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfSPI_IO_CORE                      1 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfUSB_INTERRUPT_CORE               2 /* Must be kept off I/O cores. Best kept off core 0 with the tick ISR. */
#define appconfUSB_SOF_INTERRUPT_CORE           3 /* Must be kept off I/O cores. Best kept off cores with other ISRs. */
#define appconfSPI_INTERRUPT_CORE               2 /* Must be kept off I/O cores. */

/* I/O and interrupt cores for Tile 1 */
#define appconfPDM_MIC_IO_CORE                  1 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfI2S_IO_CORE                      2 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfPDM_MIC_INTERRUPT_CORE           4 /* Must be kept off I/O cores. Best kept off core 0 with the tick ISR. */
#define appconfI2S_INTERRUPT_CORE               5 /* Must be kept off I/O cores. Best kept off core 0 with the tick ISR. */

/* Task Priorities */
#define appconfSTARTUP_TASK_PRIORITY                (configMAX_PRIORITIES / 2 + 5)
#define appconfAUDIO_PIPELINE_TASK_PRIORITY    	    (configMAX_PRIORITIES / 2)
#define appconfINFERENCE_MODEL_RUNNER_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define appconfINFERENCE_HMI_TASK_PRIORITY          (configMAX_PRIORITIES / 2)
#define appconfGPIO_RPC_PRIORITY                    (configMAX_PRIORITIES / 2)
#define appconfGPIO_TASK_PRIORITY                   (configMAX_PRIORITIES / 2 + 2)
#define appconfI2C_TASK_PRIORITY                    (configMAX_PRIORITIES / 2 + 2)
#define appconfI2C_MASTER_RPC_PRIORITY              (configMAX_PRIORITIES / 2)
#define appconfUSB_MGR_TASK_PRIORITY                (configMAX_PRIORITIES / 2 + 1)
#define appconfUSB_AUDIO_TASK_PRIORITY              (configMAX_PRIORITIES / 2 + 1)
#define appconfSPI_TASK_PRIORITY                    (configMAX_PRIORITIES / 2 + 1)
#define appconfQSPI_FLASH_TASK_PRIORITY             (configMAX_PRIORITIES - 1)
#define appconfSSD1306_TASK_PRIORITY                (configMAX_PRIORITIES / 2 - 1)
#define appconfLED_HEARTBEAT_TASK_PRIORITY          (configMAX_PRIORITIES / 2 - 1)

#include "app_conf_check.h"

#endif /* APP_CONF_H_ */
