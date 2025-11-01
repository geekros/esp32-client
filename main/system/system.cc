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
#include "system.h"

// Initialize system components
void system_init(void)
{
    // Configure SPIFFS
    esp_vfs_spiffs_conf_t conf = {};
    conf.base_path = GEEKROS_SPIFFS_BASE_PATH;
    conf.partition_label = GEEKROS_SPIFFS_LABEL;
    conf.max_files = GEEKROS_SPIFFS_MAX_FILE;
    conf.format_if_mount_failed = false;

    // Initialize SPIFFS
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}