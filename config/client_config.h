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

#ifndef GEEKROS_CLIENT_CONFIG_H
#define GEEKROS_CLIENT_CONFIG_H

// Define the firmware version
#define GEEKROS_VERSION "0.0.1"

#define GEEKROS_BOARD_BRAND "waveshare"
#define GEEKROS_BOARD_NAME "esp32s3_touch_lcd_3.5c"

// Define the geekros configuration
#define GEEKROS_SERVICE "https://platform.geekros.com"
#define GEEKROS_SERVICE_GRK "grk-DxGZLzQkgRdLNwmYbDxqE9KZjAPpeacWTM3J72ApyXze2Wvn1r5a37J86BGMqwkW"
#define GEEKROS_SERVICE_PROJECT_TOKEN "nJ3wk2e8A1BKMX4q3S1oqyWQvGz5br69"

// Define NVS namespace
#define GEEKROS_NVS_NAMESPACE "geekros"
#define GEEKROS_NVS_HOSTNAME "hostname"

#define GEEKROS_WIFI_AP_PASSWORD "geekros.com"
#define GEEKROS_WIFI_AP_CHANNEL 5
#define GEEKROS_WIFI_AP_MAX_CONNECTION 5
#define GEEKROS_WIFI_AP_IP "192.168.100.1"
#define GEEKROS_WIFI_AP_GATEWAY "192.168.100.1"
#define GEEKROS_WIFI_AP_NETMASK "255.255.255.0"

// Define SPIFFS base path
#define GEEKROS_SPIFFS_LABEL "assets"
#define GEEKROS_SPIFFS_MAX_FILE 20
#define GEEKROS_SPIFFS_BASE_PATH "/spiffs"
#define GEEKROS_SPIFFS_HTML_PATH "/spiffs/html"

#endif