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

// DNS server task
static void wifi_dns_server_task(void *arg)
{
    wifi_dns_server_t *dns = (wifi_dns_server_t *)arg;
    char buffer[512];

    while (dns->running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int len = recvfrom(dns->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if (len < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                continue;
            }

            continue;
        }

        buffer[2] |= 0x80;
        buffer[3] |= 0x80;
        buffer[7] = 1;

        // Add DNS answer
        memcpy(&buffer[len], "\xC0\x0C", 2);
        len += 2;

        memcpy(&buffer[len], "\x00\x01\x00\x01\x00\x00\x00\x1C\x00\x04", 10);
        len += 10;

        memcpy(&buffer[len], &dns->gateway.addr, 4);
        len += 4;

        ESP_LOGI(TAG, "Sending DNS redirect to %s", inet_ntoa(*(struct in_addr *)&dns->gateway.addr));

        sendto(dns->fd, buffer, len, 0, (struct sockaddr *)&client_addr, client_addr_len);
    }

    vTaskDelete(NULL);
}

// Initialize DNS server
void wifi_dns_server_init(wifi_dns_server_t *dns)
{
    memset(dns, 0, sizeof(*dns));
    dns->port = 53;
    dns->fd = -1;
    dns->running = false;
}

// Start DNS server
void wifi_dns_server_start(wifi_dns_server_t *dns, esp_ip4_addr_t gateway)
{
    dns->gateway = gateway;
    dns->running = true;

    dns->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (dns->fd < 0)
    {
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(dns->port);

    if (bind(dns->fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(dns->fd);
        dns->fd = -1;
        return;
    }

    xTaskCreate(wifi_dns_server_task, "wifi_dns_server_task", 4096, dns, 5, NULL);
}

// Stop DNS server
void wifi_dns_server_stop(wifi_dns_server_t *dns)
{
    dns->running = false;
    if (dns->fd >= 0)
    {
        close(dns->fd);
        dns->fd = -1;
    }
}