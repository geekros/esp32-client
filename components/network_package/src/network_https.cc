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
#include "network_https.h"

// Define log tag
#define TAG "[client:components:network:https]"

// Constructor
NetworkHttps::NetworkHttps()
{
    // Create FreeRTOS event group
    event_group = xEventGroupCreate();
}

// Destructor
NetworkHttps::~NetworkHttps()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
    }
}

// HTTPS initialization method
std::unique_ptr<Http> NetworkHttps::InitHttps()
{
    // Get network interface
    auto network = NetworkBasic::Instance().GetNetwork();

    // Create and return HTTP instance
    auto http = network->CreateHttp(0);

    // get system chip ID
    std::string chip_id = SystemBasic::Instance().GetChipID();

    // Get current time
    int64_t ts = SystemTime::Instance().GetUnixTimestampMs();

    // Set headers
    http->SetHeader("Content-Type", "application/json");
    http->SetHeader("Content-X-Source", "hardware");
    http->SetHeader("Content-X-Device", chip_id.c_str());
    http->SetHeader("Content-X-Time", std::to_string(ts).c_str());
    http->SetHeader("Authorization", "Bearer " GEEKROS_SERVICE_GRK);

    // Return HTTP instance
    return http;
}