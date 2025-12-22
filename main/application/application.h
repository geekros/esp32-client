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

#ifndef APPLICATION_H
#define APPLICATION_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include project-specific headers
#include "client_config.h"

// Include components headers
#include "runtime_basic.h"
#include "board_basic.h"
#include "system_basic.h"
#include "system_time.h"
#include "language_basic.h"
#include "language_sound.h"
#include "network_basic.h"
#include "realtime_basic.h"
#include "wifi_board.h"
#include "wifi_manager.h"
#include "wifi_station.h"
#include "wifi_access_point.h"
#include "service_basic.h"

// Define audio state enum
enum class AudioState
{
    LISTENING,
    SPEAKING,
};

// Define main event group bits
#define MAIN_EVENT_SEND_AUDIO (1 << 0)
#define MAIN_EVENT_VAD_CHANGE (1 << 1)
#define MAIN_EVENT_CLOCK_TICK (1 << 2)

// Application class definition
class Application
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Clock timer handle
    esp_timer_handle_t clock_timer_handle = nullptr;

    // Main event loop task handle
    TaskHandle_t main_event_loop_task_handle = nullptr;

    // Audio service instance
    AudioService audio_service;

    // Health check clock counter
    int health_check_clock = 0;

    // Current audio state
    AudioState audio_state_ = AudioState::LISTENING;

    // Last audio time for speaking state
    int64_t last_audio_time_us_ = 0;

    bool mute_uplink_audio_ = false;

public:
    // Constructor and destructor
    Application();
    ~Application();

    // Get the singleton instance of the Application class
    static Application &Instance()
    {
        static Application instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    // Set chat audio state
    void SetAudioState(AudioState state);

    // Main application entry point
    void ApplicationMain();

    // Main application loop
    void ApplicationLoop();
};

#endif