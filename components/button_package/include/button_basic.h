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

#ifndef BUTTON_BASIC_H
#define BUTTON_BASIC_H

// Include standard headers
#include <string>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include driver headers
#include "driver/gpio.h"

// Button press callback type
typedef void (*button_press_callback_t)(int gpio);

// Button level getter function type
typedef int (*button_get_level_callback_t)(int gpio);

// Button configuration structure
typedef struct
{
    int gpio_num;          // GPIO number where the button is connected
    int active_level;      // Active level for the button (0 or 1)
    int long_press_time;   // Duration (ms) to trigger a long press
    int double_click_time; // Max interval (ms) for detecting a double click
} button_config_t;

// Button state enumeration
typedef enum
{
    BUTTON_RELEASED = 0,         // Button is released
    BUTTON_PRESSED,              // Button just pressed
    BUTTON_HOLDING,              // Button is being held
    BUTTON_LONG_PRESSED_HOLDING, // Button is long pressed and holding
    BUTTON_WAIT_SECOND_PRESS     // Waiting for second press (double click detection)
} button_state_t;

// Button information structure
typedef struct ButtonInfo
{
    button_config_t config;  // Button configuration
    button_state_t state;    // Current state of the button
    int press_duration;      // Duration (ms) the button has been pressed
    int release_duration;    // Duration (ms) since last release (for double click)
    int click_count;         // Click count for double click detection
    struct ButtonInfo *next; // Pointer to the next button node
} button_info_t;

// Timer and debounce configurations
#define BUTTON_TIMER_INTERVAL_MS 5 // Timer tick interval in milliseconds
#define BUTTON_DEBOUNCE_TIME_MS 20 // Debounce threshold in milliseconds

// Button callbacks structure
struct ButtonCallbacks
{
    // Callback for button events
    std::function<void(std::string event)> on_button_calledback;
};

// ButtonBasic class definition
class ButtonBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Button callbacks
    ButtonCallbacks callbacks;

    // Start button event handling
    static void ButtonTimerCallback(void *arg);

public:
    // Constructor and Destructor
    ButtonBasic();
    ~ButtonBasic();

    // Get the singleton instance of the ButtonBasic class
    static ButtonBasic &Instance()
    {
        static ButtonBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    ButtonBasic(const ButtonBasic &) = delete;
    ButtonBasic &operator=(const ButtonBasic &) = delete;

    // Initialize button GPIO
    esp_err_t ButtonInitialize(int gpio_num, int active_level);

    // Set button callbacks
    void SetCallbacks(ButtonCallbacks &cb);
};

#endif
