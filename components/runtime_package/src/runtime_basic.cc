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
#include "runtime_basic.h"

// Define log tag
#define TAG "[client:components:runtime:basic]"

// Constructor
RuntimeBasic::RuntimeBasic()
{
    event_group = xEventGroupCreate();
}

// Destructor
RuntimeBasic::~RuntimeBasic()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Initialize basic runtime components
void RuntimeBasic::Init(void)
{
    // Disable WiFi logging
    esp_log_level_set("AFE_CONFIG", ESP_LOG_NONE);
    esp_log_level_set("pp", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("net80211", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("phy_init", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_handlers", ESP_LOG_NONE);
    esp_log_level_set("I2S_IF", ESP_LOG_NONE);
    esp_log_level_set("ES8311", ESP_LOG_NONE);
    esp_log_level_set("Adev_Codec", ESP_LOG_NONE);
    esp_log_level_set("MODEL_LOADER", ESP_LOG_NONE);
}