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
#include "opus_encoder.h"

// Define log tag
#define TAG "[client:components:opus:encoder]"

// Constructor
OpusEncoderWrapper::OpusEncoderWrapper(int sample_rate, int channels, int duration_ms) : sample_rate(sample_rate), duration_ms(duration_ms)
{
    // Declare error variable
    int error;

    // Create Opus encoder
    audio_encoder = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &error);
    if (audio_encoder == nullptr)
    {
        return;
    }

    // Default DTX enabled
    SetDtx(true);
    // Complexity 5 almost uses up all CPU of ESP32C3 while complexity 0 uses the least
    SetComplexity(0);

    // Calculate frame size
    frame_size = sample_rate / 1000 * channels * duration_ms;
}

// Destructor
OpusEncoderWrapper::~OpusEncoderWrapper()
{
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex);

    // Destroy Opus encoder
    if (audio_encoder != nullptr)
    {
        opus_encoder_destroy(audio_encoder);
    }
}

// Encode function (synchronous)
void OpusEncoderWrapper::Encode(std::vector<int16_t> &&pcm, std::function<void(std::vector<uint8_t> &&opus)> handler)
{
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex);

    // Check if encoder is initialized
    if (audio_encoder == nullptr)
    {
        return;
    }

    // Append incoming PCM data to internal buffer
    if (in_buffer.empty())
    {
        in_buffer = std::move(pcm);
    }
    else
    {
        in_buffer.reserve(in_buffer.size() + pcm.size());
        in_buffer.insert(in_buffer.end(), pcm.begin(), pcm.end());
    }

    // Process while we have enough data for a frame
    while (in_buffer.size() >= frame_size)
    {
        // Prepare Opus output buffer
        uint8_t opus[MAX_OPUS_PACKET_SIZE];

        // Encode PCM data to Opus
        auto ret = opus_encode(audio_encoder, in_buffer.data(), frame_size, opus, MAX_OPUS_PACKET_SIZE);
        if (ret < 0)
        {
            return;
        }

        // Check if handler is provided
        if (handler != nullptr)
        {
            handler(std::vector<uint8_t>(opus, opus + ret));
        }

        // Remove processed PCM data from internal buffer
        in_buffer.erase(in_buffer.begin(), in_buffer.begin() + frame_size);
    }
}

// Encode function (blocking)
bool OpusEncoderWrapper::Encode(std::vector<int16_t> &&pcm, std::vector<uint8_t> &opus)
{
    // Check if encoder is initialized
    if (audio_encoder == nullptr)
    {
        return false;
    }

    // Check if PCM size matches frame size
    if (pcm.size() != frame_size)
    {
        return false;
    }

    // Encode PCM data to Opus
    uint8_t buf[MAX_OPUS_PACKET_SIZE];
    auto ret = opus_encode(audio_encoder, pcm.data(), frame_size, buf, MAX_OPUS_PACKET_SIZE);
    if (ret < 0)
    {
        return false;
    }

    // Assign encoded data to output vector
    opus.assign(buf, buf + ret);

    // Successful encode
    return true;
}

// Reset encoder state
void OpusEncoderWrapper::ResetState()
{
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex);

    // Reset Opus encoder state
    if (audio_encoder != nullptr)
    {
        opus_encoder_ctl(audio_encoder, OPUS_RESET_STATE);
        in_buffer.clear();
    }
}

// Set DTX (Discontinuous Transmission)
void OpusEncoderWrapper::SetDtx(bool enable)
{
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex);

    // Set DTX option
    if (audio_encoder != nullptr)
    {
        opus_encoder_ctl(audio_encoder, OPUS_SET_DTX(enable ? 1 : 0));
    }
}

// Set Complexity
void OpusEncoderWrapper::SetComplexity(int complexity)
{
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex);

    // Set Complexity option
    if (audio_encoder != nullptr)
    {
        opus_encoder_ctl(audio_encoder, OPUS_SET_COMPLEXITY(complexity));
    }
}