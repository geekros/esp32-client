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
#include "system_time.h"

// Define log tag
#define TAG "[client:components:system:time]"

// Constructor
SystemTime::SystemTime()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
SystemTime::~SystemTime()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Get current time as string
std::string SystemTime::GetTimeString()
{
    // Apply timezone
    ApplyTimezone();

    // Get current time
    time_t now;
    time(&now);

    // Convert to local time
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Format time as string
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // Return time string
    return std::string(buffer);
}

// Get current Unix timestamp
time_t SystemTime::GetUnixTimestamp()
{
    // Apply timezone
    ApplyTimezone();

    // Get current time
    time_t now;
    time(&now);

    // Return Unix timestamp
    return now;
}

// Get current timezone string
std::string SystemTime::CurrentTimezone()
{
#if CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_12
    return "UTC-12";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_11
    return "UTC-11";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_10
    return "UTC-10";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_9_30
    return "UTC-09:30";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_9
    return "UTC-09";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_8
    return "UTC-08";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_7
    return "UTC-07";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_6
    return "UTC-06";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_5
    return "UTC-05";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_4
    return "UTC-04";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_3_30
    return "UTC-03:30";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_3
    return "UTC-03";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_2
    return "UTC-02";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_NEG_1
    return "UTC-01";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_0
    return "UTC";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_1
    return "UTC+01";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_2
    return "UTC+02";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_3
    return "UTC+03";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_3_30
    return "UTC+03:30";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_4
    return "UTC+04";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_4_30
    return "UTC+04:30";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_5
    return "UTC+05";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_5_30
    return "UTC+05:30"; // India
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_5_45
    return "UTC+05:45"; // Nepal
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_6
    return "UTC+06";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_6_30
    return "UTC+06:30"; // Myanmar
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_7
    return "UTC+07";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_8
    return "UTC+08"; // China default
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_8_45
    return "UTC+08:45";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_9
    return "UTC+09";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_9_30
    return "UTC+09:30";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_10
    return "UTC+10";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_10_30
    return "UTC+10:30";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_11
    return "UTC+11";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_12
    return "UTC+12";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_12_45
    return "UTC+12:45";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_13
    return "UTC+13";
#elif CONFIG_GEEKROS_SYSTEM_TIMEZONE_UTC_14
    return "UTC+14";
#else
    return "UTC+08";
#endif
}

// Apply timezone setting
void SystemTime::ApplyTimezone()
{
    // Set timezone environment variable
    auto tz = CurrentTimezone();

    // Set TZ environment variable
    setenv("TZ", tz.c_str(), 1);

    // Apply timezone settings
    tzset();
}

// Set system time in milliseconds
esp_err_t SystemTime::SetTimeMs(uint64_t timestamp_ms)
{
    // Create timeval structure
    struct timeval tv;
    tv.tv_sec = timestamp_ms / 1000;
    tv.tv_usec = (timestamp_ms % 1000) * 1000;

    // Set system time
    if (settimeofday(&tv, NULL) != 0)
    {
        return ESP_FAIL;
    }

    // Apply timezone
    ApplyTimezone();

    // Return success
    return ESP_OK;
}

// Set system time in seconds
esp_err_t SystemTime::SetTimeSec(uint32_t timestamp_sec)
{
    // Create timeval structure
    struct timeval tv{(time_t)timestamp_sec, 0};

    // Set system time
    if (settimeofday(&tv, NULL) != 0)
    {
        return ESP_FAIL;
    }

    // Apply timezone
    ApplyTimezone();

    // Return success
    return ESP_OK;
}