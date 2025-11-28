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

#ifndef WIFI_BOARD_H
#define WIFI_BOARD_H

// Include standard headers
#include <string>

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
#include "wifi_manager.h"
#include "wifi_station.h"
#include "wifi_access_point.h"

// Define audio callbacks structure
struct WifiCallbacks
{
    std::function<void(void)> on_station;
    std::function<void(void)> on_access_point;
};

// WifiBoard class declaration
class WifiBoard
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Wifi callbacks
    WifiCallbacks callbacks;

public:
    // Constructor and Destructor
    WifiBoard();
    ~WifiBoard();

    // Get singleton instance
    static WifiBoard &Instance()
    {
        static WifiBoard instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    WifiBoard(const WifiBoard &) = delete;
    WifiBoard &operator=(const WifiBoard &) = delete;

    // Start network function
    void StartNetwork();

    // Enter WiFi Access Point mode
    void EnterWifiAccessPoint();

    // Enter WiFi Station mode
    void EnterWifiStation();

    // Set WiFi callbacks
    void SetCallbacks(WifiCallbacks &cb);
};

#endif