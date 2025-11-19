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
#include "audio_codec.h"

// Define log tag
#define TAG "[client:components:audio:codec]"

// Set output volume implementation
static void audio_codec_set_output_volume(audio_codec_t *codec, int volume)
{
    codec->output_volume = volume;
    ESP_LOGI(TAG, "Set output volume to %d", volume);
}

// Set input gain implementation
static void audio_codec_set_input_gain(audio_codec_t *codec, float gain)
{
    codec->input_gain = gain;
    ESP_LOGI(TAG, "Set input gain to %.1f", gain);
}

// Enable output implementation
static void audio_codec_enable_input(audio_codec_t *codec, bool enable)
{
    if (enable == codec->input_enabled)
    {
        return;
    }
    codec->input_enabled = enable;
    ESP_LOGI(TAG, "Set input enable to %s", enable ? "true" : "false");
}

// Enable output implementation
static void audio_codec_enable_output(audio_codec_t *codec, bool enable)
{
    if (enable == codec->output_enabled)
    {
        return;
    }
    codec->output_enabled = enable;
    ESP_LOGI(TAG, "Set output enable to %s", enable ? "true" : "false");
}

// Output data implementation
static void audio_codec_output_data(audio_codec_t *codec, int16_t *data, int samples)
{
    if (codec->func && codec->func->Write)
    {
        codec->func->Write(codec, data, samples);
    }
}

// Input data implementation
static bool audio_codec_input_data(audio_codec_t *codec, int16_t *data, int samples)
{
    if (!codec->func || !codec->func->Read)
    {
        return false;
    }
    int r = codec->func->Read(codec, data, samples);
    return (r > 0);
}

// Start codec implementation
static void audio_codec_start(audio_codec_t *codec)
{
    if (codec->output_volume <= 0)
    {
        ESP_LOGW(TAG, "Output volume %d too small, set to 10", codec->output_volume);
        codec->output_volume = 10;
    }

    if (codec->tx_handle)
    {
        ESP_ERROR_CHECK(i2s_channel_enable(codec->tx_handle));
    }
    if (codec->rx_handle)
    {
        ESP_ERROR_CHECK(i2s_channel_enable(codec->rx_handle));
    }

    if (codec->func && codec->func->EnableInput)
    {
        codec->func->EnableInput(codec, true);
    }
    if (codec->func && codec->func->EnableOutput)
    {
        codec->func->EnableOutput(codec, true);
    }

    ESP_LOGI(TAG, "Audio codec started");
}

// Audio codec function implementations
const audio_codec_func_t audio_codec_func = {
    .Read = NULL,
    .Write = NULL,
    .SetOutputVolume = audio_codec_set_output_volume,
    .SetInputGain = audio_codec_set_input_gain,
    .EnableInput = audio_codec_enable_input,
    .EnableOutput = audio_codec_enable_output,
    .OutputData = audio_codec_output_data,
    .InputData = audio_codec_input_data,
    .Start = audio_codec_start,
};

// Function to initialize audio codec
void audio_codec_init(audio_codec_t *codec, const audio_codec_func_t *func)
{
    memset(codec, 0, sizeof(audio_codec_t));
    codec->func = func;

    codec->input_channels = 1;
    codec->output_channels = 1;
    codec->output_volume = 70;
    codec->input_gain = 0.0f;
}