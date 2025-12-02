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

#ifndef NETWORK_BASIC_H
#define NETWORK_BASIC_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_network.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include common headers
#include "lwip/netdb.h"

// Include configuration and module headers
#include "client_config.h"

// Network basic class
class NetworkBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Network ready check method
    static bool IsNetworkReady(uint32_t timeout_ms = 10000);

public:
    // Constructor and destructor
    NetworkBasic();
    ~NetworkBasic();

    // Get the singleton instance of the NetworkBasic class
    static NetworkBasic &Instance()
    {
        static NetworkBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    NetworkBasic(const NetworkBasic &) = delete;
    NetworkBasic &operator=(const NetworkBasic &) = delete;

    // Network ready notification method
    void CheckNetwork(uint32_t timeout_ms = 10000);

    // Pure virtual method to get the network interface
    NetworkInterface *GetNetwork();
};

#endif