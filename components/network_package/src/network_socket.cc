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
#include "network_socket.h"

// Define log tag
#define TAG "[client:components:network:socket]"

// Constructor
NetworkSocket::NetworkSocket()
{
    // Create FreeRTOS event group
    event_group = xEventGroupCreate();
}

// Destructor
NetworkSocket::~NetworkSocket()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
    }
}

// Socket connection method
std::unique_ptr<WebSocket> NetworkSocket::InitSocket()
{
    // Get network interface
    auto network = NetworkBasic::Instance().GetNetwork();

    // Create TCP socket
    auto socket = network->CreateWebSocket(0);

    // get system chip ID
    std::string chip_id = SystemBasic::Instance().GetChipID();

    // Get current time
    int64_t ts = SystemTime::Instance().GetUnixTimestampMs();

    // Set headers
    socket->SetHeader("Content-Type", "application/json");
    socket->SetHeader("Content-X-Source", "hardware");
    socket->SetHeader("Content-X-Device", chip_id.c_str());
    socket->SetHeader("Content-X-Time", std::to_string(ts).c_str());
    socket->SetHeader("Authorization", "Bearer " GEEKROS_SERVICE_GRK);

    // Return socket instance
    return socket;
}