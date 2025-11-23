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

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// Include standard headers
#include <string>
#include <vector>
#include <algorithm>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include NVS headers
#include <nvs_flash.h>
#include <nvs.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include client configuration header
#include "client_config.h"

// Maximum number of stored WiFi SSIDs
#define MAX_WIFI_SSID_COUNT 10

// WifiManagerItem structure
struct WifiManagerItem
{
    std::string ssid;
    std::string password;
};

// WifiManager class definition
class WifiManager
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Load and save configuration
    void Load();
    void Save();

    // Current connected SSID
    std::vector<WifiManagerItem> ssid_list;

public:
    // Constructor and destructor
    WifiManager();
    ~WifiManager();

    // Get the singleton instance of the WifiManager class
    static WifiManager &Instance()
    {
        static WifiManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    WifiManager(const WifiManager &) = delete;
    WifiManager &operator=(const WifiManager &) = delete;

    // SSID management functions
    void Add(const std::string &ssid, const std::string &password);
    void Remove(int index);
    void SetDefault(int index);
    void Clear();

    const std::vector<WifiManagerItem> &GetSsidList() const { return ssid_list; }
};

#endif