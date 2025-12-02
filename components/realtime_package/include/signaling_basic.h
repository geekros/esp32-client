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

#ifndef REALTIME_SIGNALING_H
#define REALTIME_SIGNALING_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_http_client.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include configuration and module headers
#include "client_config.h"

// Include components headers
#include "network_socket.h"
#include "system_basic.h"
#include "system_time.h"

// Define signaling callbacks structure
struct SignalingCallbacks
{
    std::function<void(void)> on_connected_callback;
    std::function<void(const char *data, size_t len, bool binary)> on_data_callback;
    std::function<void(void)> on_disconnected_callback;
    std::function<void(int error_code)> on_error_callback;
};

// SignalingBasic class definition
class SignalingBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // WebSocket instance
    std::shared_ptr<WebSocket> socket_instance;

    // Callbacks
    SignalingCallbacks callbacks;

public:
    SignalingBasic();
    ~SignalingBasic();

    // Get the singleton instance of the SignalingBasic class
    static SignalingBasic &Instance()
    {
        static SignalingBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    SignalingBasic(const SignalingBasic &) = delete;
    SignalingBasic &operator=(const SignalingBasic &) = delete;

    // Connection method
    void Connection(std::string token);

    // Get WebSocket instance
    std::shared_ptr<WebSocket> GetSocket();

    // Set signaling callbacks
    void SetCallbacks(SignalingCallbacks &cb);
};

#endif