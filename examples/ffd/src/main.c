// Copyright (c) 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public License: Version 1

/* System headers */
#include <platform.h>
#include <xs1.h>
#include <xcore/channel.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "queue.h"
#include "event_groups.h"

/* Library headers */
#include "rtos_printf.h"
#include "rtos_clock_control.h"

/* App headers */
#include "app_conf.h"
#include "platform/platform_conf.h"
#include "platform/platform_init.h"
#include "platform/driver_instances.h"
#include "audio_pipeline/audio_pipeline.h"
#include "intent_engine/intent_engine.h"
#include "fs_support.h"
#include "gpio_ctrl/gpi_ctrl.h"
#include "gpio_ctrl/leds.h"
#include "intent_handler/intent_handler.h"
#include "power/power_state.h"
#include "power/power_control.h"
#include "power/low_power_audio_buffer.h"

extern void startup_task(void *arg);
extern void tile_common_init(chanend_t c);

#if ON_TILE(AUDIO_PIPELINE_TILE_NO)
static power_data_t wakeup_app_data;
#endif

__attribute__((weak))
void audio_pipeline_input(void *input_app_data,
                          int32_t **input_audio_frames,
                          size_t ch_count,
                          size_t frame_count)
{
    (void) input_app_data;

    static int flushed;
    while (!flushed) {
        size_t received;
        received = rtos_mic_array_rx(mic_array_ctx,
                                     input_audio_frames,
                                     frame_count,
                                     0);
        if (received == 0) {
            rtos_mic_array_rx(mic_array_ctx,
                              input_audio_frames,
                              frame_count,
                              portMAX_DELAY);
            flushed = 1;
        }
    }

    rtos_mic_array_rx(mic_array_ctx,
                      input_audio_frames,
                      frame_count,
                      portMAX_DELAY);
}

__attribute__((weak))
int audio_pipeline_output(void *output_app_data,
                          int32_t **output_audio_frames,
                          size_t ch_count,
                          size_t frame_count)
{
#if ON_TILE(AUDIO_PIPELINE_TILE_NO) && (appconfLOW_POWER_ENABLED || appconfINTENT_ENABLED)
    power_state_t power_state = POWER_STATE_FULL;
#endif

#if ON_TILE(AUDIO_PIPELINE_TILE_NO) && appconfLOW_POWER_ENABLED
    power_state = power_state_data_add((power_data_t *)output_app_data);
#endif

#if ON_TILE(AUDIO_PIPELINE_TILE_NO) && appconfINTENT_ENABLED
    if (power_state == POWER_STATE_FULL) {
#if LOW_POWER_AUDIO_BUFFER_ENABLED
        const uint32_t max_dequeue_packets = 1;
        const uint32_t max_dequeued_samples = (max_dequeue_packets * appconfAUDIO_PIPELINE_FRAME_ADVANCE);

        if (power_control_state_get() != POWER_STATE_FULL) {
            low_power_audio_buffer_enqueue((int32_t *)output_audio_frames, frame_count);
        } else if (low_power_audio_buffer_dequeue(max_dequeue_packets) == max_dequeued_samples) {
            // Max data has been dequeued, enqueue the newest data.
            low_power_audio_buffer_enqueue((int32_t *)output_audio_frames, frame_count);
        } else // More data can be sent.
#endif // LOW_POWER_AUDIO_BUFFER_ENABLED
        {
            intent_engine_sample_push((int32_t *)output_audio_frames, frame_count);
        }
    }
#if LOW_POWER_AUDIO_BUFFER_ENABLED
    else {
        low_power_audio_buffer_enqueue((int32_t *)output_audio_frames, frame_count);
    }
#endif // LOW_POWER_AUDIO_BUFFER_ENABLED
#endif // ON_TILE(AUDIO_PIPELINE_TILE_NO) && appconfINTENT_ENABLED

    return AUDIO_PIPELINE_FREE_FRAME;
}

void vApplicationMallocFailedHook(void)
{
    rtos_printf("Malloc Failed on tile %d!\n", THIS_XCORE_TILE);
    xassert(0);
    for(;;);
}

static void mem_analysis(void)
{
	for (;;) {
		rtos_printf("Tile[%d]:\n\tMinimum heap free: %d\n\tCurrent heap free: %d\n", THIS_XCORE_TILE, xPortGetMinimumEverFreeHeapSize(), xPortGetFreeHeapSize());
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

__attribute__((weak))
void startup_task(void *arg)
{
    rtos_printf("Startup task running from tile %d on core %d\n", THIS_XCORE_TILE, portGET_CORE_ID());

    platform_start();

#if ON_TILE(1)
    gpio_gpi_init(gpio_ctx_t0);
#endif

#if ON_TILE(FS_TILE_NO)
    rtos_fatfs_init(qspi_flash_ctx);
#endif

#if appconfINTENT_ENABLED && ON_TILE(ASR_TILE_NO)
    QueueHandle_t q_intent = xQueueCreate(appconfINTENT_QUEUE_LEN, sizeof(int32_t));
    intent_handler_create(appconfINTENT_MODEL_RUNNER_TASK_PRIORITY, q_intent);
    intent_engine_create(appconfINTENT_MODEL_RUNNER_TASK_PRIORITY, q_intent);
#endif

#if ON_TILE(0)
    led_task_create(appconfLED_TASK_PRIORITY, NULL);
#endif

#if appconfLOW_POWER_ENABLED
    power_control_task_create(appconfPOWER_CONTROL_TASK_PRIORITY, NULL);
#endif

#if ON_TILE(AUDIO_PIPELINE_TILE_NO)
#if appconfINTENT_ENABLED
    // Wait until the Wanson engine is initialized before we start the
    // audio pipeline.
    {
        int ret = 0;
        rtos_intertile_rx_len(intertile_ctx, appconfINTENT_ENGINE_READY_SYNC_PORT, RTOS_OSAL_WAIT_FOREVER);
        rtos_intertile_rx_data(intertile_ctx, &ret, sizeof(ret));
    }
#endif
    audio_pipeline_init(NULL, &wakeup_app_data);
#if appconfLOW_POWER_ENABLED
    power_state_init();
#endif
#endif

#if appconfLOW_POWER_ENABLED && ON_TILE(AUDIO_PIPELINE_TILE_NO)
    set_local_tile_processor_clk_div(1);
    enable_local_tile_processor_clock_divider();
    set_local_tile_processor_clk_div(appconfLOW_POWER_CONTROL_TILE_CLK_DIV);
#endif

    // mem_analysis();
    vTaskSuspend(NULL);
    while(1){;} /* Trap */
}

void vApplicationMinimalIdleHook(void)
{
    rtos_printf("idle hook on tile %d core %d\n", THIS_XCORE_TILE, rtos_core_id_get());
    asm volatile("waiteu");
}

__attribute__((weak))
void tile_common_init(chanend_t c)
{
    platform_init(c);
    chanend_free(c);

    xTaskCreate((TaskFunction_t) startup_task,
                "startup_task",
                RTOS_THREAD_STACK_SIZE(startup_task),
                NULL,
                appconfSTARTUP_TASK_PRIORITY,
                NULL);

    rtos_printf("start scheduler on tile %d\n", THIS_XCORE_TILE);
    vTaskStartScheduler();
}

#if ON_TILE(0)
void main_tile0(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c0;
    (void) c2;
    (void) c3;

    tile_common_init(c1);
}
#endif

#if ON_TILE(1)
void main_tile1(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c1;
    (void) c2;
    (void) c3;

    tile_common_init(c0);
}
#endif
