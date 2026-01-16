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

#ifndef SYSTEM_SETTINGS_H
#define SYSTEM_SETTINGS_H

// Include standard headers
#include <string>
#include <ctime>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include client configuration header
#include "client_config.h"

// Include headers
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"

// SystemSettings class declaration
class SystemSettings
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Load and save settings
    bool Load();
    bool Save() const;

    // Parse and build JSON
    bool ParseJson(const std::string &json);
    std::string BuildJson() const;

    // Settings variables
    uint8_t audio_volume_{80};
    bool wifi_access_point_{false};

public:
    // Constructor and Destructor
    SystemSettings();
    ~SystemSettings();

    // Get the singleton instance of the SystemReboot class
    static SystemSettings &Instance()
    {
        static SystemSettings instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    SystemSettings(const SystemSettings &) = delete;
    SystemSettings &operator=(const SystemSettings &) = delete;

    // Initialize system settings
    bool Initialize();

    // Get audio volume and set audio volume
    uint8_t GetAudioVolume() const;
    bool SetAudioVolume(uint8_t volume);

    // Get and set WiFi Access Point mode
    bool IsWifiAccessPointMode() const;
    bool SetWifiAccessPointMode(bool enable);

    // Get and Update settings from JSON
    std::string GetJson() const;
    bool UpdateFromJson(const std::string &json);
};

#endif