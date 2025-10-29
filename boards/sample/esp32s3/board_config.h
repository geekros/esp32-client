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

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Board name
#define BOARD_NAME "ESP32S3 Sample DevKit"

// Board button GPIO
#define BOARD_BUTTON_GPIO GPIO_NUM_0

// Board audio sample configuration
#define BOARD_AUDIO_INPUT_SAMPLE_RATE 24000
#define BOARD_AUDIO_OUTPUT_SAMPLE_RATE 24000

// Board I2S GPIO configuration
#define BOARD_I2S_MCLK_GPIO GPIO_NUM_NC
#define BOARD_I2S_BCLK_GPIO GPIO_NUM_NC
#define BOARD_I2S_WS_GPIO GPIO_NUM_NC
#define BOARD_I2S_DIN_GPIO GPIO_NUM_NC
#define BOARD_I2S_DOUT_GPIO GPIO_NUM_NC

#endif