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

// Define log tag
#define TAG "[client:components:codec:es8311]"

// Function to create ES8311 audio codec instance
void es8311_create(es8311_audio_codec_t *codec, void *i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate, int output_sample_rate, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, gpio_num_t pa_pin, uint8_t es8311_addr, bool use_mclk, bool pa_inverted)
{
    // Initialize codec structure
    memset(codec, 0, sizeof(es8311_audio_codec_t));

    // Initialize codec interfaces
    audio_codec_init(&codec->base, es8311_audio_codec_func());

    // Initialize codec parameters
    codec->base.duplex = true;
    codec->base.input_reference = false;
    codec->base.input_channels = 1;
    codec->base.input_sample_rate = input_sample_rate;
    codec->base.output_sample_rate = output_sample_rate;
    codec->base.input_gain = 30.0f;

    // Initialize codec interfaces
    codec->pa_pin = pa_pin;
    codec->pa_inverted = pa_inverted;

    // Create data interface mutex
    codec->data_if_mutex = xSemaphoreCreateMutex();

    // Create codec device
    es8311_create_duplex_channels(codec, mclk, bclk, ws, dout, din);

    // Create codec control interface
    audio_codec_i2s_cfg_t i2s_cfg = {};
    i2s_cfg.port = I2S_NUM_0;
    i2s_cfg.rx_handle = codec->base.rx_handle;
    i2s_cfg.tx_handle = codec->base.tx_handle;

    // Create data interface
    codec->data_if = audio_codec_new_i2s_data(&i2s_cfg);

    // Create I2C configuration
    audio_codec_i2c_cfg_t i2c_cfg = {};
    i2c_cfg.port = i2c_port;
    i2c_cfg.addr = es8311_addr;
    i2c_cfg.bus_handle = i2c_master_handle;

    // Create audio codec control interface
    codec->ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    // Create GPIO interface
    codec->gpio_if = audio_codec_new_gpio();

    // Create ES8311 codec configuration
    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = codec->ctrl_if;
    es8311_cfg.gpio_if = codec->gpio_if;
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
    es8311_cfg.pa_pin = pa_pin;
    es8311_cfg.use_mclk = use_mclk;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    es8311_cfg.pa_reverted = codec->pa_inverted;

    // Create codec interface
    codec->codec_if = es8311_codec_new(&es8311_cfg);
    if (codec->codec_if == NULL)
    {
        ESP_LOGE(TAG, "Failed to create ES8311 codec interface");
    }
    else
    {
        ESP_LOGI(TAG, "ES8311 codec interface created successfully");
    }
}

// Function to update ES8311 audio codec device state
void es8311_update_device_state(es8311_audio_codec_t *codec)
{
    // Get base audio codec
    audio_codec_t *base = &codec->base;

    // Check if input or output is enabled and device is not created
    if ((base->input_enabled || base->output_enabled) && codec->dev == NULL)
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
        fs.sample_rate = (uint32_t)codec->base.input_sample_rate;
        fs.mclk_multiple = 0;

        // Open codec device and set gain/volume
        ESP_ERROR_CHECK(esp_codec_dev_open(codec->dev, &fs));
        ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(codec->dev, base->input_gain));
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(codec->dev, base->output_volume));
    }
    else if (!base->input_enabled && !base->output_enabled && codec->dev != NULL)
    {
        // Close and delete codec device
        esp_codec_dev_close(codec->dev);

        // Set device handle to NULL
        codec->dev = NULL;
    }

    // Update PA pin state
    if (codec->pa_pin != GPIO_NUM_NC)
    {
        int level = base->output_enabled ? 1 : 0;
        gpio_set_level(codec->pa_pin, codec->pa_inverted ? !level : level);
    }
}

// Function to create duplex I2S channels for ES8311 audio codec
void es8311_create_duplex_channels(es8311_audio_codec_t *codec, gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din)
{
    // Get base audio codec
    audio_codec_t *base = &codec->base;

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
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &codec->base.tx_handle, &codec->base.rx_handle));

    // Configure I2S standard mode
    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg.sample_rate_hz = (uint32_t)codec->base.output_sample_rate;
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
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(base->tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(base->rx_handle, &std_cfg));

    ESP_LOGI(TAG, "Duplex channels created");
}

// Function to set output volume for ES8311 audio codec
void es8311_set_output_volume(audio_codec_t *base, int volume)
{
    // Get ES8311 audio codec instance
    es8311_audio_codec_t *c = (es8311_audio_codec_t *)base;

    // Update base volume
    audio_codec_func.SetOutputVolume(base, volume);

    // If device exists, set output volume
    if (c->dev)
    {
        // Set output volume
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(c->dev, volume));
    }
}

