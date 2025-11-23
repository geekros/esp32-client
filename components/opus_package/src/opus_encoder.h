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

#ifndef OPUS_ENCODER_H
#define OPUS_ENCODER_H

// Include standard headers
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <mutex>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include Opus headers
#include "opus.h"

// Define maximum Opus packet size
#define MAX_OPUS_PACKET_SIZE 1000

// Opus Encoder Wrapper Class
class OpusEncoderWrapper
{
private:
    // Member variables
    std::mutex mutex;
    struct OpusEncoder *audio_encoder = nullptr;
    int sample_rate;
    int duration_ms;
    int frame_size;
    std::vector<int16_t> in_buffer;

public:
    // Constructor and Destructor
    OpusEncoderWrapper(int sample_rate, int channels, int duration_ms = 60);
    ~OpusEncoderWrapper();

    // Sample Rate
    inline int SampleRate() const
    {
        return sample_rate;
    }

    // Duration in milliseconds
    inline int DurationMS() const
    {
        return duration_ms;
    }

    // Public Methods
    void SetDtx(bool enable);
    void SetComplexity(int complexity);
    bool Encode(std::vector<int16_t> &&pcm, std::vector<uint8_t> &opus);
    void Encode(std::vector<int16_t> &&pcm, std::function<void(std::vector<uint8_t> &&opus)> handler);
    bool IsBufferEmpty() const { return in_buffer.empty(); }
    void ResetState();
};

#endif