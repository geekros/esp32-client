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

#ifndef AUDIO_AFE_PROCESSOR_H_
#define AUDIO_AFE_PROCESSOR_H_

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_afe_sr_models.h>
#include <esp_afe_sr_iface.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

// Include project headers
#include "audio_processor.h"

// Define processor running event bit
#define PROCESSOR_RUNNING (1 << 0)

// Define AFE Audio Processor structure
typedef struct
{
    audio_processor_t base;
    EventGroupHandle_t event_group;
    const esp_afe_sr_iface_t *afe_iface;
    esp_afe_sr_data_t *afe_data;
    audio_codec_t *codec;
    int frame_samples;
    bool is_speaking;
    int16_t *output_buffer;
    size_t output_buffer_size;
    size_t output_buffer_capacity;
    TaskHandle_t task_handle;
} afe_audio_processor_t;

// Create AFE Audio Processor
audio_processor_t *afe_audio_processor_create(void);

// Destroy AFE Audio Processor
void afe_audio_processor_destroy(audio_processor_t *processor);

// AFE Audio Processor interface functions
void afe_audio_processor_initialize(audio_processor_t *self, audio_codec_t *codec, int frame_duration_ms, srmodel_list_t *models_list);

// Feed data to AFE Audio Processor
void afe_audio_processor_feed(audio_processor_t *self, int16_t *data, size_t samples);

// Start AFE Audio Processor
void afe_audio_processor_start(audio_processor_t *self);

// Stop AFE Audio Processor
void afe_audio_processor_stop(audio_processor_t *self);

// Check if AFE Audio Processor is running
bool afe_audio_processor_is_running(audio_processor_t *self);

// Set output callback for AFE Audio Processor
void afe_audio_processor_set_output_callback(audio_processor_t *self, audio_processor_output_cb_t cb, void *user_ctx);

// Set VAD callback for AFE Audio Processor
void afe_audio_processor_set_vad_callback(audio_processor_t *self, audio_processor_vad_cb_t cb, void *user_ctx);

// Get feed size for AFE Audio Processor
size_t afe_audio_processor_get_feed_size(audio_processor_t *self);

// Enable or disable device AEC for AFE Audio Processor
void afe_audio_processor_enable_device_aec(audio_processor_t *self, bool enable);

// AFE Audio Processor task function
void afe_audio_processor_task(void *arg);

// Define AFE Audio Processor interface
static const audio_processor_interface_t afe_audio_processor_interface = {
    .initialize = afe_audio_processor_initialize,
    .feed = afe_audio_processor_feed,
    .start = afe_audio_processor_start,
    .stop = afe_audio_processor_stop,
    .is_running = afe_audio_processor_is_running,
    .set_output_callback = afe_audio_processor_set_output_callback,
    .set_vad_callback = afe_audio_processor_set_vad_callback,
    .get_feed_size = afe_audio_processor_get_feed_size,
    .enable_device_aec = afe_audio_processor_enable_device_aec,
};

#endif