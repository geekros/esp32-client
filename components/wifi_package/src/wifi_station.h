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

#ifndef WIFI_STATION_H
#define WIFI_STATION_H

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

// Include NVS headers
#include <nvs_flash.h>
#include <nvs.h>

// Include client configuration header
#include "client_config.h"

// Include headers
#include "wifi_manager.h"

// Define WiFi station mode parameters
#define WIFI_MAX_QUEUE 20
#define MAX_RECONNECT_COUNT 5

// Define maximum SSID and password lengths
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

// Define WiFi event bits
#define WIFI_EVENT_CONNECTED BIT0

// Define WiFi AP record structure
typedef struct
{
    char ssid[WIFI_SSID_MAX_LEN + 1];
    char password[WIFI_PASS_MAX_LEN + 1];
    int channel;
    wifi_auth_mode_t authmode;
    uint8_t bssid[6];
} wifi_ap_record_t2;

// Define WiFi station structure
typedef struct
{
    EventGroupHandle_t event_group;
    esp_timer_handle_t scan_timer;
    esp_event_handler_instance_t inst_wifi;
    esp_event_handler_instance_t inst_ip;
    esp_netif_t *netif;

    int8_t max_tx_power;
    uint8_t remember_bssid;
    int reconnect_count;

    char ssid[33];
    char password[65];
    char ip_address[16];

    void (*on_connect)(const char *ssid);
    void (*on_connected)(const char *ssid);
    void (*on_scan_begin)(void);

    wifi_ap_record_t2 connect_queue[WIFI_MAX_QUEUE];
    int connect_queue_count;

} wifi_station_t;

// Function to get WiFi station instance
wifi_station_t *wifi_station_get_instance(void);

// WiFi station control functions
void wifi_station_start(void);
void wifi_station_stop(void);
bool wifi_station_wait_connected(int timeout_ms);
bool wifi_station_is_connected(void);

// WiFi station information functions
int8_t wifi_station_get_rssi(void);
uint8_t wifi_station_get_channel(void);

// WiFi station configuration functions
const char *wifi_station_get_ssid(void);
const char *wifi_station_get_ip(void);

// WiFi station setup functions
void wifi_station_add_auth(const char *ssid, const char *password);
void wifi_station_set_ps(bool enabled);

// WiFi station event callback registration functions
void wifi_station_on_scan_begin(void (*cb)(void));
void wifi_station_on_connect(void (*cb)(const char *ssid));
void wifi_station_on_connected(void (*cb)(const char *ssid));

#endif