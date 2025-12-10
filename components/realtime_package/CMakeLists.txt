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

# Define source files directories
set(SOURCES
    "src/auth_basic.cc"
    "src/realtime_basic.cc"
    "src/signaling_basic.cc"
    "src/webrtc_basic.cc"
)

# Define include directories
set(INCLUDE_DIRS
    "include"
)

# Register the main component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES utils_package system_package network_package esp_http_client json espressif__esp_peer
)