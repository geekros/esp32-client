/*
Copyright 2025 GEEKROS, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef RESAMPLER_STRUCTS_H
#define RESAMPLER_STRUCTS_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include Opus headers
#include "opus.h"

// Define Silk resampler constants
#define SILK_RESAMPLER_MAX_FIR_ORDER 36
#define SILK_RESAMPLER_MAX_IIR_ORDER 6

typedef struct _silk_resampler_state_struct
{
    opus_int32 sIIR[SILK_RESAMPLER_MAX_IIR_ORDER]; /* this must be the first element of this struct */
    union
    {
        opus_int32 i32[SILK_RESAMPLER_MAX_FIR_ORDER];
        opus_int16 i16[SILK_RESAMPLER_MAX_FIR_ORDER];
    } sFIR;
    opus_int16 delayBuf[48];
    opus_int resampler_function;
    opus_int batchSize;
    opus_int32 invRatio_Q16;
    opus_int FIR_Order;
    opus_int FIR_Fracs;
    opus_int Fs_in_kHz;
    opus_int Fs_out_kHz;
    opus_int inputDelay;
    const opus_int16 *Coefs;
} silk_resampler_state_struct;

#endif