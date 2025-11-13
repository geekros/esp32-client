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
#include "authentication.h"

// Define log tag
#define TAG "[client:components:realtime:authentication]"

// Initialize authentication
response_access_token_t authentication_request(void)
{
    // Define access token structure
    response_access_token_t response_data;

    // Initialize response data
    response_data.access_token[0] = '\0';
    response_data.expiration = 0;

    // Response buffer
    char response[1024];

    // Make HTTP GET request to obtain access token
    if (http_request(GEEKROS_SERVICE "/open/accesstoken?token=" GEEKROS_SERVICE_PROJECT_TOKEN, HTTP_METHOD_GET, NULL, response, sizeof(response)) == ESP_OK)
    {
        // Parse JSON response
        cJSON *root = cJSON_Parse(response);
        if (!root)
        {
            ESP_LOGE(TAG, "Failed to parse JSON response");

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
                // Get access_token item
                cJSON *access_token = cJSON_GetObjectItem(data, "access_token");
                if (access_token && access_token->valuestring)
                {
                    strncpy(response_data.access_token, access_token->valuestring, sizeof(response_data.access_token) - 1);
                }

                // Get expiration item
                cJSON *expiration = cJSON_GetObjectItem(data, "expiration");
                if (expiration)
                {
                    response_data.expiration = expiration->valueint;
                }

                ESP_LOGI(TAG, "Authentication successful. Access Token: %s, Expiration: %d", response_data.access_token, response_data.expiration);
            }
            else
            {
                // Get message item for error logging
                cJSON *msg = cJSON_GetObjectItem(root, "message");

                ESP_LOGE(TAG, "Authentication failed. Message: %s", msg->valuestring);

                // Return empty response data
                return response_data;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Authentication request failed with code: %d", code);

            // Return empty response data
            return response_data;
        }
    }

    // Return response data
    return response_data;
}
