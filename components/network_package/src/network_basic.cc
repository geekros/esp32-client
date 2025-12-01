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
#include "network_basic.h"

// Define log tag
#define TAG "[client:components:network:basic]"

// Constructor
NetworkBasic::NetworkBasic()
{
    // Create FreeRTOS event group
    event_group = xEventGroupCreate();
}

// Destructor
NetworkBasic::~NetworkBasic()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
    }
}

// Network ready check method
bool NetworkBasic::IsNetworkReady(uint32_t timeout_ms)
{
    // Extract host from service URL
    const char *host = GEEKROS_SERVICE;
    if (strncmp(host, "https://", 8) == 0)
    {
        host = host + 8;
    }
    if (strncmp(host, "http://", 7) == 0)
    {
        host = host + 7;
    }

    // Declare addrinfo pointer
    struct addrinfo *res = NULL;

    // Record start time
    uint32_t start = esp_log_timestamp();

    // Check network readiness until timeout
    while (esp_log_timestamp() - start < timeout_ms)
    {
        if (getaddrinfo(host, NULL, NULL, &res) == 0 && res != NULL)
        {
            freeaddrinfo(res);
            return true;
        }
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }

    // Return false if timeout
    return false;
}

// Network ready notification method
void NetworkBasic::CheckNetwork(uint32_t timeout_ms)
{
    // Wait until network is ready
    while (!IsNetworkReady(timeout_ms))
    {
        vTaskDelay(800 / portTICK_PERIOD_MS);
    }
}

// Get the network interface
NetworkInterface *NetworkBasic::GetNetwork()
{
    static EspNetwork network;
    return &network;
}