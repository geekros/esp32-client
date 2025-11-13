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

#ifndef GEEKROS_HOST_NAME_H
#define GEEKROS_HOST_NAME_H

// Include standard headers
#include <stdio.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_mac.h>

// Include common library
#include "nvs.h"

// Include configuration header
#include "common_config.h"

// Function to set the device hostname
esp_err_t get_hostname(char *hostname, size_t len);

// Clear NVS hostname (for testing purposes)
void clear_hostname(void);

#endif