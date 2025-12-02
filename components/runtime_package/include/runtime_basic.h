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

#ifndef RUNTIME_BASIC_H
#define RUNTIME_BASIC_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// RuntimeBasic class definition
class RuntimeBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and destructor
    RuntimeBasic();
    ~RuntimeBasic();

    // Get the singleton instance of the RuntimeBasic class
    static RuntimeBasic &Instance()
    {
        static RuntimeBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    RuntimeBasic(const RuntimeBasic &) = delete;
    RuntimeBasic &operator=(const RuntimeBasic &) = delete;

    // Initialize basic runtime components
    void Init(void);
};

#endif