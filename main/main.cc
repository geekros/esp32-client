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

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_event.h>
#include <nvs_flash.h>

// Include application header
#include "application.h"

// Define log tag
#define TAG "[client:main]"

// Entry point for the ESP32 application
extern "C" void app_main(void)
{
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize NVS flash for WiFi configuration
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Log the GeekROS version
    ESP_LOGI(TAG, "Client Version: %s", GEEKROS_VERSION);

    // Start the application main function
    application_main();
}