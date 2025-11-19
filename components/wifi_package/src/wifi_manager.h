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

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include NVS headers
#include <nvs_flash.h>
#include <nvs.h>

// Include client configuration header
#include "client_config.h"

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASSWORD_MAX_LEN 64
#define WIFI_MAX_WIFI_SSID_COUNT 10

// Define SSID item structure
typedef struct
{
    char ssid[WIFI_SSID_MAX_LEN + 1];
    char password[WIFI_PASSWORD_MAX_LEN + 1];
} wifi_ssid_item_t;

// Define SSID manager structure
typedef struct
{
    wifi_ssid_item_t items[WIFI_MAX_WIFI_SSID_COUNT];
    int count;
} wifi_ssid_manager_t;

// Function to get SSID manager instance
wifi_ssid_manager_t *wifi_ssid_manager_get_instance(void);

// Functions to load and save SSID manager data
void wifi_ssid_manager_load(wifi_ssid_manager_t *mgr);
void wifi_ssid_manager_save(wifi_ssid_manager_t *mgr);

// Function to clear SSID manager data
void wifi_ssid_manager_clear(wifi_ssid_manager_t *mgr);

// Functions to manipulate SSID items
void wifi_ssid_manager_add(wifi_ssid_manager_t *mgr, const char *ssid, const char *password);
void wifi_ssid_manager_remove(wifi_ssid_manager_t *mgr, int index);
void wifi_ssid_manager_set_default(wifi_ssid_manager_t *mgr, int index);

// Function to get SSID item list
const wifi_ssid_item_t *wifi_ssid_manager_get_list(wifi_ssid_manager_t *mgr);

// Function to get SSID item count
int wifi_ssid_manager_get_count(wifi_ssid_manager_t *mgr);

#endif