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

#ifndef BOARD_BASIC_H
#define BOARD_BASIC_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include codec basic header
#include "codec_basic.h"

// BoardBasic class definition
class BoardBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor
    BoardBasic() = default;
    // Virtual destructor
    virtual ~BoardBasic() = default;

    // Pure virtual function for board initialization
    virtual void Initialization() = 0;

    // Pure virtual function to get the audio codec
    virtual AudioCodec *GetAudioCodec() = 0;
};

// Factory function to create a BoardBasic instance
extern BoardBasic *CreateBoard();

#endif
