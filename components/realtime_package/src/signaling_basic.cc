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

    // Set socket instance
    auto socket = socket_instance;

    // Define on connected callback
    auto on_connected = [this]()
    {
        // Invoke user-defined callback
        if (callbacks.on_connected_callback)
        {
            // Notify connection established
            callbacks.on_connected_callback();
        }
    };

    // Set on connected callback
    socket->OnConnected(on_connected);

    // Define on data callback
    auto on_data = [this](const char *data, size_t len, bool binary)
    {
        // Invoke user-defined callback
        if (callbacks.on_data_callback)
        {
            // Pass received data to callback
            callbacks.on_data_callback(data, len, binary);
        }
    };

    // Set on data callback
    socket->OnData(on_data);

    // Define on disconnected callback
    auto on_disconnected = [this]()
    {
        // Invoke user-defined callback
        if (callbacks.on_disconnected_callback)
        {
            // Notify disconnection
            callbacks.on_disconnected_callback();
        }
    };

    // Set on disconnected callback
    socket->OnDisconnected(on_disconnected);

    // Define on error callback
    auto on_error = [this](int error)
    {
        // Invoke user-defined callback
        if (callbacks.on_error_callback)
        {
            // Pass error code to callback
            callbacks.on_error_callback(error);
        }
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
    // Return the WebSocket instance
    return socket_instance;
}

// Set signaling callbacks
void SignalingBasic::SetCallbacks(SignalingCallbacks &cb)
{
    // Set the callbacks
    callbacks = cb;
}

// Send signaling message
void SignalingBasic::Send(const std::string &event, const std::string &data_json)
{
    // Check if socket is connected
    if (!socket_instance || !socket_instance->IsConnected())
    {
        ESP_LOGW(TAG, "Send failed: socket not connected");
        return;
    }

    // Create JSON message
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "event", event.c_str());
    cJSON_AddNumberToObject(root, "time", SystemTime::Instance().GetUnixTimestamp());

    // Add data field
    cJSON *data_obj = cJSON_Parse(data_json.c_str());
    if (!data_obj)
    {
        // If data_json is not a valid JSON, add it as raw string
        cJSON_AddRawToObject(root, "data", data_json.c_str());
    }
    else
    {
        // Add parsed JSON object
        cJSON_AddItemToObject(root, "data", data_obj);
    }

    // Convert to string and send
    char *msg = cJSON_PrintUnformatted(root);
    socket_instance->Send(msg, strlen(msg));

    // Free resources
    free(msg);

    // Delete JSON object
    cJSON_Delete(root);
}