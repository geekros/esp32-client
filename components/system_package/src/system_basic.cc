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
#include "system_basic.h"

// Define log tag
#define TAG "[client:components:system:basic]"

// Initialize system components
void system_init(const char *base_path, const char *partition_label, size_t max_files)
{
    // Configure SPIFFS
    esp_vfs_spiffs_conf_t conf = {};
    conf.base_path = base_path;
    conf.partition_label = partition_label;
    conf.max_files = max_files;
    conf.format_if_mount_failed = false;

    // Initialize SPIFFS
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

// Get system chip ID
void system_chip_id(char *out_str, size_t len)
{
    // Check length
    if (len < 13)
    {
        len = 13;
    }

    // Get MAC address as chip ID
    uint8_t mac[6] = {0};
    esp_err_t ret = esp_efuse_mac_get_default(mac);
    if (ret != ESP_OK)
    {
        snprintf(out_str, len, "UNKNOWN");
        return;
    }

    // Format chip ID as hexadecimal string
    snprintf(out_str, len, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}