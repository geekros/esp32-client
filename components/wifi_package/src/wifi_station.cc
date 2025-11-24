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
#include "wifi_station.h"

// Define log tag
#define TAG "[client:components:wifi:station]"

// Constructor
WifiStation::WifiStation()
{
    // Create event group
    event_group = xEventGroupCreate();

    // Read configuration from NVS
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %d", err);
    }

    // Read max TX power and remember BSSID settings
    err = nvs_get_i8(nvs, "max_tx_power", &max_tx_power);
    if (err != ESP_OK)
    {
        max_tx_power = 52;
    }

    // Read remember BSSID setting
    err = nvs_get_u8(nvs, "remember_bssid", &remember_bssid);
    if (err != ESP_OK)
    {
        remember_bssid = 0;
    }

    // Close NVS handle
    nvs_close(nvs);
}

// Destructor
WifiStation::~WifiStation()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Start WiFi station
void WifiStation::Start()
{
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Register WiFi event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiStation::WifiEventHandler, this, &instance_any_id));

    // Register IP event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiStation::IpEventHandler, this, &instance_got_ip));

    // Create default WiFi station
    station_netif = esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;

    // Initialize WiFi
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Set WiFi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Set maximum TX power if configured
    if (max_tx_power != 0)
    {
        ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(max_tx_power));
    }

    // Setup the timer to scan WiFi
    esp_timer_create_args_t timer_args = {
        .callback = [](void *arg)
        {
            esp_wifi_scan_start(nullptr, false);
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_scan_timer",
        .skip_unhandled_events = true,
    };

    // Create the timer
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle_));
}

// Add WiFi authentication information
void WifiStation::Authentication(const std::string &&ssid, const std::string &&password)
{
    auto &wifi_manager = WifiManager::Instance();
    wifi_manager.Add(ssid, password);
}

// Check if WiFi is connected
bool WifiStation::IsConnected()
{
    return xEventGroupGetBits(event_group) & WIFI_EVENT_CONNECTED;
}

// Wait for WiFi to be connected
bool WifiStation::WaitForConnected(int timeout_ms)
{
    auto bits = xEventGroupWaitBits(event_group, WIFI_EVENT_CONNECTED, pdFALSE, pdFALSE, timeout_ms / portTICK_PERIOD_MS);
    return (bits & WIFI_EVENT_CONNECTED) != 0;
}

// Get WiFi RSSI
int8_t WifiStation::GetRSSI()
{
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    return ap_info.rssi;
}

// Get connected WiFi channel
uint8_t WifiStation::GetChannel()
{
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    return ap_info.primary;
}

// Set WiFi power save mode
void WifiStation::SetPowerSaveMode(bool enabled)
{
    ESP_ERROR_CHECK(esp_wifi_set_ps(enabled ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE));
}

// Register on scan begin callback
void WifiStation::OnScanBegin(std::function<void()> on_scan_begin)
{
    on_scan_begin = on_scan_begin;
}

// Register on connect callback
void WifiStation::OnConnect(std::function<void(const std::string &ssid)> on_connect)
{
    on_connect = on_connect;
}

// Register on connected callback
void WifiStation::OnConnected(std::function<void(const std::string &ssid)> on_connected)
{
    on_connected = on_connected;
}

// Handle scan result
void WifiStation::HandleScanResult()
{
    // Get number of access points found
    uint16_t ap_num = 0;
    esp_wifi_scan_get_ap_num(&ap_num);

    // Allocate memory for access point records
    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(ap_num * sizeof(wifi_ap_record_t));
    esp_wifi_scan_get_ap_records(&ap_num, ap_records);

    // Sort access points by RSSI
    std::sort(ap_records, ap_records + ap_num, [](const wifi_ap_record_t &a, const wifi_ap_record_t &b)
              { return a.rssi > b.rssi; });

    // Check stored SSIDs
    auto &ssid_manager = WifiManager::Instance();

    // Get stored SSID list
    auto ssid_list = ssid_manager.GetSsidList();

    // Match scanned APs with stored SSIDs
    for (int i = 0; i < ap_num; i++)
    {
        auto ap_record = ap_records[i];
        auto it = std::find_if(ssid_list.begin(), ssid_list.end(), [ap_record](const WifiManagerItem &item)
                               { return strcmp((char *)ap_record.ssid, item.ssid.c_str()) == 0; });
        if (it != ssid_list.end())
        {
            WifiStationRecord record = {
                .ssid = it->ssid,
                .password = it->password,
                .channel = ap_record.primary,
                .authmode = ap_record.authmode,
            };
            memcpy(record.bssid, ap_record.bssid, 6);
            connect_queue.push_back(record);
        }
    }

    // Free allocated memory
    free(ap_records);

    // If no matching SSIDs, wait for next scan
    if (connect_queue.empty())
    {
        esp_timer_start_once(timer_handle_, 10 * 1000);
        return;
    }

    // Start connecting to the first SSID in the queue
    StartConnect();
}

