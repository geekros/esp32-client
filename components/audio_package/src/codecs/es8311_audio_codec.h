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

#ifndef AUDIO_ES8311_AUDIO_CODEC_H
#define AUDIO_ES8311_AUDIO_CODEC_H

// Include standard libraries
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>

// Include ESP libraries
#include <driver/i2c_master.h>
#include <driver/i2s_std.h>
#include <driver/gpio.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Include audio package headers
#define AUDIO_CODEC_DMA_DESC_NUM 6

// Define audio codec DMA frame number
#define AUDIO_CODEC_DMA_FRAME_NUM 240

typedef struct
{
    bool duplex;
    bool input_reference;
    int input_channels;
    int input_sample_rate;
    int output_sample_rate;
    int input_gain;
    int output_volume;
    bool input_enabled;
    bool output_enabled;
    gpio_num_t pa_pin;
    bool pa_inverted;

    i2s_chan_handle_t tx_handle;
    i2s_chan_handle_t rx_handle;

    const audio_codec_data_if_t *data_if;
    const audio_codec_ctrl_if_t *ctrl_if;
    const audio_codec_gpio_if_t *gpio_if;
    const audio_codec_if_t *codec_if;

    esp_codec_dev_handle_t dev;

    SemaphoreHandle_t mutex;
} Es8311AudioCodec_t;

// ES8311 audio codec constructor
Es8311AudioCodec_t *Es8311AudioCodec_Create(void *i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate, int output_sample_rate, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, gpio_num_t pa_pin, uint8_t es8311_addr, bool use_mclk, bool pa_inverted);

// ES8311 audio codec destructor
void Es8311AudioCodec_Destroy(Es8311AudioCodec_t *codec);

// Update ES8311 device state
void Es8311AudioCodec_UpdateDeviceState(Es8311AudioCodec_t *codec);

// Create ES8311 duplex channels
void Es8311AudioCodec_CreateDuplexChannels(Es8311AudioCodec_t *codec, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din);

// Set ES8311 output volume
void Es8311AudioCodec_SetOutputVolume(Es8311AudioCodec_t *codec, int volume);

// Enable or disable ES8311 input
void Es8311AudioCodec_EnableInput(Es8311AudioCodec_t *codec, bool enable);

// Enable or disable ES8311 output
void Es8311AudioCodec_EnableOutput(Es8311AudioCodec_t *codec, bool enable);

// Read samples to ES8311 codec
int Es8311AudioCodec_Read(Es8311AudioCodec_t *codec, int16_t *dest, int samples);

// Write samples to ES8311 codec
int Es8311AudioCodec_Write(Es8311AudioCodec_t *codec, const int16_t *data, int samples);

#endif