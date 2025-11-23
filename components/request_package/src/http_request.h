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

#ifndef REQUEST_H
#define REQUEST_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include configuration and module headers
#include "client_config.h"

// Include components headers
#include "system_basic.h"

// Structure to hold HTTP response data
typedef struct
{
    char *buffer;
    int buffer_len;
    int data_offset;
} http_response_t;

// HttpRequest class definition
class HttpRequest
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // HTTP event handler
    static esp_err_t EventHandler(esp_http_client_event_t *event);

public:
    // Constructor and destructor
    HttpRequest();
    ~HttpRequest();

    // Get the singleton instance of the HttpRequest class
    static HttpRequest &Instance()
    {
        static HttpRequest instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    HttpRequest(const HttpRequest &) = delete;
    HttpRequest &operator=(const HttpRequest &) = delete;

    // Function to handle HTTP request
    esp_err_t Request(const std::string &url, esp_http_client_method_t method, const std::string &post_data, std::string &response_buf, int response_buf_len);
};

#endif