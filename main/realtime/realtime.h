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

#ifndef REALTIME_H
#define REALTIME_H

// Include standard headers
#include <stdio.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include components headers
#include "authentication.h"
#include "signaling.h"

// Define realtime state structure
typedef struct
{
    bool started;
} relatime_t;

// Function to initialize realtime
void realtime_init(relatime_t &realtime);

// Function to start realtime connection
void realtime_connection(void);

#endif