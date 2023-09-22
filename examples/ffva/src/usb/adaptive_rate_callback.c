// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdbool.h>

#define TOTAL_TAIL_SECONDS 16
#define STORED_PER_SECOND 4

#if __xcore__
#include "tusb_config.h"
#include "app_conf.h"
#include "xmath/xmath.h"
#define EXPECTED_OUT_BYTES_PER_TRANSACTION (CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX * \
                                       CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX * \
                                       appconfUSB_AUDIO_SAMPLE_RATE / 1000)
#define EXPECTED_IN_BYTES_PER_TRANSACTION  (CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * \
                                       CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX * \
                                       appconfUSB_AUDIO_SAMPLE_RATE / 1000)
#else //__xcore__
// If we're compiling this for x86 we're probably testing it - just assume some values
#define EXPECTED_OUT_BYTES_PER_TRANSACTION  128 //16kbps * 16-bit * 4ch
#define EXPECTED_IN_BYTES_PER_TRANSACTION   192 //16kbps * 16-bit * 6ch
#endif //__xcore__

#define TOTAL_STORED (TOTAL_TAIL_SECONDS * STORED_PER_SECOND)
#define REF_CLOCK_TICKS_PER_SECOND 100000000
#define REF_CLOCK_TICKS_PER_STORED_AVG (REF_CLOCK_TICKS_PER_SECOND / STORED_PER_SECOND)
#define NOMINAL_RATE (1 << 31)

#define EXPECTED_OUT_BYTES_PER_BUCKET ((EXPECTED_OUT_BYTES_PER_TRANSACTION * 1000) / STORED_PER_SECOND)
#define EXPECTED_IN_BYTES_PER_BUCKET ((EXPECTED_IN_BYTES_PER_TRANSACTION * 1000) / STORED_PER_SECOND)

bool first_time[2] = {true, true};
volatile static bool data_seen = false;
volatile static bool hold_average = false;
uint32_t expected[2] = {EXPECTED_OUT_BYTES_PER_TRANSACTION, EXPECTED_IN_BYTES_PER_TRANSACTION};
uint32_t bucket_expected[2] = {EXPECTED_OUT_BYTES_PER_BUCKET, EXPECTED_IN_BYTES_PER_BUCKET};

#if __xcore__
uint32_t dsp_math_divide_unsigned(uint32_t dividend, uint32_t divisor, uint32_t q_format )
{
    //h and l hold a 64-bit value
    uint32_t h; uint32_t l;
    uint32_t quotient=0, remainder=0;

    // Create long dividend by shift dividend up q_format positions
    h = dividend >> (32-q_format);
    l = dividend << (q_format);

    // Unsigned Long division
    asm("ldivu %0,%1,%2,%3,%4":"=r"(quotient):"r"(remainder),"r"(h),"r"(l),"r"(divisor));

    return quotient;
}
#else //__xcore__
// If we're compiling this for x86 we're probably testing it - let the compiler work out the ASM for this

uint32_t dsp_math_divide_unsigned(uint32_t dividend, uint32_t divisor, uint32_t q_format )
{
    uint64_t h = (uint64_t)dividend << q_format;
    uint64_t quotient = h / divisor;

    return (uint32_t)quotient;
}
#endif //__xcore__


uint32_t dsp_math_divide_unsigned_64(uint64_t dividend, uint32_t divisor, uint32_t q_format )
{
    uint64_t h = dividend << q_format;
    uint64_t quotient = h / divisor;

    return (uint32_t)quotient;
}

uint32_t sum_array(uint32_t * array_to_sum, uint32_t array_length)
{
    uint32_t acc = 0;
    for (uint32_t i = 0; i < array_length; i++)
    {
        acc += array_to_sum[i];
    }
    return acc;
}

void reset_state()
{
    for (int direction = 0; direction < 2; direction++)
    {
        first_time[direction] = true;
    }
}

float_s32_t float_div(float_s32_t dividend, float_s32_t divisor)
{
    float_s32_t res;// = float_s32_div(dividend, divisor);

    int dividend_hr;
    int divisor_hr;

    asm( "clz %0, %1" : "=r"(dividend_hr) : "r"(dividend.mant) );
    asm( "clz %0, %1" : "=r"(divisor_hr) : "r"(divisor.mant) );

    int dividend_exp = dividend.exp - dividend_hr;
    int divisor_exp = divisor.exp - divisor_hr;

    uint64_t h_dividend = (uint64_t)((uint32_t)dividend.mant) << (dividend_hr);

    uint32_t h_divisor = ((uint32_t)divisor.mant) << (divisor_hr);

    uint32_t lhs = (h_dividend > h_divisor) ? 31 : 32;

    uint64_t quotient = (h_dividend << lhs) / h_divisor;

    res.exp = dividend_exp - divisor_exp - lhs;

    res.mant = (uint32_t)(quotient) ;
    return res;
}

uint32_t float_div_fixed_output_q_format(float_s32_t dividend, float_s32_t divisor, int32_t output_q_format)
{
    int op_q = -output_q_format;
    float_s32_t res = float_div(dividend, divisor);
    uint32_t quotient;
    if(res.exp < op_q)
    {
        int rsh = op_q - res.exp;
        quotient = ((uint32_t)res.mant >> rsh) + (((uint32_t)res.mant >> (rsh-1)) & 0x1);
    }
    else
    {
        int lsh = res.exp - op_q;
        quotient = (uint32_t)res.mant << lsh;
    }
    return quotient;
}

