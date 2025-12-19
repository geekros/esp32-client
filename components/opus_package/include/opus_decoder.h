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

#ifndef OPUS_DECODER_H
#define OPUS_DECODER_H

// Include standard headers
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include Opus headers
#include "opus.h"

// OpusDecoderWrapper class
class OpusDecoderWrapper
{
private:
    // Member variables
    std::mutex mutex;
    struct OpusDecoder *audio_decoder = nullptr;
    int frame_size;
    int sample_rate;
    int duration_ms;

public:
    // Constructor and Destructor
    OpusDecoderWrapper(int sample_rate, int channels, int duration_ms = 60);
    ~OpusDecoderWrapper();

    // Member functions
    bool Decode(std::vector<uint8_t> &&opus, std::vector<int16_t> &pcm);
    void ResetState();

    // Sample Rate
    inline int SampleRate() const
    {
        // Return sample rate
        return sample_rate;
    }

    // Duration in milliseconds
    inline int DurationMS() const
    {
        // Return duration in milliseconds
        return duration_ms;
    }
};

#endif