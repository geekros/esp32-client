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

#ifndef DISPLAY_LVGL_BASIC_H
#define DISPLAY_LVGL_BASIC_H

// Include standard headers
#include <string>
#include <vector>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_lvgl_port.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include display basic header
#include "display_basic.h"
#include "display_lcd_basic.h"
#include "lvgl.h"

// LVGL display basic class definition
class DisplayLvglBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and Destructor
    DisplayLvglBasic();
    ~DisplayLvglBasic();

    // Initialize LVGL with the given display
    void Initialize(DisplayBasic *display);
};

#endif