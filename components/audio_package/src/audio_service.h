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

#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Include headers
#include "model_path.h"
#include "audio_codec.h"
#include "audio_processor.h"
#include "opus_encoder.h"
#include "opus_decoder.h"
#include "opus_resampler.h"

#include "afe_audio_processor.h"

// Define constants
#define OPUS_FRAME_DURATION_MS 60
#define MAX_ENCODE_TASKS_IN_QUEUE 2
#define MAX_PLAYBACK_TASKS_IN_QUEUE 2
#define MAX_DECODE_PACKETS_IN_QUEUE (2400 / OPUS_FRAME_DURATION_MS)
#define MAX_SEND_PACKETS_IN_QUEUE (2400 / OPUS_FRAME_DURATION_MS)

// Power management constants
#define AUDIO_POWER_TIMEOUT_MS 15000
#define AUDIO_POWER_CHECK_INTERVAL_MS 1000

// Event group bits
#define AS_EVENT_AUDIO_PROCESSOR_RUNNING (1 << 0)

// Callback type definitions
typedef void (*audio_service_on_send_queue_available_cb_t)(void *user_ctx);
typedef void (*audio_service_on_vad_change_cb_t)(bool speaking, void *user_ctx);

// Audio task type enumeration
typedef enum
{
    AUDIO_TASK_TYPE_ENCODE_TO_SEND_QUEUE = 0,
    AUDIO_TASK_TYPE_DECODE_TO_PLAYBACK_QUEUE = 1,
} audio_task_type_t;

// Data structure definitions
typedef struct
{
    uint32_t timestamp;
    int sample_rate;
    int frame_duration_ms;
    uint8_t *payload;
    size_t payload_len;
} audio_stream_packet_t;

// Audio task structure
typedef struct
{
    audio_task_type_t type;
    int16_t *pcm;
    size_t pcm_samples;
    uint32_t timestamp;
} audio_task_t;

// Debug statistics structure
typedef struct
{
    uint32_t input_count;
    uint32_t decode_count;
    uint32_t encode_count;
    uint32_t playback_count;
} audio_debug_statistics_t;

// Audio service structure
typedef struct
{
    audio_service_on_send_queue_available_cb_t on_send_queue_available;
    audio_service_on_vad_change_cb_t on_vad_change;
    void *user_ctx;
} audio_service_callbacks_t;

// Main audio service structure
typedef struct
{
    audio_codec_t *codec;
    srmodel_list_t *models_list;
    audio_service_callbacks_t callbacks;

    /* AFE audio processor (VAD/AEC) */
    audio_processor_t *audio_processor;
    bool audio_processor_initialized;
    bool voice_detected;

    /* Opus encoder / decoder / resamplers */
    opus_encoder_wrapper_t *opus_encoder;
    opus_decoder_wrapper_t *opus_decoder;
    opus_resampler_t input_resampler;
    opus_resampler_t reference_resampler;
    opus_resampler_t output_resampler;
    bool input_resampler_inited;
    bool reference_resampler_inited;
    bool output_resampler_inited;

    audio_debug_statistics_t stats;

    EventGroupHandle_t event_group;

    /* Tasks */
    TaskHandle_t audio_input_task_handle;
    TaskHandle_t audio_output_task_handle;
    TaskHandle_t opus_codec_task_handle;

    /* Queues & synchronization */
    SemaphoreHandle_t queue_mutex; /* protects all queues */
    SemaphoreHandle_t queue_sem;   /* condition variable replacement */

    /* Decode queue: server -> speaker */
    audio_stream_packet_t *decode_queue[MAX_DECODE_PACKETS_IN_QUEUE];
    int decode_head;
    int decode_tail;
    int decode_count;

    /* Send queue: mic -> server */
    audio_stream_packet_t *send_queue[MAX_SEND_PACKETS_IN_QUEUE];
    int send_head;
    int send_tail;
    int send_count;

    /* Encode queue: PCM from processor waiting to be encoded */
    audio_task_t *encode_queue[MAX_ENCODE_TASKS_IN_QUEUE];
    int encode_head;
    int encode_tail;
    int encode_count;

    /* Playback queue: PCM ready for DAC */
    audio_task_t *playback_queue[MAX_PLAYBACK_TASKS_IN_QUEUE];
    int playback_head;
    int playback_tail;
    int playback_count;

    bool service_stopped;
    bool audio_input_need_warmup;

    esp_timer_handle_t audio_power_timer;
    int64_t last_input_time_ms;
    int64_t last_output_time_ms;
} audio_service_t;

// Function declarations
audio_service_t *audio_service_create(audio_codec_t *codec, srmodel_list_t *models_list, const audio_service_callbacks_t *cbs);

// Destroy audio service
void audio_service_destroy(audio_service_t *svc);

// Start and stop audio service
void audio_service_start(audio_service_t *svc);
void audio_service_stop(audio_service_t *svc);

// Enqueue and dequeue audio packets
void audio_service_enable_voice_processing(audio_service_t *svc, bool enable);
void audio_service_enable_device_aec(audio_service_t *svc, bool enable);

// Check service state
bool audio_service_is_voice_detected(const audio_service_t *svc);
bool audio_service_is_idle(audio_service_t *svc);

// Push packet to decode queue
bool audio_service_push_packet_to_decode_queue(audio_service_t *svc, audio_stream_packet_t *packet, bool wait);

// Pop packet from send queue
audio_stream_packet_t *audio_service_pop_packet_from_send_queue(audio_service_t *svc);

// Play sound from OGG data
void audio_service_play_sound(audio_service_t *svc, const uint8_t *ogg_data, size_t ogg_size);

#endif