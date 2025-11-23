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

// Include headers
#include "opus_decoder.h"

// Define log tag
#define TAG "[client:components:opus:decoder]"

// Constructor
OpusDecoderWrapper::OpusDecoderWrapper(int sample_rate, int channels, int duration_ms) : sample_rate(sample_rate), duration_ms(duration_ms)
{
    // Declare error variable
    int error;

    // Create Opus decoder
    audio_decoder = opus_decoder_create(sample_rate, channels, &error);
    if (audio_decoder == nullptr)
    {
        return;
    }

    // Calculate frame size
    frame_size = sample_rate / 1000 * channels * duration_ms;
}

// Destructor
OpusDecoderWrapper::~OpusDecoderWrapper()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (audio_decoder != nullptr)
    {
        opus_decoder_destroy(audio_decoder);
    }
}

// Decode function
bool OpusDecoderWrapper::Decode(std::vector<uint8_t> &&opus, std::vector<int16_t> &pcm)
{
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex);

    // Check if decoder is initialized
    if (audio_decoder == nullptr)
    {
        return false;
    }

    // Prepare PCM buffer
    pcm.resize(frame_size);

    // Decode Opus data
    auto ret = opus_decode(audio_decoder, opus.data(), opus.size(), pcm.data(), pcm.size(), 0);
    if (ret < 0)
    {
        return false;
    }

    // Resize PCM vector to actual decoded size
    pcm.resize(ret);

    // Successful decode
    return true;
}

// Reset decoder state
void OpusDecoderWrapper::ResetState()
{
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex);

    // Reset Opus decoder state
    if (audio_decoder != nullptr)
    {
        opus_decoder_ctl(audio_decoder, OPUS_RESET_STATE);
    }
}
