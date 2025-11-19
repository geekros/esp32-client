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

#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_http_server.h>

// Include cJSON header
#include "cJSON.h"

// Include client configuration header
#include "client_config.h"

// Include component headers
#include "system_reboot.h"

// Include headers
#include "wifi_configuration.h"

// Define server configuration structure and callback type
typedef struct
{
    const char *http_html;
} server_config_t;

// Function to start the WiFi server
esp_err_t wifi_server_start(void);

// Function to stop the WiFi server
esp_err_t wifi_server_stop(void);

#endif