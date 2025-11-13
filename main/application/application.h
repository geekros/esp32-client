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

#ifndef APPLICATION_H
#define APPLICATION_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include project-specific headers
#include "client_config.h"

// Include components headers
#include "board_basic.h"
#include "system_basic.h"

// Application main function
void application_main(void);

// Application loop function
void application_loop(void);

#endif