// Function to set input gain for ES8311 audio codec
void es8311_set_input_gain(audio_codec_t *base, float gain)
{
    // Get ES8311 audio codec instance
    es8311_audio_codec_t *c = (es8311_audio_codec_t *)base;

    // Update base input gain
    audio_codec_func.SetInputGain(base, gain);

    // If device exists, set input gain
    if (c->dev)
    {
        // Set input gain
        ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(c->dev, gain));
    }
}

// Function to set input gain for ES8311 audio codec
void es8311_enable_input(audio_codec_t *base, bool enable)
{
    // Get ES8311 audio codec instance
    es8311_audio_codec_t *c = (es8311_audio_codec_t *)base;

    // Take data interface mutex
    xSemaphoreTake(c->data_if_mutex, portMAX_DELAY);

    // If codec interface is NULL, release mutex and return
    if (c->codec_if == NULL)
    {
        // Release mutex and return
        xSemaphoreGive(c->data_if_mutex);
        return;
    }

    // Enable or disable input
    if (enable != base->input_enabled)
    {
        // Call base enable input function
        audio_codec_func.EnableInput(base, enable);

        // Update device state
        es8311_update_device_state(c);
    }

    // Release data interface mutex
    xSemaphoreGive(c->data_if_mutex);
}

// Function to enable output for ES8311 audio codec
void es8311_enable_output(audio_codec_t *base, bool enable)
{
    // Get ES8311 audio codec instance
    es8311_audio_codec_t *c = (es8311_audio_codec_t *)base;

    // Take data interface mutex
    xSemaphoreTake(c->data_if_mutex, portMAX_DELAY);

    // If codec interface is NULL, release mutex and return
    if (c->codec_if == NULL)
    {
        // Release mutex and return
        xSemaphoreGive(c->data_if_mutex);
        return;
    }

    // Enable or disable output
    if (enable != base->output_enabled)
    {
        // Call base enable output function
        audio_codec_func.EnableOutput(base, enable);

        // Update device state
        es8311_update_device_state(c);
    }

    // Release data interface mutex
    xSemaphoreGive(c->data_if_mutex);
}

// Function to write data to ES8311 audio codec
int es8311_read(audio_codec_t *base, int16_t *dest, int samples)
{
    // Get ES8311 audio codec instance
    es8311_audio_codec_t *c = (es8311_audio_codec_t *)base;

    // Read data if input is enabled
    if (base->input_enabled && c->dev)
    {
        // Read data from codec device
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_read(c->dev, (void *)dest, samples * sizeof(int16_t)));
    }

    // Return number of samples read
    return samples;
}

// Function to read data from ES8311 audio codec
int es8311_write(audio_codec_t *base, const int16_t *data, int samples)
{
    // Get ES8311 audio codec instance
    es8311_audio_codec_t *c = (es8311_audio_codec_t *)base;

    // Write data if output is enabled
    if (base->output_enabled && c->dev)
    {
        // Write data to codec device
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_write(c->dev, (void *)data, samples * sizeof(int16_t)));
    }

    // Return number of samples written
    return samples;
}

// Function to destroy ES8311 audio codec instance
void es8311_destroy(es8311_audio_codec_t *codec)
{
    // If codec device exists, delete it
    if (codec->dev)
    {
        // Delete codec device
        esp_codec_dev_delete(codec->dev);

        // Set device handle to NULL
        codec->dev = NULL;
    }

    // Delete codec interfaces
    audio_codec_delete_codec_if(codec->codec_if);
    audio_codec_delete_ctrl_if(codec->ctrl_if);
    audio_codec_delete_gpio_if(codec->gpio_if);
    audio_codec_delete_data_if(codec->data_if);

    // Clear codec interface pointers
    codec->codec_if = NULL;
    codec->ctrl_if = NULL;
    codec->gpio_if = NULL;
    codec->data_if = NULL;

    // Delete data interface mutex
    if (codec->data_if_mutex)
    {
        // Delete mutex
        vSemaphoreDelete(codec->data_if_mutex);

        // Set mutex handle to NULL
        codec->data_if_mutex = NULL;
    }
}

// ES8311 audio codec function implementations
static const audio_codec_func_t es8311_func = {
    .Read = es8311_read,
    .Write = es8311_write,
    .SetOutputVolume = es8311_set_output_volume,
    .SetInputGain = es8311_set_input_gain,
    .EnableInput = es8311_enable_input,
    .EnableOutput = es8311_enable_output,
    .OutputData = audio_codec_func.OutputData,
    .InputData = audio_codec_func.InputData,
    .Start = audio_codec_func.Start,
};

// Function to get ES8311 audio codec function implementations
const audio_codec_func_t *es8311_audio_codec_func(void)
{
    // Return pointer to ES8311 audio codec function implementations
    return &es8311_func;
}