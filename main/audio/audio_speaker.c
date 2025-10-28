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
#include "audio_speaker.h"

// Define log tag
#define TAG "[client:audio:speaker]"

// I2S TX channel handle
static i2s_chan_handle_t i2s_tx_handle;

// Function to initialize audio speaker
void audio_speaker_init()
{
    // Configure I2S channel
    i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);

    // Set DMA configurations
    i2s_new_channel(&chan_config, &i2s_tx_handle, NULL);

    // Configure I2S standard mode
    i2s_std_config_t tx_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .bclk = bclk,
            .dout = data_out,
            .ws = wclk,
            .mclk = GPIO_NUM_NC,
            .din = GPIO_NUM_NC,
        },
    };
    // Set slot mask to right channel only
    tx_config.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
    // Initialize I2S TX channel in standard mode
    i2s_std_tx_channel_init(i2s_tx_handle, &tx_config);
}