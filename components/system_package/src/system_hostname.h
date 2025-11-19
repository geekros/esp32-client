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

#ifndef SYSTEM_HOSTNAME_H
#define SYSTEM_HOSTNAME_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_mac.h>

// Include NVS header
#include "nvs_flash.h"

// Include client configuration header
#include "client_config.h"

// Function to set the device hostname
const char *get_hostname();

// Clear NVS hostname (for testing purposes)
void clear_hostname(void);

#endif
