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
#include <string>
#include <memory>
#include <deque>
#include <chrono>
#include <mutex>
#include <cstring>
#include <condition_variable>

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

// Include AFE headers
#include "afe_audio_processor.h"

// Include header for board
#include "board_basic.h"

// Define audio processor running event bit
#define OPUS_FRAME_DURATION_MS 60
#define MAX_ENCODE_TASKS_IN_QUEUE 2
#define MAX_PLAYBACK_TASKS_IN_QUEUE 2

#define MAX_DECODE_PACKETS_IN_QUEUE (2400 / OPUS_FRAME_DURATION_MS)
#define MAX_SEND_PACKETS_IN_QUEUE (2400 / OPUS_FRAME_DURATION_MS)

#define MAX_TIMESTAMPS_IN_QUEUE 3

#define AUDIO_POWER_TIMEOUT_MS 15000
#define AUDIO_POWER_CHECK_INTERVAL_MS 1000

#define AS_EVENT_AUDIO_PROCESSOR_RUNNING (1 << 2)

// Define audio callbacks structure
struct AudioCallbacks
{
    std::function<void(void)> on_send_queue_available;
    std::function<void(bool)> on_vad_change;
};

// Define audio task type enum
enum AudioTaskType
{
    AudioTaskTypeEncodeToSendQueue,
    AudioTaskTypeDecodeToPlaybackQueue,
};

// Define audio task structure
struct AudioTask
{
    AudioTaskType type;
    std::vector<int16_t> pcm;
    uint32_t timestamp;
};

// Define audio stream packet structure
struct AudioStreamPacket
{
    int sample_rate = 0;
    int frame_duration = 0;
    uint32_t timestamp = 0;
    std::vector<uint8_t> payload;
};

// Audio service class definition
class AudioService
{
private:
    // FreeRTOS event group handle
    EventGroupHandle_t event_group;

    // Member variables
    AudioCodec *codec = nullptr;
    AudioCallbacks callbacks;

    // Audio processor and Opus codec wrappers
    std::unique_ptr<AudioProcessor> audio_processor;
    std::unique_ptr<OpusEncoderWrapper> opus_encoder;
    std::unique_ptr<OpusDecoderWrapper> opus_decoder;

    // Resamplers for input, reference, and output audio
    OpusResampler input_resampler;
    OpusResampler reference_resampler;
    OpusResampler output_resampler;

    // Models list
    srmodel_list_t *models_list = nullptr;

    // FreeRTOS task handles
    TaskHandle_t audio_input_task_handle = nullptr;
    TaskHandle_t audio_output_task_handle = nullptr;
    TaskHandle_t opus_codec_task_handle = nullptr;

    // Mutex and condition variable for audio queues
    std::mutex audio_queue_mutex;
    std::condition_variable audio_queue_cv;

    // Audio queues
    std::deque<std::unique_ptr<AudioStreamPacket>> audio_decode_queue;
    std::deque<std::unique_ptr<AudioStreamPacket>> audio_send_queue;
    std::deque<std::unique_ptr<AudioTask>> audio_encode_queue;
    std::deque<std::unique_ptr<AudioTask>> audio_playback_queue;

    // For server AEC
    std::deque<uint32_t> timestamp_queue;

    // State variables
    bool audio_processor_initialized = false;
    bool voice_detected = false;
    bool service_stopped = true;
    bool audio_input_need_warmup = false;

    // Audio power management
    esp_timer_handle_t audio_power_timer = nullptr;
    std::chrono::steady_clock::time_point last_input_time;
    std::chrono::steady_clock::time_point last_output_time;

    // Private methods
    void AudioInputTask();
    void AudioOutputTask();
    void OpusCodecTask();
    void PushTaskToEncodeQueue(AudioTaskType type, std::vector<int16_t> &&pcm);
    void SetDecodeSampleRate(int sample_rate, int frame_duration);
    void CheckAndUpdateAudioPowerState();

public:
    // Constructor and destructor
    AudioService();
    ~AudioService();

    // Public methods
    void Initialize(AudioCodec *codec_data);
    void Start();
    void Stop();

    // Getters for state
    bool IsVoiceDetected() const { return voice_detected; }
    bool IsIdle();
    bool IsAudioProcessorRunning() const { return xEventGroupGetBits(event_group) & AS_EVENT_AUDIO_PROCESSOR_RUNNING; }

    // Enable or disable features
    void EnableVoiceProcessing(bool enable);
    void EnableDeviceAec(bool enable);

    // Set audio callbacks
    void SetCallbacks(AudioCallbacks &cb);

    // Audio data methods
    bool PushPacketToDecodeQueue(std::unique_ptr<AudioStreamPacket> packet, bool wait = false);
    std::unique_ptr<AudioStreamPacket> PopPacketFromSendQueue();
    void PlaySound(const std::string_view &sound);
    bool ReadAudioData(std::vector<int16_t> &data, int sample_rate, int samples);
    void ResetDecoder();
};

#endif