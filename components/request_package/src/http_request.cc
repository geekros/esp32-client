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
#include "http_request.h"

// Define log tag
#define TAG "[client:components:request]"

// Constructor
HttpRequest::HttpRequest()
{
    event_group = xEventGroupCreate();
}

// Destructor
HttpRequest::~HttpRequest()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// HTTP event handler
esp_err_t HttpRequest::EventHandler(esp_http_client_event_t *event)
{
    // Get the response structure from user data
    http_response_t *resp = (http_response_t *)event->user_data;

    // Handle different HTTP events
    switch (event->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        if (resp && event->data_len > 0)
        {
            int copy_len = event->data_len;
            if (resp->data_offset + copy_len >= resp->buffer_len)
                copy_len = resp->buffer_len - resp->data_offset - 1;

            if (copy_len > 0)
            {
                memcpy(resp->buffer + resp->data_offset, event->data, copy_len);
                resp->data_offset += copy_len;
                resp->buffer[resp->data_offset] = '\0';
            }
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

// Function to handle HTTP request
esp_err_t HttpRequest::Request(const std::string &url, esp_http_client_method_t method, const char *post_data, char *response_buf, int response_buf_len)
{
    // Prepare HTTP response structure
    http_response_t response = {
        .buffer = response_buf,
        .buffer_len = response_buf_len,
        .data_offset = 0,
    };

    // Configure HTTP client
    esp_http_client_config_t config = {};
    std::string full_url = GEEKROS_SERVICE + url;
    config.url = full_url.c_str();
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.event_handler = [](esp_http_client_event_t *event)
    {
        return HttpRequest::Instance().EventHandler(event);
    };
    config.user_data = &response;
    config.timeout_ms = 30000;

    // Initialize HTTP client
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, method);

    // get system chip ID
    std::string chip_id = SystemBasic::Instance().GetChipID();

    // Get current time
    int64_t ts = SystemTime::Instance().GetUnixTimestamp();

    // Set standard headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Content-X-Source", "hardware");
    esp_http_client_set_header(client, "Content-X-Device", chip_id.c_str());
    esp_http_client_set_header(client, "Content-X-Time", std::to_string(ts).c_str());
    esp_http_client_set_header(client, "Authorization", "Bearer " GEEKROS_SERVICE_GRK);

    // Set headers
    if (method == HTTP_METHOD_POST && post_data && post_data[0] != '\0')
    {
        // Set POST data
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
    }

    // Perform the HTTP request
    esp_err_t err = esp_http_client_perform(client);

    // Cleanup HTTP client
    esp_http_client_cleanup(client);

    // Return the result
    return err;
}