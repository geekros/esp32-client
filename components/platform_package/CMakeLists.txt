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
    "src/dtls_srtp_module.cc"
    "src/media_module.cc"
    "src/peer_module.cc"
)

# Define include directories
set(INCLUDE_DIRS
    "include"
)

# Register the main component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES driver mbedtls espressif__esp_libsrtp
)

# Get the base directory name
get_filename_component(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Add the prebuilt library
add_prebuilt_library(${BASE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/${IDF_TARGET}/libpeer_default.a"
    PRIV_REQUIRES ${BASE_DIR} esp_timer
)

# Link the prebuilt library
target_link_libraries(${COMPONENT_LIB}
    PRIVATE "-L${CMAKE_CURRENT_SOURCE_DIR}/libs/${IDF_TARGET}"
)

# Link the prebuilt library
target_link_libraries(${COMPONENT_LIB}
    PRIVATE ${BASE_DIR}
)