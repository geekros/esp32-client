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

#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <esp_http_server.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include cJSON header
#include "cJSON.h"

// Include client configuration header
#include "client_config.h"

// Include headers
#include "wifi_access_point.h"
#include "language_basic.h"
#include "system_reboot.h"
#include "utils_basic.h"

// WifiServer class definition
class WifiServer
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Member variables
    httpd_handle_t server = NULL;

    // Static functions
    static esp_err_t StaticHandler(httpd_req_t *req);
    static esp_err_t ScanHandler(httpd_req_t *req);
    static esp_err_t SubmitHandler(httpd_req_t *req);
    static esp_err_t ConfigHandler(httpd_req_t *req);
    static esp_err_t ConfigSubmitHandler(httpd_req_t *req);
    static esp_err_t CaptiveHandle(httpd_req_t *req);
    static esp_err_t IndexHandler(httpd_req_t *req);

public:
    // Constructor and Destructor
    WifiServer();
    ~WifiServer();

    // Get singleton instance
    static WifiServer &Instance()
    {
        static WifiServer instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    WifiServer(const WifiServer &) = delete;
    WifiServer &operator=(const WifiServer &) = delete;

    // Server functions
    void Start();
    void Stop();
};

#endif