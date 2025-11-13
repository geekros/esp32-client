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
set(SOURCES
    "main.cc"
    "button/button.cc"
    "device/host_name.cc"
    "wifi/wifi_connect.cc"
    "wifi/wifi_manage.cc"
    "server/server.cc"
    "realtime/realtime.cc"
    "${CMAKE_SOURCE_DIR}/boards/${BRAND}/${BOARD}/${BOARD}.cc"
)

# Include directories
set(INCLUDE_DIRS
    "."
    "button"
    "device"
    "wifi"
    "server"
    "realtime"
)

# Register the component with ESP-IDF
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)

# ----------------------------------------------------------------------
# SDK configuration generation
# ----------------------------------------------------------------------
set(GENERATE_SDKCONFIG_SCRIPT ${CMAKE_SOURCE_DIR}/tools/tools.py)
set(GENERATED_SDKCONFIG ${CMAKE_SOURCE_DIR}/sdkconfig)
set(SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/sdkconfig.defaults)
set(SDKCONFIG_BOARD_DEFAULTS ${CMAKE_SOURCE_DIR}/sdkconfig.defaults.${IDF_TARGET})

# Generate sdkconfig file if the script exists
if(EXISTS ${GENERATE_SDKCONFIG_SCRIPT})
    # Find Python3 interpreter
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # Custom command to generate sdkconfig
    add_custom_command(
        OUTPUT ${GENERATED_SDKCONFIG}
        COMMAND ${CMAKE_COMMAND} -E echo "Generating sdkconfig for target: ${IDF_TARGET}"
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_SOURCE_DIR}
            ${Python3_EXECUTABLE} ${GENERATE_SDKCONFIG_SCRIPT}
                --target ${IDF_TARGET}
                --output ${GENERATED_SDKCONFIG}
                --project-root ${CMAKE_SOURCE_DIR}
        DEPENDS ${GENERATE_SDKCONFIG_SCRIPT} ${SDKCONFIG_DEFAULTS} ${SDKCONFIG_BOARD_DEFAULTS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Merging sdkconfig.defaults → sdkconfig"
        VERBATIM
    )

    # Custom target to ensure sdkconfig is generated before build
    add_custom_target(generate_sdkconfig
        DEPENDS ${GENERATED_SDKCONFIG}
    )

    # Make the component depend on the sdkconfig generation
    add_dependencies(${COMPONENT_LIB} generate_sdkconfig)

    # Status message
    message(STATUS "Configured generate_sdkconfig for ${IDF_TARGET}")
else()
    # Warning if the script is not found
    message(WARNING "tools.py not found in tools/. Skipping sdkconfig generation.")
endif()

# ----------------------------------------------------------------------
# Asset management
# ----------------------------------------------------------------------
set(ASSETS_SRC_DIR ${CMAKE_SOURCE_DIR}/assets)
set(ASSETS_BUILD_DIR ${CMAKE_BINARY_DIR}/assets)

# Custom target to create the assets build directory
add_custom_target(create_assets_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ASSETS_BUILD_DIR}
    COMMENT "Creating build/assets directory..."
)

# Custom target to copy static assets to the build directory
add_custom_target(copy_static_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_SRC_DIR} ${ASSETS_BUILD_DIR}
    COMMENT "Copying static assets to build/assets..."
    DEPENDS create_assets_dir
)

# Create SPIFFS partition image from the assets directory
spiffs_create_partition_image(assets ${ASSETS_BUILD_DIR} FLASH_IN_PROJECT)

# ----------------------------------------------------------------------
# Speech recognition model
# ----------------------------------------------------------------------
set(SR_MODELS_BIN ${CMAKE_BINARY_DIR}/srmodels/srmodels.bin)

# Check if srmodels.bin exists
if(EXISTS ${SR_MODELS_BIN})
    message(STATUS "Found srmodels.bin — will flash into 'model' partition when flashing firmware.")
else()
    message(WARNING "srmodels.bin not found — skipping model flash.")
endif()
