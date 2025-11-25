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

#ifndef SYSTEM_REBOOT_H
#define SYSTEM_REBOOT_H

// Include standard headers
#include <string>
#include <ctime>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_sntp.h>
#include <esp_netif_sntp.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include headers
#include "lwip/dns.h"

// SystemReboot class definition
class SystemTime
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and Destructor
    SystemTime();
    ~SystemTime();

    // Get the singleton instance of the SystemTime class
    static SystemTime &Instance()
    {
        static SystemTime instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    SystemTime(const SystemTime &) = delete;
    SystemTime &operator=(const SystemTime &) = delete;

    // Initialize time synchronization
    void InitTimeSync();

    // Wait for time synchronization
    bool WaitForSync(int timeout_ms = 8000);

    // Get current time as string
    std::string GetTimeString();

    // Get current Unix timestamp
    time_t GetUnixTimestamp();
};

#endif