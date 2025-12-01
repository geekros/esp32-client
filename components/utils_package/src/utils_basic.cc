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

// Get captive portal URLs
const char *const *UtilsBasic::GetCaptiveUrls(size_t &count)
{
    // Define captive portal URLs
    static const char *urls[] = {
        "/hotspot-detect.html",      // Apple
        "/generate_204*",            // Android
        "/mobile/status.php",        // Android
        "/check_network_status.txt", // Windows
        "/ncsi.txt",                 // Windows
        "/fwlink/",                  // Microsoft
        "/connectivity-check.html",  // Firefox
        "/success.txt",              // Various
        "/portal.html",              // Various
        "/library/test/success.html" // Apple
    };

    // Return URLs and count
    count = sizeof(urls) / sizeof(urls[0]);
    return urls;
}

// Mask section of a string
std::string UtilsBasic::MaskSection(const std::string text, size_t start, size_t end)
{
    // Validate indices
    if (start >= text.size() || start >= end)
    {
        return text;
    }

    // Adjust end index if it exceeds text size
    if (end > text.size())
    {
        end = text.size();
    }

    // Create masked string
    std::string result;
    result.reserve(text.size() - (end - start) + 3);

    // Construct masked string
    result = text.substr(0, start);
    result += "***";
    result += text.substr(end);

    // Return masked string
    return result;
}