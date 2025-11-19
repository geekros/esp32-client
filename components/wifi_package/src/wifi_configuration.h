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

#ifndef WIFI_CONFIGURATION_H
#define WIFI_CONFIGURATION_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_wifi_types_generic.h>

// Include LWIP headers
#include <lwip/ip_addr.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// Include client configuration header
#include "client_config.h"

// Include component headers
#include "system_hostname.h"

// Include headers
#include "wifi_manager.h"
#include "wifi_station.h"
#include "wifi_server.h"
#include "wifi_server_dns.h"

// WiFi configuration constants
#define WIFI_CONFIG_AP_MAX_RECORDS 32

// WiFi configuration event bits
#define WIFI_CONFIG_CONNECTED_BIT BIT0
#define WIFI_CONFIG_FAIL_BIT BIT1

// WiFi configuration structure
typedef struct
{
    wifi_ap_record_t ap_records[WIFI_CONFIG_AP_MAX_RECORDS];
    int ap_count;

    EventGroupHandle_t event_group;
    esp_timer_handle_t scan_timer;
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_netif_t *ap_netif;

    wifi_dns_server_t dns_server;

    bool is_connecting;

    int8_t max_tx_power;
    bool remember_bssid;
    bool sleep_mode;

} wifi_configuration_t;

// Get wifi_configuration instance
wifi_configuration_t *wifi_configuration_get_instance(void);

// Start WiFi configuration
void wifi_configuration_start(void);

// Stop WiFi configuration
void wifi_configuration_stop(void);

// Test WiFi connection
bool wifi_configuration_connect_test(const char *ssid, const char *password);

// Save WiFi configuration
void wifi_configuration_save(const char *ssid, const char *password);

// Get list of available APs
int wifi_configuration_get_ap_list(wifi_ap_record_t *dest, int max);

// Get max TX power
int8_t wifi_configuration_get_max_tx_power(void);

// Get remember BSSID setting
bool wifi_configuration_get_remember_bssid(void);

// Get sleep mode setting
bool wifi_configuration_get_sleep_mode(void);

#endif