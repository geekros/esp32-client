# Copyright 2025 GEEKROS, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ----------------------------------------------------------------------
# Set the minimum required CMake version
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)

# ----------------------------------------------------------------------
# Board Auto Generation
# ----------------------------------------------------------------------
set(GEN_KCONFIG "${CMAKE_SOURCE_DIR}/Kconfig.board.auto")

file(WRITE ${GEN_KCONFIG}
"# ----------------------------------------------------------------------\n"
"# Board Auto Generation\n"
"# ----------------------------------------------------------------------\n\n"
)

set(BOARDS_ROOT "${CMAKE_SOURCE_DIR}/tools/boards")
file(GLOB BRAND_DIRS "${BOARDS_ROOT}/*")

# Brand choice
file(APPEND ${GEN_KCONFIG} "choice GEEKROS_BOARD_BRAND_CHOICE\n")
file(APPEND ${GEN_KCONFIG} "    prompt \"Select board brand\"\n\n")

foreach(BRAND_PATH ${BRAND_DIRS})
    get_filename_component(BRAND_NAME ${BRAND_PATH} NAME)

    file(APPEND ${GEN_KCONFIG}
        "config GEEKROS_BOARD_BRAND_${BRAND_NAME}\n"
        "    bool \"${BRAND_NAME}\"\n\n"
    )
endforeach()

file(APPEND ${GEN_KCONFIG} "endchoice\n\n")

# Model choices per brand
foreach(BRAND_PATH ${BRAND_DIRS})
    get_filename_component(BRAND_NAME ${BRAND_PATH} NAME)
    file(GLOB MODEL_DIRS "${BRAND_PATH}/*")

    file(APPEND ${GEN_KCONFIG}
        "if GEEKROS_BOARD_BRAND_${BRAND_NAME}\n"
        "choice GEEKROS_BOARD_MODEL_CHOICE_${BRAND_NAME}\n"
        "    prompt \"Select model for ${BRAND_NAME}\"\n\n"
    )

    foreach(MODEL_PATH ${MODEL_DIRS})
        get_filename_component(MODEL_NAME ${MODEL_PATH} NAME)

        file(APPEND ${GEN_KCONFIG}
            "config GEEKROS_BOARD_${BRAND_NAME}_${MODEL_NAME}\n"
            "    bool \"${MODEL_NAME}\"\n\n"
        )
    endforeach()

    file(APPEND ${GEN_KCONFIG} "endchoice\nendif\n\n")
endforeach()

# ----------------------------------------------------------------------
# Include configuration directory
# ----------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/config)

# ----------------------------------------------------------------------
# Include the ESP-IDF project CMake file
# ----------------------------------------------------------------------
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# ----------------------------------------------------------------------
# Define the project name
# ----------------------------------------------------------------------
project(esp32-client)
