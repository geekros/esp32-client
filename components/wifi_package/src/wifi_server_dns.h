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

#ifndef WIFI_SERVER_DNS_H
#define WIFI_SERVER_DNS_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_netif_ip_addr.h>

// Include LWIP headers
#include <lwip/sockets.h>
#include <lwip/netdb.h>

// Include FreeRTOS headers
// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// WifiServerDns class definition
class WifiServerDns
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // DNS server parameters
    int port = 53;
    int fd = -1;
    esp_ip4_addr_t gateway;

    // DNS server free task
    void Task();

public:
    // Constructor and destructor
    WifiServerDns();
    ~WifiServerDns();

    // Get the singleton instance of the WifiServerDns class
    static WifiServerDns &Instance()
    {
        static WifiServerDns instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    WifiServerDns(const WifiServerDns &) = delete;
    WifiServerDns &operator=(const WifiServerDns &) = delete;

    // Start DNS server
    void Start(esp_ip4_addr_t gateway_data);

    // Stop DNS server
    void Stop();
};

#endif