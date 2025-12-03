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

#ifndef MEDIA_MODULE_H
#define MEDIA_MODULE_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

// Define media library mutex handle type
typedef struct MediaLibMutexDef_t *media_lib_mutex_handle_t;

// Define maximum lock time for media library mutex
#define MEDIA_LIB_MAX_LOCK_TIME 0xFFFFFFFF

// Define weak attribute macro
#define WEAK __attribute__((weak))

// RTCMediaModule class definition
class RTCMediaModule
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

public:
    // Constructor and Destructor
    RTCMediaModule();
    ~RTCMediaModule();

    // Get the singleton instance of the RTCMediaModule class
    static RTCMediaModule &Instance()
    {
        static RTCMediaModule instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    RTCMediaModule(const RTCMediaModule &) = delete;
    RTCMediaModule &operator=(const RTCMediaModule &) = delete;

    // Media library memory management functions
    void *media_lib_malloc(size_t size);

    // Realloc function
    void *media_lib_realloc(void *ptr, size_t size);

    // Calloc function
    void *media_lib_calloc(size_t nmemb, size_t size);

    // Free function
    void media_lib_free(void *ptr);

    // Media library mutex management functions
    int media_lib_mutex_create(media_lib_mutex_handle_t *mutex);

    // Destroy function
    int media_lib_mutex_destroy(media_lib_mutex_handle_t mutex);

    // Lock function
    int media_lib_mutex_lock(media_lib_mutex_handle_t mutex, uint32_t timeout);

    // Unlock function
    int media_lib_mutex_unlock(media_lib_mutex_handle_t mutex);

    // Thread sleep function
    void media_lib_thread_sleep(int ms);
};

#endif