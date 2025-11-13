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

// Board interface pointer
static const board_t *board_interface = NULL;

// Define log tag
#define TAG "[client:application]"

// Application main function
void application_main(void)
{
    // Initialize system components
    system_init(GEEKROS_SPIFFS_BASE_PATH, GEEKROS_SPIFFS_LABEL, GEEKROS_SPIFFS_MAX_FILE);

    // Initialize the board-specific hardware
    board_interface = board();

    // Call the board initialization function
    board_interface->board_init();

    // Start the application loop
    application_loop();
}

// Application loop function
void application_loop(void)
{
    // Main application loop
    while (true)
    {
        // Log a heartbeat message every 500 milliseconds
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}