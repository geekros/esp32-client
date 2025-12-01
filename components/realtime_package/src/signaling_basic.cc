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
#include "signaling_basic.h"

// Define log tag
#define TAG "[client:components:realtime:signaling]"

// Constructor
SignalingBasic::SignalingBasic()
{
    // Create FreeRTOS event group
    event_group = xEventGroupCreate();
}

// Destructor
SignalingBasic::~SignalingBasic()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
    }
}

// Realtime start method
void SignalingBasic::Connection(std::string token)
{
    // Initialize signaling connection
    socket_instance = NetworkSocket::Instance().InitSocket();

    auto socket = socket_instance;

    // Define on connected callback
    auto on_connected = [this]()
    {
        ESP_LOGI(TAG, "Connected");
    };

    // Set on connected callback
    socket->OnConnected(on_connected);

    // Define on data callback
    auto on_data = [this](const char *data, size_t len, bool binary)
    {
        cJSON *root = cJSON_Parse(data);
        if (root)
        {
            // Get event item
            cJSON *event = cJSON_GetObjectItem(root, "event");
            if (event && event->valuestring)
            {
                ESP_LOGI(TAG, "Event: %s", event->valuestring);
            }

            // Delete JSON root
            cJSON_Delete(root);
        }
    };

    // Set on data callback
    socket->OnData(on_data);

    // Define on disconnected callback
    auto on_disconnected = [this]()
    {
        ESP_LOGI(TAG, "Disconnected");
    };

    // Set on disconnected callback
    socket->OnDisconnected(on_disconnected);

    // Define on error callback
    auto on_error = [this](int error)
    {
        ESP_LOGE(TAG, "Error: %d", error);
    };

    // Set on error callback
    socket->OnError(on_error);

    // Connect to signaling server
    if (!socket->Connect((GEEKROS_SIGNALING + std::string("/realtime/signaling?token=") + token).c_str()))
    {
        ESP_LOGE(TAG, "Connection failed");
        return;
    }
}

// Get WebSocket instance
std::shared_ptr<WebSocket> SignalingBasic::GetSocket()
{
    return socket_instance;
}