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

#ifndef LANGUAGE_BASIC_H
#define LANGUAGE_BASIC_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include cJSON header
#include "cJSON.h"

// Include client configuration header
#include "client_config.h"

// Initialization function for language package
void language_init(void);

// Function to get the current locale/language code
const char *get_language(void);

// Function to get localized string by key
const char *get_language_content(const char *key);

// Function to get localized music string by key
const char *get_language_audio_path(const char *key);

#endif