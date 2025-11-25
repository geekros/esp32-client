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
#include "authorize.h"

// Define log tag
#define TAG "[client:components:realtime:authorize]"

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
    response_data.access_token[0] = '\0';
    response_data.expiration = 0;

    // Response buffer
    char response[1024];

    // Send HTTP GET request to obtain access token
    if (HttpRequest::Instance().Request("/open/accesstoken?token=" GEEKROS_SERVICE_PROJECT_TOKEN, HTTP_METHOD_GET, NULL, response, sizeof(response)) == ESP_OK)
    {
        // Parse JSON response
        cJSON *root = cJSON_Parse(response);
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

                // Delete JSON root
                cJSON_Delete(root);

                ESP_LOGI(TAG, "Authorize successful. Access Token: %s, Expiration: %d", response_data.access_token, response_data.expiration);
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
    }

    // Return response data
    return response_data;
}