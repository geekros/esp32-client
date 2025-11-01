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

// Include headers
#include "audio_processor.h"

// Forward declaration for task function
static void audio_processor_task(void *arg);

// Function to create audio processor
audio_processor_t *audio_processor_create(audio_processor_mode_t mode)
{
    // Allocate memory for audio processor
    audio_processor_t *p = (audio_processor_t *)calloc(1, sizeof(audio_processor_t));
    if (!p)
    {
        return NULL;
    }

    // Create event group
    p->event_group = xEventGroupCreate();
    if (!p->event_group)
    {
        free(p);
        return NULL;
    }

    // Set mode and initialize members
    p->use_afe = (mode == AUDIO_PROC_MODE_AFE);
    p->frame_samples = 0;
    p->input_channels = 1;
    p->has_reference = false;
    p->is_speaking = false;

    return p;
}

// Function to initialize audio processor
void audio_processor_init(audio_processor_t *p, int input_channels, bool has_reference, int frame_duration_ms, srmodel_list_t *models_list)
{
    // Check for null pointer
    if (p == NULL)
    {
        return;
    }

    // Set processor parameters
    p->input_channels = input_channels;
    p->has_reference = has_reference;
    p->frame_samples = frame_duration_ms * 16000 / 1000;

    // If AFE is not used, bypass initialization
    if (!p->use_afe)
    {
        return;
    }

    // Prepare input format string
    char input_format[8] = {0};
    int idx = 0;
    int mic_num = input_channels - (has_reference ? 1 : 0);
    for (int i = 0; i < mic_num; i++)
    {
        input_format[idx++] = 'M';
    }
    if (has_reference)
    {
        input_format[idx++] = 'R';
    }
    input_format[idx] = '\0';

    // Load SR models
    srmodel_list_t *models = models_list ? models_list : esp_srmodel_init("model");

    // Filter available models (if present)
    char *ns_model_name = models ? esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL) : NULL;
    char *vad_model_name = models ? esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL) : NULL;

    // Initialize AFE configuration
    afe_config_t *afe_cfg = afe_config_init(input_format, NULL, AFE_DEFAULT_TYPE, AFE_DEFAULT_MODE);
    afe_cfg->aec_mode = AEC_MODE_VOIP_HIGH_PERF;
    afe_cfg->vad_mode = VAD_MODE_0;
    afe_cfg->vad_min_noise_ms = 100;
    afe_cfg->agc_init = false;
    afe_cfg->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    // Configure model names (if loaded)
    if (vad_model_name)
    {
        afe_cfg->vad_model_name = vad_model_name;
        afe_cfg->vad_init = true;
    }
    else
    {
        afe_cfg->vad_init = false;
    }

    if (ns_model_name)
    {
        afe_cfg->ns_init = true;
        afe_cfg->ns_model_name = ns_model_name;
        afe_cfg->afe_ns_mode = AFE_NS_MODE_NET;
    }
    else
    {
        afe_cfg->ns_init = false;
    }

#ifdef CONFIG_USE_DEVICE_AEC
    afe_cfg->aec_init = true;
#else
    afe_cfg->aec_init = false;
#endif

    // ---------------------------------------------------------
    // Create AFE interface and data instance
    // ---------------------------------------------------------
    p->afe_iface = esp_afe_handle_from_config(afe_cfg);
    if (p->afe_iface == NULL)
    {
        return;
    }

    p->afe_data = p->afe_iface->create_from_config(afe_cfg);
    if (p->afe_data == NULL)
    {
        return;
    }

    // Allocate output buffer
    p->output_buffer_cap = p->frame_samples * 2;
    p->output_buffer = (int16_t *)malloc(p->output_buffer_cap * sizeof(int16_t));
    if (p->output_buffer == NULL)
    {
        return;
    }
    p->output_buffer_len = 0;

    // Create audio processor task
    if (xTaskCreate(audio_processor_task, "audio_processor_task", 8192, p, 3, &p->task_hdl) != pdPASS)
    {

        free(p->output_buffer);
        p->output_buffer = NULL;
        return;
    }
}

// Function to start audio processor
void audio_processor_start(audio_processor_t *p)
{
    if (p == NULL)
    {
        return;
    }

    // Set running bit in event group
    xEventGroupSetBits(p->event_group, PROCESSOR_RUNNING_BIT);
}

// Function to stop audio processor
void audio_processor_stop(audio_processor_t *p)
{
    if (p == NULL)
    {
        return;
    }

    // Clear running bit in event group
    xEventGroupClearBits(p->event_group, PROCESSOR_RUNNING_BIT);

    // Reset AFE buffer
    if (p->use_afe && p->afe_data)
    {
        p->afe_iface->reset_buffer(p->afe_data);
    }
}

