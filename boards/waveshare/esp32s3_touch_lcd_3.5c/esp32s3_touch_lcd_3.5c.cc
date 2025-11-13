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

// Include standard libraries
#include <math.h>
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>

// Include driver headers
#include "driver/i2c_master.h"

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Include components headers
#include "board_basic.h"
#include "i2c_device.h"
#include "axp2101.h"
#include "es8311_audio_codec.h"

// Include headers
#include "board_config.h"

// Define log tag
#define TAG "[client:board]"

// I2C bus handle
static i2c_master_bus_handle_t i2c_bus = NULL;

// Audio codec handle
static Es8311AudioCodec_t *audio_codec = NULL;

// Microphone callback
static microphone_callback mic_callback = NULL;

// Audio playback queue
static QueueHandle_t audio_queue = NULL;

// Audio block structure
typedef struct
{
    int16_t *data;
    int samples;
} audio_block_t;

// I2C initialization function
static void InitializeI2c(void)
{
    // Configure I2C bus
    i2c_master_bus_config_t i2c_bus_cfg = {};
    i2c_bus_cfg.i2c_port = (i2c_port_t)I2C_NUM_0;
    i2c_bus_cfg.sda_io_num = BOARD_AUDIO_CODEC_I2C_SDA_PIN;
    i2c_bus_cfg.scl_io_num = BOARD_AUDIO_CODEC_I2C_SCL_PIN;
    i2c_bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_cfg.glitch_ignore_cnt = 7;
    i2c_bus_cfg.intr_priority = 0;
    i2c_bus_cfg.trans_queue_depth = 0;
    i2c_bus_cfg.flags.enable_internal_pullup = 1;

    // Create new I2C master bus
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus));

    // Initialize AXP2101 power management IC
    I2cDevice(i2c_bus, 0x34);
}

// AXP2101 initialization function
static void InitializeAxp2101(void)
{
    // Initialize I2C if not already done
    Axp2101Init(i2c_bus);

    WriteReg(0x22, 0b110); // PWRON > OFFLEVEL as POWEROFF Source enable
    WriteReg(0x27, 0x10);  // Hold 4s to power off

    // Disable All DCs but DC1
    WriteReg(0x80, 0x01);
    // Disable All LDOs
    WriteReg(0x90, 0x00);
    WriteReg(0x91, 0x00);

    // Set DC1 to 3.3V
    WriteReg(0x82, (3300 - 1500) / 100);

    // Set ALDO1 to 3.3V
    WriteReg(0x92, (3300 - 500) / 100);

    // Set ALDO2, BLDO1 voltages
    WriteReg(0x96, (1500 - 500) / 100);
    WriteReg(0x97, (2800 - 500) / 100);

    // Enable ALDO1, BLDO1, BLDO2
    WriteReg(0x90, 0x31);

    // Charger configuration
    WriteReg(0x64, 0x02); // CV charger voltage = 4.1V
    WriteReg(0x61, 0x02); // Precharge current = 50mA
    WriteReg(0x62, 0x08); // Main charge current = 400mA
    WriteReg(0x63, 0x01); // Termination current = 25mA
}

// Audio codec initialization function
static void InitializeAudioCodec(void)
{
    // Create ES8311 audio codec
    audio_codec = Es8311AudioCodec_Create(
        i2c_bus,
        I2C_NUM_0,
        BOARD_AUDIO_INPUT_SAMPLE_RATE,
        BOARD_AUDIO_OUTPUT_SAMPLE_RATE,
        BOARD_I2S_MCLK_GPIO,
        BOARD_I2S_BCLK_GPIO,
        BOARD_I2S_WS_GPIO,
        BOARD_I2S_DOUT_GPIO,
        BOARD_I2S_DIN_GPIO,
        BOARD_AUDIO_CODEC_PA_PIN,
        BOARD_AUDIO_CODEC_ES8311_ADDR, true, false);

    // Check if audio codec creation was successful
    if (!audio_codec)
    {
        return;
    }

    // Enable audio codec channels
    ESP_ERROR_CHECK(i2s_channel_enable(audio_codec->tx_handle));
    ESP_ERROR_CHECK(i2s_channel_enable(audio_codec->rx_handle));

    // Enable audio codec input and output
    Es8311AudioCodec_EnableInput(audio_codec, true);
    Es8311AudioCodec_EnableOutput(audio_codec, true);

    // Set initial output volume
    Es8311AudioCodec_SetOutputVolume(audio_codec, 90);
}

// Microphone task function
static void microphone_task(void *param)
{
    // Allocate buffer for microphone data
    const int chunk = 256;
    int16_t *buf = (int16_t *)malloc(chunk * sizeof(int16_t));
    if (!buf)
    {
        vTaskDelete(NULL);
        return;
    }

    // Continuously read microphone data
    while (true)
    {
        // If microphone callback is set, read data and invoke callback
        if (audio_codec && mic_callback)
        {
            // Read samples from audio codec
            int read = Es8311AudioCodec_Read(audio_codec, buf, chunk);
            if (read > 0)
            {
                // Invoke microphone callback with read data
                mic_callback(buf, read);
            }
        }

        // Delay for a short period
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Free buffer and delete task
    free(buf);
    vTaskDelete(NULL);
}

// Audio playback task function
static void audio_play_task(void *param)
{
    // Declare audio block variable
    audio_block_t block;

    // Continuously process audio playback
    while (true)
    {
        // Wait for audio block from queue
        if (xQueueReceive(audio_queue, &block, portMAX_DELAY))
        {
            // Play audio block using codec
            if (audio_codec && block.data && block.samples > 0)
            {
                // Write audio data to codec
                Es8311AudioCodec_Write(audio_codec, block.data, block.samples);
            }
            // Free audio data buffer
            free(block.data);
        }
    }
}

// Play audio function
static void play_audio(const int16_t *data, int samples)
{
    // Check if audio codec is available and output is enabled
    if (!audio_codec || !audio_codec->output_enabled)
    {
        return;
    }

    // Create audio block
    audio_block_t block = {};
    block.samples = samples;
    block.data = (int16_t *)malloc(samples * sizeof(int16_t));
    if (!block.data)
    {
        return;
    }

    // Copy audio data to block
    memcpy(block.data, data, samples * sizeof(int16_t));

    // Send audio block to playback queue
    if (xQueueSend(audio_queue, &block, 0) != pdTRUE)
    {
        // Temporary variable to hold old block
        audio_block_t old;
        if (xQueueReceive(audio_queue, &old, 0) == pdTRUE)
        {
            // Free the old block data
            free(old.data);
        }

        // Retry sending the new block
        xQueueSend(audio_queue, &block, 0);
    }
}

// Board initialization function
static void board_init(microphone_callback callback)
{
    ESP_LOGI(TAG, BOARD_NAME);

    // Initialize I2C bus
    InitializeI2c();

    // Initialize AXP2101 power
    InitializeAxp2101();

    // Initialize audio codec
    InitializeAudioCodec();

    // Set the microphone callback
    mic_callback = callback;

    // Start microphone task
    xTaskCreatePinnedToCore(microphone_task, "microphone_task", 8192, NULL, 5, NULL, 1);

    // Create audio playback queue
    audio_queue = xQueueCreate(8, sizeof(audio_block_t));

    // Start audio playback task
    xTaskCreatePinnedToCore(audio_play_task, "audio_play_task", 8192, NULL, 5, NULL, 1);
}

// Define the board structure
static const board_t board_structure = {
    .board_init = board_init,
    .play_audio = play_audio,
};

// Function to get the board structure
const board_t *board(void)
{
    // Return the board structure
    return &board_structure;
}