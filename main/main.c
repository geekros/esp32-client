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
#include "board_config.h"

// Include package headers
#include "board/board.h"
#include "button/button.h"

// Define log tag
#define TAG "[client:main]"

// WiFi configuration
#define CONFIG_WIFI_SSID "GEEKROS"
#define CONFIG_WIFI_PASSWORD "1234567890"

// Short press handler function
void button_short_press_handler(int gpio)
{
    ESP_LOGI(TAG, "Button short pressed on GPIO %d", gpio);
}

// Long press handler function
void button_long_press_handler(int gpio)
{
    ESP_LOGI(TAG, "Button long pressed on GPIO %d", gpio);
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
    return gpio_get_level(gpio);
}

// Entry point for the ESP32 application
void app_main(void)
{
    // Initialize NVS flash
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize the board-specific hardware
    board_init();

    // Log the GeekROS version
    ESP_LOGI(TAG, "Client Version: %s", GEEKROS_VERSION);

    // Initialize button config
    button_config_t button_cfg = {
        .gpio_num = BOARD_BOOT_GPIO,
        .active_level = 0,
        .long_press_time = 5000,
        .double_click_time = 500,
        .handler_get_level_func = get_button_level_handler,
        .short_press_callback = button_short_press_handler,
        .long_press_callback = button_long_press_handler,
        .double_click_callback = button_double_click_handler,
    };

    // Initialize button GPIO
    button_gpio_init(button_cfg.gpio_num, button_cfg.active_level);

    // Register button event
    button_event_set(&button_cfg);

    // Main application loop
    while (1)
    {
        // Log a heartbeat message every 500 milliseconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}