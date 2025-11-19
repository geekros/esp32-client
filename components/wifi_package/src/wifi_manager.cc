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

// Define static instance of SSID manager
static wifi_ssid_manager_t wifi_instance;

// Function to get SSID manager instance
wifi_ssid_manager_t *wifi_ssid_manager_get_instance(void)
{
    return &wifi_instance;
}

// Function to clear SSID manager data
void wifi_ssid_manager_clear(wifi_ssid_manager_t *mgr)
{
    mgr->count = 0;
    wifi_ssid_manager_save(mgr);
}

// Function to load SSID manager data
void wifi_ssid_manager_load(wifi_ssid_manager_t *mgr)
{
    mgr->count = 0;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK)
    {
        return;
    }

    for (int i = 0; i < WIFI_MAX_WIFI_SSID_COUNT; i++)
    {
        char key_ssid[16];
        char key_pwd[16];

        if (i == 0)
        {
            strcpy(key_ssid, "ssid");
            strcpy(key_pwd, "password");
        }
        else
        {
            sprintf(key_ssid, "ssid%d", i);
            sprintf(key_pwd, "password%d", i);
        }

        char ssid_buf[WIFI_SSID_MAX_LEN + 1];
        char pwd_buf[WIFI_PASSWORD_MAX_LEN + 1];

        size_t len = sizeof(ssid_buf);
        if (nvs_get_str(nvs, key_ssid, ssid_buf, &len) != ESP_OK)
        {
            continue;
        }

        len = sizeof(pwd_buf);
        if (nvs_get_str(nvs, key_pwd, pwd_buf, &len) != ESP_OK)
        {
            continue;
        }

        strncpy(mgr->items[mgr->count].ssid, ssid_buf, WIFI_SSID_MAX_LEN);
        strncpy(mgr->items[mgr->count].password, pwd_buf, WIFI_PASSWORD_MAX_LEN);
        mgr->count++;
    }

    nvs_close(nvs);
}

// Function to save SSID manager data
void wifi_ssid_manager_save(wifi_ssid_manager_t *mgr)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs));

    for (int i = 0; i < WIFI_MAX_WIFI_SSID_COUNT; i++)
    {
        char key_ssid[16];
        char key_pwd[16];

        if (i == 0)
        {
            strcpy(key_ssid, "ssid");
            strcpy(key_pwd, "password");
        }
        else
        {
            sprintf(key_ssid, "ssid%d", i);
            sprintf(key_pwd, "password%d", i);
        }

        if (i < mgr->count)
        {
            nvs_set_str(nvs, key_ssid, mgr->items[i].ssid);
            nvs_set_str(nvs, key_pwd, mgr->items[i].password);
        }
        else
        {
            nvs_erase_key(nvs, key_ssid);
            nvs_erase_key(nvs, key_pwd);
        }
    }

    nvs_commit(nvs);
    nvs_close(nvs);
}

// Function to add SSID item
void wifi_ssid_manager_add(wifi_ssid_manager_t *mgr, const char *ssid, const char *password)
{
    for (int i = 0; i < mgr->count; i++)
    {
        if (strcmp(mgr->items[i].ssid, ssid) == 0)
        {
            ESP_LOGW(TAG, "SSID %s exists, overwrite", ssid);
            strncpy(mgr->items[i].password, password, WIFI_PASSWORD_MAX_LEN);
            wifi_ssid_manager_save(mgr);
            return;
        }
    }

    if (mgr->count >= WIFI_MAX_WIFI_SSID_COUNT)
    {
        for (int i = WIFI_MAX_WIFI_SSID_COUNT - 1; i > 0; i--)
        {
            mgr->items[i] = mgr->items[i - 1];
        }

        mgr->count = WIFI_MAX_WIFI_SSID_COUNT - 1;
    }

    for (int i = mgr->count; i > 0; i--)
    {
        mgr->items[i] = mgr->items[i - 1];
    }

    strncpy(mgr->items[0].ssid, ssid, WIFI_SSID_MAX_LEN);
    strncpy(mgr->items[0].password, password, WIFI_PASSWORD_MAX_LEN);

    mgr->count++;

    wifi_ssid_manager_save(mgr);
}

// Function to set default SSID item
void wifi_ssid_manager_remove(wifi_ssid_manager_t *mgr, int index)
{
    if (index < 0 || index >= mgr->count)
    {
        return;
    }

    for (int i = index; i < mgr->count - 1; i++)
        mgr->items[i] = mgr->items[i + 1];

    mgr->count--;

    wifi_ssid_manager_save(mgr);
}

// Function to get SSID item list
void wifi_ssid_manager_set_default(wifi_ssid_manager_t *mgr, int index)
{
    if (index < 0 || index >= mgr->count)
    {
        ESP_LOGW(TAG, "Invalid index %d", index);
        return;
    }

    wifi_ssid_item_t temp = mgr->items[index];

    for (int i = index; i > 0; i--)
    {
        mgr->items[i] = mgr->items[i - 1];
    }

    mgr->items[0] = temp;

    wifi_ssid_manager_save(mgr);
}

// Function to get SSID item list
const wifi_ssid_item_t *wifi_ssid_manager_get_list(wifi_ssid_manager_t *mgr)
{
    return mgr->items;
}

// Function to get SSID item count
int wifi_ssid_manager_get_count(wifi_ssid_manager_t *mgr)
{
    return mgr->count;
}