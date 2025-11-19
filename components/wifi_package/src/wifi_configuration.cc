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
#include "wifi_configuration.h"
#include "wifi_server.h"

// Define log tag
#define TAG "[client:components:wifi:configuration]"

// Define static wifi_configuration instance
static wifi_configuration_t g_wifi_configuration;

// Define initialization flag
static bool g_initialized = false;

// STA netif for APSTA mode (local to this module)
static esp_netif_t *s_sta_netif = NULL;

// Forward declaration of static functions
static void wifi_configuration_start_ap(wifi_configuration_t *cfg);

// WiFi configuration event handlers
static void wifi_configuration_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

// IP event handler
static void wifi_configuration_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

// Scan timer callback
static void wifi_configuration_scan_timer_cb(void *arg);

// Get wifi_configuration instance
wifi_configuration_t *wifi_configuration_get_instance(void)
{
    if (!g_initialized)
    {
        memset(&g_wifi_configuration, 0, sizeof(g_wifi_configuration));
        g_wifi_configuration.event_group = xEventGroupCreate();
        g_wifi_configuration.sleep_mode = false;
        g_initialized = true;
    }

    return &g_wifi_configuration;
}

// Get max TX power
int8_t wifi_configuration_get_max_tx_power(void)
{
    return wifi_configuration_get_instance()->max_tx_power;
}

// Get remember BSSID setting
bool wifi_configuration_get_remember_bssid(void)
{
    return wifi_configuration_get_instance()->remember_bssid;
}

// Get sleep mode setting
bool wifi_configuration_get_sleep_mode(void)
{
    return wifi_configuration_get_instance()->sleep_mode;
}

// Start WiFi configuration
void wifi_configuration_start(void)
{
    // Disable WiFi logging
    esp_log_level_set("pp", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("net80211", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("phy_init", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_NONE);

    wifi_configuration_t *cfg = wifi_configuration_get_instance();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_configuration_wifi_event_handler,
        cfg,
        &cfg->instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_configuration_ip_event_handler,
        cfg,
        &cfg->instance_got_ip));

    wifi_configuration_start_ap(cfg);

    esp_wifi_scan_start(NULL, false);

    wifi_server_start();

    esp_timer_create_args_t timer_args = {
        .callback = wifi_configuration_scan_timer_cb,
        .arg = cfg,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_configuration_scan_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &cfg->scan_timer));
}

// Stop WiFi configuration
void wifi_configuration_stop(void)
{
    wifi_configuration_t *cfg = wifi_configuration_get_instance();

    if (cfg->scan_timer)
    {
        esp_timer_stop(cfg->scan_timer);
        esp_timer_delete(cfg->scan_timer);
        cfg->scan_timer = NULL;
    }

    wifi_dns_server_stop(&cfg->dns_server);
    wifi_server_stop();

    if (cfg->instance_any_id)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, cfg->instance_any_id);
        cfg->instance_any_id = NULL;
    }

    if (cfg->instance_got_ip)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, cfg->instance_got_ip);
        cfg->instance_got_ip = NULL;
    }

    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_set_mode(WIFI_MODE_NULL);

    if (cfg->ap_netif)
    {
        esp_netif_destroy(cfg->ap_netif);
        cfg->ap_netif = NULL;
    }

    if (s_sta_netif)
    {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
    }
}

