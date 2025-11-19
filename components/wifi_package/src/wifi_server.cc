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

// Define static server handle variable
static httpd_handle_t server_handle = NULL;

// Static handler function
static esp_err_t static_handler(httpd_req_t *req)
{

    ESP_LOGI(TAG, "Received request: %s", req->uri);

    char filepath[512];
    strlcpy(filepath, GEEKROS_SPIFFS_HTML_PATH, sizeof(filepath));
    strlcat(filepath, req->uri, sizeof(filepath));

    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    if (strstr(req->uri, ".html"))
    {
        httpd_resp_set_type(req, "text/html");
    }

    if (strstr(req->uri, ".css"))
    {
        httpd_resp_set_type(req, "text/css");
    }

    if (strstr(req->uri, ".js"))
    {
        httpd_resp_set_type(req, "application/javascript");
    }

    if (strstr(req->uri, ".json"))
    {
        httpd_resp_set_type(req, "text/html");
    }

    if (strstr(req->uri, ".png"))
    {
        httpd_resp_set_type(req, "image/png");
    }

    if (strstr(req->uri, ".jpg") || strstr(req->uri, ".jpeg"))
    {
        httpd_resp_set_type(req, "image/jpeg");
    }

    if (strstr(req->uri, ".gif"))
    {
        httpd_resp_set_type(req, "image/gif");
    }

    if (strstr(req->uri, ".svg"))
    {
        httpd_resp_set_type(req, "image/svg+xml");
    }

    if (strstr(req->uri, ".ico"))
    {
        httpd_resp_set_type(req, "image/x-icon");
    }

    httpd_resp_set_hdr(req, "Connection", "close");

    char chunk[512];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), file)) > 0)
    {
        httpd_resp_send_chunk(req, chunk, n);
    }

    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// HTTP handler function
static esp_err_t http_handler(httpd_req_t *req)
{
    if (http_html)
    {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Connection", "close");
        return httpd_resp_send(req, http_html, HTTPD_RESP_USE_STRLEN);
    }

    char filepath[512];
    strlcpy(filepath, GEEKROS_SPIFFS_HTML_PATH "/index.html", sizeof(filepath));

    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        return httpd_resp_send_404(req);
    }

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

    return ESP_OK;
}

// Scan handler function
static esp_err_t scan_handler(httpd_req_t *req)
{
    wifi_ap_record_t *list = (wifi_ap_record_t *)calloc(WIFI_CONFIG_AP_MAX_RECORDS, sizeof(wifi_ap_record_t));
    if (list == NULL)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no mem");
        return ESP_FAIL;
    }

    int count = wifi_configuration_get_ap_list(list, WIFI_CONFIG_AP_MAX_RECORDS);
    if (count < 0)
    {
        free(list);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "scan failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");

    httpd_resp_sendstr_chunk(req, "{\"aps\":[");

    for (int i = 0; i < count; i++)
    {
        char temp[256];

        char ssid_buf[33] = {0};
        memcpy(ssid_buf, list[i].ssid, sizeof(list[i].ssid));

        snprintf(temp, sizeof(temp), "{\"ssid\":\"%s\",\"rssi\":%d,\"authmode\":%d}", ssid_buf, list[i].rssi, list[i].authmode);

        httpd_resp_sendstr_chunk(req, temp);

        if (i != (count - 1))
        {
            httpd_resp_sendstr_chunk(req, ",");
        }
    }

    httpd_resp_sendstr_chunk(req, "]}");
    httpd_resp_sendstr_chunk(req, NULL);

    free(list);

    return ESP_OK;
}

// Submit handler function
static esp_err_t submit_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Connection", "close");

    char buf[256];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0)
    {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
    }
    buf[len] = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (!json)
    {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    }

    cJSON *j_ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *j_pass = cJSON_GetObjectItem(json, "password");

    if (!cJSON_IsString(j_ssid) || strlen(j_ssid->valuestring) == 0)
    {
        cJSON_Delete(json);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
    }

    const char *ssid = j_ssid->valuestring;
    const char *pass = (cJSON_IsString(j_pass) ? j_pass->valuestring : "");

    ESP_LOGI(TAG, "Submit WiFi SSID=%s PASS=%s", ssid, pass);

    bool ok = wifi_configuration_connect_test(ssid, pass);

    if (ok)
    {
        wifi_configuration_save(ssid, pass);
        httpd_resp_sendstr(req, "{\"success\":true}");
        ESP_LOGI(TAG, "WiFi credentials saved.");
    }
    else
    {
        httpd_resp_sendstr(req, "{\"success\":false}");
        ESP_LOGW(TAG, "WiFi credentials invalid.");
    }

    cJSON_Delete(json);

    return ESP_OK;
}

// Reboot handler function
static esp_err_t roboot_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_sendstr(req, "{\"success\":true}");

    xTaskCreate(system_reboot, "reboot_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}

// Configuration handler function
static esp_err_t config_handler(httpd_req_t *req)
{
    wifi_configuration_t *cfg = wifi_configuration_get_instance();

    cJSON *json = cJSON_CreateObject();
    if (!json)
    {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON failed");
    }

    cJSON_AddNumberToObject(json, "max_tx_power", cfg->max_tx_power);
    cJSON_AddBoolToObject(json, "remember_bssid", cfg->remember_bssid);
    cJSON_AddBoolToObject(json, "sleep_mode", cfg->sleep_mode);

    char *out = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, out, strlen(out));
    free(out);

    return ESP_OK;
}