// Start connecting to WiFi
void WifiStation::StartConnect()
{
    ESP_LOGI(TAG, "Starting WiFi connection...");

    // Get the first AP record from the connect queue
    auto ap_record = connect_queue.front();
    connect_queue.erase(connect_queue.begin());
    ssid = ap_record.ssid;
    password = ap_record.password;

    // If on_connect_ callback is set, call it
    if (on_connect)
    {
        on_connect(ssid);
    }

    // Prepare WiFi configuration
    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config));
    strcpy((char *)wifi_config.sta.ssid, ap_record.ssid.c_str());
    strcpy((char *)wifi_config.sta.password, ap_record.password.c_str());

    // Set BSSID and channel if remember_bssid_ is enabled
    if (remember_bssid)
    {
        wifi_config.sta.channel = ap_record.channel;
        memcpy(wifi_config.sta.bssid, ap_record.bssid, 6);
        wifi_config.sta.bssid_set = true;
    }

    // Set WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Reset reconnect count
    reconnect_count = 0;
    ESP_ERROR_CHECK(esp_wifi_connect());
}

// WiFi event handler
void WifiStation::WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Get the WifiStation instance
    auto *this_ = static_cast<WifiStation *>(arg);

    // Handle different WiFi events
    if (event_id == WIFI_EVENT_STA_START)
    {
        // Start initial WiFi scan
        esp_wifi_scan_start(nullptr, false);
        if (this_->on_scan_begin)
        {
            this_->on_scan_begin();
        }
    }

    // Handle scan done event
    if (event_id == WIFI_EVENT_SCAN_DONE)
    {
        this_->HandleScanResult();
    }

    // Handle disconnection event
    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Clear connected bit
        xEventGroupClearBits(this_->event_group, WIFI_EVENT_CONNECTED);

        // If max reconnect count not reached, try to reconnect
        if (this_->reconnect_count < MAX_RECONNECT_COUNT)
        {
            esp_wifi_connect();
            this_->reconnect_count++;
            return;
        }

        // If there are more SSIDs in the queue, try to connect to the next one
        if (!this_->connect_queue.empty())
        {
            this_->StartConnect();
            return;
        }

        // Start a new scan after timeout
        esp_timer_start_once(this_->timer_handle_, 10 * 1000);
    }

    // Handle connection event
    if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        // Reset reconnect count
        this_->reconnect_count = 0;
    }
}

// IP event handler
void WifiStation::IpEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Get the WifiStation instance
    auto *this_ = static_cast<WifiStation *>(arg);

    // Handle got IP event
    auto *event = static_cast<ip_event_got_ip_t *>(event_data);

    char ip_address[16];
    esp_ip4addr_ntoa(&event->ip_info.ip, ip_address, sizeof(ip_address));
    this_->ip_address = ip_address;

    // Set connected bit
    xEventGroupSetBits(this_->event_group, WIFI_EVENT_CONNECTED);

    // If on_connected_ callback is set, call it
    if (this_->on_connected)
    {
        this_->on_connected(this_->ssid);
    }

    // Clear connect queue and reset reconnect count
    this_->connect_queue.clear();

    // Reset reconnect count
    this_->reconnect_count = 0;

    ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
    ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
    ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "Connected to WiFi SSID: %s, IP Address: %s", this_->ssid.c_str(), ip_address);
    ESP_LOGI(TAG, "Wifi connection successful");
}

// Stop WiFi station
void WifiStation::Stop()
{
    // If timer exists, stop and delete it
    if (timer_handle_ != nullptr)
    {
        esp_timer_stop(timer_handle_);
        esp_timer_delete(timer_handle_);
        timer_handle_ = nullptr;
    }

    // Stop WiFi
    ESP_ERROR_CHECK(esp_wifi_stop());

    // Unregister WiFi event handler
    if (instance_any_id != nullptr)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        instance_any_id = nullptr;
    }

    // Unregister IP event handler
    if (instance_got_ip != nullptr)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
        instance_got_ip = nullptr;
    }

    // Deinitialize WiFi
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    // Clear station netif
    if (station_netif != nullptr)
    {
        station_netif = nullptr;
    }

    ESP_LOGI(TAG, "WiFi station stopped");
}