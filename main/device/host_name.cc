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
#include "host_name.h"

// Define log tag
#define TAG "[client:host]"

// Function to get the device hostname
esp_err_t get_hostname(char *hostname, size_t len)
{
    // Validate input parameters
    if (hostname == NULL || len < 8)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize NVS
    esp_err_t err;
    nvs_handle_t handle;

    // Open NVS namespace
    err = nvs_open(GEEKROS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        snprintf(hostname, len, GEEKROS_NVS_NAMESPACE "-xxxxxx");
        return err;
    }

    // Read hostname from NVS
    size_t stored_len = len;
    err = nvs_get_str(handle, GEEKROS_NVS_HOSTNAME, hostname, &stored_len);

    if (err != ESP_OK)
    {
        // Generate new hostname based on MAC address
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        snprintf(hostname, len, GEEKROS_NVS_NAMESPACE "-%02x%02x%02x", mac[3], mac[4], mac[5]);

        // Save it persistently
        nvs_set_str(handle, GEEKROS_NVS_HOSTNAME, hostname);
        nvs_commit(handle);

        // Set return value to ESP_OK
        err = ESP_OK;
    }

    // Close NVS handle
    nvs_close(handle);
    return err;
}

// Clear NVS hostname (for testing purposes)
void clear_hostname(void)
{
    // Open NVS namespace
    nvs_handle_t handle;
    esp_err_t err = nvs_open(GEEKROS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK)
    {
        // Erase hostname key
        err = nvs_erase_key(handle, GEEKROS_NVS_HOSTNAME);
        if (err == ESP_OK)
        {
            // Commit changes
            nvs_commit(handle);
        }
    }

    // Close NVS handle
    nvs_close(handle);
}