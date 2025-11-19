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

#ifndef ES8311_CODEC_H
#define ES8311_CODEC_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include driver headers
#include <driver/i2c_master.h>
#include <driver/gpio.h>

// Include codec device headers
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Include headers
#include "audio_codec.h"

// Define ES8311 audio codec structure
typedef struct
{
    audio_codec_t base;

    const audio_codec_data_if_t *data_if;
    const audio_codec_ctrl_if_t *ctrl_if;
    const audio_codec_if_t *codec_if;
    const audio_codec_gpio_if_t *gpio_if;

    esp_codec_dev_handle_t dev;

    gpio_num_t pa_pin;
    bool pa_inverted;

    SemaphoreHandle_t data_if_mutex;
} es8311_audio_codec_t;

// Function to create ES8311 audio codec instance
void es8311_create(es8311_audio_codec_t *codec, void *i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate, int output_sample_rate, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, gpio_num_t pa_pin, uint8_t es8311_addr, bool use_mclk, bool pa_inverted);

// Function to update ES8311 audio codec device state
void es8311_update_device_state(es8311_audio_codec_t *codec);

// Function to create duplex I2S channels for ES8311 audio codec
void es8311_create_duplex_channels(es8311_audio_codec_t *codec, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din);

// Function to set output volume for ES8311 audio codec
void es8311_set_output_volume(audio_codec_t *base, int volume);

// Function to set input gain for ES8311 audio codec
void es8311_set_input_gain(audio_codec_t *base, float gain);

// Function to set input gain for ES8311 audio codec
void es8311_enable_input(audio_codec_t *base, bool enable);

// Function to enable output for ES8311 audio codec
void es8311_enable_output(audio_codec_t *base, bool enable);

// Function to write data to ES8311 audio codec
int es8311_read(audio_codec_t *base, int16_t *dest, int samples);

// Function to read data from ES8311 audio codec
int es8311_write(audio_codec_t *base, const int16_t *data, int samples);

// Function to destroy ES8311 audio codec instance
void es8311_destroy(es8311_audio_codec_t *codec);

// Function to get ES8311 audio codec function implementations
const audio_codec_func_t *es8311_audio_codec_func(void);

#endif