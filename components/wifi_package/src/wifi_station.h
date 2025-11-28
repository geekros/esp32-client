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
#include <string>
#include <vector>
#include <functional>

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

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include client configuration header
#include "client_config.h"

// Include headers
#include "wifi_manager.h"

// Event group bits
#define WIFI_EVENT_CONNECTED BIT0
#define MAX_RECONNECT_COUNT 5

// WifiApRecord structure
struct WifiStationRecord
{
    std::string ssid;
    std::string password;
    int channel;
    wifi_auth_mode_t authmode;
    uint8_t bssid[6];
};

// WifiStation class definition
class WifiStation
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    esp_timer_handle_t timer_handle = nullptr;
    esp_event_handler_instance_t instance_any_id = nullptr;
    esp_event_handler_instance_t instance_got_ip = nullptr;
    esp_netif_t *station_netif = nullptr;
    std::string ssid;
    std::string password;
    std::string ip_address;
    int8_t max_tx_power;
    uint8_t remember_bssid;
    int reconnect_count = 0;
    std::function<void(const std::string &ssid)> on_connect;
    std::function<void(const std::string &ssid)> on_connected;
    std::function<void()> on_scan_begin;
    std::vector<WifiStationRecord> connect_queue;

    void HandleScanResult();
    void StartConnect();
    static void WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void IpEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

public:
    // Constructor and destructor
    WifiStation();
    ~WifiStation();

    // Get the singleton instance of the WifiStation class
    static WifiStation &Instance()
    {
        static WifiStation instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    WifiStation(const WifiStation &) = delete;
    WifiStation &operator=(const WifiStation &) = delete;

    void Authentication(const std::string &&ssid, const std::string &&password);
    void Start();
    bool IsConnected();
    bool WaitForConnected(int timeout_ms = 10000);
    int8_t GetRSSI();
    std::string GetSSID() const { return ssid; }
    std::string GetIpAddress() const { return ip_address; }
    uint8_t GetChannel();
    void SetPowerSaveMode(bool enabled);
    void Stop();

    void OnConnect(std::function<void(const std::string &ssid)> on_connect_cb);
    void OnConnected(std::function<void(const std::string &ssid)> on_connected_cb);
    void OnScanBegin(std::function<void()> on_scan_begin_cb);
};

#endif