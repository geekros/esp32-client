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

#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

// Include standard libraries
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>
#include <esp_afe_sr_models.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Define processor running bit
#define PROCESSOR_RUNNING_BIT (1 << 0)

// Define AFE types and modes
#define AFE_DEFAULT_TYPE AFE_TYPE_VC
#define AFE_DEFAULT_MODE AFE_MODE_HIGH_PERF

// Define audio processor modes
typedef enum
{
    AUDIO_PROC_MODE_AFE,
    AUDIO_PROC_MODE_NO_AFE
} audio_processor_mode_t;

// Define AFE output callback type
typedef void (*afe_output_callback_t)(const int16_t *data, int samples, void *user_ctx);

// Define VAD callback type
typedef void (*afe_vad_callback_t)(bool speaking, void *user_ctx);

// Define audio processor structure
typedef struct
{
    EventGroupHandle_t event_group;

    const esp_afe_sr_iface_t *afe_iface;
    esp_afe_sr_data_t *afe_data;

    afe_output_callback_t output_cb;
    void *output_user_ctx;

    afe_vad_callback_t vad_cb;
    void *vad_user_ctx;

    int frame_samples;
    int input_channels;
    bool has_reference;

    bool is_speaking;
    bool use_afe;

    int16_t *output_buffer;
    int output_buffer_len;
    int output_buffer_cap;

    TaskHandle_t task_hdl;
} audio_processor_t;

// Function prototypes
audio_processor_t *audio_processor_create(audio_processor_mode_t mode);

// Function to initialize audio processor
void audio_processor_init(audio_processor_t *p, int input_channels, bool has_reference, int frame_duration_ms, srmodel_list_t *models_list);

// Function to start audio processor
void audio_processor_start(audio_processor_t *p);

// Function to stop audio processor
void audio_processor_stop(audio_processor_t *p);

// Function to destroy audio processor
void audio_processor_destroy(audio_processor_t *p);

// Function to set AFE output callback
void audio_processor_set_output_callback(audio_processor_t *p, afe_output_callback_t cb, void *user_ctx);

// Function to set VAD callback
void audio_processor_set_vad_callback(audio_processor_t *p, afe_vad_callback_t cb, void *user_ctx);

// Function to feed audio data to processor
void audio_processor_feed(audio_processor_t *p, const int16_t *data);

// Function to get required feed size
int audio_processor_get_feed_size(audio_processor_t *p);

// Function to enable or disable device AEC
void audio_processor_enable_device_aec(audio_processor_t *p, bool enable);

#endif
