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

// Include board configuration header
#include "board_config.h"

// Include driver headers
#include "driver/i2c_master.h"

// Include component headers
#include "i2c_device.h"
#include "axp2101_driver.h"
#include "es8311_audio_codec.h"

// Define log tag
#define TAG "[client:board]"

// Board initialization function
static void board_init(void)
{
}

// Define the board interface
static const board_t board_interface = {
    .board_init = board_init,
};

// Function to get the board interface
const board_t *board(void)
{
    // Return the board interface
    return &board_interface;
}