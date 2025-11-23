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
#include "application.h"

// Define log tag
#define TAG "[client:application]"

// Constructor
Application::Application()
{
    event_group = xEventGroupCreate();
}

// Destructor
Application::~Application()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Main application entry point
void Application::Main()
{
    // Log the GeekROS version
    ESP_LOGI(TAG, "Client Version: %s", GEEKROS_VERSION);

    // Check if GeekROS service GRK and project token are configured
    if (GEEKROS_SERVICE_GRK == NULL || strlen(GEEKROS_SERVICE_GRK) == 0 || GEEKROS_SERVICE_PROJECT_TOKEN == NULL || strlen(GEEKROS_SERVICE_PROJECT_TOKEN) == 0)
    {
        ESP_LOGE(TAG, "Please configure GEEKROS_SERVICE_GRK and GEEKROS_SERVICE_PROJECT_TOKEN in menuconfig");
        return;
    }

    // Initialize basic runtime components
    RuntimeBasic::Instance().Init();

    // Initialize system components
    SystemBasic::Instance().Init(GEEKROS_SPIFFS_BASE_PATH, GEEKROS_SPIFFS_LABEL, GEEKROS_SPIFFS_MAX_FILE);

    // Initialize locale and language components
    LanguageBasic::Instance().Init();

    // Initialize WiFi manager
    auto &wifi_manager = WifiManager::Instance();
    auto ssid_list = wifi_manager.GetSsidList();
    if (ssid_list.empty())
    {
        // Initialize access point mode if no SSIDs are configured
        auto &wifi_access_point = WifiAccessPoint::Instance();

        // Start access point mode
        wifi_access_point.Start();
    }
    else
    {
        // Initialize station mode
        auto &wifi_station = WifiStation::Instance();

        wifi_station.OnScanBegin([this]()
                                 { ESP_LOGI(TAG, "WiFi scan started"); });

        wifi_station.OnConnect([this](const std::string &ssid)
                               { ESP_LOGI(TAG, "Connecting to SSID: %s", ssid.c_str()); });

        wifi_station.OnConnected([this](const std::string &ssid)
                                 { ESP_LOGI(TAG, "Connected to SSID: %s", ssid.c_str()); });

        // Start station mode
        wifi_station.Start();

        // Wait for connection
        if (!wifi_station.WaitForConnected(60 * 1000))
        {
            wifi_station.Stop();
            return;
        }
    }

    // Start the application loop
    Loop();
}

// Main application loop
void Application::Loop()
{
    // Main application loop
    while (true)
    {
        // Log a heartbeat message every 500 milliseconds
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}