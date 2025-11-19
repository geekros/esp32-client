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

#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include driver headers
#include <driver/i2s_std.h>

// Define audio codec DMA descriptor number
#define AUDIO_CODEC_DMA_DESC_NUM 6

// Define audio codec DMA frame number
#define AUDIO_CODEC_DMA_FRAME_NUM 240

// Forward declaration of AudioCodec structure
typedef struct audio_codec_t audio_codec_t;

// Define AudioCodecFunc structure
typedef struct
{
    int (*Read)(struct audio_codec_t *codec, int16_t *dest, int samples);
    int (*Write)(struct audio_codec_t *codec, const int16_t *data, int samples);

    void (*SetOutputVolume)(struct audio_codec_t *codec, int volume);
    void (*SetInputGain)(struct audio_codec_t *codec, float gain);
    void (*EnableInput)(struct audio_codec_t *codec, bool enable);
    void (*EnableOutput)(struct audio_codec_t *codec, bool enable);

    void (*OutputData)(struct audio_codec_t *codec, int16_t *data, int samples);
    bool (*InputData)(struct audio_codec_t *codec, int16_t *data, int samples);
    void (*Start)(struct audio_codec_t *codec);
} audio_codec_func_t;

// Define AudioCodec structure
struct audio_codec_t
{
    const audio_codec_func_t *func;

    i2s_chan_handle_t tx_handle;
    i2s_chan_handle_t rx_handle;

    bool duplex;
    bool input_reference;
    bool input_enabled;
    bool output_enabled;
    int input_sample_rate;
    int output_sample_rate;
    int input_channels;
    int output_channels;
    int output_volume;
    float input_gain;
};

// Function to initialize audio codec
void audio_codec_init(audio_codec_t *codec, const audio_codec_func_t *func);

// External declaration of audio codec function implementations
extern const audio_codec_func_t audio_codec_func;

#endif