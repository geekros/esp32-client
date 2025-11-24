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

#ifndef WIFI_ACCESS_POINT_H
#define WIFI_ACCESS_POINT_H

// Include standard headers
#include <string>
#include <string>
#include <vector>
#include <mutex>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_wifi_types_generic.h>

// Include NVS headers
#include <nvs_flash.h>
#include <nvs.h>

// Include LWIP headers
#include <lwip/ip_addr.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include cJSON header
#include "cJSON.h"

// Include client configuration header
#include "client_config.h"

// Include headers
#include "wifi_manager.h"
#include "wifi_server.h"
#include "wifi_server_dns.h"
#include "system_hostname.h"

// Define event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// WifiAccessPoint class definition
class WifiAccessPoint
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Member variables
    WifiServerDns dns_server;
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_timer_handle_t scan_timer = nullptr;
    bool is_connecting = false;
    esp_netif_t *ap_netif = nullptr;

    // Internal functions
    void StartAccessPoint();
    void StartServer();

    // Event handlers
    static void WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void IpEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void SmartConfigEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    // SmartConfig event handler instance
    esp_event_handler_instance_t sc_event_instance_ = nullptr;

public:
    // Constructor and Destructor
    WifiAccessPoint();
    ~WifiAccessPoint();

    // Get singleton instance
    static WifiAccessPoint &Instance()
    {
        static WifiAccessPoint instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    WifiAccessPoint(const WifiAccessPoint &) = delete;
    WifiAccessPoint &operator=(const WifiAccessPoint &) = delete;

    // Access point functions
    void Start();
    void Stop();
    bool Connect(const std::string &ssid, const std::string &password);
    void Save(const std::string &ssid, const std::string &password);

    // Get list of available access points
    std::vector<wifi_ap_record_t> GetAccessPoints();

    // Member variables
    std::mutex mutex;
    std::vector<wifi_ap_record_t> ap_records;

    // Configuration parameters
    int8_t max_tx_power = 52;
    bool remember_bssid;
    bool sleep_mode;
};

#endif