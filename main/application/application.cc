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

// Define log tag
#define TAG "[client:application]"

// Constructor
Application::Application()
{
    // Create event group
    event_group = xEventGroupCreate();

    // Define clock timer arguments
    esp_timer_create_args_t clock_timer_args = {
        .callback = [](void *arg)
        {
            Application *app = (Application *)arg;
            xEventGroupSetBits(app->event_group, MAIN_EVENT_CLOCK_TICK);
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "application_clock_timer",
        .skip_unhandled_events = true,
    };

    // Create clock timer
    esp_timer_create(&clock_timer_args, &clock_timer_handle);
}

// Destructor
Application::~Application()
{
    // Delete clock timer
    if (clock_timer_handle != nullptr)
    {
        esp_timer_stop(clock_timer_handle);
        esp_timer_delete(clock_timer_handle);
    }

    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Main application entry point
void Application::ApplicationMain()
{
    // Log the GeekROS version
    ESP_LOGI(TAG, "Client Version: %s", GEEKROS_VERSION);

    // Check if GeekROS service GRK and project token are configured
    if (GEEKROS_SERVICE_GRK == NULL || strlen(GEEKROS_SERVICE_GRK) == 0 || GEEKROS_SERVICE_PROJECT_TOKEN == NULL || strlen(GEEKROS_SERVICE_PROJECT_TOKEN) == 0)
    {
        ESP_LOGE(TAG, "Please configure GEEKROS_SERVICE_GRK and GEEKROS_SERVICE_PROJECT_TOKEN in menuconfig");
        return;
    }

    // Initialize basic runtime components
    RuntimeBasic::Instance().Init();

    // Initialize system components
    SystemBasic::Instance().Init(GEEKROS_SPIFFS_BASE_PATH, GEEKROS_SPIFFS_LABEL, GEEKROS_SPIFFS_MAX_FILE);

    // Initialize locale and language components
    LanguageBasic::Instance().Init();

    // Initialize board-specific components
    BoardBasic *board = CreateBoard();
    board->Initialization();

    // Initialize WiFi board
    auto &wifi_board = WifiBoard::Instance();

    // Set WiFi board callbacks
    WifiCallbacks wifi_board_callbacks;
    wifi_board_callbacks.on_access_point = [this, board]()
    {
        ESP_LOGI(TAG, "Entered Access Point Mode");
    };
    wifi_board_callbacks.on_station = [this, board]()
    {
        ESP_LOGI(TAG, "Entered Station Mode");

        // Check network status
        NetworkBasic::Instance().CheckNetwork();

        // Set Realtime basic callbacks
        RealtimeCallbacks realtime_callbacks;
        realtime_callbacks.on_signaling_calledback = [this](std::string event, std::string data)
        {
            ESP_LOGI(TAG, "Realtime Signaling Event: %s %s", event.c_str(), data.c_str());
        };
        RealtimeBasic::Instance().SetCallbacks(realtime_callbacks);

        // Connect realtime service
        RealtimeBasic::Instance().RealtimeConnect();
    };
    wifi_board.SetCallbacks(wifi_board_callbacks);

    // Create main event loop task
    auto application_loop_task = [](void *param)
    {
        ((Application *)param)->ApplicationLoop();
        vTaskDelete(nullptr);
    };

    // Create the task with a larger stack size
    xTaskCreate(application_loop_task, "application_loop", 4096, this, 3, &main_event_loop_task_handle);

    // Start clock timer with 1 second period
    esp_timer_start_periodic(clock_timer_handle, 1000000);

    // Start network
    wifi_board.StartNetwork();
}

// Main application loop
void Application::ApplicationLoop()
{
    // Main application loop
    while (true)
    {
        // Wait for clock tick event
        auto bits = xEventGroupWaitBits(event_group, MAIN_EVENT_CLOCK_TICK, pdTRUE, pdFALSE, portMAX_DELAY);

        // Handle clock tick event
        if (bits & MAIN_EVENT_CLOCK_TICK)
        {
            // TODO: Handle clock tick event
            // ESP_LOGI(TAG, "Clock Tick Event");
        }

        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}