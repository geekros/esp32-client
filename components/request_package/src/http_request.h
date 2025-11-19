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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>

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

// Function to handle HTTP request
esp_err_t http_request(const char *url, esp_http_client_method_t method, const char *post_data, char *response_buf, int response_buf_len);

#endif