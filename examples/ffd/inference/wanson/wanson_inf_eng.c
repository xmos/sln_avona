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
#include "inference_engine.h"
#include "rtos_swmem.h"
#include "ssd1306_rtos_support.h"
#include "wanson_inf_eng.h"
#include "wanson_api.h"
#include "xcore_device_memory.h"
#include "gpio_ctrl/leds.h"

#if appconfLOW_POWER_ENABLED
#include "power/power_state.h"
#include "power/power_control.h"
#endif

#if ON_TILE(INFERENCE_TILE_NO)

#define WANSON_SAMPLES_PER_INFERENCE    (2 * appconfINFERENCE_SAMPLE_BLOCK_LENGTH)

typedef enum inference_state {
    STATE_EXPECTING_WAKEWORD,
    STATE_EXPECTING_COMMAND,
    STATE_PROCESSING_COMMAND
} inference_state_t;

typedef enum inference_power_state {
    STATE_REQUESTING_LOW_POWER,
    STATE_ENTERING_LOW_POWER,
    STATE_ENTERED_LOW_POWER,
    STATE_EXITING_LOW_POWER,
    STATE_EXITED_LOW_POWER
} inference_power_state_t;

enum timeout_event {
    TIMEOUT_EVENT_NONE = 0,
    TIMEOUT_EVENT_INTENT = 1,
    TIMEOUT_EVENT_FULL_POWER = 2
};

static inference_state_t inference_state;

#if appconfLOW_POWER_ENABLED
static inference_power_state_t inference_power_state;
static uint8_t requested_full_power;
#endif

static uint32_t timeout_event = TIMEOUT_EVENT_NONE;

static void vInferenceTimerCallback(TimerHandle_t pxTimer);
static void receive_audio_frames(StreamBufferHandle_t input_queue, int32_t *buf,
                                 int16_t *buf_short, size_t *buf_short_index);
static void timeout_event_handler(TimerHandle_t pxTimer);
static void hold_inf_state(TimerHandle_t pxTimer);

#if appconfLOW_POWER_ENABLED

static void hold_full_power(TimerHandle_t pxTimer);
static uint8_t low_power_handler(TimerHandle_t pxTimer, int32_t *buf,
                                 int16_t *buf_short, size_t *buf_short_index);
static void proc_keyword_wait_for_completion(void);

#endif

static void vInferenceTimerCallback(TimerHandle_t pxTimer)
{
    if (inference_state == STATE_EXPECTING_WAKEWORD) {
#if appconfLOW_POWER_ENABLED
        timeout_event |= TIMEOUT_EVENT_FULL_POWER;
#endif
    } else if ((inference_state == STATE_EXPECTING_COMMAND) ||
               (inference_state == STATE_PROCESSING_COMMAND)) {
        timeout_event |= TIMEOUT_EVENT_INTENT;
    }
}

static void receive_audio_frames(StreamBufferHandle_t input_queue, int32_t *buf,
                                 int16_t *buf_short, size_t *buf_short_index)
{
    uint8_t *buf_ptr = (uint8_t*)buf;
    size_t buf_len = appconfINFERENCE_SAMPLE_BLOCK_LENGTH * sizeof(int32_t);

    do {
        size_t bytes_rxed = xStreamBufferReceive(input_queue,
                                                 buf_ptr,
                                                 buf_len,
                                                 portMAX_DELAY);
        buf_len -= bytes_rxed;
        buf_ptr += bytes_rxed;
    } while (buf_len > 0);

    for (int i = 0; i < appconfINFERENCE_SAMPLE_BLOCK_LENGTH; i++) {
        buf_short[(*buf_short_index)++] = buf[i] >> 16;
    }
}

static void timeout_event_handler(TimerHandle_t pxTimer)
{
    if (timeout_event & TIMEOUT_EVENT_INTENT) {
        timeout_event &= ~TIMEOUT_EVENT_INTENT;
        // Playback "stop listening for command" sound (50)
        wanson_engine_proc_keyword_result(NULL, 50);
        led_indicate_waiting();
        inference_state = STATE_EXPECTING_WAKEWORD;
#if appconfLOW_POWER_ENABLED
        /* Restart the timer for the "hold" full power period. If the device,
         * remains in STATE_EXPECTING_WAKEWORD for this period of time, this
         * will result in the assertion of TIMEOUT_EVENT_FULL_POWER which may
         * request the device to enter low power. */
        hold_full_power(pxTimer);
    } else if (timeout_event & TIMEOUT_EVENT_FULL_POWER) {
        timeout_event &= ~TIMEOUT_EVENT_FULL_POWER;
        /* Determine if the tile should request low power or extend full power
         * operation based on whether there are more keywords to process. */
        if (inference_engine_low_power_ready()) {
            inference_power_state = STATE_REQUESTING_LOW_POWER;
            power_control_req_low_power();
        } else {
            hold_full_power(pxTimer);
        }
#endif
    }
}

