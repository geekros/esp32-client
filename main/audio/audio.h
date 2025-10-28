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

#ifndef AUDIO_H
#define AUDIO_H

// Include standard libraries
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>
#include <esp_spiffs.h>
#include <esp_heap_caps.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/i2s_std.h>
#include <driver/i2s_tdm.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include module headers
#include "board_config.h"

#define USE_IDF_I2C_MASTER

// Define SPIFFS base path
#define SPIFFS_AUDIO_BASE_PATH "/spiffs"

// Function to initialize audio system
void audio_init(void);

// Function to start audio streaming
void audio_start_stream(void);

// Function to stop audio streaming
void audio_stop_stream(void);

#endif