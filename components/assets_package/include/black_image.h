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

#ifndef ASSETS_BLACK_IMAGE_H
#define ASSETS_BLACK_IMAGE_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// AssetBlackImage class definition
class AssetBlackImage
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    static const uint8_t black_jpeg[];
    static const size_t black_jpeg_len;

public:
    // Constructor and Destructor
    AssetBlackImage();
    ~AssetBlackImage();

    // Get the singleton instance of the AssetBlackImage class
    static AssetBlackImage &Instance()
    {
        static AssetBlackImage instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    AssetBlackImage(const AssetBlackImage &) = delete;
    AssetBlackImage &operator=(const AssetBlackImage &) = delete;

    // Get black JPEG data methods
    static const uint8_t *Data();

    // Get black JPEG length method
    static size_t Length();
};

#endif