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

# Define source files and include directories
set(SOURCES
    "main.c"
    "board/board.c"
    "button/button.c"
    "device/host_name.c"
    "wifi/wifi_connect.c"
    "wifi/wifi_manage.c"
    "server/server.c"
)

# Define include directories
set(INCLUDE_DIRS
    "."
    "config"
    "board"
    "button"
    "device"
    "wifi"
    "server"
)

# Register the main component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)

# Create SPIFFS partition image
spiffs_create_partition_image(html ../html FLASH_IN_PROJECT)