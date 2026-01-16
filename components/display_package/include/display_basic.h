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

#ifndef DISPLAY_BASIC_H
#define DISPLAY_BASIC_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include display theme basic header
#include "display_theme_basic.h"

// Display basic class definition
class DisplayBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

protected:
    // Display dimensions
    int width_ = 0;
    int height_ = 0;

    // Current theme
    DisplayThemeBasic *current_theme_ = nullptr;

    // Define protected members
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;

public:
    // Constructor and Destructor
    DisplayBasic() = default;
    virtual ~DisplayBasic() = default;

    // Get display width and height
    virtual int width() const = 0;
    virtual int height() const = 0;

    // Set and get current theme
    virtual void SetTheme(DisplayThemeBasic *theme);
    virtual DisplayThemeBasic *GetTheme() { return current_theme_; }
};

#endif