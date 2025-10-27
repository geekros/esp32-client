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

#include "server.h"

// Define log tag
#define TAG "[client:server]"

// Define static HTTP HTML variable
static const char *http_html = NULL;

// Define static receive callback variable
static server_receive_callback_t receive_callback = NULL;

// Define static server handle variable
static httpd_handle_t server_handle = NULL;

// Define static socket id variable
static int socket_id = -1;

// HTTP GET handler function
static esp_err_t http_get_handler(httpd_req_t *req)
{
    // Check if HTML content is loaded
    if (http_html == NULL)
    {
        const char *error_msg = "Internal Server Error: html not loaded.";
        return httpd_resp_send(req, error_msg, HTTPD_RESP_USE_STRLEN);
    }
    // Return success
    return httpd_resp_send(req, http_html, HTTPD_RESP_USE_STRLEN);
}

// Socket GET handler function
static esp_err_t socket_get_handler(httpd_req_t *req)
{
    // Check if the request method is GET
    if (req->method == HTTP_GET)
    {
        // Get the socket file descriptor
        socket_id = httpd_req_to_sockfd(req);

        // Return success
        return ESP_OK;
    }

    // Define WebSocket frame variable
    httpd_ws_frame_t socket_pkt;

    // Clear the WebSocket frame variable
    memset(&socket_pkt, 0, sizeof(socket_pkt));

    // Receive WebSocket data
    esp_err_t ret = httpd_ws_recv_frame(req, &socket_pkt, 0);
    if (ret != ESP_OK)
    {
        // Return error
        return ret;
    }

    // Allocate buffer for received data
    uint8_t *buf = (uint8_t *)malloc(socket_pkt.len);
    if (buf == NULL)
    {
        // Return error
        return ESP_FAIL;
    }

    // Set the payload buffer
    socket_pkt.payload = buf;

    // Receive the WebSocket frame data
    ret = httpd_ws_recv_frame(req, &socket_pkt, socket_pkt.len);
    if (ret == ESP_OK)
    {
        // Check if the frame type is text
        if (socket_pkt.type == HTTPD_WS_TYPE_TEXT)
        {
            // Call the receive callback if set
            if (receive_callback != NULL)
            {
                ESP_LOGI(TAG, "data len: %d data message: %s", socket_pkt.len, socket_pkt.payload);
                // Invoke the callback with the received data
                receive_callback(socket_pkt.payload, socket_pkt.len);
            }
        }
    }

    // Free the allocated buffer
    free(buf);

    // Return success
    return ESP_OK;
}

// Initialize the server
esp_err_t server_start(server_config_t *config)
{
    // If config is NULL, return error
    if (config == NULL)
    {
        // Return error
        return ESP_FAIL;
    }

    // Set the receive callback
    http_html = config->http_html;

    // Set the receive callback
    receive_callback = config->receive_callback;

    // Configure HTTP server
    httpd_config_t config_httpd = HTTPD_DEFAULT_CONFIG();

    // Start the HTTP server
    ESP_ERROR_CHECK(httpd_start(&server_handle, &config_httpd));

    // Register HTTP GET handler
    httpd_uri_t uri_http = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = http_get_handler,
    };

    // Register the URI handler
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &uri_http));

    // Register socket URI handler
    httpd_uri_t uri_socket = {
        .uri = "/socket",
        .method = HTTP_GET,
        .handler = socket_get_handler,
        .is_websocket = true,
    };

    // Register the URI handler
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &uri_socket));

    // Return success
    return ESP_OK;
}

// Stop the server
esp_err_t server_stop(void)
{
    if (server_handle != NULL)
    {
        // Stop the HTTP server
        ESP_ERROR_CHECK(httpd_stop(server_handle));

        // Clear the server handle
        server_handle = NULL;
    }

    // Return success
    return ESP_OK;
}

// Send data through the server
esp_err_t server_send(uint8_t *data, int len)
{
    if (server_handle == NULL)
    {
        // Return error
        return ESP_FAIL;
    }

    // Define WebSocket frame variable
    httpd_ws_frame_t socket_pkt;

    // Clear the WebSocket frame variable
    memset(&socket_pkt, 0, sizeof(socket_pkt));

    // Set the WebSocket frame parameters
    socket_pkt.payload = data;
    socket_pkt.len = len;
    socket_pkt.type = HTTPD_WS_TYPE_TEXT;

    // Return the result of sending data
    return httpd_ws_send_data(server_handle, socket_id, &socket_pkt);
}