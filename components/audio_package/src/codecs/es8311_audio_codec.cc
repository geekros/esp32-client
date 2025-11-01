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
#include "es8311_audio_codec.h"

// ES8311 audio codec constructor
Es8311AudioCodec_t *Es8311AudioCodec_Create(void *i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate, int output_sample_rate, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, gpio_num_t pa_pin, uint8_t es8311_addr, bool use_mclk, bool pa_inverted)
{
    // Allocate memory for codec structure
    Es8311AudioCodec_t *codec = (Es8311AudioCodec_t *)calloc(1, sizeof(Es8311AudioCodec_t));
    if (!codec)
    {
        return NULL;
    }

    // Initialize codec parameters
    codec->duplex = true;
    codec->input_reference = false;
    codec->input_channels = 1;
    codec->input_sample_rate = input_sample_rate;
    codec->output_sample_rate = output_sample_rate;
    codec->pa_pin = pa_pin;
    codec->pa_inverted = pa_inverted;
    codec->input_gain = 40;
    codec->output_volume = 70;

    // Create mutex for codec
    codec->mutex = xSemaphoreCreateMutex();
    if (codec->mutex == NULL)
    {
        free(codec);
        return NULL;
    }

    // Create duplex I2S channels
    Es8311AudioCodec_CreateDuplexChannels(codec, mclk, bclk, ws, dout, din);

    // Configure I2S for audio codec data interface
    audio_codec_i2s_cfg_t i2s_cfg = {};
    i2s_cfg.port = I2S_NUM_0;
    i2s_cfg.rx_handle = codec->rx_handle;
    i2s_cfg.tx_handle = codec->tx_handle;

    // Create audio codec data interface
    codec->data_if = audio_codec_new_i2s_data(&i2s_cfg);

    // Configure I2C for audio codec control interface
    audio_codec_i2c_cfg_t i2c_cfg = {};
    i2c_cfg.port = i2c_port;
    i2c_cfg.addr = es8311_addr;
    i2c_cfg.bus_handle = i2c_master_handle;

    // Create audio codec control interface
    codec->ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    // Create audio codec GPIO interface
    codec->gpio_if = audio_codec_new_gpio();

    // Configure ES8311 codec
    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = codec->ctrl_if;
    es8311_cfg.gpio_if = codec->gpio_if;
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
    es8311_cfg.pa_pin = pa_pin;
    es8311_cfg.use_mclk = use_mclk;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    es8311_cfg.pa_reverted = pa_inverted;

    // Create ES8311 codec interface
    codec->codec_if = es8311_codec_new(&es8311_cfg);
    if (!codec->codec_if)
    {
        vSemaphoreDelete(codec->mutex);
        free(codec);
        return NULL;
    }

    // Return codec instance
    return codec;
}

// ES8311 audio codec destructor
void Es8311AudioCodec_Destroy(Es8311AudioCodec_t *codec)
{
    // Free codec resources
    if (!codec)
    {
        return;
    }

    // Take mutex before deleting
    if (codec->mutex)
    {
        xSemaphoreTake(codec->mutex, portMAX_DELAY);
    }

    // Delete codec device
    esp_codec_dev_delete(codec->dev);
    audio_codec_delete_codec_if(codec->codec_if);
    audio_codec_delete_ctrl_if(codec->ctrl_if);
    audio_codec_delete_gpio_if(codec->gpio_if);
    audio_codec_delete_data_if(codec->data_if);

    // Give and delete mutex
    if (codec->mutex)
    {
        vSemaphoreDelete(codec->mutex);
    }

    // Free codec structure
    free(codec);
}

// Update ES8311 device state
void Es8311AudioCodec_UpdateDeviceState(Es8311AudioCodec_t *codec)
{
    // Free codec resources
    if (!codec)
    {
        return;
    }

    // If input or output is enabled, create codec device if not already created
    if ((codec->input_enabled || codec->output_enabled) && codec->dev == NULL)
    {
        // Configure codec device
        esp_codec_dev_cfg_t dev_cfg = {};
        dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN_OUT;
        dev_cfg.codec_if = codec->codec_if;
        dev_cfg.data_if = codec->data_if;

        // Create codec device
        codec->dev = esp_codec_dev_new(&dev_cfg);

        // Open codec device with sample format
        esp_codec_dev_sample_info_t fs = {};
        fs.bits_per_sample = 16;
        fs.channel = 1;
        fs.channel_mask = 0;
        fs.sample_rate = (uint32_t)codec->input_sample_rate;
        fs.mclk_multiple = 0;

        // Open codec device and set gains/volume
        ESP_ERROR_CHECK(esp_codec_dev_open(codec->dev, &fs));
        ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(codec->dev, codec->input_gain));
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(codec->dev, codec->output_volume));
    }
    else if (!codec->input_enabled && !codec->output_enabled && codec->dev != NULL)
    {
        // Delete codec device
        esp_codec_dev_close(codec->dev);

        // Set device handle to NULL
        codec->dev = NULL;
    }

    // Set PA pin level
    if (codec->pa_pin != GPIO_NUM_NC)
    {
        // Set PA pin level based on output enabled state
        int level = codec->output_enabled ? 1 : 0;

        // Set GPIO level
        gpio_set_level(codec->pa_pin, codec->pa_inverted ? !level : level);
    }
}