static void hold_inf_state(TimerHandle_t pxTimer)
{
    xTimerStop(pxTimer, 0);
    xTimerChangePeriod(pxTimer, pdMS_TO_TICKS(appconfINFERENCE_RESET_DELAY_MS), 0);
    timeout_event = TIMEOUT_EVENT_NONE;
    xTimerReset(pxTimer, 0);
}

#if appconfLOW_POWER_ENABLED

static void proc_keyword_wait_for_completion(void)
{
    const uint32_t bits_to_clear_on_entry = 0x00000000UL;
    const uint32_t bits_to_clear_on_exit = 0xFFFFFFFFUL;
    uint32_t notif_value;

    if (inference_engine_keyword_queue_count()) {
        xTaskNotifyWait(bits_to_clear_on_entry,
                        bits_to_clear_on_exit,
                        &notif_value,
                        portMAX_DELAY);
    }
}

static void hold_full_power(TimerHandle_t pxTimer)
{
    xTimerStop(pxTimer, 0);
    xTimerChangePeriod(pxTimer, pdMS_TO_TICKS(appconfPOWER_FULL_HOLD_DURATION), 0);
    timeout_event = TIMEOUT_EVENT_NONE;
    xTimerReset(pxTimer, 0);
}

static uint8_t low_power_handler(TimerHandle_t pxTimer, int32_t *buf,
                                 int16_t *buf_short, size_t *buf_short_index)
{
    uint8_t low_power = 0;

    switch (inference_power_state) {
    case STATE_REQUESTING_LOW_POWER:
        low_power = 1;
        // Wait here until other tile accepts/rejects the request.
        if (requested_full_power) {
            requested_full_power = 0;
            // Aborting low power transition.
            inference_power_state = STATE_EXITING_LOW_POWER;
        }
        break;
    case STATE_ENTERING_LOW_POWER:
        /* Reset the tasks internal buffers. The keyword/audio buffers are
         * to be reset after the the power control takes driver locks. */
        memset(buf, 0, appconfINFERENCE_SAMPLE_BLOCK_LENGTH);
        memset(buf_short, 0, WANSON_SAMPLES_PER_INFERENCE);
        *buf_short_index = 0;

        proc_keyword_wait_for_completion();
        inference_power_state = STATE_ENTERED_LOW_POWER;
        break;
    case STATE_ENTERED_LOW_POWER:
        low_power = 1;
        if (requested_full_power) {
            requested_full_power = 0;
            inference_power_state = STATE_EXITING_LOW_POWER;
        }
        break;
    case STATE_EXITING_LOW_POWER:
        hold_full_power(pxTimer);
        inference_power_state = STATE_EXITED_LOW_POWER;
        break;
    case STATE_EXITED_LOW_POWER:
    default:
        break;
    }

    return low_power;
}

void wanson_engine_full_power_request(void)
{
    requested_full_power = 1;
}

void wanson_engine_low_power_accept(void)
{
    // The request has been accepted proceed with finalizing low power transition.
    inference_power_state = STATE_ENTERING_LOW_POWER;
}

#endif /* appconfLOW_POWER_ENABLED */

