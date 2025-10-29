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
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>

// Include driver headers
#include "driver/i2c_master.h"

// Include components headers
#include "i2c_device.h"
#include "axp2101.h"

// Include configuration and module headers
#include "common_config.h"

// Include headers
#include "board_config.h"

// Define log tag
#define TAG "[client:board]"

// I2C bus handle
static i2c_master_bus_handle_t i2c_bus = NULL;

static void InitializeI2c(void)
{
    // Configure I2C bus
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = (i2c_port_t)I2C_NUM_0,
        .sda_io_num = BOARD_AUDIO_CODEC_I2C_SDA_PIN,
        .scl_io_num = BOARD_AUDIO_CODEC_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
            .allow_pd = false,
        },
    };

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

// Board initialization function
static void board_init(void)
{
    ESP_LOGI(TAG, "Initializing Waveshare ESP32S3 Touch LCD 3.5C");

    // Initialize I2C bus
    InitializeI2c();

    // Initialize AXP2101 power
    InitializeAxp2101();
}

// Define the board structure
static const board_t board_structure = {
    .board_init = board_init,
};

// Function to get the board structure
const board_t *board(void)
{
    // Return the board structure
    return &board_structure;
}