// Configuration submit handler function
static esp_err_t config_submit_handler(httpd_req_t *req)
{
    char buf[256];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0)
    {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid payload");
    }

    buf[len] = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (!json)
    {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    }

    wifi_configuration_t *cfg = wifi_configuration_get_instance();

    nvs_handle_t nvs;
    if (nvs_open(GEEKROS_WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK)
    {
        cJSON_Delete(json);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NVS open failed");
    }

    cJSON *j_tx = cJSON_GetObjectItem(json, "max_tx_power");
    if (cJSON_IsNumber(j_tx))
    {
        cfg->max_tx_power = j_tx->valueint;
        esp_wifi_set_max_tx_power(cfg->max_tx_power);
        nvs_set_i8(nvs, "max_tx_power", cfg->max_tx_power);
    }

    cJSON *j_rb = cJSON_GetObjectItem(json, "remember_bssid");
    if (cJSON_IsBool(j_rb))
    {
        cfg->remember_bssid = cJSON_IsTrue(j_rb);
        nvs_set_u8(nvs, "remember_bssid", cfg->remember_bssid ? 1 : 0);
    }

    cJSON *j_sleep = cJSON_GetObjectItem(json, "sleep_mode");
    if (cJSON_IsBool(j_sleep))
    {
        cfg->sleep_mode = cJSON_IsTrue(j_sleep);
        nvs_set_u8(nvs, "sleep_mode", cfg->sleep_mode ? 1 : 0);
    }

    nvs_commit(nvs);
    nvs_close(nvs);
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

// Captive portal handler function
static esp_err_t captive_handle(httpd_req_t *req)
{
    char url[128];
    uint64_t now = esp_timer_get_time();

    snprintf(url, sizeof(url), "http://" GEEKROS_WIFI_AP_IP "/?_time=%llu", (unsigned long long)now);

    // Redirect to root
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", url);
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, NULL, 0);

    // Return success
    return ESP_OK;
}

// Function to start the WiFi server
esp_err_t wifi_server_start(void)
{
    // Configure HTTP server
    httpd_config_t config_httpd = HTTPD_DEFAULT_CONFIG();
    config_httpd.uri_match_fn = httpd_uri_match_wildcard;
    config_httpd.max_uri_handlers = 100;

    // 5G Network takes longer to connect
    config_httpd.recv_wait_timeout = 15;
    config_httpd.send_wait_timeout = 15;

    // Start the HTTP server
    ESP_ERROR_CHECK(httpd_start(&server_handle, &config_httpd));

    // Register HTTP GET handler
    httpd_uri_t static_http = {};
    static_http.uri = "/static/*";
    static_http.method = HTTP_GET;
    static_http.handler = static_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &static_http));

    // Register HTTP GET handler
    httpd_uri_t images_http = {};
    images_http.uri = "/images/*";
    images_http.method = HTTP_GET;
    images_http.handler = static_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &images_http));

    // Register HTTP GET handler
    httpd_uri_t script_http = {};
    script_http.uri = "/script/*";
    script_http.method = HTTP_GET;
    script_http.handler = static_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &script_http));

    // Register HTTP GET handler
    httpd_uri_t locales_http = {};
    locales_http.uri = "/locales/*";
    locales_http.method = HTTP_GET;
    locales_http.handler = static_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &locales_http));

    httpd_uri_t scan_http = {};
    scan_http.uri = "/scan";
    scan_http.method = HTTP_GET;
    scan_http.handler = scan_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &scan_http));

    httpd_uri_t submit_http = {};
    submit_http.uri = "/submit";
    submit_http.method = HTTP_POST;
    submit_http.handler = submit_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &submit_http));

    httpd_uri_t config_http = {};
    config_http.uri = "/config";
    config_http.method = HTTP_GET;
    config_http.handler = config_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &config_http));

    httpd_uri_t config_submit_http = {};
    config_submit_http.uri = "/config/submit";
    config_submit_http.method = HTTP_POST;
    config_submit_http.handler = config_submit_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &config_submit_http));

    httpd_uri_t reboot_http = {};
    reboot_http.uri = "/reboot";
    reboot_http.method = HTTP_GET;
    reboot_http.handler = roboot_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &reboot_http));

    // Register captive portal URIs
    const char *captive_urls[] = {
        "/hotspot-detect.html",       // Apple
        "/generate204",               // Android
        "/generate_204*",             // Android
        "/mobile/status.php",         // Android
        "/check_network_status.txt",  // Windows
        "/ncsi.txt",                  // Windows
        "/connecttest.txt",           // Windows
        "/redirect",                  // Windows
        "/fwlink/",                   // Microsoft
        "/connectivity-check.html",   // Firefox
        "/success.txt",               // Various
        "/portal.html",               // Various
        "/library/test/success.html", // Apple
        "/mmtls/*",                   // Wechat\QQ
        "/wifidog/*"                  // WifiDog
    };

    // Register captive portal URIs
    for (size_t i = 0; i < sizeof(captive_urls) / sizeof(captive_urls[0]); ++i)
    {
        httpd_uri_t captive_http = {};
        captive_http.uri = captive_urls[i];
        captive_http.method = HTTP_GET;
        captive_http.handler = captive_handle;
        ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &captive_http));
    }

    // Register HTTP GET handler
    httpd_uri_t index_http = {};
    index_http.uri = "/";
    index_http.method = HTTP_GET;
    index_http.handler = http_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &index_http));

    // Return success
    return ESP_OK;
}

// Stop the server
esp_err_t wifi_server_stop(void)
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