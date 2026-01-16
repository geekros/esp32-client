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

#ifndef POWER_BASIC_H
#define POWER_BASIC_H

// Include standard headers
#include <string>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_pm.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// PowerBasic class definition
class PowerBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    bool in_sleep_mode = false;
    int cpu_max_freq;

    // Callback functions
    std::function<void()> on_enter_sleep_mode;
    std::function<void()> on_exit_sleep_mode;
    std::function<void()> on_shutdown_request;

public:
    // Constructor and Destructor
    PowerBasic(int cpu_max_freq_value = 80);
    ~PowerBasic();

    // Get the singleton instance of the PowerBasic class
    static PowerBasic &Instance(int cpu_max_freq_value = 80)
    {
        static PowerBasic instance(cpu_max_freq_value);
        return instance;
    }

    // Delete copy constructor and assignment operator
    PowerBasic(const PowerBasic &) = delete;
    PowerBasic &operator=(const PowerBasic &) = delete;

    // Power control APIs (manual)
    void EnterSleepMode();
    void ExitSleepMode();
    void RequestShutdown();

    // Power state query
    bool IsSleepMode() const;

    // Callback setters
    void OnEnterSleepMode(std::function<void()> callback);
    void OnExitSleepMode(std::function<void()> callback);
    void OnShutdownRequest(std::function<void()> callback);
};

#endif