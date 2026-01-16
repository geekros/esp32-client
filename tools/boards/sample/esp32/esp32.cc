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

// Include driver libraries
#include <driver/gpio.h>
#include <driver/spi_master.h>

// Include board configuration header
#include "board_config.h"

// Define log tag
#define TAG "[client:sample:board]"

// CustomBoard class definition
class CustomBoard : public BoardBasic
{
private:
    // Define private members
public:
    // Override Initialization method
    void Initialization() override
    {
        ESP_LOGI(TAG, "%s %s", BOARD_NAME, CONFIG_IDF_TARGET);
    }

    // Override GetAudioCodec method
    virtual AudioCodec *GetAudioCodec() override
    {
        return nullptr;
    }
};

// Factory function to create a BoardBasic instance
BoardBasic *CreateBoard()
{
    // Return a static CustomBoard instance
    static CustomBoard instance;

    // Return the instance
    return &instance;
}