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
#include "auth_basic.h"

// Define log tag
#define TAG "[client:components:realtime:auth]"

// Constructor
RealtimeAuthorize::RealtimeAuthorize()
{
    // Create the event group
    event_group = xEventGroupCreate();
}

// Destructor
RealtimeAuthorize::~RealtimeAuthorize()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Request access token
response_access_token_t RealtimeAuthorize::Request(void)
{
    // Define access token structure
    response_access_token_t response_data;

    // Initialize response data
    memset(&response_data, 0, sizeof(response_data));

    // Initialize HTTPS
    auto https = NetworkHttps::Instance().InitHttps();

    // Open HTTPS connection
    if (!https->Open("GET", GEEKROS_SERVICE + std::string("/open/accesstoken?token=") + GEEKROS_SERVICE_PROJECT_TOKEN))
    {
        ESP_LOGE(TAG, "Failed to open HTTPS connection");
        return response_data;
    }

    // Read HTTPS response
    auto status_code = https->GetStatusCode();
    if (status_code != 200)
    {
        ESP_LOGE(TAG, "HTTPS request failed with status code %d", status_code);
        return response_data;
    }

    // Parse response body
    std::string response_body;
    response_body = https->ReadAll();

    // Clear HTTPS connection
    https->Close();

    // Parse JSON response
    cJSON *root = cJSON_Parse(response_body.c_str());
    if (!root)
    {
        // Return empty response data
        return response_data;
    }

    // Get code item
    cJSON *code = cJSON_GetObjectItem(root, "code");
    if (code && code->valueint == 0)
    {
        // Get data item
        cJSON *data = cJSON_GetObjectItem(root, "data");
        if (data)
        {
            // Get access token item
            cJSON *access_token = cJSON_GetObjectItem(data, "access_token");
            if (access_token && access_token->valuestring)
            {
                // Copy access token to response data
                strncpy(response_data.access_token, access_token->valuestring, sizeof(response_data.access_token) - 1);
            }

            // Get expiration item
            cJSON *expiration = cJSON_GetObjectItem(data, "expiration");
            if (expiration)
            {
                // Set expiration to response data
                response_data.expiration = expiration->valueint;
            }

            // Get time item
            cJSON *time = cJSON_GetObjectItem(data, "time");
            if (time)
            {
                // Set time to response data
                response_data.time = time->valueint;
            }

            // Delete JSON root
            cJSON_Delete(root);
        }
        else
        {
            // Delete JSON root
            cJSON_Delete(root);

            // Return empty response data
            return response_data;
        }
    }
    else
    {
        // Delete JSON root
        cJSON_Delete(root);

        // Return empty response data
        return response_data;
    }

    // Return response data
    return response_data;
}