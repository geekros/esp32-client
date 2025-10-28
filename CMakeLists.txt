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

# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.16)

# Include the ESP-IDF project CMake file
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set default brand if not defined
if(NOT DEFINED BRAND)
    # Set default brand to sample or your preferred brand
    set(BRAND "waveshare")
endif()

# Set default board if not defined
if(NOT DEFINED BOARD)
    # Set default board to esp32s3 or your preferred board
    set(BOARD "esp32s3_touch_lcd_3.5c")
endif()

# Add board-specific include directory
include_directories(${CMAKE_SOURCE_DIR}/boards/${BRAND}/${BOARD})

# Define the project name
project(esp32-client)
