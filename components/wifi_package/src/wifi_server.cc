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
#include "wifi_server.h"

// Define log tag
#define TAG "[client:components:wifi:server]"

// Define static HTTP HTML variable
static const char *http_html = NULL;

// Constructor
WifiServer::WifiServer()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
WifiServer::~WifiServer()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Static file handler
esp_err_t WifiServer::StaticHandler(httpd_req_t *req)
{
    // Serve static files from SPIFFS
    char filepath[256];

    // Construct file path
    strlcpy(filepath, GEEKROS_SPIFFS_HTML_PATH, sizeof(filepath));
    strlcat(filepath, req->uri, sizeof(filepath));

    // Open file
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Set MIME type
    httpd_resp_set_type(req, UtilsBasic::GetMimeType(req->uri));

    // Send file content
    httpd_resp_set_hdr(req, "Connection", "close");

    // Read and send file in chunks
    char chunk[512];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), file)) > 0)
    {
        httpd_resp_send_chunk(req, chunk, n);
    }

    // Close file and end response
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Scan handler
esp_err_t WifiServer::ScanHandler(httpd_req_t *req)
{
    // Get WifiAccessPoint instance
    auto *this_ = static_cast<WifiAccessPoint *>(req->user_ctx);
    std::lock_guard<std::mutex> lock(this_->mutex);

    // Check 5G support
    bool support_5g = false;
#ifdef CONFIG_SOC_WIFI_SUPPORT_5G
    support_5g = true;
#endif

    // Set response headers
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");

    // Build JSON response
    httpd_resp_sendstr_chunk(req, "{\"support_5g\":");
    httpd_resp_sendstr_chunk(req, support_5g ? "true" : "false");
    httpd_resp_sendstr_chunk(req, ",\"aps\":[");
    for (int i = 0; i < this_->ap_records.size(); i++)
    {
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d, Authmode: %d", (char *)this_->ap_records[i].ssid, this_->ap_records[i].rssi, this_->ap_records[i].authmode);
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"ssid\":\"%s\",\"rssi\":%d,\"authmode\":%d}", (char *)this_->ap_records[i].ssid, this_->ap_records[i].rssi, this_->ap_records[i].authmode);
        httpd_resp_sendstr_chunk(req, buf);
        if (i < this_->ap_records.size() - 1)
        {
            httpd_resp_sendstr_chunk(req, ",");
        }
    }
    httpd_resp_sendstr_chunk(req, "]}");
    httpd_resp_sendstr_chunk(req, NULL);

    // Return success
    return ESP_OK;
}

