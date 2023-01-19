// Copyright (c) 2022 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef WANSON_INF_ENG_H_
#define WANSON_INF_ENG_H_

#include <stdint.h>
#include "rtos_intertile.h"

#define IS_WAKEWORD(id)   (id <= 2)
#define IS_COMMAND(id)    (id > 2)

void wanson_engine_task(void *args);

void wanson_engine_task_create(unsigned priority);
void wanson_engine_stream_buf_reset(void);
void wanson_engine_samples_send_local(
        size_t frame_count,
        int32_t *processed_audio_frame);

void wanson_engine_intertile_task_create(uint32_t priority);
void wanson_engine_samples_send_remote(
        rtos_intertile_t *intertile_ctx,
        size_t frame_count,
        int32_t *processed_audio_frame);

void wanson_engine_proc_keyword_result(const char **text, int id);

void wanson_engine_full_power_request(void);
void wanson_engine_low_power_accept(void);

#endif /* WANSON_INF_ENG_H_ */
