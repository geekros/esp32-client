
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

// Include headers
#include "power_basic.h"

// Define log tag
#define TAG "[client:components:power:basic]"

// Constructor
PowerBasic::PowerBasic(int cpu_max_freq_value)
{
    // Initialize member variables
    cpu_max_freq = cpu_max_freq_value;

    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
PowerBasic::~PowerBasic()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
    }
}

// Power control APIs (manual)
void PowerBasic::EnterSleepMode()
{
    // Check if already in sleep mode
    if (in_sleep_mode)
    {
        return;
    }

    // Update state
    in_sleep_mode = true;

    // Notify upper layer
    if (on_enter_sleep_mode)
    {
        // Invoke callback
        on_enter_sleep_mode();
    }

    // Configure power management
    if (cpu_max_freq != -1)
    {
        // Configure PM to enter light sleep
        esp_pm_config_t pm_config = {
            .max_freq_mhz = cpu_max_freq,
            .min_freq_mhz = 40,
            .light_sleep_enable = true,
        };

        // Apply PM configuration
        esp_err_t ret = esp_pm_configure(&pm_config);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to configure PM (enter sleep): %s", esp_err_to_name(ret));
        }
    }
}

// Exit sleep / power save mode
void PowerBasic::ExitSleepMode()
{
    // Check if not in sleep mode
    if (!in_sleep_mode)
    {
        return;
    }

    // Update state
    in_sleep_mode = false;

    // Restore power management
    if (cpu_max_freq != -1)
    {
        // Configure PM to normal operation
        esp_pm_config_t pm_config = {
            .max_freq_mhz = cpu_max_freq,
            .min_freq_mhz = cpu_max_freq,
            .light_sleep_enable = false,
        };

        // Apply PM configuration
        esp_err_t ret = esp_pm_configure(&pm_config);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to configure PM (exit sleep): %s", esp_err_to_name(ret));
        }
    }

    // Notify upper layer
    if (on_exit_sleep_mode)
    {
        // Invoke callback
        on_exit_sleep_mode();
    }
}

// Request shutdown (delegated to board / PMIC)
void PowerBasic::RequestShutdown()
{
    // Notify upper layer
    if (on_shutdown_request)
    {
        // Invoke callback
        on_shutdown_request();
    }
}

// Power state query
bool PowerBasic::IsSleepMode() const
{
    // Return sleep mode state
    return in_sleep_mode;
}

// Enter sleep mode callback setter
void PowerBasic::OnEnterSleepMode(std::function<void()> callback)
{
    // Set callback
    on_enter_sleep_mode = callback;
}

// Exit sleep mode callback setter
void PowerBasic::OnExitSleepMode(std::function<void()> callback)
{
    // Set callback
    on_exit_sleep_mode = callback;
}

// Shutdown request callback setter
void PowerBasic::OnShutdownRequest(std::function<void()> callback)
{
    // Set callback
    on_shutdown_request = callback;
}