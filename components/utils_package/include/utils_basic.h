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

#ifndef UTILS_BASIC_H
#define UTILS_BASIC_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// UtilsBasic class definition
class UtilsBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and Destructor
    UtilsBasic();
    ~UtilsBasic();

    // Get the singleton instance of the UtilsBasic class
    static UtilsBasic &Instance()
    {
        static UtilsBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    UtilsBasic(const UtilsBasic &) = delete;
    UtilsBasic &operator=(const UtilsBasic &) = delete;

    // Get captive portal URLs
    static const char *const *GetCaptiveUrls(size_t &count);

    // Mask section of a string
    static std::string MaskSection(const std::string text, size_t start, size_t end);
};

#endif