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

#ifndef ES8311_CODEC_H
#define ES8311_CODEC_H

// Include standard headers
#include <string>
#include <mutex>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include driver headers
#include <driver/i2c_master.h>
#include <driver/gpio.h>

// Include codec device headers
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Include headers
#include "audio_codec.h"

// ES8311AudioCodec class definition
class ES8311AudioCodec : public AudioCodec
{
private:
    // Define private members
    const audio_codec_data_if_t *data_if_t = nullptr;
    const audio_codec_ctrl_if_t *ctrl_if_t = nullptr;
    const audio_codec_if_t *codec_if_t = nullptr;
    const audio_codec_gpio_if_t *gpio_if_t = nullptr;

    // Device handle and GPIO settings
    esp_codec_dev_handle_t dev = nullptr;
    gpio_num_t pa_pin = GPIO_NUM_NC;
    bool pa_inverted = false;
    std::mutex data_if_mutex;

    // Define private methods
    void CreateDuplexChannels(gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din);
    void UpdateDeviceState();

    // Override AudioCodec methods
    virtual int Read(int16_t *dest, int samples) override;
    virtual int Write(const int16_t *data, int samples) override;

public:
    // Constructor and destructor
    ES8311AudioCodec(void *i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate_, int output_sample_rate_, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, gpio_num_t pa_pin_, uint8_t es8311_addr, bool use_mclk = true, bool pa_inverted_ = false);
    virtual ~ES8311AudioCodec();

    // Override AudioCodec public methods
    virtual void SetOutputVolume(int volume) override;
    virtual void EnableInput(bool enable) override;
    virtual void EnableOutput(bool enable) override;
};

#endif