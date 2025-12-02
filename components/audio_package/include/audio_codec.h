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

#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

// Include standard headers
#include <string>
#include <vector>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include driver headers
#include <driver/i2s_std.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Define audio codec DMA descriptor number
#define AUDIO_CODEC_DMA_DESC_NUM 6

// Define audio codec DMA frame number
#define AUDIO_CODEC_DMA_FRAME_NUM 240

// AudioCodec class definition
class AudioCodec
{
protected:
    // I2S channel handles
    i2s_chan_handle_t tx_handle = nullptr;
    i2s_chan_handle_t rx_handle = nullptr;

    // Audio codec state variables
    bool duplex = false;
    bool input_reference = false;
    bool input_enabled = false;
    bool output_enabled = false;
    int input_sample_rate = 0;
    int output_sample_rate = 0;
    int input_channels = 1;
    int output_channels = 1;
    int output_volume = 85;
    float input_gain = 0.0;

    // Pure virtual methods for reading and writing audio data
    virtual int Read(int16_t *dest, int samples) = 0;
    virtual int Write(const int16_t *data, int samples) = 0;

public:
    // Constructor and destructor
    AudioCodec();
    virtual ~AudioCodec();

    // Define public methods
    virtual void SetOutputVolume(int volume);
    virtual void SetInputGain(float gain);
    virtual void EnableInput(bool enable);
    virtual void EnableOutput(bool enable);

    // Define data input/output methods
    virtual void OutputData(std::vector<int16_t> &data);
    virtual bool InputData(std::vector<int16_t> &data);

    // Define start method
    virtual void Start();

    // Define getter methods
    inline bool Duplex() const { return duplex; }
    inline bool InputReference() const { return input_reference; }
    inline int InputSampleRate() const { return input_sample_rate; }
    inline int OutputSampleRate() const { return output_sample_rate; }
    inline int InputChannels() const { return input_channels; }
    inline int OutputChannels() const { return output_channels; }
    inline int OutputVolume() const { return output_volume; }
    inline float InputGain() const { return input_gain; }
    inline bool InputEnabled() const { return input_enabled; }
    inline bool OutputEnabled() const { return output_enabled; }
};

#endif