// Create ES8311 duplex channels
void Es8311AudioCodec_CreateDuplexChannels(Es8311AudioCodec_t *codec, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din)
{
    // Configure I2S channel
    i2s_chan_config_t chan_cfg = {};
    chan_cfg.id = I2S_NUM_0;
    chan_cfg.role = I2S_ROLE_MASTER;
    chan_cfg.dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM;
    chan_cfg.dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM;
    chan_cfg.auto_clear_after_cb = true;
    chan_cfg.auto_clear_before_cb = false;
    chan_cfg.intr_priority = 0;

    // Configure I2S pins
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &codec->tx_handle, &codec->rx_handle));

    // Configure I2S standard mode
    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg.sample_rate_hz = (uint32_t)codec->output_sample_rate;
    std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    std_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
    std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
    std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    std_cfg.slot_cfg.ws_width = I2S_DATA_BIT_WIDTH_16BIT;
    std_cfg.slot_cfg.ws_pol = false;
    std_cfg.slot_cfg.bit_shift = true;
    std_cfg.gpio_cfg.mclk = mclk;
    std_cfg.gpio_cfg.bclk = bclk;
    std_cfg.gpio_cfg.ws = ws;
    std_cfg.gpio_cfg.dout = dout;
    std_cfg.gpio_cfg.din = din;
    std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.ws_inv = false;

    // Initialize I2S channels in standard mode
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(codec->tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(codec->rx_handle, &std_cfg));
}

// Set ES8311 output volume
void Es8311AudioCodec_SetOutputVolume(Es8311AudioCodec_t *codec, int volume)
{
    // If codec or device handle is NULL, return
    if (!codec || !codec->dev)
    {
        return;
    }

    // Set output volume
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(codec->dev, volume));

    // Update codec output volume
    codec->output_volume = volume;
}

// Enable or disable ES8311 input
void Es8311AudioCodec_EnableInput(Es8311AudioCodec_t *codec, bool enable)
{
    // If codec or codec interface is NULL, return
    if (!codec || !codec->codec_if)
    {
        return;
    }

    // Take mutex before updating input state
    xSemaphoreTake(codec->mutex, portMAX_DELAY);

    // If input state is unchanged, return
    if (enable != codec->input_enabled)
    {
        // Update input enabled state
        codec->input_enabled = enable;

        // Update device state
        Es8311AudioCodec_UpdateDeviceState(codec);
    }

    // Give mutex after updating input state
    xSemaphoreGive(codec->mutex);
}

// Enable or disable ES8311 output
void Es8311AudioCodec_EnableOutput(Es8311AudioCodec_t *codec, bool enable)
{
    // If codec or codec interface is NULL, return
    if (!codec || !codec->codec_if)
    {
        return;
    }

    xSemaphoreTake(codec->mutex, portMAX_DELAY);

    // If output state is unchanged, return
    if (enable != codec->output_enabled)
    {
        // Update output enabled state
        codec->output_enabled = enable;

        // Update device state
        Es8311AudioCodec_UpdateDeviceState(codec);
    }

    // Give mutex after updating output state
    xSemaphoreGive(codec->mutex);
}

// Read samples from ES8311 codec
int Es8311AudioCodec_Read(Es8311AudioCodec_t *codec, int16_t *dest, int samples)
{
    // If codec is NULL or input is not enabled, return 0
    if (!codec || !codec->input_enabled)
    {
        return 0;
    }

    // Read samples from codec device
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_read(codec->dev, dest, samples * sizeof(int16_t)));

    // Return number of samples read
    return samples;
}

// Write samples to ES8311 codec
int Es8311AudioCodec_Write(Es8311AudioCodec_t *codec, const int16_t *data, int samples)
{
    // If codec is NULL or output is not enabled, return 0
    if (!codec || !codec->output_enabled)
    {
        return 0;
    }

    // Write samples to codec device
    void *buf = (void *)data;

    // Write samples to codec device
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_write(codec->dev, buf, samples * sizeof(int16_t)));

    // Return number of samples written
    return samples;
}