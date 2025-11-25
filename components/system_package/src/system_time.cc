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
#include "system_time.h"

// Define log tag
#define TAG "[client:components:system:time]"

// Constructor
SystemTime::SystemTime()
{
    event_group = xEventGroupCreate();
}

// Destructor
SystemTime::~SystemTime()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Initialize time synchronization
void SystemTime::InitTimeSync()
{
    // Deinit SNTP to ensure a clean state
    esp_netif_sntp_deinit();

    // Set timezone to CST-8
    setenv("TZ", "CST-8", 1);
    tzset();

    // Configure SNTP
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.start = true;
    config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;

    // Initialize SNTP
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));

    // Start SNTP
    ESP_ERROR_CHECK(esp_netif_sntp_start());
}

// Wait for time synchronization
bool SystemTime::WaitForSync(int timeout_ms)
{
    int retry = 0;
    const int retry_count = timeout_ms / 2000;

    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }

    time_t now = 0;
    struct tm timeinfo = {0};
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2016 - 1900))
    {
        // Deinitialize SNTP on failure
        esp_netif_sntp_deinit();

        // Return failure
        return false;
    }

    // Log synchronized time and timestamp
    ESP_LOGI(TAG, "Current Time: %s (Timestamp %d)", GetTimeString().c_str(), (long)GetUnixTimestamp());

    // Deinitialize SNTP
    esp_netif_sntp_deinit();

    // Return success
    return true;
}

// Get current time as string
std::string SystemTime::GetTimeString()
{
    time_t now;
    time(&now);

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

    return std::string(buffer);
}

// Get current Unix timestamp
time_t SystemTime::GetUnixTimestamp()
{
    time_t now;
    time(&now);
    return now;
}