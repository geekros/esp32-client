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

#ifndef BOARD_BASIC_H
#define BOARD_BASIC_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Forward declaration of audio codec structure
struct audio_codec_t;

// Define board structure
typedef struct
{
    void (*board_init)(void);
    struct audio_codec_t *(*get_audio_codec)(void);
} board_t;

// Function to get the board structure
const board_t *board(void);

#endif
