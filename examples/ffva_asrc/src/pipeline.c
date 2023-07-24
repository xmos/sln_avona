// Copyright (c) 2022 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

/* STD headers */
#include <string.h>
#include <stdint.h>
#include <xcore/hwtimer.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "stream_buffer.h"

/* Library headers */
#include "generic_pipeline.h"

/* App headers */
#include "app_conf.h"
#include "audio_pipeline.h"
#include "audio_pipeline_dsp.h"
#include "platform/driver_instances.h"

#if appconfAUDIO_PIPELINE_FRAME_ADVANCE != 240
#error This pipeline is only configured for 240 frame advance
#endif

static agc_stage_ctx_t DWORD_ALIGNED agc_stage_state = {};

static void stage_agc(frame_data_t *frame_data)
{
#if appconfAUDIO_PIPELINE_SKIP_AGC
#else
    int32_t DWORD_ALIGNED agc_output[appconfAUDIO_PIPELINE_FRAME_ADVANCE];
    configASSERT(AGC_FRAME_ADVANCE == appconfAUDIO_PIPELINE_FRAME_ADVANCE);

    agc_stage_state.md.vnr_flag = frame_data->vnr_pred_flag;
    agc_stage_state.md.aec_ref_power = frame_data->max_ref_energy;
    agc_stage_state.md.aec_corr_factor = frame_data->aec_corr_factor;

    agc_process_frame(
            &agc_stage_state.state,
            agc_output,
            frame_data->samples[0],
            &agc_stage_state.md);
    memcpy(frame_data->samples, agc_output, appconfAUDIO_PIPELINE_FRAME_ADVANCE * sizeof(int32_t));
#endif
}

void audio_pipeline_input(void *input_app_data,
                        int32_t **input_audio_frames,
                        size_t ch_count,
                        size_t frame_count)
{
    (void) input_app_data;
    int32_t **mic_ptr = (int32_t **)(input_audio_frames + (2 * frame_count));

    static int flushed;
    while (!flushed) {
        size_t received;
        received = rtos_mic_array_rx(mic_array_ctx,
                                     mic_ptr,
                                     frame_count,
                                     0);
        if (received == 0) {
            rtos_mic_array_rx(mic_array_ctx,
                              mic_ptr,
                              frame_count,
                              portMAX_DELAY);
            flushed = 1;
        }
    }

    /*
     * NOTE: ALWAYS receive the next frame from the PDM mics,
     * even if USB is the current mic source. The controls the
     * timing since usb_audio_recv() does not block and will
     * receive all zeros if no frame is available yet.
     */
    rtos_mic_array_rx(mic_array_ctx,
                      mic_ptr,
                      frame_count,
                      portMAX_DELAY);
#if appconfI2S_ENABLED
    /* This shouldn't need to block given it shares a clock with the PDM mics */

    xassert(frame_count == appconfAUDIO_PIPELINE_FRAME_ADVANCE);
    /* I2S provides sample channel format */
    int32_t tmp[appconfAUDIO_PIPELINE_FRAME_ADVANCE][appconfAUDIO_PIPELINE_CHANNELS];
    int32_t *tmpptr = (int32_t *)input_audio_frames;

    size_t rx_count =
    rtos_i2s_rx(i2s_ctx,
                (int32_t*) tmp,
                frame_count,
                portMAX_DELAY);
    xassert(rx_count == frame_count);

    for (int i=0; i<frame_count; i++) {
        /* ref is first */
        *(tmpptr + i) = tmp[i][0];
        *(tmpptr + i + frame_count) = tmp[i][1];
    }
#endif
}

static void* audio_pipeline_input_i(void *input_app_data)
{
    frame_data_t *frame_data;

    frame_data = pvPortMalloc(sizeof(frame_data_t));
    memset(frame_data, 0x00, sizeof(frame_data_t));

    //uint32_t start = get_reference_time();
    audio_pipeline_input(input_app_data,
                       (int32_t **)frame_data->aec_reference_audio_samples,
                       4,
                       appconfAUDIO_PIPELINE_FRAME_ADVANCE);

    //uint32_t end = get_reference_time();
    //printuintln(end - start);
    frame_data->vnr_pred_flag = 0;

    memcpy(frame_data->samples, frame_data->mic_samples_passthrough, sizeof(frame_data->samples));

    return frame_data;
}

static int audio_pipeline_output_i(frame_data_t *frame_data,
                                   void *output_app_data)
{
    /* I2S expects sample channel format */
    int32_t tmp[appconfAUDIO_PIPELINE_FRAME_ADVANCE][appconfAUDIO_PIPELINE_CHANNELS];
    int32_t *tmpptr = &frame_data->samples[0][0];
    for (int j=0; j<appconfAUDIO_PIPELINE_FRAME_ADVANCE; j++) {
        /* ASR output is first */
        tmp[j][0] = *(tmpptr+j);
        tmp[j][1] = *(tmpptr+j+appconfAUDIO_PIPELINE_FRAME_ADVANCE);
    }

    rtos_i2s_tx(i2s_ctx,
            (int32_t*) tmp,
            appconfAUDIO_PIPELINE_FRAME_ADVANCE,
            portMAX_DELAY);

    return AUDIO_PIPELINE_FREE_FRAME;
}

static void agc_task(void *args)
{
    for(;;)
    {
        // Get pipeline input
        frame_data_t *frame_data = audio_pipeline_input_i(NULL);

        // Process
        stage_agc(frame_data);

        // Send pipeline output
        //uint32_t start = get_reference_time();
        int ret = audio_pipeline_output_i(frame_data, NULL);
        if(ret == AUDIO_PIPELINE_FREE_FRAME)
        {
            rtos_osal_free(frame_data);
        }
        //uint32_t end = get_reference_time();
        //printuintln(end - start);

        (void) rtos_osal_task_yield();
    }
}

void pipeline_init()
{
#if ON_TILE(1)
    agc_init(&agc_stage_state.state, &AGC_PROFILE_FIXED_GAIN);
    agc_stage_state.state.config.gain = f32_to_float_s32(500);

    // Create the AGC task
    //configSTACK_DEPTH_TYPE stage_stack_size = configMINIMAL_STACK_SIZE + RTOS_THREAD_STACK_SIZE(stage_agc) + RTOS_THREAD_STACK_SIZE(audio_pipeline_input_i) + RTOS_THREAD_STACK_SIZE(audio_pipeline_output_i);

    (void) rtos_osal_thread_create(
        (rtos_osal_thread_t *) NULL,
        (char *) "AGC",
        (rtos_osal_entry_function_t) agc_task,
        (void *) NULL,
        (size_t) RTOS_THREAD_STACK_SIZE(agc_task),
        (unsigned int) appconfAUDIO_PIPELINE_TASK_PRIORITY);
#endif
}
