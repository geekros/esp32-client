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
#include <string>
#include <string_view>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Namespace for language sounds
namespace Lang
{
    // Namespace for sound assets
    namespace Sounds
    {
        // WiFi configuration sound asset
        extern const char ogg_wifi_config_start[] asm("_binary_wifi_config_ogg_start");
        extern const char ogg_wifi_config_end[] asm("_binary_wifi_config_ogg_end");
        static const std::string_view OGG_WIFI_CONFIG{
            static_cast<const char *>(ogg_wifi_config_start),
            static_cast<size_t>(ogg_wifi_config_end - ogg_wifi_config_start),
        };

        // WiFi success sound asset
        extern const char ogg_wifi_success_start[] asm("_binary_wifi_success_ogg_start");
        extern const char ogg_wifi_success_end[] asm("_binary_wifi_success_ogg_end");
        static const std::string_view OGG_WIFI_SUCCESS{
            static_cast<const char *>(ogg_wifi_success_start),
            static_cast<size_t>(ogg_wifi_success_end - ogg_wifi_success_start),
        };
    }
}

#endif