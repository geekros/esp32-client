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

#ifndef OPUS_RESAMPLER_H
#define OPUS_RESAMPLER_H

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
#include "resampler_silk.h"

// Include Silk resampler headers
typedef struct opus_resampler
{
    silk_resampler_state_struct state;
    int input_sample_rate;
    int output_sample_rate;
} opus_resampler_t;

// Function to create and initialize an Opus resampler
opus_resampler_t *opus_resampler_create(void);

// Function to destroy and free an Opus resampler
void opus_resampler_destroy(opus_resampler_t *res);

// Function to configure the Opus resampler with input and output sample rates
bool opus_resampler_configure(opus_resampler_t *res, int input_sample_rate, int output_sample_rate);

// Function to process audio data through the Opus resampler
bool opus_resampler_process(opus_resampler_t *res, const int16_t *input, int input_samples, int16_t *output);

// Function to get the number of output samples for a given number of input samples
int opus_resampler_get_output_samples(opus_resampler_t *res, int input_samples);

#endif