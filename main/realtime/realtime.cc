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

// Include the headers
#include "realtime.h"

// Define log tag
#define TAG "[client:realtime]"

// Initialize realtime
void realtime_init(relatime_t &realtime)
{
    // Set running state to false
    realtime.started = false;
}

// Initialize realtime
void realtime_connection(void)
{
    // Start authentication request
    response_access_token_t authentication = authentication_request();
    if (strlen(authentication.access_token) > 0)
    {
        ESP_LOGI(TAG, "Access Token: %s", authentication.access_token);
        ESP_LOGI(TAG, "Expiration: %d", authentication.expiration);
    }
}