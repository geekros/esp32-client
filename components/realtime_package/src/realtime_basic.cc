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

// Include the headers
#include "realtime_basic.h"

// Define log tag
#define TAG "[client:components:realtime:basic]"

// Constructor
RealtimeBasic::RealtimeBasic()
{
    // Create FreeRTOS event group
    event_group = xEventGroupCreate();
}

// Destructor
RealtimeBasic::~RealtimeBasic()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
    }
}

// Realtime start method
void RealtimeBasic::RealtimeStart(void)
{
    // Get access token
    response_access_token_t token_response = RealtimeAuthorize::Instance().Request();
    if (strlen(token_response.access_token) > 0)
    {
        // Convert access token to string
        std::string token_str(token_response.access_token);
        std::string masked = UtilsBasic::MaskSection(token_str, 30, token_str.size() - 30);

        // Log access token info
        ESP_LOGI(TAG, "Access token %s, Time: %d", masked.c_str(), token_response.time);

        // Set system time
        SystemTime::Instance().SetTimeSec(token_response.time);

        // Start signaling connection
        auto &signaling = SignalingBasic::Instance();

        // Set signaling callbacks
        SignalingCallbacks signaling_callbacks;
        signaling_callbacks.on_connected_callback = [this]()
        {
            ESP_LOGI(TAG, "connected");
        };
        signaling_callbacks.on_data_callback = [this](const char *data, size_t len, bool binary)
        {
            cJSON *root = cJSON_Parse(data);
            if (root)
            {
                // Get event item
                cJSON *event = cJSON_GetObjectItem(root, "event");
                if (event && event->valuestring)
                {
                    ESP_LOGI(TAG, "rtc event: %s", event->valuestring);
                }

                // Delete JSON root
                cJSON_Delete(root);
            }
        };
        signaling_callbacks.on_disconnected_callback = [this]()
        {
            ESP_LOGI(TAG, "disconnected");
        };
        signaling_callbacks.on_error_callback = [this](int error_code)
        {
            ESP_LOGE(TAG, "error: %d", error_code);
        };

        // Assign callbacks to signaling instance
        signaling.SetCallbacks(signaling_callbacks);

        // Assign callbacks to signaling instance
        signaling.Connection(token_str);

        // Create heartbeat task
        auto heartbeat = [](void *arg)
        {
            // Get WebSocket instance
            auto socket = SignalingBasic::Instance().GetSocket();

            while (true)
            {
                // Get current time
                int64_t ts = SystemTime::Instance().GetUnixTimestamp();

                // Create heartbeat message
                std::string message = "{\"event\": \"client:signaling:heartbeat\", \"data\": \"heartbeat\", \"time\": " + std::to_string(ts) + "}";

                // Send heartbeat message
                socket->Send(message);

                // Wait for next heartbeat
                vTaskDelay(pdMS_TO_TICKS(22125));
            }

            // Delete task
            vTaskDelete(NULL);
        };

        // Create heartbeat task
        xTaskCreate(heartbeat, "realtime_signaling_heartbeat_task", 4096, NULL, 4, NULL);
    }
}