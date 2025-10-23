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
#define TAG "[client:wifi]"

// Maximum WiFi connection retry count
#define WIFI_CONNECT_MAX_RETRY 5

// WiFi connection retry counter
static int wifi_connect_retry_count = 0;

// WiFi connected flag
static bool wifi_connected = false;

// Network interface pointer
static esp_netif_t *sta_netif = NULL;

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
            // Log WiFi connected
            ESP_LOGI(TAG, "WiFi connected");
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

    ESP_ERROR_CHECK(esp_netif_set_hostname(sta_netif, hostname));
    ESP_LOGI(TAG, "Hostname: %s", hostname);

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Event handler for WiFi events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Set WiFi state change callback
    wifi_state_change_callback = callback;

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
    esp_wifi_get_mode(&mode);

    // If not in STA mode, switch to STA mode
    if (mode != WIFI_MODE_STA)
    {
        // Stop WiFi before changing mode
        esp_wifi_stop();
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