// Submit handler
esp_err_t WifiServer::SubmitHandler(httpd_req_t *req)
{
    // Read POST data
    char *buf;
    size_t buf_len = req->content_len;

    // Limit maximum payload size
    if (buf_len > 1024)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }

    // Allocate buffer
    buf = (char *)malloc(buf_len + 1);
    if (!buf)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate memory");
        return ESP_FAIL;
    }

    // Receive data
    int ret = httpd_req_recv(req, buf, buf_len);
    if (ret <= 0)
    {
        free(buf);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive request");
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Parse JSON data
    cJSON *json = cJSON_Parse(buf);
    free(buf);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Extract SSID and password
    cJSON *ssid_item = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON *password_item = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (!cJSON_IsString(ssid_item) || (ssid_item->valuestring == NULL) || (strlen(ssid_item->valuestring) >= 33))
    {
        cJSON_Delete(json);
        httpd_resp_send(req, "{\"success\":false,\"error\":\"Invalid SSID\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    // Save credentials
    std::string ssid_str = ssid_item->valuestring;
    std::string password_str = "";
    if (cJSON_IsString(password_item) && (password_item->valuestring != NULL) && (strlen(password_item->valuestring) < 65))
    {
        password_str = password_item->valuestring;
    }

    // Connect to the Access Point
    auto *this_ = static_cast<WifiAccessPoint *>(req->user_ctx);
    if (!this_->Connect(ssid_str, password_str))
    {
        cJSON_Delete(json);
        httpd_resp_send(req, "{\"success\":false,\"error\":\"Failed to connect to the Access Point\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    // Save credentials
    this_->Save(ssid_str, password_str);

    // Delete JSON object
    cJSON_Delete(json);

    // Send success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);

    // restart the system to apply new settings
    auto reboot_task = [](void *param)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        auto *self = static_cast<WifiServer *>(param);
        if (self->server)
        {
            self->Stop();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        auto &system_reboot = SystemReboot::Instance();
        system_reboot.Reboot(param);
        vTaskDelete(NULL);
    };

    // Create reboot task
    xTaskCreate(reboot_task, "reboot_task", 4096, this_, 5, NULL);

    // Return success
    return ESP_OK;
}

// Config handler
esp_err_t WifiServer::ConfigHandler(httpd_req_t *req)
{
    // Get WifiAccessPoint instance
    auto *this_ = static_cast<WifiAccessPoint *>(req->user_ctx);

    // Build JSON response
    cJSON *json = cJSON_CreateObject();
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create JSON");
        return ESP_FAIL;
    }

    // Set response headers
    cJSON_AddNumberToObject(json, "max_tx_power", this_->max_tx_power);
    cJSON_AddBoolToObject(json, "remember_bssid", this_->remember_bssid);
    cJSON_AddBoolToObject(json, "sleep_mode", this_->sleep_mode);

    // Send JSON response
    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!json_str)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to print JSON");
        return ESP_FAIL;
    }

    // Send response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, json_str, strlen(json_str));

    // Free JSON string
    free(json_str);

    // Return success
    return ESP_OK;
}

// Config submit handler
esp_err_t WifiServer::ConfigSubmitHandler(httpd_req_t *req)
{
    // Read POST data
    char *buf;
    size_t buf_len = req->content_len;

    // Limit maximum payload size
    if (buf_len > 1024)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }

    // Allocate buffer
    buf = (char *)malloc(buf_len + 1);
    if (!buf)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate memory");
        return ESP_FAIL;
    }

    // Receive data
    int ret = httpd_req_recv(req, buf, buf_len);
    if (ret <= 0)
    {
        free(buf);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive request");
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Parse JSON data
    cJSON *json = cJSON_Parse(buf);
    free(buf);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Get WifiAccessPoint instance
    auto *this_ = static_cast<WifiAccessPoint *>(req->user_ctx);

    // Open NVS handle
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open NVS");
        return ESP_FAIL;
    }

    // Update max_tx_power
    cJSON *max_tx_power = cJSON_GetObjectItem(json, "max_tx_power");
    if (cJSON_IsNumber(max_tx_power))
    {
        this_->max_tx_power = max_tx_power->valueint;
        err = esp_wifi_set_max_tx_power(this_->max_tx_power);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set WiFi power: %d", err);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set WiFi power");
            return ESP_FAIL;
        }
        err = nvs_set_i8(nvs, "max_tx_power", this_->max_tx_power);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to save WiFi power: %d", err);
        }
    }

    // Update sleep_mode
    cJSON *remember_bssid = cJSON_GetObjectItem(json, "remember_bssid");
    if (cJSON_IsBool(remember_bssid))
    {
        this_->remember_bssid = cJSON_IsTrue(remember_bssid);
        err = nvs_set_u8(nvs, "remember_bssid", this_->remember_bssid ? 1 : 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to save remember_bssid: %d", err);
        }
    }

    // Update sleep_mode
    cJSON *sleep_mode = cJSON_GetObjectItem(json, "sleep_mode");
    if (cJSON_IsBool(sleep_mode))
    {
        this_->sleep_mode = cJSON_IsTrue(sleep_mode);
        err = nvs_set_u8(nvs, "sleep_mode", this_->sleep_mode ? 1 : 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to save sleep_mode: %d", err);
        }
    }

    // Commit changes and clean up
    err = nvs_commit(nvs);
    nvs_close(nvs);
    cJSON_Delete(json);

    // Check for errors
    if (err != ESP_OK)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
        return ESP_FAIL;
    }

    // Send success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);

    // Return success
    return ESP_OK;
}

