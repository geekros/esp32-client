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

// Define static wifi_station instance
static wifi_station_t wifi_station;

// Forward declaration of static functions
static void wifi_station_start_connect(void);

// Get wifi_station instance
wifi_station_t *wifi_station_get_instance(void)
{
    static bool initialized = false;

    if (!initialized)
    {
        memset(&wifi_station, 0, sizeof(wifi_station));

        wifi_station.event_group = xEventGroupCreate();

        nvs_handle_t nvs;
        esp_err_t err = nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
        if (err == ESP_OK)
        {
            nvs_get_i8(nvs, "max_tx_power", &wifi_station.max_tx_power);
            nvs_get_u8(nvs, "remember_bssid", &wifi_station.remember_bssid);
            nvs_close(nvs);
        }
        else
        {
            wifi_station.max_tx_power = 0;
            wifi_station.remember_bssid = 0;
        }

        initialized = true;
    }

    return &wifi_station;
}

// Add WiFi authentication credentials
void wifi_station_add_auth(const char *ssid, const char *password)
{
    wifi_ssid_manager_add(wifi_ssid_manager_get_instance(), ssid, password);
}

// WiFi event handler
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    wifi_station_t *w = &wifi_station;

    if (id == WIFI_EVENT_STA_START)
    {
        esp_wifi_scan_start(NULL, false);
        if (w->on_scan_begin)
        {
            w->on_scan_begin();
        }
    }

    if (id == WIFI_EVENT_SCAN_DONE)
    {
        uint16_t ap_num = 0;
        esp_wifi_scan_get_ap_num(&ap_num);
        wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_num);
        esp_wifi_scan_get_ap_records(&ap_num, ap_records);

        wifi_ssid_manager_t *mgr = wifi_ssid_manager_get_instance();
        const wifi_ssid_item_t *list = wifi_ssid_manager_get_list(mgr);
        int list_count = wifi_ssid_manager_get_count(mgr);

        w->connect_queue_count = 0;

        for (int i = 0; i < ap_num; i++)
        {
            for (int j = 0; j < list_count; j++)
            {
                wifi_ap_record_t2 *item = &w->connect_queue[w->connect_queue_count++];
                strncpy(item->ssid, list[j].ssid, 32);
                strncpy(item->password, list[j].password, 64);
                item->channel = ap_records[i].primary;
                item->authmode = ap_records[i].authmode;
                memcpy(item->bssid, ap_records[i].bssid, 6);
            }
        }
        free(ap_records);

        if (w->connect_queue_count == 0)
        {
            esp_timer_start_once(w->scan_timer, 10 * 1000);
            return;
        }

        wifi_station_start_connect();
    }

    if (id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(w->event_group, WIFI_EVENT_CONNECTED);

        if (w->reconnect_count < MAX_RECONNECT_COUNT)
        {
            w->reconnect_count++;
            esp_wifi_connect();
            return;
        }

        if (w->connect_queue_count > 0)
        {
            wifi_station_start_connect();
            return;
        }

        esp_timer_start_once(w->scan_timer, 10 * 1000);
    }
}

// IP event handler
static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    wifi_station_t *w = &wifi_station;

    ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;

    esp_ip4addr_ntoa(&event->ip_info.ip, w->ip_address, sizeof(w->ip_address));

    xEventGroupSetBits(w->event_group, WIFI_EVENT_CONNECTED);

    if (w->on_connected)
    {
        w->on_connected(w->ssid);
    }

    w->connect_queue_count = 0;
    w->reconnect_count = 0;

    // Log WiFi connected with IP
    ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
    ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
    ESP_LOGI(TAG, "Connected to WiFi SSID: %s, IP Address: %s", w->ssid, w->ip_address);
}

// Start connecting to WiFi
static void wifi_station_start_connect(void)
{
    wifi_station_t *w = &wifi_station;

    if (w->connect_queue_count <= 0)
    {
        return;
    }

    wifi_ap_record_t2 next = w->connect_queue[0];

    for (int i = 1; i < w->connect_queue_count; i++)
    {
        w->connect_queue[i - 1] = w->connect_queue[i];
    }

    w->connect_queue_count--;

    strcpy(w->ssid, next.ssid);
    strcpy(w->password, next.password);

    if (w->on_connect)
    {
        w->on_connect(w->ssid);
    }

    wifi_config_t cfg = {0};
    strcpy((char *)cfg.sta.ssid, next.ssid);
    strcpy((char *)cfg.sta.password, next.password);

    if (w->remember_bssid)
    {
        cfg.sta.channel = next.channel;
        memcpy(cfg.sta.bssid, next.bssid, 6);
        cfg.sta.bssid_set = true;
    }

    esp_wifi_set_config(WIFI_IF_STA, &cfg);
    w->reconnect_count = 0;

    esp_wifi_connect();
}

static void wifi_scan_timer_callback(void *arg)
{
    esp_wifi_scan_start(NULL, false);
}

// Start WiFi station
void wifi_station_start(void)
{
    // Disable WiFi logging
    esp_log_level_set("pp", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("net80211", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("phy_init", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_handlers", ESP_LOG_NONE);

    wifi_station_t *w = wifi_station_get_instance();

    ESP_ERROR_CHECK(esp_netif_init());

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &w->inst_wifi);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_handler, NULL, &w->inst_ip);

    w->netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    if (w->max_tx_power != 0)
    {
        esp_wifi_set_max_tx_power(w->max_tx_power);
    }

    esp_timer_create_args_t timer_args = {
        .callback = wifi_scan_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_station_scan_timer",
    };
    esp_timer_create(&timer_args, &w->scan_timer);

    ESP_LOGI(TAG, "Connecting to WiFi...");
}

// Stop WiFi station
void wifi_station_stop(void)
{
    wifi_station_t *w = wifi_station_get_instance();

    if (w->scan_timer)
    {
        esp_timer_stop(w->scan_timer);
        esp_timer_delete(w->scan_timer);
        w->scan_timer = NULL;
    }

    esp_wifi_scan_stop();

    if (w->inst_wifi)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, w->inst_wifi);
    }

    if (w->inst_ip)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, w->inst_ip);
    }

    esp_wifi_stop();
    esp_wifi_deinit();
}

// Wait until connected to WiFi or timeout
bool wifi_station_is_connected(void)
{
    return xEventGroupGetBits(wifi_station.event_group) & WIFI_EVENT_CONNECTED;
}

// Wait until connected to WiFi or timeout
bool wifi_station_wait_connected(int timeout_ms)
{
    EventBits_t bits = xEventGroupWaitBits(wifi_station.event_group, WIFI_EVENT_CONNECTED, pdFALSE, pdFALSE, timeout_ms / portTICK_PERIOD_MS);

    return (bits & WIFI_EVENT_CONNECTED) != 0;
}

// Get WiFi RSSI
int8_t wifi_station_get_rssi(void)
{
    wifi_ap_record_t info;

    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK)
    {
        return info.rssi;
    }

    return 0;
}

// Get WiFi channel
uint8_t wifi_station_get_channel(void)
{
    wifi_ap_record_t info;

    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK)
    {
        return info.primary;
    }

    return 0;
}

// Get WiFi SSID
const char *wifi_station_get_ssid(void)
{
    return wifi_station.ssid;
}

// Get WiFi IP address
const char *wifi_station_get_ip(void)
{
    return wifi_station.ip_address;
}

// Enable or disable WiFi power save
void wifi_station_set_ps(bool enabled)
{
    esp_wifi_set_ps(enabled ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE);
}