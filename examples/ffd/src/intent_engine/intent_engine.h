// Copyright (c) 2022 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef INTENT_ENGINE_H_
#define INTENT_ENGINE_H_

#include <stdint.h>
#include <stddef.h>

#include "asr.h"
#include "rtos_intertile.h"

int32_t intent_engine_create(uint32_t priority, void *args);

void intent_engine_task(void *args);
void intent_engine_task_create(unsigned priority);
void intent_engine_intertile_task_create(uint32_t priority);

int32_t intent_engine_sample_push(int32_t *buf, size_t frames);
void intent_engine_samples_send_local(
        size_t frame_count,
        int32_t *processed_audio_frame);
void intent_engine_samples_send_remote(
        rtos_intertile_t *intertile,
        size_t frame_count,
        int32_t *processed_audio_frame);


int32_t intent_engine_keyword_queue_count(void);
void intent_engine_keyword_queue_complete(void);
void intent_engine_stream_buf_reset(void);
void intent_engine_play_response(int wav_id);
void intent_engine_process_asr_result(asr_keyword_t keyword, asr_command_t command);

uint8_t intent_engine_low_power_ready(void);
void intent_engine_low_power_reset(void);
void intent_engine_full_power_request(void);
void intent_engine_low_power_accept(void);

#endif /* INTENT_ENGINE_H_ */
