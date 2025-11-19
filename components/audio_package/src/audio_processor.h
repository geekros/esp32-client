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

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include headers
#include "model_path.h"
#include "audio_codec.h"

// Forward declaration of AudioProcessor structure
typedef struct audio_processor_t audio_processor_t;

// Define callback types
typedef void (*audio_processor_output_cb_t)(int16_t *data, size_t samples, void *user_ctx);

// VAD callback type
typedef void (*audio_processor_vad_cb_t)(bool speaking, void *user_ctx);

// Define AudioProcessorInterface structure
typedef struct audio_processor_interface
{
    void (*initialize)(struct audio_processor_t *self, audio_codec_t *codec, int frame_duration_ms, srmodel_list_t *models_list);
    void (*feed)(struct audio_processor_t *self, int16_t *data, size_t samples);
    void (*start)(struct audio_processor_t *self);
    void (*stop)(struct audio_processor_t *self);
    bool (*is_running)(struct audio_processor_t *self);
    void (*set_output_callback)(struct audio_processor_t *self, audio_processor_output_cb_t cb, void *user_ctx);
    void (*set_vad_callback)(struct audio_processor_t *self, audio_processor_vad_cb_t cb, void *user_ctx);
    size_t (*get_feed_size)(struct audio_processor_t *self);
    void (*enable_device_aec)(struct audio_processor_t *self, bool enable);
} audio_processor_interface_t;

// Define AudioProcessor structure
struct audio_processor_t
{
    const audio_processor_interface_t *itf;
    audio_processor_output_cb_t output_cb;
    void *output_ctx;

    audio_processor_vad_cb_t vad_cb;
    void *vad_ctx;
};

#endif