// Function to destroy audio processor
void audio_processor_destroy(audio_processor_t *p)
{
    if (p == NULL)
    {
        return;
    }

    // Stop task execution
    xEventGroupClearBits(p->event_group, PROCESSOR_RUNNING_BIT);

    // Wait for task to exit safely
    for (int i = 0; i < 10 && p->task_hdl; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Delete task if still valid
    if (p->task_hdl != NULL)
    {
        vTaskDelete(p->task_hdl);
        p->task_hdl = NULL;
    }

    // Destroy AFE data and free resources
    if (p->use_afe && p->afe_data && p->afe_iface)
    {
        p->afe_iface->destroy(p->afe_data);
        p->afe_data = NULL;
    }

    // Free output buffer
    if (p->output_buffer)
    {
        free(p->output_buffer);
        p->output_buffer = NULL;
    }

    // Delete event group
    if (p->event_group)
    {
        vEventGroupDelete(p->event_group);
        p->event_group = NULL;
    }

    free(p);
}

// Function to set AFE output callback
void audio_processor_set_output_callback(audio_processor_t *p, afe_output_callback_t cb, void *user_ctx)
{
    if (p == NULL)
    {
        return;
    }

    p->output_cb = cb;
    p->output_user_ctx = user_ctx;
}

// Function to set VAD callback
void audio_processor_set_vad_callback(audio_processor_t *p, afe_vad_callback_t cb, void *user_ctx)
{
    if (p == NULL)
    {
        return;
    }

    p->vad_cb = cb;
    p->vad_user_ctx = user_ctx;
}

// Function to enable or disable device AEC
void audio_processor_enable_device_aec(audio_processor_t *p, bool enable)
{
    if (!p || !p->use_afe || !p->afe_iface || !p->afe_data)
    {
        return;
    }

    if (enable)
    {
#ifdef CONFIG_USE_DEVICE_AEC
        p->afe_iface->enable_aec(p->afe_data);
        p->afe_iface->disable_vad(p->afe_data);
#endif
    }
    else
    {
        p->afe_iface->disable_aec(p->afe_data);
        p->afe_iface->enable_vad(p->afe_data);
    }
}

// Function to feed audio data to processor
void audio_processor_feed(audio_processor_t *p, const int16_t *data)
{
    if (p == NULL)
    {
        return;
    }

    if (p->use_afe)
    {
        if (p->afe_data && p->afe_iface)
        {
            p->afe_iface->feed(p->afe_data, (int16_t *)data);
        }
    }
    else
    {
        if (p->output_cb)
        {
            // Directly pass data to output callback
            p->output_cb(data, p->frame_samples, p->output_user_ctx);
        }
    }
}

// Function to get required feed size
int audio_processor_get_feed_size(audio_processor_t *p)
{
    if (!p)
    {
        return 0;
    }

    if (p->use_afe && p->afe_data && p->afe_iface)
    {
        return p->afe_iface->get_feed_chunksize(p->afe_data);
    }

    return p->frame_samples;
}

// Audio processor task function
static void audio_processor_task(void *arg)
{
    audio_processor_t *p = (audio_processor_t *)arg;

    while (1)
    {
        // Check for valid AFE interface and data
        if (!p || !p->afe_iface || !p->afe_data)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // Wait until processor is running
        xEventGroupWaitBits(p->event_group, PROCESSOR_RUNNING_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

        // Check if processor is still running
        if ((xEventGroupGetBits(p->event_group) & PROCESSOR_RUNNING_BIT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        // Fetch processed audio data with delay
        const afe_fetch_result_t *res = p->afe_iface->fetch_with_delay(p->afe_data, portMAX_DELAY);

        // Recheck running state after fetch
        if ((xEventGroupGetBits(p->event_group) & PROCESSOR_RUNNING_BIT) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        // Validate fetch result
        if (res == NULL || res->ret_value == ESP_FAIL)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        // Handle VAD state if callback registered
        if (p->vad_cb != NULL)
        {
            if (res->vad_state == VAD_SPEECH && !p->is_speaking)
            {
                p->is_speaking = true;
                p->vad_cb(true, p->vad_user_ctx);
            }
            else if (res->vad_state == VAD_SILENCE && p->is_speaking)
            {
                p->is_speaking = false;
                p->vad_cb(false, p->vad_user_ctx);
            }
        }

        // Calculate number of samples
        int samples = res->data_size / sizeof(int16_t);
        if (samples <= 0)
        {
            continue;
        }

        // Limit samples to output buffer capacity
        if (samples > p->output_buffer_cap)
        {
            samples = p->output_buffer_cap;
        }

        // Prevent buffer overflow
        if (p->output_buffer_len + samples > p->output_buffer_cap)
        {
            p->output_buffer_len = 0;
        }

        // Copy fetched audio data
        memcpy(p->output_buffer + p->output_buffer_len, res->data, samples * sizeof(int16_t));
        p->output_buffer_len += samples;

        // Deliver frames to output callback
        while (p->output_buffer_len >= p->frame_samples)
        {
            if (p->output_cb != NULL)
            {
                p->output_cb(p->output_buffer, p->frame_samples, p->output_user_ctx);
            }

            int remain = p->output_buffer_len - p->frame_samples;
            if (remain > 0)
            {
                memmove(p->output_buffer, p->output_buffer + p->frame_samples, remain * sizeof(int16_t));
            }
            p->output_buffer_len = remain;
        }
    }
}
