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
#include "language_basic.h"

// Define log tag
#define TAG "[client:components:language]"

// Global variable to hold current locale code
static char language_code[16] = "ZH_CN";

// Global variable to hold parsed JSON root
static cJSON *json_root = NULL;

// Global variable to hold music path
static char *language_audio_path = NULL;

// Function to load locale JSON file
static void load_locale_json()
{
    // Example: construct file path based on locale_code
    char path[128];

    // Construct the path to the locale JSON file
    snprintf(path, sizeof(path), GEEKROS_SPIFFS_LANGUAGE_PATH "/strings.json");

    // Open the file
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        ESP_LOGW(TAG, "Failed to open locale file: %s", path);
        return;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    // Allocate buffer to read file content
    char *buffer = (char *)malloc(size + 1);
    if (!buffer)
    {
        ESP_LOGW(TAG, "Failed to allocate memory for locale file");
        fclose(fp);
        return;
    }

    // Read file content
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);

    // Parse JSON content
    json_root = cJSON_Parse(buffer);
    free(buffer);
}

// Initialization function for language package
void language_init(void)
{
    // Load locale JSON file
    load_locale_json();

    // Set music path (stub value)
    if (!language_audio_path)
    {
        // Allocate and set music path
        language_audio_path = (char *)malloc(256);
    }

    ESP_LOGI(TAG, "Current language: %s %s", get_language(), get_language_content("language"));
}

// Function to get the current locale/language code
const char *get_language(void)
{
    // Determine locale code based on configuration
#if CONFIG_GEEKROS_SYSTEM_LANGUAGE_EN_US
    strcpy(language_code, "EN_US");
#elif CONFIG_GEEKROS_SYSTEM_LANGUAGE_ZH_CN
    strcpy(language_code, "ZH_CN");
#elif CONFIG_GEEKROS_SYSTEM_LANGUAGE_ZH_TW
    strcpy(language_code, "ZH_TW");
#else
    strcpy(language_code, "ZH_CN");
#endif

    // Return the current locale code
    return language_code;
}

// Function to get localized string by key
const char *get_language_content(const char *key)
{
    // Check if JSON root is loaded
    if (!json_root)
    {
        ESP_LOGW(TAG, "Locale JSON not loaded");
        // Return the key itself if not found
        return key;
    }

    // Get the item from JSON by key
    cJSON *item = cJSON_GetObjectItem(json_root, key);
    if (!item || !cJSON_IsString(item))
    {
        ESP_LOGW(TAG, "Key not found in locale JSON: %s", key);
        // Return the key itself if not found
        return key;
    }

    // Return the localized string
    return item->valuestring;
}

// Function to get localized music string by key
const char *get_language_audio_path(const char *key)
{
    // Check if music path is allocated
    if (!language_audio_path)
    {
        ESP_LOGW(TAG, "Music path not set");
        // Return NULL if music path is not set
        return NULL;
    }

    // Construct the music file path based on locale and key
    snprintf(language_audio_path, 256, "/spiffs/language/%s/%s.ogg", language_code, key);

    // Return the music file path
    return language_audio_path;
}
