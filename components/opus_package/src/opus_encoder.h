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

#ifndef OPUS_ENCODER_H
#define OPUS_ENCODER_H

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

// Include Opus headers
#define MAX_OPUS_PACKET_SIZE 1000

// Define the opus_encoder_wrapper structure
typedef struct
{
    OpusEncoder *enc;
    int sample_rate;
    int channels;
    int duration_ms;
    int frame_size;

    int16_t *in_buffer;
    size_t in_buffer_size;
    size_t in_buffer_capacity;
} opus_encoder_wrapper_t;

// Define the callback type for encoded Opus data
typedef void (*opus_encode_handler_cb_t)(const uint8_t *opus, size_t opus_len, void *cb_ctx);

// Function to create and initialize an Opus encoder wrapper
opus_encoder_wrapper_t *opus_encoder_wrapper_create(int sample_rate, int channels, int duration_ms);

// Function to destroy and free an Opus encoder wrapper
void opus_encoder_wrapper_destroy(opus_encoder_wrapper_t *wrapper);

// Function to encode PCM data into Opus format
void opus_encoder_wrapper_set_dtx(opus_encoder_wrapper_t *wrapper, bool enable);

// Function to set the complexity of the Opus encoder
void opus_encoder_wrapper_set_complexity(opus_encoder_wrapper_t *wrapper, int complexity);

// Function to encode a frame of PCM samples into Opus format
int opus_encoder_wrapper_encode_frame(opus_encoder_wrapper_t *wrapper, const int16_t *pcm_samples, size_t pcm_count, uint8_t *opus_out, size_t opus_max);

// Function to encode PCM data into Opus format using a callback
void opus_encoder_wrapper_encode(opus_encoder_wrapper_t *wrapper, const int16_t *pcm, size_t samples, opus_encode_handler_cb_t handler_cb, void *cb_ctx);

// Function to check if the input buffer is empty
bool opus_encoder_wrapper_is_buffer_empty(opus_encoder_wrapper_t *wrapper);

// Function to reset the Opus encoder state
void opus_encoder_wrapper_reset(opus_encoder_wrapper_t *wrapper);

#endif