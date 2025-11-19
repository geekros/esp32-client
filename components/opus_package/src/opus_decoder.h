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

#ifndef OPUS_DECODER_H
#define OPUS_DECODER_H

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

// Define the opus_decoder_wrapper structure
typedef struct
{
    OpusDecoder *dec;
    int frame_size;
    int sample_rate;
    int channels;
    int duration_ms;
} opus_decoder_wrapper_t;

// Function to create and initialize an Opus decoder wrapper
opus_decoder_wrapper_t *opus_decoder_wrapper_create(int sample_rate, int channels, int duration_ms);

// Function to destroy and free an Opus decoder wrapper
void opus_decoder_wrapper_destroy(opus_decoder_wrapper_t *wrapper);

// Function to decode Opus data into PCM format
int opus_decoder_wrapper_decode(opus_decoder_wrapper_t *wrapper, const uint8_t *opus_data, size_t opus_len, int16_t *output_pcm, size_t max_pcm_samples);

// Function to reset the Opus decoder state
void opus_decoder_wrapper_reset(opus_decoder_wrapper_t *wrapper);

#endif