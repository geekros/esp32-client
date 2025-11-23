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
#include <string>
#include <vector>
#include <functional>

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
#define PROCESSOR_RUNNING 0x01

// AfeAudioProcessor class definition
class AfeAudioProcessor : public AudioProcessor
{
public:
    // Constructor and destructor
    AfeAudioProcessor();
    ~AfeAudioProcessor();

    // Define public methods
    void Initialize(AudioCodec *codec_data, int frame_duration_ms, srmodel_list_t *models_list) override;
    void Feed(std::vector<int16_t> &&data) override;
    void Start() override;
    void Stop() override;
    bool IsRunning() override;
    void OnOutput(std::function<void(std::vector<int16_t> &&data)> callback) override;
    void OnVadStateChange(std::function<void(bool speaking)> callback) override;
    size_t GetFeedSize() override;
    void EnableDeviceAec(bool enable) override;

private:
    // Define private members
    EventGroupHandle_t event_group = nullptr;
    esp_afe_sr_iface_t *afe_iface = nullptr;
    esp_afe_sr_data_t *afe_data = nullptr;
    std::function<void(std::vector<int16_t> &&data)> output_callback;
    std::function<void(bool speaking)> vad_state_change_callback;
    AudioCodec *codec = nullptr;
    int frame_samples = 0;
    bool is_speaking = false;
    std::vector<int16_t> output_buffer;

    // Define private methods
    void AudioProcessorTask();
};

#endif