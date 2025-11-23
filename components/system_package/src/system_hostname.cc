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
#include "system_hostname.h"

// Define log tag
#define TAG "[client:components:system:hostname]"

// Constructor
SystemHostname::SystemHostname()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
SystemHostname::~SystemHostname()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Get the device hostname
std::string SystemHostname::Get()
{
    // Define static hostname buffer
    static char hostname[32];

    // Define MAC address buffer
    uint8_t mac[6];

    // Read MAC address
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Format hostname
    snprintf(hostname, sizeof(hostname), GEEKROS_NVS_NAMESPACE "-%02x%02x%02x", mac[3], mac[4], mac[5]);

    // Return hostname as string
    return std::string(hostname);
}