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

// Include standard libraries
#include <math.h>
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>

// Include board basic header
#include "board_basic.h"

// Board name
#define BOARD_NAME "ESP32S3 Waveshare Zero DevKit"

// Board button GPIO
#define BOARD_BUTTON_GPIO GPIO_NUM_NC
#define BOARD_BUILTIN_LED_GPIO GPIO_NUM_NC
#define BOARD_VOLUME_UP_BUTTON_GPIO GPIO_NUM_NC
#define BOARD_VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

// Board audio sample configuration
#define BOARD_AUDIO_INPUT_SAMPLE_RATE 24000
#define BOARD_AUDIO_OUTPUT_SAMPLE_RATE 24000

// Board I2S GPIO configuration
#define BOARD_I2S_MCLK_GPIO GPIO_NUM_NC
#define BOARD_I2S_BCLK_GPIO GPIO_NUM_NC
#define BOARD_I2S_WS_GPIO GPIO_NUM_NC
#define BOARD_I2S_DIN_GPIO GPIO_NUM_NC
#define BOARD_I2S_DOUT_GPIO GPIO_NUM_NC

// Board audio codec configuration
#define BOARD_AUDIO_CODEC_PA_PIN GPIO_NUM_NC
#define BOARD_AUDIO_CODEC_I2C_SDA_PIN GPIO_NUM_NC
#define BOARD_AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_NC
#define BOARD_AUDIO_CODEC_ES8311_ADDR ES8311_CODEC_DEFAULT_ADDR

// Board display configuration
#define BOARD_DISPLAY_SPI_MODE 0
#define BOARD_DISPLAY_CS_PIN GPIO_NUM_NC
#define BOARD_DISPLAY_MOSI_PIN GPIO_NUM_NC
#define BOARD_DISPLAY_MISO_PIN GPIO_NUM_NC
#define BOARD_DISPLAY_CLK_PIN GPIO_NUM_NC
#define BOARD_DISPLAY_DC_PIN GPIO_NUM_NC
#define BOARD_DISPLAY_RST_PIN GPIO_NUM_NC

#define BOARD_DISPLAY_WIDTH 480
#define BOARD_DISPLAY_HEIGHT 320
#define BOARD_DISPLAY_MIRROR_X false
#define BOARD_DISPLAY_MIRROR_Y false
#define BOARD_DISPLAY_SWAP_XY true
#define BOARD_DISPLAY_RGB_ORDER LCD_RGB_ELEMENT_ORDER_BGR
#define BOARD_DISPLAY_INVERT_COLOR true

#define BOARD_DISPLAY_OFFSET_X 0
#define BOARD_DISPLAY_OFFSET_Y 0

#define BOARD_DISPLAY_BACKLIGHT_PIN GPIO_NUM_NC
#define BOARD_DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// Board camera configuration
#define BOARD_CAMERA_PIN_PWDN GPIO_NUM_NC
#define BOARD_CAMERA_PIN_RESET GPIO_NUM_NC
#define BOARD_CAMERA_PIN_VSYNC GPIO_NUM_NC
#define BOARD_CAMERA_PIN_HREF GPIO_NUM_NC
#define BOARD_CAMERA_PIN_PCLK GPIO_NUM_NC
#define BOARD_CAMERA_PIN_XCLK GPIO_NUM_NC
#define BOARD_CAMERA_PIN_SIOD GPIO_NUM_NC
#define BOARD_CAMERA_PIN_SIOC GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D0 GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D1 GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D2 GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D3 GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D4 GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D5 GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D6 GPIO_NUM_NC
#define BOARD_CAMERA_PIN_D7 GPIO_NUM_NC

#endif