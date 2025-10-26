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
#include "wifi_manage.h"

// Define log tag
#define TAG "[client:wifi:manage]"

// Maximum WiFi connection retry count
#define WIFI_CONNECT_MAX_RETRY 5

// WiFi connection retry counter
static int wifi_connect_retry_count = 0;

// WiFi connected flag
static bool wifi_connected = false;

// Network interface pointer
static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;

// WiFi scan semaphore
static SemaphoreHandle_t wifi_scan_semaphore = NULL;

// WiFi state change callback pointer
static p_wifi_state_change_callback wifi_state_change_callback = NULL;

// WiFi event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Handle WiFi events
    if (event_base == WIFI_EVENT)
    {
        // Process WiFi events
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            // Connect to WiFi
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            // Reset retry counter
            wifi_connect_retry_count = 0;
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (wifi_connected)
            {
                // Update WiFi connected flag
                wifi_connected = false;
                // Invoke WiFi state change callback if set
                if (wifi_state_change_callback)
                {
                    // Notify disconnected state
                    wifi_state_change_callback(WIFI_STATE_DISCONNECTED);
                }
            }
            // Retry connecting to WiFi
            if (wifi_connect_retry_count < WIFI_CONNECT_MAX_RETRY)
            {
                // Retry connecting to WiFi
                esp_wifi_connect();
                // Increment retry counter
                wifi_connect_retry_count++;
            }
            break;
        default:
            break;
        }
    }

    // Handle IP events
    if (event_base == IP_EVENT)
    {
        // Process IP events
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            // Cast event data to IP event structure
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

            // Log WiFi connected with IP
            ESP_LOGI(TAG, "IPv4: " IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
            ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));

            const char *hostname = NULL;
            esp_netif_get_hostname(sta_netif, &hostname);
            ESP_LOGI(TAG, "Hostname: %s", hostname ? hostname : "(unknown)");

            // Reset retry counter
            wifi_connect_retry_count = 0;

            // Set WiFi connected flag
            wifi_connected = true;

            // Invoke WiFi state change callback if set
            if (wifi_state_change_callback)
            {
                // Notify connected state
                wifi_state_change_callback(WIFI_STATE_CONNECTED);
            }
        }
    }
}

// Initialize WiFi management
void wifi_manage_init(const char *hostname, p_wifi_state_change_callback callback)
{
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi STA interface and store handle
    sta_netif = esp_netif_create_default_wifi_sta();

    // Set hostname for STA interface
    ESP_ERROR_CHECK(esp_netif_set_hostname(sta_netif, hostname));
    ESP_LOGI(TAG, "Hostname: %s", hostname);

    // Create default WiFi AP interface and store handle
    ap_netif = esp_netif_create_default_wifi_ap();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Event handler for WiFi events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Set WiFi state change callback
    wifi_state_change_callback = callback;

    // Create WiFi scan semaphore
    wifi_scan_semaphore = xSemaphoreCreateBinary();
    // Give back WiFi scan semaphore
    xSemaphoreGive(wifi_scan_semaphore);

    // Set WiFi mode to STA and start WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// Connect to a WiFi network
void wifi_manage_connect(const char *ssid, const char *password)
{
    // Configure WiFi connection
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    // Set ssid and password
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", ssid);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", password);

    // Get current WiFi mode
    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));

    // If not in STA mode, switch to STA mode
    if (mode != WIFI_MODE_STA)
    {
        // Stop WiFi before changing mode
        ESP_ERROR_CHECK(esp_wifi_stop());
        // Set WiFi mode to STA
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    }
    // Reset retry counter
    wifi_connect_retry_count = 0;
    // Set WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
}

// Start WiFi in Access Point mode
esp_err_t wifi_manage_ap(const char *hostname)
{
    // Get current WiFi mode
    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));

    // If not in AP mode, switch to AP mode
    if (mode != WIFI_MODE_APSTA)
    {
        // Return error if unable to switch to APSTA mode
        return ESP_OK;
    }

    // Disconnect and stop WiFi
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());

    // Set WiFi mode to APSTA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Configure WiFi AP settings
    wifi_config_t wifi_config = {
        .ap = {
            .channel = GEEKROS_WIFI_AP_CHANNEL,
            .max_connection = GEEKROS_WIFI_AP_MAX_CONNECTION,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    // Set SSID and password for AP
    snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), hostname);
    snprintf((char *)wifi_config.ap.password, sizeof(wifi_config.ap.password), GEEKROS_WIFI_AP_PASSWORD);

    // Set WiFi configuration for AP
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // Configure static IP for AP
    esp_netif_ip_info_t ip_info;
    ip4addr_aton(GEEKROS_WIFI_AP_IP, (ip4_addr_t *)&ip_info.ip);
    ip4addr_aton(GEEKROS_WIFI_AP_GATEWAY, (ip4_addr_t *)&ip_info.gw);
    ip4addr_aton(GEEKROS_WIFI_AP_NETMASK, (ip4_addr_t *)&ip_info.netmask);

    // Stop DHCP server for AP interface
    esp_netif_dhcps_stop(ap_netif);

    // Set static IP info for AP interface
    esp_netif_set_ip_info(ap_netif, &ip_info);

    // Restart DHCP server for AP interface
    esp_netif_dhcps_start(ap_netif);

    // Return wifi start result
    return esp_wifi_start();
}

// WiFi scan task
static void wifi_scan_task(void *param)
{
    // Cast parameter to WiFi scan callback
    p_wifi_scan_callback callback = (p_wifi_scan_callback)param;

    // Start WiFi scan
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

    // Get number of APs found
    uint16_t ap_num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));

    // Allocate memory for AP records
    wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * ap_num);
    if (ap_records == NULL)
    {
        vTaskDelete(NULL);
        return;
    }

    // Get AP records
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

    // Log number and details of found APs
    ESP_LOGI(TAG, "Found %d access points:", ap_num);
    for (int i = 0; i < ap_num; i++)
    {
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d, Authmode: %d", ap_records[i].ssid, ap_records[i].rssi, ap_records[i].authmode);
    }

    // Invoke callback with scan results
    if (callback)
    {
        // Call the provided callback function
        callback(ap_num, ap_records);
    }

    // Free allocated memory
    free(ap_records);

    // Give back WiFi scan semaphore
    xSemaphoreGive(wifi_scan_semaphore);

    // Delete the task
    vTaskDelete(NULL);
}

// Scan for available WiFi networks
esp_err_t wifi_manage_scan(p_wifi_scan_callback callback)
{
    // Take WiFi scan semaphore
    if (pdTRUE == xSemaphoreTake(wifi_scan_semaphore, 0))
    {
        // Clear previous AP list
        esp_wifi_clear_ap_list();

        // Return task creation result
        return xTaskCreatePinnedToCore(wifi_scan_task, "wifi_scan_task", 8192, callback, 3, NULL, 1);
    }

    // Return error if semaphore not available
    return ESP_OK;
}