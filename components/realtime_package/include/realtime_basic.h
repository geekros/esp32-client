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

#ifndef REALTIME_BASIC_H
#define REALTIME_BASIC_H

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
#include "ping/ping_sock.h"
#include "lwip/netdb.h"

// Include configuration and module headers
#include "client_config.h"

// Include headers
#include "auth_basic.h"
#include "signaling_basic.h"
#include "platform_basic.h"
#include "system_time.h"
#include "utils_basic.h"

// Realtime basic class
class RealtimeBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and destructor
    RealtimeBasic();
    ~RealtimeBasic();

    // Get the singleton instance of the RealtimeBasic class
    static RealtimeBasic &Instance()
    {
        static RealtimeBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    RealtimeBasic(const RealtimeBasic &) = delete;
    RealtimeBasic &operator=(const RealtimeBasic &) = delete;

    // Realtime start and stop methods
    void RealtimeStart(void);
};

#endif