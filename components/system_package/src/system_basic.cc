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

// Constructor
SystemBasic::SystemBasic()
{
    event_group = xEventGroupCreate();
}

// Destructor
SystemBasic::~SystemBasic()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Initialize system components
void SystemBasic::Init(const std::string &base_path, const std::string &partition_label, size_t max_files)
{
    // Configure SPIFFS
    esp_vfs_spiffs_conf_t conf = {};
    conf.base_path = base_path.c_str();
    conf.partition_label = partition_label.c_str();
    conf.max_files = max_files;
    conf.format_if_mount_failed = false;

    // Initialize SPIFFS
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

// Get system chip ID
std::string SystemBasic::GetChipID()
{
    uint8_t mac[6] = {0};

    esp_err_t ret = esp_efuse_mac_get_default(mac);
    if (ret != ESP_OK)
    {
        return "UNKNOWN";
    }

    char chip_id[13] = {0};
    snprintf(chip_id, sizeof(chip_id), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return std::string(chip_id);
}

// Print heap statistics
void SystemBasic::PrintHeaps()
{
    int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Free sram: %u Minimal sram: %u", free_sram, min_free_sram);
}