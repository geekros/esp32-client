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
#include "wifi_access_point.h"

// Define log tag
#define TAG "[client:components:wifi:access:point]"

// Constructor
WifiAccessPoint::WifiAccessPoint()
{
    // Create event group
    event_group = xEventGroupCreate();

    // Initialize member variables
    sleep_mode = false;
}

// Destructor
WifiAccessPoint::~WifiAccessPoint()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }

    // Delete scan timer
    if (scan_timer)
    {
        esp_timer_stop(scan_timer);
        esp_timer_delete(scan_timer);
    }

    // Unregister event handlers
    if (instance_any_id)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    }
    if (instance_got_ip)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    }
}

// Get list of available access points
std::vector<wifi_ap_record_t> WifiAccessPoint::GetAccessPoints()
{
    std::lock_guard<std::mutex> lock(mutex);
    return ap_records;
}

// Start the access point
void WifiAccessPoint::Start()
{
    // Register WiFi event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiAccessPoint::WifiEventHandler, this, &instance_any_id));

    // Register IP event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiAccessPoint::IpEventHandler, this, &instance_got_ip));

    // Start access point and server
    StartAccessPoint();

    // Start server
    WifiServer::Instance().Start();

    // Start initial scan
    esp_wifi_scan_start(nullptr, false);

    // Create scan timer
    esp_timer_create_args_t timer_args = {
        .callback = [](void *arg)
        {
            auto *self = static_cast<WifiAccessPoint *>(arg);
            if (!self->is_connecting)
            {
                esp_wifi_scan_start(nullptr, false);
            }
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_scan_timer",
        .skip_unhandled_events = true,
    };

    // Create the timer
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &scan_timer));
}

// Start the access point
void WifiAccessPoint::StartAccessPoint()
{
    // Initialize TCP/IP network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default WiFi access point network interface
    ap_netif = esp_netif_create_default_wifi_ap();

    // Configure static IP for the access point
    esp_netif_ip_info_t ip_info;
    ip4addr_aton(GEEKROS_WIFI_AP_IP, (ip4_addr_t *)&ip_info.ip);
    ip4addr_aton(GEEKROS_WIFI_AP_GATEWAY, (ip4_addr_t *)&ip_info.gw);
    ip4addr_aton(GEEKROS_WIFI_AP_NETMASK, (ip4_addr_t *)&ip_info.netmask);
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

    // Start DNS server
    dns_server.Start(ip_info.gw);

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Get device hostname for SSID
    std::string ssid = SystemHostname::Instance().Get();

    // Configure WiFi access point
    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.ap.ssid, ssid.c_str());
    strcpy((char *)wifi_config.ap.password, GEEKROS_WIFI_AP_PASSWORD);
    wifi_config.ap.ssid_len = ssid.length();
    wifi_config.ap.channel = GEEKROS_WIFI_AP_CHANNEL;
    wifi_config.ap.max_connection = GEEKROS_WIFI_AP_MAX_CONNECTION;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    // Set WiFi mode and configuration
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());

#ifdef CONFIG_SOC_WIFI_SUPPORT_5G
    ESP_ERROR_CHECK(esp_wifi_set_band_mode(WIFI_BAND_MODE_AUTO));
#else
    ESP_ERROR_CHECK(esp_wifi_set_band_mode(WIFI_BAND_MODE_2G_ONLY));
#endif

    ESP_LOGI(TAG, "WiFi AP IP address: " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "WiFi AP Gateway address: " IPSTR, IP2STR(&ip_info.gw));
    ESP_LOGI(TAG, "WiFi AP Netmask address: " IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI(TAG, "WiFi AP started, ssid: %s, password: %s", ssid.c_str(), GEEKROS_WIFI_AP_PASSWORD);
    ESP_LOGW(TAG, "Open the browser and navigate to http://%s to configure WiFi", GEEKROS_WIFI_AP_IP);

    // Save NVS handle for later use
    nvs_handle_t nvs;

    // Open NVS namespace for WiFi
    esp_err_t err = nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK)
    {
        // Get maximum TX power from NVS
        err = nvs_get_i8(nvs, "max_tx_power", &max_tx_power);
        if (err == ESP_OK)
        {
            ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(max_tx_power));
        }
        else
        {
            esp_wifi_get_max_tx_power(&max_tx_power);
        }

        // Get remember BSSID flag from NVS
        uint8_t remember_bssid = 0;
        err = nvs_get_u8(nvs, "remember_bssid", &remember_bssid);
        if (err == ESP_OK)
        {
            remember_bssid = remember_bssid != 0;
        }
        else
        {
            remember_bssid = false;
        }

        // Get sleep mode flag from NVS
        uint8_t sleep_mode = 0;
        err = nvs_get_u8(nvs, "sleep_mode", &sleep_mode);
        if (err == ESP_OK)
        {
            sleep_mode = sleep_mode != 0;
        }
        else
        {
            sleep_mode = true;
        }

        // Close NVS handle
        nvs_close(nvs);
    }
}

