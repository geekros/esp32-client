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

#ifndef LANGUAGE_SOUND_H
#define LANGUAGE_SOUND_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Define sound asset structure
typedef struct
{
    const uint8_t *data;
    size_t size;
} sound_asset_t;

// Declare sound assets
extern const sound_asset_t SOUND_WIFI_CONFIG;
extern const sound_asset_t SOUND_WIFI_SUCCESS;

#endif