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

// Include standard libraries
#include <math.h>
#include <stdio.h>
#include <esp_log.h>
#include <nvs_flash.h>

// Include configuration and module headers
#include "common_config.h"

// Include components headers
#include "board_basic.h"
#include "system_basic.h"

// Include main headers
#include "button/button.h"
#include "wifi/wifi_connect.h"
#include "wifi/wifi_manage.h"
#include "realtime/realtime.h"

// Define log tag
#define TAG "[client:main]"

// Board interface pointer
static const board_t *board_interface = NULL;

// Define realtime state variable
static relatime_t realtime_interface;

// Short press handler function
static void button_short_press_handler(int gpio)
{
    ESP_LOGI(TAG, "Button short pressed on GPIO %d", gpio);
}

// Long press handler function
static void button_long_press_handler(int gpio)
{
    ESP_LOGI(TAG, "Button long pressed on GPIO %d", gpio);

    // Configure WiFi network
    wifi_network_configure();
}

// Double click handler function
static void button_double_click_handler(int gpio)
{
    ESP_LOGI(TAG, "Double click detected on GPIO %d", gpio);
}

// Get button level handler function
static int get_button_level_handler(int gpio)
{
    // Return the GPIO level
    return gpio_get_level((gpio_num_t)gpio);
}

// WiFi state change callback function
static void wifi_state_change_callback(wifi_state_t state)
{
    // Handle WiFi state changes
    if (state == WIFI_STATE_CONNECTED)
    {
        ESP_LOGI(TAG, "WiFi connected");

        vTaskDelay(pdMS_TO_TICKS(1000));

        // Start realtime connection
        realtime_interface.started = true;
    }

    // Handle WiFi disconnected state
    if (state == WIFI_STATE_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi disconnected");

        // Stop realtime connection
        realtime_interface.started = false;
    }

    // Handle WiFi connection failed state
    if (state == WIFI_STATE_CONNECTION_FAILED)
    {
        ESP_LOGI(TAG, "WiFi connection failed");

        // Stop realtime connection
        realtime_interface.started = false;
    }
}

// Microphone data callback function
static void audio_microphone_callback(const int16_t *data, int samples)
{
    // Play audio through board interface
    if (board_interface && board_interface->play_audio)
    {
        // Push audio data to playback
        // board_interface->play_audio(data, samples);
    }
}

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

    // Initialize system components
    system_init(GEEKROS_SPIFFS_BASE_PATH, GEEKROS_SPIFFS_LABEL, GEEKROS_SPIFFS_MAX_FILE);

    // Initialize button GPIO
    button_gpio_init(get_button_level_handler, button_short_press_handler, button_double_click_handler, button_long_press_handler);

    // Initialize the board-specific hardware
    board_interface = board();

    // Call the board initialization function
    board_interface->board_init(audio_microphone_callback);

    // Initialize WiFi
    wifi_connect_init(wifi_state_change_callback);

    // Main application loop
    while (true)
    {
        if (realtime_interface.started)
        {
            // Handle realtime tasks here
            realtime_connection();

            // For demonstration, stop the realtime after one iteration
            realtime_interface.started = false;
        }

        // Log a heartbeat message every 500 milliseconds
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}