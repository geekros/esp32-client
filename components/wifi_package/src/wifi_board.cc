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
#include "wifi_board.h"

// Define log tag
#define TAG "[client:components:wifi:board]"

// Constructor
WifiBoard::WifiBoard()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
WifiBoard::~WifiBoard()
{
    // Delete event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
        event_group = nullptr;
    }
}

// Start network function
void WifiBoard::StartNetwork()
{
    // Initialize WiFi manager
    auto &wifi_manager = WifiManager::Instance();
    auto ssid_list = wifi_manager.GetSsidList();
    if (ssid_list.empty())
    {
        // No SSIDs configured, enter WiFi Access Point mode
        EnterWifiAccessPoint();
    }
    else
    {
        // SSIDs configured, enter WiFi Station mode
        EnterWifiStation();
    }
}

// Enter WiFi Access Point mode
void WifiBoard::EnterWifiAccessPoint()
{
    // Initialize access point mode if no SSIDs are configured
    auto &wifi_access_point = WifiAccessPoint::Instance();

    // Start access point mode
    wifi_access_point.Start();

    // Call the callback if set
    if (callbacks.on_access_point)
    {
        callbacks.on_access_point();
    }

    // Keep the application running in access point mode
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// Enter WiFi Station mode
void WifiBoard::EnterWifiStation()
{
    // Initialize station mode and start
    auto &wifi_station = WifiStation::Instance();

    // Start WiFi station
    wifi_station.Start();

    // Wait for connection
    if (!wifi_station.WaitForConnected(60 * 1000))
    {
        wifi_station.Stop();
        return;
    }

    // Call the callback if set
    if (callbacks.on_station)
    {
        callbacks.on_station();
    }
}

// Set WiFi callbacks
void WifiBoard::SetCallbacks(WifiCallbacks &cb)
{
    // Update callbacks
    callbacks = cb;
}