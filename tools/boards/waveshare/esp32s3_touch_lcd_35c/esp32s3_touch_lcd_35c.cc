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
#include <string>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>

// Include board configuration header
#include "board_config.h"

// Include board basic header
#include "board_basic.h"

// Include driver headers
#include "driver/i2c_master.h"

// Include component headers
#include "i2c_device.h"
#include "axp2101_driver.h"
#include "es8311_basic.h"

// Define log tag
#define TAG "[client:waveshare:board]"

// Pmic class definition inheriting from AXP2101Driver
class Pmic : public AXP2101Driver
{
public:
    // Constructor
    Pmic(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : AXP2101Driver(i2c_bus, addr)
    {
        WriteReg(0x22, 0b110); // PWRON > OFFLEVEL as POWEROFF Source enable
        WriteReg(0x27, 0x10);  // hold 4s to power off

        // Disable All DCs but DC1
        WriteReg(0x80, 0x01);
        // Disable All LDOs
        WriteReg(0x90, 0x00);
        WriteReg(0x91, 0x00);

        // Set DC1 to 3.3V
        WriteReg(0x82, (3300 - 1500) / 100);

        // Set ALDO1 to 3.3V
        WriteReg(0x92, (3300 - 500) / 100);

        WriteReg(0x96, (1500 - 500) / 100);
        WriteReg(0x97, (2800 - 500) / 100);

        // Enable ALDO1 BLDO1 BLDO2
        WriteReg(0x90, 0x31);

        WriteReg(0x64, 0x02); // CV charger voltage setting to 4.1V

        WriteReg(0x61, 0x02); // set Main battery precharge current to 50mA
        WriteReg(0x62, 0x08); // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
        WriteReg(0x63, 0x01); // set Main battery term charge current to 25mA
    }
};

// CustomBoard class definition
class CustomBoard : public BoardBasic
{
private:
    // Define private members
    Pmic *pmic = nullptr;
    i2c_master_bus_handle_t i2c_bus;

    // Private method to initialize I2C
    void InitializeI2C()
    {
        // Configure I2C master bus
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
            },
        };

        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus));
    }

    // Private method to initialize AXP2101 PMIC
    void InitializeAXP2101()
    {
        pmic = new Pmic(i2c_bus, 0x34);
    }

public:
    // Override Initialization method
    void Initialization() override
    {
        ESP_LOGI(TAG, BOARD_NAME);

        // Initialize I2C
        InitializeI2C();

        // Initialize AXP2101 PMIC
        InitializeAXP2101();
    }

    // Override GetAudioCodec method
    virtual AudioCodec *GetAudioCodec() override
    {
        // Create ES8311 audio codec instance
        static ES8311AudioCodec audio_codec(i2c_bus, I2C_NUM_0, BOARD_AUDIO_INPUT_SAMPLE_RATE, BOARD_AUDIO_OUTPUT_SAMPLE_RATE, BOARD_I2S_MCLK_GPIO, BOARD_I2S_BCLK_GPIO, BOARD_I2S_WS_GPIO, BOARD_I2S_DOUT_GPIO, BOARD_I2S_DIN_GPIO, BOARD_AUDIO_CODEC_PA_PIN, BOARD_AUDIO_CODEC_ES8311_ADDR);

        // Return audio codec instance
        return &audio_codec;
    }
};

// Factory function to create a BoardBasic instance
BoardBasic *CreateBoard()
{
    static CustomBoard instance;
    return &instance;
}