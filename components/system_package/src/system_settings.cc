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
#include "system_settings.h"

// Define log tag
#define TAG "[client:components:system:settings]"

// Constructor
SystemSettings::SystemSettings()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
SystemSettings::~SystemSettings()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Initialize system settings
bool SystemSettings::Initialize()
{
    // Load settings from storage
    if (!Load())
    {
        // If loading failed, save default settings
        Save();

        // Return false to indicate settings were not loaded
        return false;
    }

    // Return true to indicate settings were loaded successfully
    return true;
}

// Load settings from NVS
bool SystemSettings::Load()
{
    // Open NVS handle
    nvs_handle_t nvs;
    if (nvs_open(GEEKROS_SYS_SETTINGS_NS, NVS_READONLY, &nvs) != ESP_OK)
    {
        return false;
    }

    // Get JSON string length
    size_t len = 0;
    if (nvs_get_str(nvs, GEEKROS_SYS_SETTINGS_KEY, nullptr, &len) != ESP_OK || len == 0)
    {
        nvs_close(nvs);
        return false;
    }

    // Read JSON string
    std::string json(len, '\0');
    if (nvs_get_str(nvs, GEEKROS_SYS_SETTINGS_KEY, json.data(), &len) != ESP_OK)
    {
        nvs_close(nvs);
        return false;
    }

    // Close NVS handle
    nvs_close(nvs);

    // Parse JSON and update settings
    return ParseJson(json);
}

// Save settings to NVS
bool SystemSettings::Save() const
{
    // Open NVS handle
    nvs_handle_t nvs;
    if (nvs_open(GEEKROS_SYS_SETTINGS_NS, NVS_READWRITE, &nvs) != ESP_OK)
    {
        return false;
    }

    // Build JSON string
    std::string json = BuildJson();
    esp_err_t err = nvs_set_str(nvs, GEEKROS_SYS_SETTINGS_KEY, json.c_str());
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs);
    }

    // Close NVS handle
    nvs_close(nvs);

    // Return true if save was successful
    return err == ESP_OK;
}

// Get audio volume
uint8_t SystemSettings::GetAudioVolume() const
{
    // Return audio volume
    return audio_volume_;
}

// Get WiFi Access Point mode
bool SystemSettings::IsWifiAccessPointMode() const
{
    // Return WiFi Access Point mode status
    return wifi_access_point_;
}

// Set audio volume
bool SystemSettings::SetAudioVolume(uint8_t volume)
{
    // Clamp volume to 0-100 range
    if (volume > 100)
    {
        volume = 100;
    }

    // Set audio volume and save settings
    audio_volume_ = volume;
    return Save();
}

// Set WiFi Access Point mode
bool SystemSettings::SetWifiAccessPointMode(bool enable)
{
    // Set WiFi Access Point mode and save settings
    wifi_access_point_ = enable;
    return Save();
}

// Parse JSON string and update settings
bool SystemSettings::ParseJson(const std::string &json)
{
    // Parse JSON string
    cJSON *root = cJSON_Parse(json.c_str());
    if (!root)
    {
        return false;
    }

    // Parse audio volume and WiFi Access Point mode
    cJSON *audio = cJSON_GetObjectItem(root, "audio");
    if (cJSON_IsObject(audio))
    {
        // Get volume
        cJSON *vol = cJSON_GetObjectItem(audio, "volume");
        if (cJSON_IsNumber(vol))
        {
            int v = vol->valueint;
            audio_volume_ = (v < 0) ? 0 : (v > 100 ? 100 : v);
        }
    }

    // Parse WiFi settings
    cJSON *wifi = cJSON_GetObjectItem(root, "wifi");
    if (cJSON_IsObject(wifi))
    {
        // Get access point mode
        cJSON *ap = cJSON_GetObjectItem(wifi, "access_point");
        if (cJSON_IsBool(ap))
        {
            wifi_access_point_ = cJSON_IsTrue(ap);
        }
    }

    // Delete JSON object
    cJSON_Delete(root);

    // Return true to indicate successful parsing
    return true;
}

// Build JSON string from settings
std::string SystemSettings::BuildJson() const
{
    // Create JSON object
    cJSON *root = cJSON_CreateObject();

    // Add audio volume and WiFi Access Point mode
    cJSON *audio = cJSON_AddObjectToObject(root, "audio");
    cJSON_AddNumberToObject(audio, "volume", audio_volume_);

    // Add WiFi settings
    cJSON *wifi = cJSON_AddObjectToObject(root, "wifi");
    cJSON_AddBoolToObject(wifi, "access_point", wifi_access_point_);

    // Print JSON string
    char *out = cJSON_PrintUnformatted(root);
    std::string json(out);
    cJSON_free(out);
    cJSON_Delete(root);

    // Return JSON string
    return json;
}

// Get settings as JSON string
std::string SystemSettings::GetJson() const
{
    // Build and return JSON string
    return BuildJson();
}

// Update settings from JSON string
bool SystemSettings::UpdateFromJson(const std::string &json)
{
    // Parse JSON and update settings
    if (!ParseJson(json))
    {
        return false;
    }

    // Save updated settings
    return Save();
}
