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

#ifndef GEEKROS_WIFI_CONNECT_H
#define GEEKROS_WIFI_CONNECT_H

// Include standard libraries
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>
#include <esp_spiffs.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include common headers
#include "cJSON.h"

// Include configuration header
#include "common_config.h"
#include "device/host_name.h"
#include "wifi_manage.h"
#include "server.h"

// Define SPIFFS base path
#define SPIFFS_BASE_PATH "/spiffs"
#define SPIFFS_HTML_FILE_PATH "/spiffs/html/index.html"

// Define WiFi connect scan done bit
#define WIFI_CONNECT_SCAN_DONE_BIT (BIT0)

// Function to initialize AP WiFi mode
void wifi_connect_init(p_wifi_state_change_callback callback);

// Function to configure AP WiFi network
void wifi_network_configure(void);

#endif