uint32_t determine_USB_audio_rate(uint32_t timestamp,
                                    uint32_t data_length,
                                    uint32_t direction,
                                    bool update
#ifdef DEBUG_ADAPTIVE
                                                ,
                                    uint32_t * debug
#endif
)
{
    static uint32_t data_lengths[2][TOTAL_STORED];
    static uint32_t time_buckets[2][TOTAL_STORED];
    static uint32_t current_data_bucket_size[2];
    static uint32_t first_timestamp[2];
    static bool buckets_full[2];
    static uint32_t times_overflowed[2];
    static uint32_t previous_result[2] = {NOMINAL_RATE, NOMINAL_RATE};
    static float_s32_t expected_data_per_tick = {0, 0};

    //data_length = data_length / (CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX); // Number of samples per channels per transaction

    if (data_seen == false)
    {
        data_seen = true;
    }

    if (hold_average)
    {
        hold_average = false;
        first_timestamp[direction] = timestamp;
        current_data_bucket_size[direction] = 0;
        return previous_result[direction];
    }

    if (first_time[direction])
    {
        first_time[direction] = false;
        first_timestamp[direction] = timestamp;

        // Because we use "first_time" to also reset the rate determinator,
        // reset all the static variables to default.
        current_data_bucket_size[direction] = 0;
        times_overflowed[direction] = 0;
        buckets_full[direction] = false;

        for (int i = 0; i < TOTAL_STORED - STORED_PER_SECOND; i++)
        {
            data_lengths[direction][i] = 0;
            time_buckets[direction][i] = 0;
        }
        // Seed the final second of initialised data with a "perfect" second - should make the start a bit more stable
        for (int i = TOTAL_STORED - STORED_PER_SECOND; i < TOTAL_STORED; i++)
        {
            data_lengths[direction][i] = 0; //bucket_expected[direction];
            time_buckets[direction][i] = 0; //REF_CLOCK_TICKS_PER_STORED_AVG;
        }
        expected_data_per_tick = float_div((float_s32_t){appconfUSB_AUDIO_SAMPLE_RATE * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX, 0}, (float_s32_t){REF_CLOCK_TICKS_PER_SECOND, 0});
        return NOMINAL_RATE;
    }

    if (update)
    {
        current_data_bucket_size[direction] += data_length;
    }

    // total_timespan is always correct regardless of whether the reference clock has overflowed.
    // The point at which it becomes incorrect is the point at which it would overflow - the
    // point at which timestamp == first_timestamp again. This will be at 42.95 seconds of operation.
    // If current_data_bucket_size overflows we have bigger issues, so this case is not guarded.

    uint32_t timespan = timestamp - first_timestamp[direction];
    uint32_t total_data_intermed = current_data_bucket_size[direction] + sum_array(data_lengths[direction], TOTAL_STORED);




    uint64_t total_data = (uint64_t)(total_data_intermed) * 12500;
    uint32_t total_timespan = timespan + sum_array(time_buckets[direction], TOTAL_STORED);

    float_s32_t data_per_tick = float_div((float_s32_t){total_data_intermed, 0}, (float_s32_t){total_timespan, 0});

    uint32_t data_per_sample = dsp_math_divide_unsigned_64(total_data, (total_timespan / 8), 19);

    uint32_t test = float_div_fixed_output_q_format(data_per_tick, expected_data_per_tick, 31);
    uint32_t result = dsp_math_divide_unsigned(data_per_sample, expected[direction], 12);
    result = test;


    if (update && (timespan >= REF_CLOCK_TICKS_PER_STORED_AVG))
    {
        if (buckets_full[direction])
        {
            // We've got enough data for a new bucket - replace the oldest bucket data with this new data
            uint32_t oldest_bucket = times_overflowed[direction] % TOTAL_STORED;

            time_buckets[direction][oldest_bucket] = timespan;
            data_lengths[direction][oldest_bucket] = current_data_bucket_size[direction];

            current_data_bucket_size[direction] = 0;
            first_timestamp[direction] = timestamp;

            times_overflowed[direction]++;
        }
        else
        {
            // We've got enough data for this bucket - save this one and start the next one
            time_buckets[direction][times_overflowed[direction]] = timespan;
            data_lengths[direction][times_overflowed[direction]] = current_data_bucket_size[direction];

            current_data_bucket_size[direction] = 0;
            first_timestamp[direction] = timestamp;

            times_overflowed[direction]++;
            if (times_overflowed[direction] == TOTAL_STORED)
            {
                buckets_full[direction] = true;
            }
        }
    }

#ifdef DEBUG_ADAPTIVE
    #define DEBUG_QUANT 4

    uint32_t debug_out[DEBUG_QUANT] = {result, data_per_sample, total_data_intermed, total_timespan};
    for (int i = 0; i < DEBUG_QUANT; i++)
    {
        debug[i] = debug_out[i];
    }
#endif

    previous_result[direction] = result;
    return result;
}

void sof_toggle()
{
    static uint32_t sof_count;
    if (data_seen)
    {
        sof_count = 0;
        data_seen = false;
    }
    else
    {
        sof_count++;
        if (sof_count > 8 && !hold_average)
        {
            //rtos_printf("holding...........................................\n");
            hold_average = true;
        }
    }
}