// Start AP mode
static void wifi_configuration_start_ap(wifi_configuration_t *cfg)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(err);
    }

    cfg->ap_netif = esp_netif_create_default_wifi_ap();
    if (!s_sta_netif)
    {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }

    esp_netif_ip_info_t ip_info;
    ip4addr_aton(GEEKROS_WIFI_AP_IP, (ip4_addr_t *)&ip_info.ip);
    ip4addr_aton(GEEKROS_WIFI_AP_GATEWAY, (ip4_addr_t *)&ip_info.gw);
    ip4addr_aton(GEEKROS_WIFI_AP_NETMASK, (ip4_addr_t *)&ip_info.netmask);

    esp_netif_dhcps_stop(cfg->ap_netif);
    esp_netif_set_ip_info(cfg->ap_netif, &ip_info);
    esp_netif_dhcps_start(cfg->ap_netif);

    wifi_dns_server_init(&cfg->dns_server);
    wifi_dns_server_start(&cfg->dns_server, ip_info.gw);

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));

    wifi_config_t ap_config = {};
    ap_config.ap.channel = GEEKROS_WIFI_AP_CHANNEL;
    ap_config.ap.max_connection = GEEKROS_WIFI_AP_MAX_CONNECTION;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    const char *hostname = get_hostname();
    snprintf((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), "%s", hostname);
    snprintf((char *)ap_config.ap.password, sizeof(ap_config.ap.password), GEEKROS_WIFI_AP_PASSWORD);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
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
    ESP_LOGI(TAG, "WiFi AP started, ssid: %s, password: %s", ap_config.ap.ssid, ap_config.ap.password);
    ESP_LOGI(TAG, "Open the browser and navigate to http://%s to configure WiFi", GEEKROS_WIFI_AP_IP);

    nvs_handle_t nvs;
    err = nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK)
    {
        err = nvs_get_i8(nvs, "max_tx_power", &cfg->max_tx_power);
        if (err == ESP_OK)
        {
            ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(cfg->max_tx_power));
        }
        else
        {
            esp_wifi_get_max_tx_power(&cfg->max_tx_power);
        }

        uint8_t remember_bssid = 0;
        err = nvs_get_u8(nvs, "remember_bssid", &remember_bssid);
        cfg->remember_bssid = (err == ESP_OK && remember_bssid != 0) ? true : false;

        uint8_t sleep_mode = 0;
        err = nvs_get_u8(nvs, "sleep_mode", &sleep_mode);
        if (err == ESP_OK)
        {
            cfg->sleep_mode = (sleep_mode != 0);
        }
        else
        {
            cfg->sleep_mode = true;
        }

        nvs_close(nvs);
    }
    else
    {
        cfg->max_tx_power = 0;
        cfg->remember_bssid = false;
        cfg->sleep_mode = true;
    }
}

// Scan timer callback
static void wifi_configuration_scan_timer_cb(void *arg)
{
    wifi_configuration_t *cfg = (wifi_configuration_t *)arg;
    if (!cfg->is_connecting)
    {
        esp_err_t err = esp_wifi_scan_start(NULL, false);
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "esp_wifi_scan_start failed: %s", esp_err_to_name(err));
        }
    }
}

// WiFi event handler
static void wifi_configuration_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_configuration_t *cfg = (wifi_configuration_t *)arg;

    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *ev = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "STA " MACSTR " joined, AID=%d", MAC2STR(ev->mac), ev->aid);
    }

    if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *ev = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "STA " MACSTR " left, AID=%d", MAC2STR(ev->mac), ev->aid);
    }

    if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        xEventGroupSetBits(cfg->event_group, WIFI_CONFIG_CONNECTED_BIT);
    }

    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupSetBits(cfg->event_group, WIFI_CONFIG_FAIL_BIT);
    }

    if (event_id == WIFI_EVENT_SCAN_DONE)
    {
        uint16_t ap_num = 0;
        esp_wifi_scan_get_ap_num(&ap_num);
        if (ap_num > WIFI_CONFIG_AP_MAX_RECORDS)
        {
            ap_num = WIFI_CONFIG_AP_MAX_RECORDS;
        }

        cfg->ap_count = ap_num;
        if (ap_num > 0)
        {
            esp_wifi_scan_get_ap_records(&ap_num, cfg->ap_records);
        }

        if (cfg->scan_timer)
        {
            esp_timer_start_once(cfg->scan_timer, 10 * 1000000ULL);
        }
    }
}

