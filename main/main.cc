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

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include drivers
#include "driver/gpio.h"

// Include configuration and module headers
#include "common_config.h"

// Include components headers
#include "board_basic.h"

// Include package headers
#include "button/button.h"
#include "wifi/wifi_connect.h"
#include "wifi/wifi_manage.h"
#include "audio_service.h"
#include "audio_processor.h"

// Define log tag
#define TAG "[client:main]"

static const board_t *board_interface = NULL;

static audio_processor_t *g_audio_processor = NULL;

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
    }

    // Handle WiFi disconnected state
    if (state == WIFI_STATE_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi disconnected");
    }
}

// -----------------------------------------------------------
// AFE output callback function
// -----------------------------------------------------------
// Called by audio_processor when a frame of processed audio is ready
static void afe_output_callback(const int16_t *data, int samples, void *user_ctx)
{
    static int frame_count = 0;
    frame_count++;

    // Print frame count for debugging
    ESP_LOGI(TAG, "[AFE] Output frame %d (%d samples)", frame_count, samples);

    // Play processed audio through the board interface
    if (board_interface && board_interface->play_audio)
    {
        board_interface->play_audio(data, samples);
    }
}

// -----------------------------------------------------------
// AFE VAD callback function
// -----------------------------------------------------------
// Called by audio_processor when voice activity changes
static void afe_vad_callback(bool speaking, void *user_ctx)
{
    ESP_LOGI(TAG, "[AFE] VAD state: %s", speaking ? "SPEAKING" : "SILENCE");
}

// Microphone data callback function
static void audio_microphone_callback(const int16_t *data, int samples)
{

    audio_processor_feed(g_audio_processor, data);

    // Play audio through board interface
    // if (board_interface && board_interface->play_audio)
    // {
    //     // Push audio data to playback
    //     board_interface->play_audio(data, samples);
    // }
}

// Entry point for the ESP32 application
extern "C" void app_main(void)
{
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

    // Initialize button GPIO
    button_gpio_init(get_button_level_handler, button_short_press_handler, button_double_click_handler, button_long_press_handler);

    // Initialize WiFi
    wifi_connect_init(wifi_state_change_callback);

    // Initialize the board-specific hardware
    board_interface = board();

    // Call the board initialization function
    board_interface->board_init(audio_microphone_callback);

    g_audio_processor = audio_processor_create(AUDIO_PROC_MODE_AFE);
    if (!g_audio_processor)
    {
        ESP_LOGE(TAG, "Failed to create audio processor");
        return;
    }

    audio_processor_init(g_audio_processor, 1, false, 30, NULL);

    audio_processor_set_output_callback(g_audio_processor, afe_output_callback, NULL);
    audio_processor_set_vad_callback(g_audio_processor, afe_vad_callback, NULL);

    audio_processor_start(g_audio_processor);
    ESP_LOGI(TAG, "AFE audio processor started");

    // Main application loop
    while (true)
    {
        // Log a heartbeat message every 500 milliseconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    audio_processor_destroy(g_audio_processor);
    g_audio_processor = NULL;
}