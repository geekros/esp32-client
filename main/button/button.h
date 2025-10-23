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

#ifndef GEEKROS_BUTTON_H
#define GEEKROS_BUTTON_H

// Include standard libraries
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>
#include <esp_timer.h>

// Include drivers
#include "driver/gpio.h"

// Button press callback type
typedef void (*button_press_callback_t)(int gpio);

// Button level getter function type
typedef int (*button_get_level_callback_t)(int gpio);

// Button configuration structure
typedef struct
{
    int gpio_num;                                       // GPIO number where the button is connected
    int active_level;                                   // Active level for the button (0 or 1)
    int long_press_time;                                // Duration (ms) to trigger a long press
    int double_click_time;                              // Max interval (ms) for detecting a double click
    button_press_callback_t short_press_callback;       // Callback for short press
    button_press_callback_t long_press_callback;        // Callback for long press
    button_press_callback_t double_click_callback;      // Callback for double click (optional)
    button_get_level_callback_t handler_get_level_func; // Function to get the button GPIO level
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

// Initialize GPIO for button input
void button_gpio_init(int gpio_num, int active_level);

// Register and start button event monitoring
esp_err_t button_event_set(button_config_t *config);

#endif
