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
#include "button_basic.h"

// Define log tag
#define TAG "[client:components:button:basic]"

// Anonymous
namespace
{
    // Head of button list
    button_info_t *button_list_head = nullptr;

    // Timer handle
    esp_timer_handle_t button_timer_handle = nullptr;

    // Flag for timer status
    bool button_running = false;
}

// Constructor
ButtonBasic::ButtonBasic()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
ButtonBasic::~ButtonBasic()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Timer callback function
void ButtonBasic::ButtonTimerCallback(void *arg)
{
    // Get the ButtonBasic instance
    ButtonBasic *self = static_cast<ButtonBasic *>(arg);

    // Use the configured timer interval
    const int interval = BUTTON_TIMER_INTERVAL_MS;

    // Traverse the button list
    button_info_t *button = button_list_head;

    // Update each button state
    while (button)
    {
        // Get GPIO number and level
        int gpio = button->config.gpio_num;

        // Get current button level
        int level = gpio_get_level(static_cast<gpio_num_t>(gpio));

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
                    if (self->callbacks.on_button_calledback)
                    {
                        // Invoke short press callback
                        self->callbacks.on_button_calledback("button:short:press");
                    }
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
                    if (self->callbacks.on_button_calledback)
                    {
                        // Invoke long press callback
                        self->callbacks.on_button_calledback("button:long:press");
                    }

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
                        if (self->callbacks.on_button_calledback)
                        {
                            // Invoke double click callback
                            self->callbacks.on_button_calledback("button:double:click");
                        }

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

// Initialize button GPIO
esp_err_t ButtonBasic::ButtonInitialize(int gpio_num, int active_level)
{
    // Configure GPIO settings
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.pull_up_en = (active_level == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = (active_level == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;

    // Configure GPIO
    gpio_config(&io_conf);

    // Create default button configuration
    button_info_t button_config = {};
    button_config.config.gpio_num = gpio_num;
    button_config.config.active_level = active_level;
    button_config.config.long_press_time = 5000;
    button_config.config.double_click_time = 500;

    // Allocate memory for new button node
    button_info_t *new_button = (button_info_t *)malloc(sizeof(button_info_t));
    if (!new_button)
    {
        // Return failure if memory allocation fails
        return ESP_FAIL;
    }

    // Initialize button info
    memset(new_button, 0, sizeof(button_info_t));

    // Copy configuration
    memcpy(&new_button->config, &button_config, sizeof(button_config_t));

    // Append to linked list
    if (!button_list_head)
    {
        // First button in the list
        button_list_head = new_button;
    }
    else
    {
        // Traverse to the end of the list
        button_info_t *current = button_list_head;

        // Append new button
        while (current->next)
        {
            current = current->next;
        }

        // Link the new button
        current->next = new_button;
    }

    // Start periodic timer if not already running
    if (!button_running)
    {
        // Create and start the timer
        esp_timer_create_args_t button_timer = {};
        button_timer.callback = ButtonTimerCallback;
        button_timer.arg = this;
        button_timer.name = "button_timer";
        button_timer.dispatch_method = ESP_TIMER_TASK;

        // Create and start the timer
        esp_timer_create(&button_timer, &button_timer_handle);

        // Start periodic timer
        esp_timer_start_periodic(button_timer_handle, BUTTON_TIMER_INTERVAL_MS * 1000);

        // Set running flag
        button_running = true;
    }

    return ESP_OK;
}

// Set button callbacks
void ButtonBasic::SetCallbacks(ButtonCallbacks &cb)
{
    // Set the callbacks
    callbacks = cb;
}
