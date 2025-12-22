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

#ifndef NETWORK_HTTPS_H
#define NETWORK_HTTPS_H

// Include standard headers
#include <string>
#include <memory>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include configuration and module headers
#include "client_config.h"

// Include headers
#include "http.h"
#include "network_basic.h"
#include "system_basic.h"
#include "system_time.h"

// HTTPS basic class
class NetworkHttps
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and destructor
    NetworkHttps();
    ~NetworkHttps();

    // Get the singleton instance of the NetworkBasic class
    static NetworkHttps &Instance()
    {
        static NetworkHttps instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    NetworkHttps(const NetworkHttps &) = delete;
    NetworkHttps &operator=(const NetworkHttps &) = delete;

    // HTTPS initialization method
    std::unique_ptr<Http> InitHttps();
};

#endif