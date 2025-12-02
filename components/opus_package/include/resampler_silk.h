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

#ifndef RESAMPLER_SILK_H
#define RESAMPLER_SILK_H

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
#include "resampler_structs.h"

// Declare Silk resampler initialization function
extern "C" opus_int silk_resampler_init(
    silk_resampler_state_struct *S,
    opus_int32 Fs_Hz_in,
    opus_int32 Fs_Hz_out,
    opus_int forEnc);

// Declare Silk resampler function
extern "C" opus_int silk_resampler(
    silk_resampler_state_struct *S,
    opus_int16 out[],
    const opus_int16 in[],
    opus_int32 inLen);

#endif
