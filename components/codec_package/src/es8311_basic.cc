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
#include "es8311_basic.h"

// Define log tag
#define TAG "[client:components:codec:es8311]"

// Constructor
ES8311AudioCodec::ES8311AudioCodec(void *i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate_, int output_sample_rate_, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, gpio_num_t pa_pin_, uint8_t es8311_addr, bool use_mclk, bool pa_inverted_)
{
    // Initialize member variables
    duplex = true;
    input_reference = false;
    input_channels = 1;
    input_sample_rate = input_sample_rate_;
    output_sample_rate = output_sample_rate_;
    pa_pin = pa_pin_;
    pa_inverted = pa_inverted_;
    input_gain = 30;

    // Create duplex I2S channels
    CreateDuplexChannels(mclk, bclk, ws, dout, din);

    // Define I2S configuration
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
    };

    // New codec control interface
    data_if_t = audio_codec_new_i2s_data(&i2s_cfg);

    // Define I2C configuration
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = i2c_port,
        .addr = es8311_addr,
        .bus_handle = i2c_master_handle,
    };

    // New codec control interface
    ctrl_if_t = audio_codec_new_i2c_ctrl(&i2c_cfg);

    // New codec GPIO interface
    gpio_if_t = audio_codec_new_gpio();

    // Define ES8311 codec configuration
    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = ctrl_if_t;
    es8311_cfg.gpio_if = gpio_if_t;
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
    es8311_cfg.pa_pin = pa_pin;
    es8311_cfg.use_mclk = use_mclk;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    es8311_cfg.pa_reverted = pa_inverted;
    codec_if_t = es8311_codec_new(&es8311_cfg);

    // Check if codec interface was created successfully
    if (codec_if_t == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create Es8311AudioCodec");
    }
    else
    {
        ESP_LOGI(TAG, "Es8311AudioCodec initialized");
    }
}

// Destructor
ES8311AudioCodec::~ES8311AudioCodec()
{
    esp_codec_dev_delete(dev);

    audio_codec_delete_codec_if(codec_if_t);
    audio_codec_delete_ctrl_if(ctrl_if_t);
    audio_codec_delete_gpio_if(gpio_if_t);
    audio_codec_delete_data_if(data_if_t);
}

// Private method to create duplex channels
void ES8311AudioCodec::CreateDuplexChannels(gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din)
{
    // Configure I2S channel
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle));

    // Configure standard I2S settings
    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg.sample_rate_hz = (uint32_t)output_sample_rate;
    std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
#ifdef I2S_HW_VERSION_2
    std_cfg.clk_cfg.ext_clk_freq_hz = 0;
#endif
    std_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
    std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
    std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    std_cfg.slot_cfg.ws_width = I2S_DATA_BIT_WIDTH_16BIT;
    std_cfg.slot_cfg.ws_pol = false;
    std_cfg.slot_cfg.bit_shift = true;
#ifdef I2S_HW_VERSION_2
    std_cfg.slot_cfg.left_align = true;
    std_cfg.slot_cfg.big_endian = false;
    std_cfg.slot_cfg.bit_order_lsb = false;
#endif
    std_cfg.gpio_cfg.mclk = mclk;
    std_cfg.gpio_cfg.bclk = bclk;
    std_cfg.gpio_cfg.ws = ws;
    std_cfg.gpio_cfg.dout = dout;
    std_cfg.gpio_cfg.din = din;
    std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.ws_inv = false;

    // Initialize I2S channels in standard mode
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
}

// Private method to update device state
void ES8311AudioCodec::UpdateDeviceState()
{
    // Check if input or output is enabled
    if ((input_enabled || output_enabled) && dev == nullptr)
    {
        // Create codec device
        esp_codec_dev_cfg_t dev_cfg = {};
        dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN_OUT;
        dev_cfg.codec_if = codec_if_t;
        dev_cfg.data_if = data_if_t;
        dev = esp_codec_dev_new(&dev_cfg);

        // Configure sample info
        esp_codec_dev_sample_info_t fs = {};
        fs.bits_per_sample = 16;
        fs.channel = 1;
        fs.channel_mask = 0;
        fs.sample_rate = (uint32_t)input_sample_rate;
        fs.mclk_multiple = 0;

        // Open codec device and set parameters
        ESP_ERROR_CHECK(esp_codec_dev_open(dev, &fs));
        ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(dev, input_gain));
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(dev, output_volume));
    }
    else if (!input_enabled && !output_enabled && dev != nullptr)
    {
        esp_codec_dev_close(dev);
        dev = nullptr;
    }

    // Configure PA pin
    if (pa_pin != GPIO_NUM_NC)
    {
        int level = output_enabled ? 1 : 0;
        gpio_set_level(pa_pin, pa_inverted ? !level : level);
    }
}

// Override public method to set output volume
void ES8311AudioCodec::SetOutputVolume(int volume)
{
    // Set output volume on codec device
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(dev, volume));

    // Call base class method
    AudioCodec::SetOutputVolume(volume);
}

void ES8311AudioCodec::EnableInput(bool enable)
{
    // Lock mutex and check codec interface
    std::lock_guard<std::mutex> lock(data_if_mutex);
    if (codec_if_t == nullptr)
    {
        return;
    }
    if (enable == input_enabled)
    {
        return;
    }

    // Call base class method and update device state
    AudioCodec::EnableInput(enable);
    UpdateDeviceState();
}

void ES8311AudioCodec::EnableOutput(bool enable)
{
    // Acquire lock and check codec interface
    std::lock_guard<std::mutex> lock(data_if_mutex);
    if (codec_if_t == nullptr)
    {
        return;
    }
    if (enable == output_enabled)
    {
        return;
    }

    // Call base class method and update device state
    AudioCodec::EnableOutput(enable);
    UpdateDeviceState();
}

// Private method to read audio data
int ES8311AudioCodec::Read(int16_t *dest, int samples)
{
    if (input_enabled)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_read(dev, (void *)dest, samples * sizeof(int16_t)));
    }
    return samples;
}

// Private method to write audio data
int ES8311AudioCodec::Write(const int16_t *data, int samples)
{
    // If output is enabled, write data to codec device
    if (output_enabled)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_write(dev, (void *)data, samples * sizeof(int16_t)));
    }

    // Return number of samples written
    return samples;
}