// Captive portal handler
esp_err_t WifiServer::CaptiveHandle(httpd_req_t *req)
{
    // Redirect to the main page with a timestamp to avoid caching
    std::string url = std::string("http://") + GEEKROS_WIFI_AP_IP + "/?_time=" + std::to_string(esp_timer_get_time());

    // Send redirect response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", url.c_str());
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);

    // Return success
    return ESP_OK;
}

// Index file handler
esp_err_t WifiServer::IndexHandler(httpd_req_t *req)
{
    // Serve static HTML content if available
    if (http_html)
    {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Connection", "close");
        return httpd_resp_send(req, http_html, HTTPD_RESP_USE_STRLEN);
    }

    // Load index.html from SPIFFS
    char filepath[512];
    strlcpy(filepath, GEEKROS_SPIFFS_HTML_PATH "/index.html", sizeof(filepath));

    // Open file
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        return httpd_resp_send_404(req);
    }

    // Send file content
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");
    char chunk[512];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), file)) > 0)
    {
        httpd_resp_send_chunk(req, chunk, n);
    }
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);

    // Return success
    return ESP_OK;
}

// Start the WiFi server
void WifiServer::Start()
{
    // Initialize HTTP server with default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 50;
    config.uri_match_fn = httpd_uri_match_wildcard;
    // 5G Network takes longer to connect
    config.recv_wait_timeout = 15;
    config.send_wait_timeout = 15;

    // Start the HTTP server
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    // Register static file handler
    httpd_uri_t static_http = {};
    static_http.uri = "/static/*";
    static_http.method = HTTP_GET;
    static_http.handler = StaticHandler;
    static_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &static_http));

    // Register images file handler
    httpd_uri_t images_http = {};
    images_http.uri = "/images/*";
    images_http.method = HTTP_GET;
    images_http.handler = StaticHandler;
    images_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &images_http));

    // Register script file handler
    httpd_uri_t script_http = {};
    script_http.uri = "/script/*";
    script_http.method = HTTP_GET;
    script_http.handler = StaticHandler;
    static_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &script_http));

    httpd_uri_t locales_http = {};
    locales_http.uri = "/locales/*";
    locales_http.method = HTTP_GET;
    locales_http.handler = StaticHandler;
    locales_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &locales_http));

    // Register scan handler
    httpd_uri_t scan_http = {};
    scan_http.uri = "/scan";
    scan_http.method = HTTP_GET;
    scan_http.handler = ScanHandler;
    scan_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &scan_http));

    // Register submit handler
    httpd_uri_t submit_http = {};
    submit_http.uri = "/submit";
    submit_http.method = HTTP_POST;
    submit_http.handler = SubmitHandler;
    submit_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &submit_http));

    httpd_uri_t config_http = {};
    config_http.uri = "/config";
    config_http.method = HTTP_GET;
    config_http.handler = ConfigHandler;
    config_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_http));

    httpd_uri_t config_submit_http = {};
    config_submit_http.uri = "/config/submit";
    config_submit_http.method = HTTP_POST;
    config_submit_http.handler = ConfigSubmitHandler;
    config_submit_http.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &config_submit_http));

    // Get captive portal URLs
    size_t url_count;
    auto urls = UtilsBasic::GetCaptiveUrls(url_count);
    for (size_t i = 0; i < url_count; i++)
    {
        // Register captive portal handler for each URL
        httpd_uri_t captive_http = {};
        captive_http.uri = urls[i];
        captive_http.method = HTTP_GET;
        captive_http.handler = CaptiveHandle;
        captive_http.user_ctx = &WifiAccessPoint::Instance();
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &captive_http));
    }

    // Register index file handler
    httpd_uri_t index_html = {};
    index_html.uri = "/";
    index_html.method = HTTP_GET;
    index_html.handler = IndexHandler;
    index_html.user_ctx = &WifiAccessPoint::Instance();
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &index_html));
}

// Stop the WiFi server
void WifiServer::Stop()
{
    // Stop the HTTP server
    if (server)
    {
        httpd_stop(server);
        server = NULL;
    }

    ESP_LOGI(TAG, "WiFi server stopped");
}