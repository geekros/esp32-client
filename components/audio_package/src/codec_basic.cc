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

// Include the headers
#include "codec_basic.h"

// Define log tag
#define TAG "[client:components:audio:codec:basic]"

// Constructor
AudioCodec::AudioCodec()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
AudioCodec::~AudioCodec()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Start the audio codec
void AudioCodec::Start()
{
    // Enable RX channel if applicable
    if (tx_handle != nullptr)
    {
        // Enable TX channel
        ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    }

    // Enable TX channel if applicable
    if (rx_handle != nullptr)
    {
        // Enable RX channel
        ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    }

    // Enable input and output by default
    EnableInput(true);
    EnableOutput(true);
}

// Set output volume
void AudioCodec::SetOutputVolume(int volume)
{
    // Set output volume
    output_volume = volume;
}

// Set input gain
void AudioCodec::SetInputGain(float gain)
{
    // Set input gain
    input_gain = gain;
}

// Enable or disable input
void AudioCodec::EnableInput(bool enable)
{
    // Set input enabled state
    input_enabled = enable;
}

// Enable or disable output
void AudioCodec::EnableOutput(bool enable)
{
    // Set output enabled state
    output_enabled = enable;
}

// Output audio data
void AudioCodec::OutputData(std::vector<int16_t> &data)
{
    // Write audio data
    Write(data.data(), data.size());
}

// Input audio data
bool AudioCodec::InputData(std::vector<int16_t> &data)
{
    // Read audio data
    int samples = Read(data.data(), data.size());

    // Return true if samples were read
    return samples > 0;
}