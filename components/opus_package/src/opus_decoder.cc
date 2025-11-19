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

// Include headers
#include "opus_decoder.h"

// Define log tag
#define TAG "[client:components:opus:decoder]"

// Function to create and initialize an Opus decoder wrapper
opus_decoder_wrapper_t *opus_decoder_wrapper_create(int sample_rate, int channels, int duration_ms)
{
    // Variable to hold error code
    int error;

    // Allocate memory for the wrapper
    opus_decoder_wrapper_t *wrap = (opus_decoder_wrapper_t *)calloc(1, sizeof(opus_decoder_wrapper_t));
    if (!wrap)
    {
        return NULL;
    }

    // Create the Opus decoder
    wrap->dec = opus_decoder_create(sample_rate, channels, &error);
    if (wrap->dec == NULL || error != OPUS_OK)
    {
        free(wrap);
        return NULL;
    }

    // Initialize wrapper parameters
    wrap->sample_rate = sample_rate;
    wrap->channels = channels;
    wrap->duration_ms = duration_ms;

    // Calculate frame size
    wrap->frame_size = (sample_rate / 1000) * duration_ms * channels;

    // Return the created wrapper
    return wrap;
}

// Function to decode Opus data into PCM format
int opus_decoder_wrapper_decode(opus_decoder_wrapper_t *wrapper, const uint8_t *opus_data, size_t opus_len, int16_t *output_pcm, size_t max_pcm_samples)
{
    // Check for null pointer
    if (!wrapper || !wrapper->dec)
    {
        return OPUS_INVALID_STATE;
    }

    // Check if output buffer is large enough
    if (max_pcm_samples < (size_t)wrapper->frame_size)
    {
        return OPUS_BUFFER_TOO_SMALL;
    }

    // Decode the Opus data
    int ret = opus_decode(wrapper->dec, opus_data, opus_len, output_pcm, wrapper->frame_size, 0);
    if (ret < 0)
    {
        return ret;
    }

    // Return number of samples decoded
    return ret;
}

// Function to decode Opus data into PCM format
void opus_decoder_wrapper_destroy(opus_decoder_wrapper_t *wrapper)
{
    // Check for null pointer
    if (!wrapper)
    {
        return;
    }

    // Destroy the Opus decoder
    if (wrapper->dec)
    {
        // Destroy the decoder instance
        opus_decoder_destroy(wrapper->dec);
    }

    // Free the wrapper memory
    free(wrapper);
}

// Function to reset the Opus decoder state
void opus_decoder_wrapper_reset(opus_decoder_wrapper_t *wrapper)
{
    // Check for null pointer
    if (!wrapper || !wrapper->dec)
    {
        return;
    }

    // Reset the decoder state
    opus_decoder_ctl(wrapper->dec, OPUS_RESET_STATE);
}