// Connect to a WiFi access point
bool WifiAccessPoint::Connect(const std::string &ssid, const std::string &password)
{
    // Validate SSID
    if (ssid.empty())
    {
        return false;
    }

    // Validate SSID length
    if (ssid.length() > 32)
    {
        return false;
    }

    // Validate password length
    if (password.length() > 64)
    {
        return false;
    }

    // Set connecting flag and stop ongoing scan
    is_connecting = true;
    esp_wifi_scan_stop();
    xEventGroupClearBits(event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // Configure WiFi station
    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config));
    strlcpy((char *)wifi_config.sta.ssid, ssid.c_str(), 32);
    strlcpy((char *)wifi_config.sta.password, password.c_str(), 64);
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.failure_retry_cnt = 1;

    // Set WiFi station configuration and connect
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    auto ret = esp_wifi_connect();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to connect to WiFi: %d", ret);
        is_connecting = false;
        return false;
    }
    ESP_LOGI(TAG, "Connecting to WiFi %s", ssid.c_str());

    // Wait for connection result
    EventBits_t bits = xEventGroupWaitBits(event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
    is_connecting = false;
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to WiFi %s", ssid.c_str());
        esp_wifi_disconnect();
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to connect to WiFi %s", ssid.c_str());
        return false;
    }
}

// Save WiFi access point credentials
void WifiAccessPoint::Save(const std::string &ssid, const std::string &password)
{
    ESP_LOGI(TAG, "Save SSID %s %d", ssid.c_str(), ssid.length());
    WifiManager::Instance().Add(ssid, password);
}

// WiFi event handler
void WifiAccessPoint::WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Get instance pointer
    WifiAccessPoint *self = static_cast<WifiAccessPoint *>(arg);

    // Handle station connected event
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);
    }

    // Handle station disconnected event
    if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d", MAC2STR(event->mac), event->aid);
    }

    // Handle station connected to AP event
    if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        xEventGroupSetBits(self->event_group, WIFI_CONNECTED_BIT);
    }

    // Handle station disconnected from AP event
    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupSetBits(self->event_group, WIFI_FAIL_BIT);
    }

    // Handle scan done event
    if (event_id == WIFI_EVENT_SCAN_DONE)
    {
        std::lock_guard<std::mutex> lock(self->mutex);
        uint16_t ap_num = 0;
        esp_wifi_scan_get_ap_num(&ap_num);

        self->ap_records.resize(ap_num);
        esp_wifi_scan_get_ap_records(&ap_num, self->ap_records.data());

        esp_timer_start_once(self->scan_timer, 10 * 1000000);
    }
}

// IP event handler
void WifiAccessPoint::IpEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Get instance pointer
    WifiAccessPoint *self = static_cast<WifiAccessPoint *>(arg);

    // Handle got IP event
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        // Get IP event data
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        // Log IP information
        ESP_LOGI(TAG, "IPv4: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));

        // Set connected bit
        xEventGroupSetBits(self->event_group, WIFI_CONNECTED_BIT);
    }
}

// Stop the access point
void WifiAccessPoint::Stop()
{
    // Stop scan timer
    if (scan_timer)
    {
        esp_timer_stop(scan_timer);
        esp_timer_delete(scan_timer);
        scan_timer = nullptr;
    }

    // Stop server
    WifiServer::Instance().Stop();

    // Stop DNS server
    dns_server.Stop();

    // Stop event handlers
    if (instance_any_id)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
        instance_any_id = nullptr;
    }
    if (instance_got_ip)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
        instance_got_ip = nullptr;
    }

    // Stop and deinitialize WiFi
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_set_mode(WIFI_MODE_NULL);

    // Destroy access point network interface
    if (ap_netif)
    {
        esp_netif_destroy(ap_netif);
        ap_netif = nullptr;
    }

    ESP_LOGI(TAG, "Access Point stopped");
}