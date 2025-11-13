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

#ifndef REALTIME_AUTHENTICATION_H
#define REALTIME_AUTHENTICATION_H

// Include standard headers
#include <stdio.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_http_client.h>

// Include common headers
#include "cJSON.h"

// Include configuration and module headers
#include "common_config.h"

// Include components headers
#include "http_request.h"

// Define response access token structure
typedef struct
{
    char access_token[128];
    int expiration;
} response_access_token_t;

// Function to initialize authentication
response_access_token_t authentication_request(void);

#endif