// IP event handler
static void wifi_configuration_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_configuration_t *cfg = (wifi_configuration_t *)arg;

    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IPv4: " IPSTR, IP2STR(&ev->ip_info.ip));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&ev->ip_info.gw));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&ev->ip_info.netmask));

        xEventGroupSetBits(cfg->event_group, WIFI_CONFIG_CONNECTED_BIT);
    }
}

bool wifi_configuration_connect_test(const char *ssid, const char *password)
{
    wifi_configuration_t *cfg = wifi_configuration_get_instance();

    if (!ssid || strlen(ssid) == 0 || strlen(ssid) > 32)
    {
        ESP_LOGW(TAG, "Invalid SSID");
        return false;
    }

    if (password && strlen(password) > 64)
    {
        ESP_LOGW(TAG, "Invalid password length");
        return false;
    }

    cfg->is_connecting = true;

    esp_wifi_scan_stop();
    if (cfg->scan_timer)
    {
        esp_timer_stop(cfg->scan_timer);
    }

    xEventGroupClearBits(cfg->event_group, WIFI_CONFIG_CONNECTED_BIT | WIFI_CONFIG_FAIL_BIT);

    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err != ESP_OK || mode == WIFI_MODE_NULL)
    {
        ESP_LOGE(TAG, "Invalid WiFi mode, cannot connect_test (err=%s, mode=%d)",
                 esp_err_to_name(err), mode);
        cfg->is_connecting = false;
        if (cfg->scan_timer)
        {
            esp_timer_start_once(cfg->scan_timer, 10 * 1000000ULL);
        }
        return false;
    }

    wifi_config_t sta_cfg;
    memset(&sta_cfg, 0, sizeof(sta_cfg));

    strlcpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid));
    if (password)
    {
        strlcpy((char *)sta_cfg.sta.password, password, sizeof(sta_cfg.sta.password));
    }

    sta_cfg.sta.scan_method = WIFI_FAST_SCAN;
    sta_cfg.sta.failure_retry_cnt = 1;

    err = esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
        cfg->is_connecting = false;
        if (cfg->scan_timer)
        {
            esp_timer_start_once(cfg->scan_timer, 10 * 1000000ULL);
        }
        return false;
    }

    err = esp_wifi_connect();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
        cfg->is_connecting = false;
        if (cfg->scan_timer)
        {
            esp_timer_start_once(cfg->scan_timer, 10 * 1000000ULL);
        }
        return false;
    }

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    EventBits_t bits = xEventGroupWaitBits(
        cfg->event_group,
        WIFI_CONFIG_CONNECTED_BIT | WIFI_CONFIG_FAIL_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(10000));

    cfg->is_connecting = false;

    bool success = false;

    if (bits & WIFI_CONFIG_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "WiFi connect test success");
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_wifi_disconnect();
        success = true;
    }
    else
    {
        ESP_LOGW(TAG, "WiFi connect test failed");
        success = false;
    }

    if (cfg->scan_timer)
    {
        esp_timer_start_once(cfg->scan_timer, 10 * 1000000ULL);
    }

    return success;
}

// Save SSID and password
void wifi_configuration_save(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Save SSID %s (%d)", ssid ? ssid : "", ssid ? (int)strlen(ssid) : 0);
    if (!ssid || !password)
    {
        return;
    }

    wifi_ssid_manager_t *mgr = wifi_ssid_manager_get_instance();
    wifi_ssid_manager_add(mgr, ssid, password);
}

// Get AP list
int wifi_configuration_get_ap_list(wifi_ap_record_t *dest, int max)
{
    wifi_configuration_t *cfg = wifi_configuration_get_instance();

    int n = cfg->ap_count;
    if (n > max)
    {
        n = max;
    }

    if (dest && n > 0)
    {
        memcpy(dest, cfg->ap_records, n * sizeof(wifi_ap_record_t));
    }

    return n;
}
