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

#ifndef PEER_TYPE_MODULE_H
#define PEER_TYPE_MODULE_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Define peer error codes
typedef enum
{
    ESP_PEER_ERR_NONE = 0,          /*!< None error */
    ESP_PEER_ERR_INVALID_ARG = -1,  /*!< Invalid argument */
    ESP_PEER_ERR_NO_MEM = -2,       /*!< Not enough memory */
    ESP_PEER_ERR_WRONG_STATE = -3,  /*!< Operate on wrong state */
    ESP_PEER_ERR_NOT_SUPPORT = -4,  /*!< Not supported operation */
    ESP_PEER_ERR_NOT_EXISTS = -5,   /*!< Not existed */
    ESP_PEER_ERR_FAIL = -6,         /*!< General error code */
    ESP_PEER_ERR_OVER_LIMITED = -7, /*!< Overlimited */
    ESP_PEER_ERR_BAD_DATA = -8,     /*!< Bad input data */
    ESP_PEER_ERR_WOULD_BLOCK = -9,  /*!< Not enough buffer for output packet, need sleep and retry later */
} esp_peer_err_t;

// Define ICE server configuration structure
typedef struct
{
    char *stun_url; /*!< STUN/Relay server URL */
    char *user;     /*!< User name */
    char *psw;      /*!< User password */
} esp_peer_ice_server_cfg_t;

// Define peer role types
typedef enum
{
    ESP_PEER_ROLE_CONTROLLING, /*!< Controlling role who initialize the connection */
    ESP_PEER_ROLE_CONTROLLED,  /*!< Controlled role */
} esp_peer_role_t;

#endif