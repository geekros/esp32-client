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

// Include the headers
#include "model_basic.h"

// Define log tag
#define TAG "[client:components:model]"

// PSRAM buffer for model data
static void *model_buffer = NULL;

// Load model from SPIFFS
srmodel_list_t *model_load_from_path(void)
{
    // set AFE_CONFIG log level to NONE to suppress logs from srmodel_load
    esp_log_level_set("AFE_CONFIG", ESP_LOG_NONE);

    // model file path
    const char *path = GEEKROS_SPIFFS_MODEL_PATH "/srmodels.bin";

    // open file
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        return NULL;
    }

    // get file size
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate PSRAM buffer
    model_buffer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!model_buffer)
    {
        fclose(fp);
        return NULL;
    }

    // read file to PSRAM
    int read_bytes = fread(model_buffer, 1, size, fp);
    fclose(fp);

    // check read result
    if (read_bytes != size)
    {
        free(model_buffer);
        model_buffer = NULL;
        return NULL;
    }

    // parse with official SR loader
    srmodel_list_t *models = srmodel_load(model_buffer);

    if (!models)
    {
        free(model_buffer);
        model_buffer = NULL;
        return NULL;
    }

    ESP_LOGI(TAG, "Model initialized successfully, number of models: %d", models->num);

    return models;
}

// Unload model
void model_unload(srmodel_list_t *models)
{
    // free model resources
    if (models)
    {
        esp_srmodel_deinit(models);
    }

    // free PSRAM buffer
    if (model_buffer)
    {
        free(model_buffer);
        model_buffer = NULL;
    }
}