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

#ifndef SYSTEM_H
#define SYSTEM_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_spiffs.h>
#include <esp_mac.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// SystemBasic class definition
class SystemBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and Destructor
    SystemBasic();
    ~SystemBasic();

    // Get the singleton instance of the SystemBasic class
    static SystemBasic &Instance()
    {
        static SystemBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    SystemBasic(const SystemBasic &) = delete;
    SystemBasic &operator=(const SystemBasic &) = delete;

    // Initialize system components
    void Init(const std::string &base_path, const std::string &partition_label, size_t max_files);

    // Get system chip ID
    static std::string GetChipID();
};

#endif