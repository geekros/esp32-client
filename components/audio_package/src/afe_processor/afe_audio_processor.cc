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
#include "afe_audio_processor.h"

// Define log tag
#define TAG "[client:components:audio:afe_processor]"

// Create AFE Audio Processor
audio_processor_t *afe_audio_processor_create(void)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)calloc(1, sizeof(afe_audio_processor_t));
    if (!proc)
    {
        return NULL;
    }

    proc->base.itf = &afe_audio_processor_interface;
    proc->event_group = xEventGroupCreate();
    return &proc->base;
}

// Destroy AFE Audio Processor
void afe_audio_processor_destroy(audio_processor_t *processor)
{
    if (!processor)
    {
        return;
    }

    afe_audio_processor_t *proc = (afe_audio_processor_t *)processor;

    if (proc->afe_iface && proc->afe_data)
    {
        proc->afe_iface->destroy(proc->afe_data);
    }

    if (proc->event_group)
    {
        vEventGroupDelete(proc->event_group);
    }

    if (proc->output_buffer)
    {
        free(proc->output_buffer);
    }

    free(proc);
}

// Get feed size
size_t afe_audio_processor_get_feed_size(audio_processor_t *self)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)self;
    if (!proc->afe_iface || !proc->afe_data)
    {
        return 0;
    }

    return proc->afe_iface->get_feed_chunksize(proc->afe_data);
}

// Feed data to AFE Audio Processor
void afe_audio_processor_feed(audio_processor_t *self, int16_t *data, size_t samples)
{
    (void)samples;
    afe_audio_processor_t *proc = (afe_audio_processor_t *)self;
    if (proc->afe_iface && proc->afe_data)
    {
        proc->afe_iface->feed(proc->afe_data, data);
    }
}

// Start AFE Audio Processor
void afe_audio_processor_start(audio_processor_t *self)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)self;
    xEventGroupSetBits(proc->event_group, PROCESSOR_RUNNING);
}

// Stop AFE Audio Processor
void afe_audio_processor_stop(audio_processor_t *self)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)self;
    xEventGroupClearBits(proc->event_group, PROCESSOR_RUNNING);
    if (proc->afe_iface && proc->afe_data)
    {
        proc->afe_iface->reset_buffer(proc->afe_data);
    }

    proc->output_buffer_size = 0;
}

// Check if AFE Audio Processor is running
bool afe_audio_processor_is_running(audio_processor_t *self)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)self;
    return (xEventGroupGetBits(proc->event_group) & PROCESSOR_RUNNING) != 0;
}

// Set output callback for AFE Audio Processor
void afe_audio_processor_set_output_callback(audio_processor_t *self, audio_processor_output_cb_t cb, void *user_ctx)
{
    self->output_cb = cb;
    self->output_ctx = user_ctx;
}

// Set VAD callback for AFE Audio Processor
void afe_audio_processor_set_vad_callback(audio_processor_t *self, audio_processor_vad_cb_t cb, void *user_ctx)
{
    self->vad_cb = cb;
    self->vad_ctx = user_ctx;
}

// Enable or disable device AEC for AFE Audio Processor
void afe_audio_processor_enable_device_aec(audio_processor_t *self, bool enable)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)self;

    if (!proc->afe_iface || !proc->afe_data)
    {
        return;
    }

    if (enable)
    {
        proc->afe_iface->disable_vad(proc->afe_data);
        proc->afe_iface->enable_aec(proc->afe_data);
    }
    else
    {
        proc->afe_iface->disable_aec(proc->afe_data);
        proc->afe_iface->enable_vad(proc->afe_data);
    }
}

// Initialize AFE Audio Processor
void afe_audio_processor_initialize(audio_processor_t *self, audio_codec_t *codec, int frame_duration_ms, srmodel_list_t *models_list)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)self;
    proc->codec = codec;

    proc->frame_samples = frame_duration_ms * 16;

    srmodel_list_t *models = models_list ? models_list : esp_srmodel_init("model");
    char *ns = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);
    char *vad = esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL);

    const char *fmt = "M";
    afe_config_t *cfg = afe_config_init(fmt, NULL, AFE_TYPE_VC, AFE_MODE_HIGH_PERF);

    cfg->vad_min_noise_ms = 100;
    cfg->vad_model_name = vad;

    if (ns)
    {
        cfg->ns_init = true;
        cfg->ns_model_name = ns;
        cfg->afe_ns_mode = AFE_NS_MODE_NET;
    }

    proc->afe_iface = esp_afe_handle_from_config(cfg);
    proc->afe_data = proc->afe_iface->create_from_config(cfg);

    xTaskCreate(afe_audio_processor_task, "afe_audio_processor_task", 4096, proc, 3, &proc->task_handle);
}

// AFE Audio Processor task function
void afe_audio_processor_task(void *arg)
{
    afe_audio_processor_t *proc = (afe_audio_processor_t *)arg;
    audio_processor_t *base = &proc->base;

    size_t fetch_size = proc->afe_iface->get_fetch_chunksize(proc->afe_data);

    ESP_LOGI(TAG, "AFE Audio Processor task started, fetch size: %d samples", fetch_size);

    while (1)
    {
        xEventGroupWaitBits(proc->event_group, PROCESSOR_RUNNING, pdFALSE, pdTRUE, portMAX_DELAY);

        afe_fetch_result_t *res = proc->afe_iface->fetch_with_delay(proc->afe_data, portMAX_DELAY);
        if (!res || res->ret_value == ESP_FAIL)
        {
            continue;
        }

        if (base->vad_cb)
        {
            bool speaking = (res->vad_state == VAD_SPEECH);
            if (speaking != proc->is_speaking)
            {
                proc->is_speaking = speaking;
                base->vad_cb(speaking, base->vad_ctx);
            }
        }

        if (base->output_cb)
        {
            size_t samples = res->data_size / sizeof(int16_t);
            base->output_cb(res->data, samples, base->output_ctx);
        }
    }
}