#pragma stackfunction 1500
void wanson_engine_task(void *args)
{
    assert(WANSON_SAMPLES_PER_INFERENCE == 480); // Wanson ASR engine expects 480 samples per inference

    inference_state = STATE_EXPECTING_WAKEWORD;

#if appconfLOW_POWER_ENABLED
    /* This state will trigger the start of the full power timer needed for
     * low power logic to behave correctly at startup. */
    inference_power_state = STATE_EXITING_LOW_POWER;
    requested_full_power = 0;
#endif

    // NOTE: The Wanson model uses the .SwMem_data attribute but no SwMem event handling code is required.
    //       This may cause xflash to whine if the compiler optimizes out the __swmem_address symbol.
    //       To work around this, we simply need to init the swmem.
    rtos_swmem_init(0);

    size_t model_file_size;
    model_file_size = model_file_init();
    if (model_file_size == 0) {
        rtos_printf("ERROR: Failed to load model file\n");
        vTaskDelete(NULL);
    }

    rtos_printf("Wanson init\n");
    Wanson_ASR_Init();
    rtos_printf("Wanson init done\n");

    StreamBufferHandle_t input_queue = (StreamBufferHandle_t)args;
    TimerHandle_t inf_eng_tmr = xTimerCreate(
        "inf_eng_tmr",
        pdMS_TO_TICKS(appconfINFERENCE_RESET_DELAY_MS),
        pdFALSE,
        NULL,
        vInferenceTimerCallback);

    int32_t buf[appconfINFERENCE_SAMPLE_BLOCK_LENGTH] = {0};
    int16_t buf_short[WANSON_SAMPLES_PER_INFERENCE] = {0};

    /* Perform any initialization here */
#if 1   // domain doesn't do anything right now, 0 is both wakeup and asr
    rtos_printf("Wanson reset for wakeup\n");
    int ret = Wanson_ASR_Reset(0);
#else
    rtos_printf("Wanson reset for asr\n");
    int ret = Wanson_ASR_Reset(1);
#endif
    rtos_printf("Wanson reset ret: %d\n", ret);

    /* Alert other tile to start the audio pipeline */
    int dummy = 0;
    rtos_intertile_tx(intertile_ctx, appconfWANSON_READY_SYNC_PORT, &dummy, sizeof(dummy));

    char *text_ptr = NULL;
    int id = 0;
    size_t buf_short_index = 0;

    while (1)
    {
        timeout_event_handler(inf_eng_tmr);

    #if appconfLOW_POWER_ENABLED
        if (low_power_handler(inf_eng_tmr, buf, buf_short, &buf_short_index)) {
            // Low power, processing stopped.
            continue;
        }
    #endif

        receive_audio_frames(input_queue, buf, buf_short, &buf_short_index);

        if (buf_short_index < WANSON_SAMPLES_PER_INFERENCE)
            continue;

        buf_short_index = 0; // reset the offset into the buffer of int16s.
                             // Note, we do not need to overlap the window of samples.
                             // This is handled in the Wanson ASR engine.

        /* Perform inference here */
        ret = Wanson_ASR_Recog(buf_short, WANSON_SAMPLES_PER_INFERENCE, (const char **)&text_ptr, &id);

        if (ret == 0) {
            // No keyword detected.
            continue;
        } else if (ret < 0) {
            debug_printf("Wanson recog ret: %d\n", ret);
            continue;
        }

    #if appconfINFERENCE_RAW_OUTPUT
    #if appconfLOW_POWER_ENABLED
        hold_inf_state(inf_eng_tmr);
    #endif
        wanson_engine_proc_keyword_result((const char **)&text_ptr, id);
    #else
        if (inference_state == STATE_EXPECTING_WAKEWORD && IS_WAKEWORD(id)) {
            led_indicate_listening();
            hold_inf_state(inf_eng_tmr);
            wanson_engine_proc_keyword_result((const char **)&text_ptr, id);
            inference_state = STATE_EXPECTING_COMMAND;
        } else if (inference_state == STATE_EXPECTING_COMMAND && IS_COMMAND(id)) {
            hold_inf_state(inf_eng_tmr);
            wanson_engine_proc_keyword_result((const char **)&text_ptr, id);
            inference_state = STATE_PROCESSING_COMMAND;
        } else if (inference_state == STATE_EXPECTING_COMMAND && IS_WAKEWORD(id)) {
            hold_inf_state(inf_eng_tmr);
            wanson_engine_proc_keyword_result((const char **)&text_ptr, id);
            // remain in STATE_EXPECTING_COMMAND state
        } else if (inference_state == STATE_PROCESSING_COMMAND && IS_WAKEWORD(id)) {
            hold_inf_state(inf_eng_tmr);
            wanson_engine_proc_keyword_result((const char **)&text_ptr, id);
            inference_state = STATE_EXPECTING_COMMAND;
        } else if (inference_state == STATE_PROCESSING_COMMAND && IS_COMMAND(id)) {
            hold_inf_state(inf_eng_tmr);
            wanson_engine_proc_keyword_result((const char **)&text_ptr, id);
            // remain in STATE_PROCESSING_COMMAND state
        }
    #endif
    }
}

#endif /* ON_TILE(INFERENCE_TILE_NO) */
