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
#include "application.h"

// Board interface pointer
static const board_t *board_interface = NULL;

// Define log tag
#define TAG "[client:application]"

// Application main function
void application_main(void)
{
    // Check if GeekROS service GRK and project token are configured
    if (GEEKROS_SERVICE_GRK == NULL || strlen(GEEKROS_SERVICE_GRK) == 0 || GEEKROS_SERVICE_PROJECT_TOKEN == NULL || strlen(GEEKROS_SERVICE_PROJECT_TOKEN) == 0)
    {
        ESP_LOGE(TAG, "Please configure GEEKROS_SERVICE_GRK and GEEKROS_SERVICE_PROJECT_TOKEN in menuconfig");
        return;
    }

    // Initialize system components
    system_init(GEEKROS_SPIFFS_BASE_PATH, GEEKROS_SPIFFS_LABEL, GEEKROS_SPIFFS_MAX_FILE);

    // Initialize locale and language components
    language_init();

    // Load speech recognition models from SPIFFS
    srmodel_list_t *models = model_load_from_path();
    if (!models)
    {
        ESP_LOGE(TAG, "Failed to load model");
        return;
    }

    // Initialize the board-specific hardware
    board_interface = board();

    // Call the board initialization function
    board_interface->board_init();

    // Initialize WiFi SSID manager
    wifi_ssid_manager_t *wifi_mgr = wifi_ssid_manager_get_instance();
    wifi_ssid_manager_load(wifi_mgr);

    // Get the list of stored WiFi SSIDs
    const wifi_ssid_item_t *wifi_list = wifi_ssid_manager_get_list(wifi_mgr);
    if (wifi_list && wifi_mgr->count > 0)
    {
        // connect to the Wi-Fi network
        wifi_station_start();
    }
    else
    {
        // No stored WiFi SSIDs, start in AP mode or prompt for configuration
        wifi_configuration_start();
    }

    // Start the application loop
    application_loop();
}

// Application loop function
void application_loop(void)
{
    // Main application loop
    while (true)
    {
        // Log a heartbeat message every 500 milliseconds
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}