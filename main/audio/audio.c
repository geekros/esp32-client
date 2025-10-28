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
#include "audio.h"

// Define log tag
#define TAG "[client:audio]"

// I2S TX channel handle
static i2s_chan_handle_t i2s_tx_handle;

// I2S RX channel handle
static i2s_chan_handle_t i2s_rx_handle;

// Codec device handles
static esp_codec_dev_handle_t input_dev = NULL;

// Output codec device handle
static esp_codec_dev_handle_t output_dev = NULL;

// I2C bus handle
static i2c_master_bus_handle_t g_i2c_bus = NULL;

// Streaming flag
static bool is_streaming = false;

static void audio_i2c_init(void)
{
    if (g_i2c_bus != NULL)
    {
        return;
    }

    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_AUDIO_CODEC_I2C_SDA_PIN,
        .scl_io_num = BOARD_AUDIO_CODEC_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = 1,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &g_i2c_bus));
}

// Function to initialize audio system
void audio_init(void)
{
    // Initialize audio speaker
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = SPIFFS_AUDIO_BASE_PATH,
        .partition_label = "audio",
        .max_files = 5,
        .format_if_mount_failed = true,
    };

    // Register SPIFFS for audio
    esp_vfs_spiffs_register(&spiffs_conf);

    // Initialize I2C for codec control
    audio_i2c_init();

    // Initialize codec device
    i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_config, &i2s_tx_handle, &i2s_rx_handle);

    // Configure I2S standard mode
    i2s_std_config_t i2s_tx_rx_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(24000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG((i2s_data_bit_width_t)16, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = BOARD_I2S_MCLK_GPIO,
            .bclk = BOARD_I2S_BCLK_GPIO,
            .ws = BOARD_I2S_WS_GPIO,
            .dout = BOARD_I2S_DOUT_GPIO,
            .din = BOARD_I2S_DIN_GPIO,
        },
    };

    // Initialize I2S TX channel in standard mode
    i2s_channel_init_std_mode(i2s_tx_handle, &i2s_tx_rx_config);

    // Initialize I2S RX channel in standard mode
    i2s_channel_init_std_mode(i2s_rx_handle, &i2s_tx_rx_config);

    // Start I2S TX channel
    i2s_channel_enable(i2s_tx_handle);

    // Start I2S RX channel
    i2s_channel_enable(i2s_rx_handle);

    // Create codec device for input
    audio_codec_i2s_cfg_t i2s_cfg = {
        .rx_handle = i2s_rx_handle,
        .tx_handle = i2s_tx_handle,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    // Create I2C codec control interface
    static audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES8311_CODEC_DEFAULT_ADDR,
    };
    i2c_cfg.bus_handle = g_i2c_bus;
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    // Create ES8311 codec device
    es8311_codec_cfg_t es_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .pa_pin = BOARD_AUDIO_CODEC_PA_PIN,
        .use_mclk = true,
        .hw_gain = {
            .pa_voltage = 5.0f,
            .codec_dac_voltage = 3.3f,
        },
    };
    const audio_codec_if_t *codec_if = es8311_codec_new(&es_cfg);

    uint8_t data = 0x01;
    if (ctrl_if && ctrl_if->write_reg)
    {
        int ret = ctrl_if->write_reg(ctrl_if, 0x90, 1, &data, 1);
        if (ret == 0)
        {
            ESP_LOGI(TAG, "Enabled ES8311 MIC ALDO1 (0x90=0x01)");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to write ES8311 reg 0x90: %d", ret);
        }
    }

    // Create input codec device
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    output_dev = esp_codec_dev_new(&dev_cfg);

    // Create output codec device
    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    input_dev = esp_codec_dev_new(&dev_cfg);

    // Disable codec when closed
    esp_codec_set_disable_when_closed(output_dev, false);
    esp_codec_set_disable_when_closed(input_dev, false);

    esp_codec_dev_sample_info_t fmt = {
        .sample_rate = 48000,
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .mclk_multiple = 0,
    };

    esp_codec_dev_open(output_dev, &fmt);
    esp_codec_dev_open(input_dev, &fmt);

    // Set codec volume and gain
    esp_codec_dev_set_in_gain(input_dev, 40.0);
    esp_codec_dev_set_out_vol(output_dev, 70.0);
}

// Function to start audio streaming
static void audio_stream_task(void *arg)
{
    ESP_LOGI(TAG, "Audio stream task running");

    const int BUF_SIZE = 4096;
    uint8_t *data = (uint8_t *)heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DEFAULT);
    if (!data)
    {
        ESP_LOGE(TAG, "malloc failed");
        vTaskDelete(NULL);
        return;
    }

    // while streaming
    while (is_streaming)
    {
        // Read audio data from input device
        int ret = esp_codec_dev_read(input_dev, data, BUF_SIZE);
        ESP_LOGI(TAG, "esp_codec_dev_read ret=%d", ret);
        // If data read successfully
        if (ret > 0)
        {
            // Write audio data to output device
            int w = esp_codec_dev_write(output_dev, data, ret);
            ESP_LOGI(TAG, "read=%d write=%d", ret, w);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }

    ESP_LOGI(TAG, "Audio stream task exit");

    // Free buffer
    free(data);
    data = NULL;

    // Delete task
    vTaskDelete(NULL);
}

// Function to start audio streaming
void audio_start_stream(void)
{
    // If already streaming, return
    if (is_streaming)
    {
        return;
    }

    // Set streaming flag
    is_streaming = true;

    // Create audio stream task
    BaseType_t ret = xTaskCreatePinnedToCore(audio_stream_task, "audio_stream_task", 4096, NULL, 5, NULL, 1);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create audio stream task");
    }
    else
    {
        ESP_LOGI(TAG, "Audio stream task started");
    }
}

// Function to stop audio streaming
void audio_stop_stream(void)
{
    // If not streaming, return
    if (!is_streaming)
    {
        return;
    }

    // Stop streaming
    is_streaming = false;
}