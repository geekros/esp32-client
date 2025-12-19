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
#include <string>
#include <vector>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include headers
#include "model_basic.h"
#include "codec_basic.h"

// AudioProcessor class definition
class AudioProcessor
{
protected:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and destructor
    AudioProcessor();
    virtual ~AudioProcessor();

    // Define public methods
    virtual void Initialize(AudioCodec *codec, int frame_duration_ms) = 0;
    virtual void Feed(std::vector<int16_t> &&data) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() = 0;
    virtual void OnOutput(std::function<void(std::vector<int16_t> &&data)> callback) = 0;
    virtual void OnVadStateChange(std::function<void(bool speaking)> callback) = 0;
    virtual size_t GetFeedSize() = 0;
    virtual void EnableDeviceAec(bool enable) = 0;
};

#endif