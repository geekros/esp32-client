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
    "src/language_basic.cc"
    "src/language_sound.cc"
)

# Define include directories
set(INCLUDE_DIRS
    "src"
)

# Determine selected language for embedding sound files
set(LANG_SRC_DIR "${CMAKE_SOURCE_DIR}/tools/assets/locale")

# Determine selected language
if(CONFIG_GEEKROS_SYSTEM_LANGUAGE_EN_US)
    set(SELECTED_LANG "EN_US")
elseif(CONFIG_GEEKROS_SYSTEM_LANGUAGE_ZH_CN)
    set(SELECTED_LANG "ZH_CN")
elseif(CONFIG_GEEKROS_SYSTEM_LANGUAGE_ZH_TW)
    set(SELECTED_LANG "ZH_TW")
else()
    set(SELECTED_LANG "ZH_CN")
endif()

# Gather OGG sound files for the selected language
file(GLOB LANG_OGG_FILES
    "${LANG_SRC_DIR}/${SELECTED_LANG}/*.ogg"
)

message(STATUS "GEEKROS language_package: embedding sound files from ${LANG_SRC_DIR}/${SELECTED_LANG}:")
foreach(f ${LANG_OGG_FILES})
    message(STATUS " - ${f}")
endforeach()

# Register the main component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    EMBED_FILES ${LANG_OGG_FILES}
    REQUIRES driver json
)