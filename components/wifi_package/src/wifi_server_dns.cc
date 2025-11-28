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
#include "wifi_server_dns.h"

// Define log tag
#define TAG "[client:components:wifi:server:dns]"

// Constructor
WifiServerDns::WifiServerDns()
{
    event_group = xEventGroupCreate();
}

// Destructor
WifiServerDns::~WifiServerDns()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// DNS server task
void WifiServerDns::Task()
{
    char buffer[512];

    // DNS server main loop
    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int len = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0)
        {
            continue;
        }

        // Simple DNS response: always respond with the gateway IP
        buffer[2] |= 0x80;
        buffer[3] |= 0x80;
        buffer[7] = 1;

        // Add answer section
        memcpy(&buffer[len], "\xc0\x0c", 2);
        len += 2;
        memcpy(&buffer[len], "\x00\x01\x00\x01\x00\x00\x00\x1c\x00\x04", 10);
        len += 10;
        memcpy(&buffer[len], &gateway.addr, 4);
        len += 4;

        ESP_LOGI(TAG, "Sending DNS response to %s", inet_ntoa(gateway.addr));

        sendto(fd, buffer, len, 0, (struct sockaddr *)&client_addr, client_addr_len);
    }
}

// Initialize DNS server
void WifiServerDns::Start(esp_ip4_addr_t gateway_data)
{
    ESP_LOGI(TAG, "Starting wifi DNS server");

    gateway = gateway_data;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
    {
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(fd);
        return;
    }

    auto wifi_service_dns_task = [](void *arg)
    {
        WifiServerDns *dns_server = static_cast<WifiServerDns *>(arg);
        dns_server->Task();
    };

    xTaskCreate(wifi_service_dns_task, "wifi_service_dns_task", 4096, this, 5, NULL);
}

// Stop DNS server
void WifiServerDns::Stop()
{
    ESP_LOGI(TAG, "Stopping wifi DNS server");
}