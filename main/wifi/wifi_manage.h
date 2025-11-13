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

#ifndef GEEKROS_WIFI_MANAGE_H
#define GEEKROS_WIFI_MANAGE_H

// Include standard headers
#include <stdio.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Include common headers
#include "lwip/ip4_addr.h"
#include "nvs_flash.h"

// Include configuration header
#include "common_config.h"

// Define WiFi state enumeration
typedef enum
{
    WIFI_STATE_CONNECTED = 0,    // WiFi connected
    WIFI_STATE_DISCONNECTED,     // WiFi disconnected
    WIFI_STATE_CONNECTION_FAILED // WiFi Connection failed
} wifi_state_t;

// Define WiFi state change callback type
typedef void (*p_wifi_state_change_callback)(wifi_state_t state);

// Define WiFi scan callback type
typedef void (*p_wifi_scan_callback)(int num_networks, wifi_ap_record_t *ap_records);

// Initialize WiFi management
void wifi_manage_init(const char *hostname, p_wifi_state_change_callback callback);

// Start WiFi in Access Point mode
esp_err_t wifi_manage_ap(const char *hostname);

// Scan for available WiFi networks
esp_err_t wifi_manage_scan(p_wifi_scan_callback callback);

// Connect to a WiFi network
void wifi_manage_connect(const char *ssid, const char *password);

#endif