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

#ifndef REALTIME_AUTHORIZE_H
#define REALTIME_AUTHORIZE_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_http_client.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include common headers
#include "cJSON.h"

// Include configuration and module headers
#include "client_config.h"

// Include components headers
#include "network_https.h"

// Define response access token structure
typedef struct
{
    char access_token[256];
    int expiration;
    int time;
} response_access_token_t;

// Realtime authorize class
class RealtimeAuthorize
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and destructor
    RealtimeAuthorize();
    ~RealtimeAuthorize();

    // Get the singleton instance of the RealtimeAuthorize class
    static RealtimeAuthorize &Instance()
    {
        static RealtimeAuthorize instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    RealtimeAuthorize(const RealtimeAuthorize &) = delete;
    RealtimeAuthorize &operator=(const RealtimeAuthorize &) = delete;

    // Request access token
    response_access_token_t Request(void);
};

#endif