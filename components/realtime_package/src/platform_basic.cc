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

// Include the headers
#include "platform_basic.h"

// Define log tag
#define TAG "[client:components:realtime:platform]"

// Constructor
PlatformBasic::PlatformBasic()
{
    // Create the event group
    event_group = xEventGroupCreate();
}

// Destructor
PlatformBasic::~PlatformBasic()
{
    // Delete the event group
    if (event_group != nullptr)
    {
        // Delete event group
        vEventGroupDelete(event_group);

        // Set event group to nullptr
        event_group = nullptr;
    }
}

// Platform create peer function
int PlatformBasic::PlatformCreatePeer()
{
    // Create the peer
    return ESP_OK;
}