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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_netif_ip_addr.h>

// Include LWIP headers
#include <lwip/sockets.h>
#include <lwip/netdb.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// DNS server structure
typedef struct
{
    int port;
    int fd;
    esp_ip4_addr_t gateway;
    bool running;
} wifi_dns_server_t;

// DNS server function declarations
void wifi_dns_server_init(wifi_dns_server_t *dns);

// Start DNS server
void wifi_dns_server_start(wifi_dns_server_t *dns, esp_ip4_addr_t gateway);

// Stop DNS server
void wifi_dns_server_stop(wifi_dns_server_t *dns);

#endif