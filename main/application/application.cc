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

    // Initialize board-specific components
    BoardBasic *board = CreateBoard();
    board->Initialization();

    // Initialize audio service
    auto codec = board->GetAudioCodec();
    audio_service.Initialize(codec);

    // Set audio service callbacks
    AudioCallbacks callbacks;
    callbacks.on_send_queue_available = [this]()
    {
        xEventGroupSetBits(event_group, MAIN_EVENT_SEND_AUDIO);
    };
    callbacks.on_vad_change = [this](bool speaking)
    {
        xEventGroupSetBits(event_group, MAIN_EVENT_VAD_CHANGE);
    };
    audio_service.SetCallbacks(callbacks);

    // Enable voice processing
    audio_service.EnableVoiceProcessing(true);

    // Initialize WiFi manager
    auto &wifi_manager = WifiManager::Instance();
    auto ssid_list = wifi_manager.GetSsidList();
    if (ssid_list.empty())
    {
        // Start audio task for playback
        audio_service.StartAudioTask();

        // No SSIDs configured, enter WiFi configuration mode
        audio_service.EnableVoiceProcessing(false);

        // Play WiFi configuration sound
        audio_service.PlaySound(Lang::Sounds::OGG_WIFI_CONFIG);

        // Wait until audio service is idle
        while (!audio_service.IsIdle())
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // Initialize access point mode if no SSIDs are configured
        auto &wifi_access_point = WifiAccessPoint::Instance();

        // Start access point mode
        wifi_access_point.Start();

        // Keep the application running in access point mode
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
    else
    {
        // Initialize station mode and start
        auto &wifi_station = WifiStation::Instance();
        wifi_station.Start();

        // Wait for connection
        if (!wifi_station.WaitForConnected(60 * 1000))
        {
            wifi_station.Stop();
            return;
        }
        else
        {
            // Small delay to ensure stability
            vTaskDelay(pdMS_TO_TICKS(2000));

            // Initialize system time synchronization
            SystemTime::Instance().InitTimeSync();

            if (SystemTime::Instance().WaitForSync(10000))
            {
                // Start audio service
                audio_service.Start();

                // Play WiFi success sound
                audio_service.PlaySound(Lang::Sounds::OGG_WIFI_SUCCESS);

                // Start the application loop
                Loop();
            }
            else
            {
                ESP_LOGW(TAG, "System time synchronization timed out");
                return;
            }
        }
    }
}

// Main application loop
void Application::Loop()
{
    // Main application loop
    while (true)
    {
        // Wait for send audio or VAD change events
        auto bits = xEventGroupWaitBits(event_group, MAIN_EVENT_SEND_AUDIO | MAIN_EVENT_VAD_CHANGE, pdTRUE, pdFALSE, portMAX_DELAY);

        // Handle send audio event
        if (bits & MAIN_EVENT_SEND_AUDIO)
        {
            auto packet = audio_service.PopPacketFromSendQueue();
            if (packet)
            {
                ESP_LOGI(TAG, "Sending audio packet with timestamp: %u, payload size: %zu bytes", packet->timestamp, packet->payload.size());
            }
        }

        // Handle VAD change event
        if (bits & MAIN_EVENT_VAD_CHANGE)
        {
            // Get current VAD state
            bool speaking = audio_service.IsVoiceDetected();
            ESP_LOGI(TAG, "VAD State: %s", speaking ? "Speaking" : "Silent");
        }

        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}