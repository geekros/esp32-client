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
#include "wifi_connect.h"

// Define log tag
#define TAG "[client:wifi:connect]"

// Web page content buffer
static char *web_page_content = NULL;

// Current WiFi credentials
static char current_ssid[32];
static char current_password[64];

// WiFi event group
static EventGroupHandle_t wifi_event_group;

// Function to get web page buffer
static char *web_page_buffer(void)
{
    // Configure SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_BASE_PATH,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    // Initialize SPIFFS
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    // Check if SPIFFS is mounted
    struct stat st;

    // Verify HTML path exists
    if (stat(SPIFFS_HTML_FILE_PATH, &st))
    {
        // Return NULL if path does not exist
        return NULL;
    }

    // Allocate buffer for file content
    char *buffer = (char *)malloc(st.st_size + 1);
    memset(buffer, 0, st.st_size + 1);

    // Open index.html file
    FILE *file = fopen(SPIFFS_HTML_FILE_PATH, "r");
    if (file)
    {
        // Read file content into buffer
        if (fread(buffer, st.st_size, 1, file) == 0)
        {
            // Close file and free buffer on read error
            free(buffer);
            buffer = NULL;
        }

        // Close file
        fclose(file);
    }
    else
    {
        // Free buffer if file open fails
        free(buffer);
        buffer = NULL;
    }

    // Return file content buffer
    return buffer;
}

// WiFi connect task
static void wifi_connect_task(void *param)
{
    // Declare event bits variable
    EventBits_t bits;

    // While loop
    while (1)
    {
        // Wait for WiFi connect scan done bit
        bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECT_SCAN_DONE_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(10 * 1000));

        // Check if WiFi connect scan done bit is set
        if (bits & WIFI_CONNECT_SCAN_DONE_BIT)
        {
            // Stop server
            server_stop();

            // Connect to WiFi with current credentials
            wifi_manage_connect(current_ssid, current_password);
        }
    }

    // Delete task
    // vTaskDelete(NULL);
}

// Function to initialize AP WiFi mode
void wifi_connect_init(p_wifi_state_change_callback callback)
{
    // Get device hostname
    char hostname[32] = {0};
    get_hostname(hostname, sizeof(hostname));

    // Initialize WiFi management with hostname and callback
    wifi_manage_init(hostname, callback);

    // Load web page content
    web_page_content = web_page_buffer();

    // Create WiFi event group
    wifi_event_group = xEventGroupCreate();

    // Create WiFi connect AP task
    xTaskCreatePinnedToCore(wifi_connect_task, "wifi_connect_task", 4096, NULL, 3, NULL, 1);
}

// WiFi scan callback function
void wifi_scan_handle(int num_networks, wifi_ap_record_t *ap_records)
{
    // Create JSON object
    cJSON *root = cJSON_CreateObject();

    // Add WiFi list array to JSON object
    cJSON *wifi_list = cJSON_AddArrayToObject(root, "wifi_list");

    // Populate WiFi list array with scanned networks
    for (int i = 0; i < num_networks; i++)
    {
        // Create WiFi item object
        cJSON *wifi_item = cJSON_CreateObject();

        // Add SSID and RSSI to WiFi item
        cJSON_AddStringToObject(wifi_item, "ssid", (const char *)ap_records[i].ssid);
        cJSON_AddNumberToObject(wifi_item, "rssi", ap_records[i].rssi);
        // Determine if network is secure
        if (ap_records[i].authmode == WIFI_AUTH_OPEN)
        {
            // Add secure field as false for open networks
            cJSON_AddBoolToObject(wifi_item, "secure", false);
        }
        else
        {
            // Add secure field as true for secured networks
            cJSON_AddBoolToObject(wifi_item, "secure", true);
        }

        // Add WiFi item to WiFi list array
        cJSON_AddItemToArray(wifi_list, wifi_item);
    }

    // Convert JSON object to string
    char *json_str = cJSON_Print(root);

    // Log JSON string
    ESP_LOGI(TAG, "WiFi Scan Result: %s", json_str);

    // Send JSON string through server
    server_send((uint8_t *)json_str, strlen(json_str));

    // Free JSON string and object
    cJSON_free(json_str);

    // Free root JSON object
    cJSON_Delete(root);
}

// Server receive callback function
static void server_receive_handle(uint8_t *payload, int len)
{
    // Parse received payload as JSON
    cJSON *jsonRoot = cJSON_Parse((const char *)payload);
    if (jsonRoot)
    {
        // Extract scan item
        cJSON *scan_item = cJSON_GetObjectItem(jsonRoot, "scan");
        // Extract ssid item
        cJSON *ssid_item = cJSON_GetObjectItem(jsonRoot, "ssid");
        // Extract password item
        cJSON *password_item = cJSON_GetObjectItem(jsonRoot, "password");

        // Handle scan request
        if (scan_item)
        {
            // Handle scan request
            char *scan_value = cJSON_GetStringValue(scan_item);
            // Check if scan value is "start"
            if (strcmp(scan_value, "start") == 0)
            {
                // Perform WiFi scan
                wifi_manage_scan(wifi_scan_handle);
            }
        }

        // Handle WiFi connection request
        if (ssid_item && password_item)
        {
            // Handle WiFi connection request
            char *ssid_value = cJSON_GetStringValue(ssid_item);
            char *password_value = cJSON_GetStringValue(password_item);

            // Save current credentials
            snprintf(current_ssid, sizeof(current_ssid), "%s", ssid_value);
            snprintf(current_password, sizeof(current_password), "%s", password_value);

            // Set WiFi connect scan done bit
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECT_SCAN_DONE_BIT);
        }
    }
}

// Function to configure AP WiFi network
void wifi_network_configure(void)
{
    // Get device hostname
    char hostname[32] = {0};
    get_hostname(hostname, sizeof(hostname));

    // Configure AP WiFi with hostname
    wifi_manage_ap(hostname);

    // Prepare server configuration
    server_config_t server_cfg = {
        .http_html = web_page_content,
        .receive_callback = server_receive_handle,
    };

    // Start server
    server_start(&server_cfg);
}