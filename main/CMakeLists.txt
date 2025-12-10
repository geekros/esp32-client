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
set(SOURCES
    "main.cc"
    "application/application.cc"
)

# ----------------------------------------------------------------------
# Include directories
# ----------------------------------------------------------------------
set(INCLUDE_DIRS
    "."
    "application"
)

# ----------------------------------------------------------------------
# Register the component with ESP-IDF
# ----------------------------------------------------------------------
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)

# -------------------------------------------------------
# Add board-specific code AFTER project() → CONFIG ready
# -------------------------------------------------------
set(BOARDS_ROOT "${CMAKE_SOURCE_DIR}/tools/boards")

file(GLOB BRAND_DIRS "${BOARDS_ROOT}/*")

foreach(BRAND_PATH ${BRAND_DIRS})
    get_filename_component(BRAND_NAME ${BRAND_PATH} NAME)

    if(CONFIG_GEEKROS_BOARD_BRAND_${BRAND_NAME})
        file(GLOB MODEL_DIRS "${BRAND_PATH}/*")

        foreach(MODEL_PATH ${MODEL_DIRS})
            get_filename_component(MODEL_NAME ${MODEL_PATH} NAME)

            if(CONFIG_GEEKROS_BOARD_${BRAND_NAME}_${MODEL_NAME})

                set(SELECTED_DIR "${BOARDS_ROOT}/${BRAND_NAME}/${MODEL_NAME}")

                message(STATUS "GEEKROS: Selected board → ${BRAND_NAME}/${MODEL_NAME}")

                # Add include
                target_include_directories(${COMPONENT_LIB} PRIVATE ${SELECTED_DIR})

                # Add board source files
                file(GLOB BOARD_SRCS "${SELECTED_DIR}/*.cc")
                target_sources(${COMPONENT_LIB} PRIVATE ${BOARD_SRCS})

            endif()
        endforeach()
    endif()
endforeach()

# ----------------------------------------------------------------------
# SDK configuration generation
# ----------------------------------------------------------------------
set(GENERATE_SDKCONFIG_SCRIPT ${CMAKE_SOURCE_DIR}/tools/tools.py)
set(GENERATED_SDKCONFIG ${CMAKE_SOURCE_DIR}/sdkconfig)
set(SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/sdkconfig.defaults)
set(SDKCONFIG_BOARD_DEFAULTS ${CMAKE_SOURCE_DIR}/sdkconfig.defaults.${IDF_TARGET})

if(EXISTS ${GENERATE_SDKCONFIG_SCRIPT})
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
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
    add_custom_target(generate_sdkconfig
        DEPENDS ${GENERATED_SDKCONFIG}
    )
    add_dependencies(${COMPONENT_LIB} generate_sdkconfig)
    message(STATUS "GEEKROS: Configured generate_sdkconfig for ${IDF_TARGET}")
else()
    message(WARNING "GEEKROS: tools.py not found in tools/. Skipping sdkconfig generation.")
endif()

# ----------------------------------------------------------------------
# Asset html management
# ----------------------------------------------------------------------
set(ASSETS_SRC_DIR ${CMAKE_SOURCE_DIR}/tools/assets/html)
set(ASSETS_BUILD_DIR ${CMAKE_BINARY_DIR}/assets/html)

add_custom_target(copy_static_assets_html ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ASSETS_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ASSETS_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_SRC_DIR} ${ASSETS_BUILD_DIR}
    COMMENT "Copying static assets to build/assets/html..."
    VERBATIM
)

# ----------------------------------------------------------------------
# Asset locale&language management
# ----------------------------------------------------------------------
set(LANG_SRC_DIR ${CMAKE_SOURCE_DIR}/tools/assets/locale)
set(LANG_BUILD_DIR ${CMAKE_BINARY_DIR}/assets/language)

add_custom_target(create_assets_language_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LANG_BUILD_DIR}
    COMMENT "Creating build/assets/language directory..."
)

if(CONFIG_GEEKROS_SYSTEM_LANGUAGE_EN_US)
    set(SELECTED_LANG "EN_US")
elseif(CONFIG_GEEKROS_SYSTEM_LANGUAGE_ZH_CN)
    set(SELECTED_LANG "ZH_CN")
elseif(CONFIG_GEEKROS_SYSTEM_LANGUAGE_ZH_TW)
    set(SELECTED_LANG "ZH_TW")
else()
    set(SELECTED_LANG "ZH_CN")
endif()

add_custom_target(copy_static_assets_language ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${LANG_SRC_DIR}/${SELECTED_LANG} ${LANG_BUILD_DIR}
    COMMENT "Copying language assets: ${SELECTED_LANG}"
    DEPENDS create_assets_language_dir
)

# ----------------------------------------------------------------------
# Create SPIFFS partition image from the assets directory
# ----------------------------------------------------------------------
spiffs_create_partition_image(assets ${CMAKE_BINARY_DIR}/assets
    FLASH_IN_PROJECT
    DEPENDS copy_static_assets_html copy_static_assets_language
)