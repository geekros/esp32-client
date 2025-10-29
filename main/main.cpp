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
#include <stdio.h>
#include <esp_log.h>
#include <nvs_flash.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include drivers
#include "driver/gpio.h"

// Include configuration and module headers
#include "common_config.h"

// Include package headers
#include "board/board.h"
#include "button/button.h"
#include "audio/audio_server.h"
#include "wifi/wifi_connect.h"
#include "wifi/wifi_manage.h"

// Define log tag
#define TAG "[client:main]"

// Short press handler function
void button_short_press_handler(int gpio)
{
    ESP_LOGI(TAG, "Button short pressed on GPIO %d", gpio);
}

// Long press handler function
void button_long_press_handler(int gpio)
{
    ESP_LOGI(TAG, "Button long pressed on GPIO %d", gpio);

    // Configure WiFi network
    wifi_network_configure();
}

// Double click handler function
void button_double_click_handler(int gpio)
{
    ESP_LOGI(TAG, "Double click detected on GPIO %d", gpio);
}

// Get button level handler function
int get_button_level_handler(int gpio)
{
    // Return the GPIO level
    return gpio_get_level((gpio_num_t)gpio);
}

// WiFi state change callback function
void wifi_state_change_callback(wifi_state_t state)
{
    // Handle WiFi state changes
    if (state == WIFI_STATE_CONNECTED)
    {
        ESP_LOGI(TAG, "WiFi connected");
    }

    // Handle WiFi disconnected state
    if (state == WIFI_STATE_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi disconnected");
    }
}

// Entry point for the ESP32 application
extern "C" void app_main(void)
{
    // Initialize NVS flash
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize the board-specific hardware
    board_init();

    // Log the GeekROS version
    ESP_LOGI(TAG, "Client Version: %s", GEEKROS_VERSION);

    // Initialize button GPIO
    button_gpio_init(get_button_level_handler, button_short_press_handler, button_double_click_handler, button_long_press_handler);

    // Initialize WiFi
    wifi_connect_init(wifi_state_change_callback);

    // Main application loop
    while (1)
    {
        // Log a heartbeat message every 500 milliseconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}