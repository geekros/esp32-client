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
#include "media_module.h"

// Define log tag
#define TAG "[client:components:webrtc:media]"

// Constructor
RTCMediaModule::RTCMediaModule()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
RTCMediaModule::~RTCMediaModule()
{
    // Delete event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
        event_group = nullptr;
    }
}

// Memory allocation function
void *WEAK RTCMediaModule::media_lib_malloc(size_t size)
{
    // Use standard malloc
    return malloc(size);
}

// Calloc function
void *WEAK RTCMediaModule::media_lib_calloc(size_t nmemb, size_t size)
{
    // Use standard calloc
    return calloc(nmemb, size);
}

// Free function
void *WEAK RTCMediaModule::media_lib_realloc(void *ptr, size_t size)
{
    // Use standard realloc
    return realloc(ptr, size);
}

// Free function
void WEAK RTCMediaModule::media_lib_free(void *ptr)
{
    // Use standard free
    free(ptr);
}

// Mutex create function
int WEAK RTCMediaModule::media_lib_mutex_create(media_lib_mutex_handle_t *mutex)
{
    // Check for null pointer
    if (mutex == NULL)
    {
        return -1;
    }

    // Create recursive mutex
    *mutex = (media_lib_mutex_handle_t)xSemaphoreCreateRecursiveMutex();

    // Return success or failure
    return (*mutex ? 0 : -1);
}

// Mutex destroy function
int WEAK RTCMediaModule::media_lib_mutex_destroy(media_lib_mutex_handle_t mutex)
{
    // Check for null pointer
    if (mutex == NULL)
    {
        return -1;
    }

    // Delete the mutex
    vSemaphoreDelete((QueueHandle_t)mutex);

    // Return success
    return 0;
}

// Mutex lock function
int WEAK RTCMediaModule::media_lib_mutex_lock(media_lib_mutex_handle_t mutex, uint32_t timeout)
{
    // Check for null pointer
    if (mutex == NULL)
    {
        // Return failure
        return -1;
    }

    // Convert timeout from milliseconds to ticks
    if (timeout != 0xFFFFFFFF)
    {
        timeout = pdMS_TO_TICKS(timeout);
    }

    // Take the mutex
    return xSemaphoreTakeRecursive((QueueHandle_t)mutex, timeout);
}

// Mutex unlock function
int WEAK RTCMediaModule::media_lib_mutex_unlock(media_lib_mutex_handle_t mutex)
{
    // Check for null pointer
    if (mutex == NULL)
    {
        // Return failure
        return -1;
    }

    // Give the mutex
    xSemaphoreGive((QueueHandle_t)mutex);

    // Return success
    return 0;
}

// Thread sleep function
void WEAK RTCMediaModule::media_lib_thread_sleep(int ms)
{
    // Delay for specified milliseconds
    vTaskDelay(pdMS_TO_TICKS(ms));
}
