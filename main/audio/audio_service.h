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

#ifndef GEEKROS_AUDIO_SERVER_H
#define GEEKROS_AUDIO_SERVER_H

// Include standard libraries
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include module headers
#include "board_config.h"

// Audio server callback type
typedef void (*audio_server_callback_t)(void);

// Audio configuration structure
typedef struct
{
    audio_server_callback_t callback; // Callback function for audio events
} audio_server_config_t;

// Function to initialize the audio server
void audio_server_start(audio_server_callback_t callback);

#endif