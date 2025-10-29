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
#include "audio_server.h"

// Define log tag
#define TAG "[client:audio]"

// Audio server task
static void audio_server_task(void *param)
{
    // Log audio server start
    ESP_LOGI(TAG, "Audio server task started");

    // Main loop
    while (1)
    {
    }
}

// Function to initialize the audio server
void audio_server_start(audio_server_callback_t callback)
{
    // Create audio server configuration
    audio_server_config_t config = {};
    config.callback = callback;

    // Create WiFi connect AP task
    xTaskCreatePinnedToCore(audio_server_task, "audio_server_task", 4096, NULL, 3, NULL, 1);
}