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
#include "peer_module.h"

// Define log tag
#define TAG "[client:components:webrtc:peer]"

// Constructor
RTCPeerModule::RTCPeerModule()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
RTCPeerModule::~RTCPeerModule()
{
    // Delete event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
        event_group = nullptr;
    }
}
