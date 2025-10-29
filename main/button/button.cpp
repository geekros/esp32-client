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
#include "button.h"

// Define log tag
#define TAG "[client:button]"

// Timer and debounce configurations
#define BUTTON_TIMER_INTERVAL_MS 5 // Timer tick interval in milliseconds
#define BUTTON_DEBOUNCE_TIME_MS 20 // Debounce threshold in milliseconds

static button_info_t *button_list_head = NULL; // Head of button list
static esp_timer_handle_t button_timer_handle; // Timer handle
static bool button_running = false;            // Flag for timer status

// Forward declaration of timer callback
static void button_timer_callback(void *arg);

// Initialize button GPIO
void button_gpio_init(button_get_level_callback_t get_level_func, button_press_callback_t short_press_handler, button_press_callback_t double_click_handler, button_press_callback_t long_press_handler)
{
    // Initialize button config
    button_config_t button_cfg = {};
    button_cfg.gpio_num = BOARD_BUTTON_GPIO;
    button_cfg.active_level = 0;
    button_cfg.long_press_time = 5000;
    button_cfg.double_click_time = 500;
    button_cfg.handler_get_level_func = get_level_func;
    button_cfg.short_press_callback = short_press_handler;
    button_cfg.long_press_callback = long_press_handler;
    button_cfg.double_click_callback = double_click_handler;

    // Configure GPIO for button input
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << button_cfg.gpio_num);
    io_conf.pull_up_en = (button_cfg.active_level == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = (button_cfg.active_level == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;

    // Apply GPIO configuration
    gpio_config(&io_conf);

    // Register button event
    button_event_set(&button_cfg);
}

// Register button event
esp_err_t button_event_set(button_config_t *config)
{
    // Allocate memory for new button node
    button_info_t *new_button = (button_info_t *)malloc(sizeof(button_info_t));
    if (!new_button)
        // Return failure if memory allocation fails
        return ESP_FAIL;

    // Initialize button info
    memset(new_button, 0, sizeof(button_info_t));

    // Copy configuration
    memcpy(&new_button->config, config, sizeof(button_config_t));

    // Append to linked list
    if (!button_list_head)
        // First button in the list
        button_list_head = new_button;
    else
    {
        // Traverse to the end of the list
        button_info_t *current = button_list_head;

        // Append new button
        while (current->next)
            current = current->next;

        // Link the new button
        current->next = new_button;
    }

    // Start periodic timer if not already running
    if (!button_running)
    {
        // Create and start the timer
        esp_timer_create_args_t button_timer = {};
        button_timer.callback = button_timer_callback;
        button_timer.arg = (void *)(intptr_t)BUTTON_TIMER_INTERVAL_MS;
        button_timer.name = "button_timer";
        button_timer.dispatch_method = ESP_TIMER_TASK;

        // Create and start the timer
        esp_timer_create(&button_timer, &button_timer_handle);

        // Start periodic timer
        esp_timer_start_periodic(button_timer_handle, BUTTON_TIMER_INTERVAL_MS * 1000);

        // Set running flag
        button_running = true;
    }

    // Return success
    return ESP_OK;
}

// Timer callback for button state management
static void button_timer_callback(void *arg)
{
    // Get the timer interval
    int interval = (int)(intptr_t)arg;

    // Traverse the button list
    button_info_t *button = button_list_head;

    // Update each button state
    while (button)
    {
        // Get GPIO number and level
        int gpio = button->config.gpio_num;

        // Get current button level
        int level = button->config.handler_get_level_func(gpio);

        // State machine for button handling
        switch (button->state)
        {
        case BUTTON_RELEASED:
            if (level == button->config.active_level)
            {
                // Update state to pressed
                button->state = BUTTON_PRESSED;
                // Reset press duration
                button->press_duration = 0;
            }
            else if (button->click_count == 1)
            {
                // Waiting for second press (double click detection)
                button->release_duration += interval;
                // If no second press within double click window -> single click
                if (button->release_duration >= button->config.double_click_time)
                {
                    // Trigger short press callback
                    if (button->config.short_press_callback)
                        // Invoke short press callback
                        button->config.short_press_callback(gpio);
                    // Reset click count and release duration
                    button->click_count = 0;
                    button->release_duration = 0;
                }
            }
            break;
        case BUTTON_PRESSED:
            if (level == button->config.active_level)
            {
                // Update press duration
                button->press_duration += interval;
                // Debounce check
                if (button->press_duration >= BUTTON_DEBOUNCE_TIME_MS)
                    // Update state to holding
                    button->state = BUTTON_HOLDING;
            }
            else
            {
                // Button released before debounce threshold
                button->state = BUTTON_RELEASED;
                button->press_duration = 0;
            }
            break;
        case BUTTON_HOLDING:
            if (level == button->config.active_level)
            {
                // Update press duration
                button->press_duration += interval;
                // Long press detection
                if (button->press_duration >= button->config.long_press_time)
                {
                    // Trigger long press callback
                    if (button->config.long_press_callback)
                        // Invoke long press callback
                        button->config.long_press_callback(gpio);
                    // Update state to long pressed holding
                    button->state = BUTTON_LONG_PRESSED_HOLDING;
                    button->click_count = 0; // Cancel pending double click
                }
            }
            else
            {
                // Released before long press threshold
                if (button->press_duration >= BUTTON_DEBOUNCE_TIME_MS && button->press_duration < button->config.long_press_time)
                {
                    // Short press detected
                    button->click_count++;
                    button->release_duration = 0;
                    // Check for double click
                    if (button->click_count == 2)
                    {
                        // Trigger double click callback
                        if (button->config.double_click_callback)
                            // Invoke double click callback
                            button->config.double_click_callback(gpio);
                        // Reset click count and release duration
                        button->click_count = 0;
                        button->release_duration = 0;
                    }
                }
                // Update state to released
                button->state = BUTTON_RELEASED;
                button->press_duration = 0;
            }
            break;
        case BUTTON_LONG_PRESSED_HOLDING:
            if (level != button->config.active_level)
            {
                // Button released from long press
                button->state = BUTTON_RELEASED;
                button->press_duration = 0;
            }
            break;
        default:
            break;
        }

        // Move to the next button
        button = button->next;
    }
}
