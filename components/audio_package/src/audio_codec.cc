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
#include "audio_codec.h"

// Define log tag
#define TAG "[client:components:audio:codec]"

// Constructor
AudioCodec::AudioCodec()
{
}

// Destructor
AudioCodec::~AudioCodec()
{
}

// Set output volume
void AudioCodec::SetOutputVolume(int volume)
{
    output_volume = volume;
}

// Set input gain
void AudioCodec::SetInputGain(float gain)
{
    input_gain = gain;
}

// Enable or disable input
void AudioCodec::EnableInput(bool enable)
{
    input_enabled = enable;
}

// Enable or disable output
void AudioCodec::EnableOutput(bool enable)
{
    output_enabled = enable;
}

// Output audio data
void AudioCodec::OutputData(std::vector<int16_t> &data)
{
    Write(data.data(), data.size());
}

// Input audio data
bool AudioCodec::InputData(std::vector<int16_t> &data)
{
    int samples = Read(data.data(), data.size());
    return samples > 0;
}

// Start the audio codec
void AudioCodec::Start()
{
    // Enable RX channel if applicable
    if (tx_handle != nullptr)
    {
        ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    }

    // Enable TX channel if applicable
    if (rx_handle != nullptr)
    {
        ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    }

    // Enable input and output by default
    EnableInput(true);
    EnableOutput(true);
}