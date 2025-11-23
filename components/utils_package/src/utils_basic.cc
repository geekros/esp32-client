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
#include "utils_basic.h"

// Define log tag
#define TAG "[client:components:utils:basic]"

// Constructor
UtilsBasic::UtilsBasic()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
UtilsBasic::~UtilsBasic()
{
    // Delete event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
    }
}

// Get MIME type based on URI
const char *UtilsBasic::GetMimeType(const char *uri)
{
    if (strstr(uri, ".css"))
    {
        return "text/css";
    }
    if (strstr(uri, ".js"))
    {
        return "application/javascript";
    }
    if (strstr(uri, ".json"))
    {
        return "application/json";
    }
    if (strstr(uri, ".png"))
    {
        return "image/png";
    }
    if (strstr(uri, ".jpg") || strstr(uri, ".jpeg"))
    {
        return "image/jpeg";
    }
    if (strstr(uri, ".gif"))
    {
        return "image/gif";
    }
    if (strstr(uri, ".svg"))
    {
        return "image/svg+xml";
    }
    if (strstr(uri, ".ico"))
    {
        return "image/x-icon";
    }
    return "text/plain";
}

// Get captive portal URLs
const char *const *UtilsBasic::GetCaptiveUrls(size_t &count)
{
    // Define captive portal URLs
    static const char *urls[] = {
        "/hotspot-detect.html",
        "/generate204",
        "/generate_204*",
        "/mobile/status.php",
        "/check_network_status.txt",
        "/ncsi.txt",
        "/connecttest.txt",
        "/redirect",
        "/fwlink/",
        "/connectivity-check.html",
        "/success.txt",
        "/portal.html",
        "/library/test/success.html",
        "/mmtls/*",
        "/wifidog/*",
    };

    // Return URLs and count
    count = sizeof(urls) / sizeof(urls[0]);
    return urls;
}