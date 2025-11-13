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

#ifndef SYSTEM_H
#define SYSTEM_H

// Include standard headers
#include <stdio.h>
#include <string.h>
#include <time.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_spiffs.h>
#include <esp_mac.h>

//  Initialize system components
void system_init(const char *base_path, const char *partition_label, size_t max_files);

// Get system chip ID
void system_chip_id(char *out_str, size_t len);

#endif