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

#ifndef GEEKROS_ESP32S3_BOARD_CONFIG_H
#define GEEKROS_ESP32S3_BOARD_CONFIG_H

// Board name
#define BOARD_NAME "ESP32S3 Waveshare Touch LCD 3.5C DevKit"

// Board button GPIO
#define BOARD_BUTTON_GPIO GPIO_NUM_0

// Board audio sample configuration
#define BOARD_AUDIO_INPUT_SAMPLE_RATE 24000
#define BOARD_AUDIO_OUTPUT_SAMPLE_RATE 24000

// Board I2S GPIO configuration
#define BOARD_I2S_MCLK_GPIO GPIO_NUM_12
#define BOARD_I2S_BCLK_GPIO GPIO_NUM_13
#define BOARD_I2S_WS_GPIO GPIO_NUM_15
#define BOARD_I2S_DIN_GPIO GPIO_NUM_14
#define BOARD_I2S_DOUT_GPIO GPIO_NUM_16

#define BOARD_AUDIO_CODEC_PA_PIN GPIO_NUM_NC
#define BOARD_AUDIO_CODEC_I2C_SDA_PIN GPIO_NUM_8
#define BOARD_AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_7
#define BOARD_AUDIO_CODEC_ES8311_ADDR ES8311_CODEC_DEFAULT_ADDR

#endif