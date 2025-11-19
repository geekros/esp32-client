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
#include "opus_encoder.h"

// Define log tag
#define TAG "[client:components:opus:encoder]"

// Function to reserve PCM buffer capacity
static bool reserve_pcm_buffer(opus_encoder_wrapper_t *wrapper, size_t capacity)
{
    // Check if current capacity is sufficient
    if (wrapper->in_buffer_capacity >= capacity)
    {
        return true;
    }

    // Calculate new capacity
    size_t new_cap = wrapper->in_buffer_capacity == 0 ? capacity : wrapper->in_buffer_capacity * 2;
    if (new_cap < capacity)
    {
        new_cap = capacity;
    }

    // Reallocate input buffer
    int16_t *new_buf = (int16_t *)realloc(wrapper->in_buffer, new_cap * sizeof(int16_t));
    if (!new_buf)
    {
        return false;
    }

    // Update wrapper with new buffer
    wrapper->in_buffer = new_buf;
    wrapper->in_buffer_capacity = new_cap;

    // Return success
    return true;
}

// Function to create and initialize an Opus encoder wrapper
opus_encoder_wrapper_t *opus_encoder_wrapper_create(int sample_rate, int channels, int duration_ms)
{
    // Declare error variable
    int error;

    // Allocate memory for the wrapper
    opus_encoder_wrapper_t *wrapper = (opus_encoder_wrapper_t *)calloc(1, sizeof(opus_encoder_wrapper_t));
    if (!wrapper)
    {
        return NULL;
    }

    // Create the Opus encoder
    wrapper->enc = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &error);
    if (!wrapper->enc || error != OPUS_OK)
    {
        free(wrapper);
        return NULL;
    }

    // Initialize wrapper fields
    wrapper->sample_rate = sample_rate;
    wrapper->channels = channels;
    wrapper->duration_ms = duration_ms;

    // Calculate frame size
    wrapper->frame_size = (sample_rate / 1000) * duration_ms * channels;

    // Ctrl settings for DTX and complexity
    opus_encoder_ctl(wrapper->enc, OPUS_SET_DTX(1));
    opus_encoder_ctl(wrapper->enc, OPUS_SET_COMPLEXITY(0));

    // Return the initialized wrapper
    return wrapper;
}

// Function to set the DTX (Discontinuous Transmission) option
void opus_encoder_wrapper_set_dtx(opus_encoder_wrapper_t *wrapper, bool enable)
{
    // Check for null pointer
    if (!wrapper || !wrapper->enc)
    {
        return;
    }

    // Set the DTX option
    opus_encoder_ctl(wrapper->enc, OPUS_SET_DTX(enable ? 1 : 0));
}

// Function to set the complexity of the Opus encoder
void opus_encoder_wrapper_set_complexity(opus_encoder_wrapper_t *wrapper, int complexity)
{
    // Check for null pointer
    if (!wrapper || !wrapper->enc)
    {
        return;
    }

    // Set the complexity option
    opus_encoder_ctl(wrapper->enc, OPUS_SET_COMPLEXITY(complexity));
}

// Function to encode a frame of PCM samples into Opus format
int opus_encoder_wrapper_encode_frame(opus_encoder_wrapper_t *wrapper, const int16_t *pcm_samples, size_t pcm_count, uint8_t *opus_out, size_t opus_max)
{
    // Check for null pointer
    if (!wrapper || !wrapper->enc)
    {
        return OPUS_INVALID_STATE;
    }

    // Check for correct frame size
    if (pcm_count != (size_t)wrapper->frame_size)
    {
        return OPUS_BUFFER_TOO_SMALL;
    }

    // Encode the PCM samples into Opus format
    int ret = opus_encode(wrapper->enc, pcm_samples, wrapper->frame_size / wrapper->channels, opus_out, opus_max);
    if (ret < 0)
    {
        return ret;
    }

    // Return the number of bytes written
    return ret;
}

// Function to encode PCM data into Opus format using a callback
void opus_encoder_wrapper_encode(opus_encoder_wrapper_t *wrapper, const int16_t *pcm, size_t samples, opus_encode_handler_cb_t handler_cb, void *cb_ctx)
{
    // Check for null pointer
    if (!wrapper || !wrapper->enc)
    {
        return;
    }

    //
    size_t required = wrapper->in_buffer_size + samples;
    if (!reserve_pcm_buffer(wrapper, required))
    {
        return;
    }

    // Append new PCM samples to input buffer
    memcpy(wrapper->in_buffer + wrapper->in_buffer_size, pcm, samples * sizeof(int16_t));
    wrapper->in_buffer_size += samples;

    // Process complete frames
    while (wrapper->in_buffer_size >= (size_t)wrapper->frame_size)
    {
        // Encode a frame
        uint8_t opus[MAX_OPUS_PACKET_SIZE];

        // encode the frame
        int ret = opus_encode(wrapper->enc, wrapper->in_buffer, wrapper->frame_size / wrapper->channels, opus, MAX_OPUS_PACKET_SIZE);
        if (ret < 0)
        {
            return;
        }

        // handler callback
        if (handler_cb)
        {
            handler_cb(opus, ret, cb_ctx);
        }

        // Remove the processed frame from the input buffer
        size_t remain = wrapper->in_buffer_size - wrapper->frame_size;
        memmove(wrapper->in_buffer, wrapper->in_buffer + wrapper->frame_size, remain * sizeof(int16_t));

        // Update input buffer size
        wrapper->in_buffer_size = remain;
    }
}

// Function to check if the input buffer is empty
bool opus_encoder_wrapper_is_buffer_empty(opus_encoder_wrapper_t *wrapper)
{
    // Check for null pointer
    if (!wrapper)
    {
        return true;
    }

    // Return whether the input buffer is empty
    return wrapper->in_buffer_size == 0;
}

// Function to reset the Opus encoder state
void opus_encoder_wrapper_reset(opus_encoder_wrapper_t *wrapper)
{
    // Check for null pointer
    if (!wrapper || !wrapper->enc)
    {
        return;
    }

    // Reset the Opus encoder state
    opus_encoder_ctl(wrapper->enc, OPUS_RESET_STATE);
    wrapper->in_buffer_size = 0;
}

// Function to destroy and free an Opus encoder wrapper
void opus_encoder_wrapper_destroy(opus_encoder_wrapper_t *wrapper)
{
    // Check for null pointer
    if (!wrapper)
    {
        return;
    }

    // Destroy the Opus encoder and free buffers
    if (wrapper->enc)
    {
        // Destroy the Opus encoder
        opus_encoder_destroy(wrapper->enc);
    }

    // Free input buffer
    if (wrapper->in_buffer)
    {
        // Free the input buffer
        free(wrapper->in_buffer);
    }

    // Free the wrapper itself
    free(wrapper);
}