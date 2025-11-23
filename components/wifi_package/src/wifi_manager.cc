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
#include "wifi_manager.h"

// Define log tag
#define TAG "[client:components:wifi:manager]"

// Constructor
WifiManager::WifiManager()
{
    // Create event group
    event_group = xEventGroupCreate();

    // Load configuration
    Load();
}

// Destructor
WifiManager::~WifiManager()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Remove SSID from the manager
void WifiManager::Load()
{
    // Clear the SSID list
    ssid_list.clear();

    // Load from NVS
    nvs_handle_t nvs_handle;
    auto ret = nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
    {
        return;
    }

    // Load SSIDs
    for (int i = 0; i < MAX_WIFI_SSID_COUNT; i++)
    {
        std::string ssid_key = "ssid";
        if (i > 0)
        {
            ssid_key += std::to_string(i);
        }
        std::string password_key = "password";
        if (i > 0)
        {
            password_key += std::to_string(i);
        }

        char ssid[33];
        char password[65];
        size_t length = sizeof(ssid);
        if (nvs_get_str(nvs_handle, ssid_key.c_str(), ssid, &length) != ESP_OK)
        {
            continue;
        }
        length = sizeof(password);
        if (nvs_get_str(nvs_handle, password_key.c_str(), password, &length) != ESP_OK)
        {
            continue;
        }
        ssid_list.push_back({ssid, password});
    }

    // Close NVS handle
    nvs_close(nvs_handle);
}

// Save SSID list to NVS
void WifiManager::Save()
{
    // Open NVS handle
    nvs_handle_t nvs_handle;

    // Open NVS namespace
    ESP_ERROR_CHECK(nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle));
    for (int i = 0; i < MAX_WIFI_SSID_COUNT; i++)
    {
        std::string ssid_key = "ssid";
        if (i > 0)
        {
            ssid_key += std::to_string(i);
        }
        std::string password_key = "password";
        if (i > 0)
        {
            password_key += std::to_string(i);
        }

        if (i < ssid_list.size())
        {
            nvs_set_str(nvs_handle, ssid_key.c_str(), ssid_list[i].ssid.c_str());
            nvs_set_str(nvs_handle, password_key.c_str(), ssid_list[i].password.c_str());
        }
        else
        {
            nvs_erase_key(nvs_handle, ssid_key.c_str());
            nvs_erase_key(nvs_handle, password_key.c_str());
        }
    }

    // Commit changes and close NVS handle
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

// Add SSID to the manager
void WifiManager::Add(const std::string &ssid, const std::string &password)
{
    // Check if the SSID already exists
    for (auto &item : ssid_list)
    {
        if (item.ssid == ssid)
        {
            // Update password
            item.password = password;

            // Save to NVS
            Save();

            // Return
            return;
        }
    }

    // If the list is full, remove the last one
    if (ssid_list.size() >= MAX_WIFI_SSID_COUNT)
    {
        ssid_list.pop_back();
    }

    // Add the new ssid to the front of the list
    ssid_list.insert(ssid_list.begin(), {ssid, password});

    // Save to NVS
    Save();
}

// Remove SSID from the manager
void WifiManager::Remove(int index)
{
    // Check index
    if (index < 0 || index >= ssid_list.size())
    {
        ESP_LOGW(TAG, "Invalid index %d", index);
        return;
    }

    // Remove the ssid at index
    ssid_list.erase(ssid_list.begin() + index);

    // Save to NVS
    Save();
}

// Set default SSID
void WifiManager::SetDefault(int index)
{
    // Check index
    if (index < 0 || index >= ssid_list.size())
    {
        ESP_LOGW(TAG, "Invalid index %d", index);
        return;
    }

    // Move the ssid at index to the front of the list
    auto item = ssid_list[index];
    ssid_list.erase(ssid_list.begin() + index);
    ssid_list.insert(ssid_list.begin(), item);

    // Save to NVS
    Save();
}

// Remove SSID from the manager
void WifiManager::Clear()
{
    // Clear the SSID list
    ssid_list.clear();

    // Save to NVS
    Save();
}