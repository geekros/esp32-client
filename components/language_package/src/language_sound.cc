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

// Include headers
#include "language_sound.h"

// Define log tag
#define TAG "[client:components:language:sound]"

// Declare sound assets
extern const uint8_t _binary_wifi_config_ogg_start[];
extern const uint8_t _binary_wifi_config_ogg_end[];

// Declare sound assets
extern const uint8_t _binary_wifi_success_ogg_start[];
extern const uint8_t _binary_wifi_success_ogg_end[];

// Define wifi configuration sound asset
const sound_asset_t SOUND_WIFI_CONFIG = {
    .data = _binary_wifi_config_ogg_start,
    .size = (size_t)(_binary_wifi_config_ogg_end - _binary_wifi_config_ogg_start),
};

// Define wifi success sound asset
const sound_asset_t SOUND_WIFI_SUCCESS = {
    .data = _binary_wifi_success_ogg_start,
    .size = (size_t)(_binary_wifi_success_ogg_end - _binary_wifi_success_ogg_start),
};