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
#include "audio_service.h"

// Define log tag
#define TAG "[client:components:audio:service]"

// Forward declarations of internal functions
static void audio_input_task(void *arg);
static void audio_output_task(void *arg);
static void opus_codec_task(void *arg);

// Get current time in milliseconds
static int64_t audio_service_get_time_ms(void)
{
    return esp_timer_get_time() / 1000;
}

// Signal queue semaphore
static void queue_signal(audio_service_t *svc)
{
    xSemaphoreGive(svc->queue_sem);
}

// Lock queues mutex
static void lock_queues(audio_service_t *svc)
{
    xSemaphoreTake(svc->queue_mutex, portMAX_DELAY);
}

// Unlock queues mutex
static void unlock_queues(audio_service_t *svc)
{
    xSemaphoreGive(svc->queue_mutex);
}

// Push decode packet into decode queue
static bool push_decode_packet(audio_service_t *svc, audio_stream_packet_t *pkt)
{
    if (svc->decode_count >= MAX_DECODE_PACKETS_IN_QUEUE)
    {
        return false;
    }
    svc->decode_queue[svc->decode_tail] = pkt;
    svc->decode_tail = (svc->decode_tail + 1) % MAX_DECODE_PACKETS_IN_QUEUE;
    svc->decode_count++;

    return true;
}

// Pop decode packet from decode queue
static audio_stream_packet_t *pop_decode_packet(audio_service_t *svc)
{
    if (svc->decode_count == 0)
    {
        return NULL;
    }

    audio_stream_packet_t *pkt = svc->decode_queue[svc->decode_head];
    svc->decode_head = (svc->decode_head + 1) % MAX_DECODE_PACKETS_IN_QUEUE;
    svc->decode_count--;

    return pkt;
}

// Push send packet into send queue
static bool push_send_packet(audio_service_t *svc, audio_stream_packet_t *pkt)
{
    if (svc->send_count >= MAX_SEND_PACKETS_IN_QUEUE)
    {
        return false;
    }
    svc->send_queue[svc->send_tail] = pkt;
    svc->send_tail = (svc->send_tail + 1) % MAX_SEND_PACKETS_IN_QUEUE;
    svc->send_count++;

    return true;
}

// Pop send packet from send queue
static audio_stream_packet_t *pop_send_packet(audio_service_t *svc)
{
    if (svc->send_count == 0)
    {
        return NULL;
    }

    audio_stream_packet_t *pkt = svc->send_queue[svc->send_head];
    svc->send_head = (svc->send_head + 1) % MAX_SEND_PACKETS_IN_QUEUE;
    svc->send_count--;

    return pkt;
}

// Push encode task into encode queue
static bool push_encode_task(audio_service_t *svc, audio_task_t *task)
{
    if (svc->encode_count >= MAX_ENCODE_TASKS_IN_QUEUE)
    {
        return false;
    }
    svc->encode_queue[svc->encode_tail] = task;
    svc->encode_tail = (svc->encode_tail + 1) % MAX_ENCODE_TASKS_IN_QUEUE;
    svc->encode_count++;

    return true;
}

// Pop encode task from encode queue
static audio_task_t *pop_encode_task(audio_service_t *svc)
{
    if (svc->encode_count == 0)
    {
        return NULL;
    }

    audio_task_t *t = svc->encode_queue[svc->encode_head];
    svc->encode_head = (svc->encode_head + 1) % MAX_ENCODE_TASKS_IN_QUEUE;
    svc->encode_count--;

    return t;
}

// Push playback task into playback queue
static bool push_playback_task(audio_service_t *svc, audio_task_t *task)
{
    if (svc->playback_count >= MAX_PLAYBACK_TASKS_IN_QUEUE)
    {
        return false;
    }
    svc->playback_queue[svc->playback_tail] = task;
    svc->playback_tail = (svc->playback_tail + 1) % MAX_PLAYBACK_TASKS_IN_QUEUE;
    svc->playback_count++;

    return true;
}

// Pop playback task from playback queue
static audio_task_t *pop_playback_task(audio_service_t *svc)
{
    if (svc->playback_count == 0)
    {
        return NULL;
    }

    audio_task_t *t = svc->playback_queue[svc->playback_head];
    svc->playback_head = (svc->playback_head + 1) % MAX_PLAYBACK_TASKS_IN_QUEUE;
    svc->playback_count--;

    return t;
}

// Audio processor output callback (called when processed audio is ready)
static void audio_processor_output_cb(int16_t *data, size_t samples, void *user_ctx)
{
    audio_service_t *svc = (audio_service_t *)user_ctx;

    audio_task_t *task = (audio_task_t *)calloc(1, sizeof(audio_task_t));
    if (!task)
    {
        return;
    }

    task->type = AUDIO_TASK_TYPE_ENCODE_TO_SEND_QUEUE;
    task->timestamp = 0;
    task->pcm_samples = samples;
    task->pcm = (int16_t *)malloc(samples * sizeof(int16_t));
    if (!task->pcm)
    {
        free(task);
        return;
    }
    memcpy(task->pcm, data, samples * sizeof(int16_t));

    lock_queues(svc);
    bool ok = push_encode_task(svc, task);
    unlock_queues(svc);

    if (!ok)
    {
        free(task->pcm);
        free(task);
    }
    else
    {
        queue_signal(svc);
    }
}

// Audio processor VAD callback (called when voice activity is detected/changed)
static void audio_processor_vad_cb(bool speaking, void *user_ctx)
{
    audio_service_t *svc = (audio_service_t *)user_ctx;
    svc->voice_detected = speaking;
    if (svc->callbacks.on_vad_change)
    {
        svc->callbacks.on_vad_change(speaking, svc->callbacks.user_ctx);
    }
}

// Read audio data from service, resampling if necessary
static bool audio_service_read_audio_data(audio_service_t *svc, int target_sample_rate, int target_samples, int16_t **out_data, size_t *out_samples)
{
    if (!svc || !svc->codec || !out_data || !out_samples)
    {
        return false;
    }

    audio_codec_t *codec = svc->codec;

    if (!codec->input_enabled)
    {
        esp_timer_stop(svc->audio_power_timer);
        esp_timer_start_periodic(svc->audio_power_timer,
                                 AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
        if (codec->func && codec->func->EnableInput)
        {
            codec->func->EnableInput(codec, true);
        }
    }

    int input_sr = codec->input_sample_rate;
    int channels = codec->input_channels;

    if (input_sr != target_sample_rate)
    {
        size_t raw_samples = (size_t)target_samples * input_sr / target_sample_rate * channels;
        int16_t *raw = (int16_t *)malloc(raw_samples * sizeof(int16_t));
        if (!raw)
        {
            return false;
        }

        if (!codec->func || !codec->func->InputData || !codec->func->InputData(codec, raw, raw_samples))
        {
            free(raw);
            return false;
        }

        if (channels == 2)
        {
            size_t mono_count = raw_samples / 2;
            int16_t *mic_ch = (int16_t *)malloc(mono_count * sizeof(int16_t));
            int16_t *ref_ch = (int16_t *)malloc(mono_count * sizeof(int16_t));
            if (!mic_ch || !ref_ch)
            {
                free(raw);
                free(mic_ch);
                free(ref_ch);
                return false;
            }

            for (size_t i = 0, j = 0; i < mono_count; ++i, j += 2)
            {
                mic_ch[i] = raw[j];
                ref_ch[i] = raw[j + 1];
            }

            if (!svc->input_resampler_inited)
            {
                svc->input_resampler = *opus_resampler_create();
                opus_resampler_configure(&svc->input_resampler, input_sr, target_sample_rate);
                svc->input_resampler_inited = true;
            }
            if (!svc->reference_resampler_inited)
            {
                svc->reference_resampler = *opus_resampler_create();
                opus_resampler_configure(&svc->reference_resampler, input_sr, target_sample_rate);
                svc->reference_resampler_inited = true;
            }

            int out_mic_samples = opus_resampler_get_output_samples(&svc->input_resampler, mono_count);
            int out_ref_samples = opus_resampler_get_output_samples(&svc->reference_resampler, mono_count);

            int16_t *res_mic = (int16_t *)malloc(out_mic_samples * sizeof(int16_t));
            int16_t *res_ref = (int16_t *)malloc(out_ref_samples * sizeof(int16_t));
            if (!res_mic || !res_ref)
            {
                free(raw);
                free(mic_ch);
                free(ref_ch);
                free(res_mic);
                free(res_ref);
                return false;
            }

            opus_resampler_process(&svc->input_resampler, mic_ch, mono_count, res_mic);
            opus_resampler_process(&svc->reference_resampler, ref_ch, mono_count, res_ref);

            size_t total = (size_t)out_mic_samples + out_ref_samples;
            int16_t *interleaved = (int16_t *)malloc(total * sizeof(int16_t));
            if (!interleaved)
            {
                free(raw);
                free(mic_ch);
                free(ref_ch);
                free(res_mic);
                free(res_ref);
                return false;
            }

            for (size_t i = 0, j = 0; i < (size_t)out_mic_samples; ++i, j += 2)
            {
                interleaved[j] = res_mic[i];
                interleaved[j + 1] = res_ref[i];
            }

            free(raw);
            free(mic_ch);
            free(ref_ch);
            free(res_mic);
            free(res_ref);

            *out_data = interleaved;
            *out_samples = out_mic_samples * 2;
        }
        else
        {
            if (!svc->input_resampler_inited)
            {
                svc->input_resampler = *opus_resampler_create();
                opus_resampler_configure(&svc->input_resampler, input_sr, target_sample_rate);
                svc->input_resampler_inited = true;
            }
            int out_samples_int = opus_resampler_get_output_samples(&svc->input_resampler, raw_samples);
            int16_t *resampled = (int16_t *)malloc(out_samples_int * sizeof(int16_t));

            // TODO: Handle resampling failure
            if (!resampled)
            {
                free(raw);
                return false;
            }
        }
    }
    else
    {
        size_t count = (size_t)target_samples * channels;
        int16_t *buf = (int16_t *)malloc(count * sizeof(int16_t));
        if (!buf)
        {
            return false;
        }

        if (!codec->func || !codec->func->InputData || !codec->func->InputData(codec, buf, count))
        {
            free(buf);
            return false;
        }
        *out_data = buf;
        *out_samples = count;
    }

    svc->last_input_time_ms = audio_service_get_time_ms();
    svc->stats.input_count++;

    return true;
}

// Audio power timer callback
static void audio_power_timer_cb(void *arg)
{
    audio_service_t *svc = (audio_service_t *)arg;

    int64_t now = audio_service_get_time_ms();

    int64_t in_elapsed = now - svc->last_input_time_ms;
    int64_t out_elapsed = now - svc->last_output_time_ms;

    if (svc->codec->input_enabled && in_elapsed > AUDIO_POWER_TIMEOUT_MS)
    {
        if (svc->codec->func && svc->codec->func->EnableInput)
        {
            svc->codec->func->EnableInput(svc->codec, false);
        }
    }
    if (svc->codec->output_enabled && out_elapsed > AUDIO_POWER_TIMEOUT_MS)
    {
        if (svc->codec->func && svc->codec->func->EnableOutput)
        {
            svc->codec->func->EnableOutput(svc->codec, false);
        }
    }

    if (!svc->codec->input_enabled && !svc->codec->output_enabled)
    {
        esp_timer_stop(svc->audio_power_timer);
    }
}

// Create audio service
audio_service_t *audio_service_create(audio_codec_t *codec, srmodel_list_t *models_list, const audio_service_callbacks_t *cbs)
{
    if (!codec)
    {
        return NULL;
    }

    audio_service_t *svc = (audio_service_t *)calloc(1, sizeof(audio_service_t));
    if (!svc)
    {
        return NULL;
    }

    svc->codec = codec;
    svc->models_list = models_list;
    if (cbs)
    {
        svc->callbacks = *cbs;
    }

    svc->event_group = xEventGroupCreate();
    svc->queue_mutex = xSemaphoreCreateMutex();
    svc->queue_sem = xSemaphoreCreateBinary();

    svc->service_stopped = true;

    if (svc->codec->func && svc->codec->func->Start)
    {
        svc->codec->func->Start(svc->codec);
    }

    svc->opus_decoder = opus_decoder_wrapper_create(svc->codec->output_sample_rate, 1, OPUS_FRAME_DURATION_MS);
    svc->opus_encoder = opus_encoder_wrapper_create(16000, 1, OPUS_FRAME_DURATION_MS);
    opus_encoder_wrapper_set_complexity(svc->opus_encoder, 0);

    svc->audio_processor = (audio_processor_t *)afe_audio_processor_create();
    if (svc->audio_processor)
    {
        svc->audio_processor_initialized = false;
        svc->voice_detected = false;
        svc->audio_input_need_warmup = false;
    }

    esp_timer_create_args_t timer_args = {
        .callback = audio_power_timer_cb,
        .arg = svc,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "audio_power_timer",
        .skip_unhandled_events = true,
    };

    esp_timer_create(&timer_args, &svc->audio_power_timer);

    svc->last_input_time_ms = audio_service_get_time_ms();
    svc->last_output_time_ms = audio_service_get_time_ms();

    return svc;
}

// Start audio service
void audio_service_start(audio_service_t *svc)
{
    if (!svc)
    {
        return;
    }

    svc->service_stopped = false;
    xEventGroupClearBits(svc->event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);

    esp_timer_start_periodic(svc->audio_power_timer, 1000000);

    xTaskCreatePinnedToCore(audio_input_task, "audio_input", 2048 * 3, svc, 8, &svc->audio_input_task_handle, 0);

    xTaskCreate(audio_output_task, "audio_output", 2048 * 2, svc, 4, &svc->audio_output_task_handle);

    xTaskCreate(opus_codec_task, "opus_codec", 2048 * 6, svc, 2, &svc->opus_codec_task_handle);
}

// Stop audio service
void audio_service_stop(audio_service_t *svc)
{
    if (!svc)
    {
        return;
    }

    svc->service_stopped = true;
    xEventGroupClearBits(svc->event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);

    queue_signal(svc);

    if (svc->audio_input_task_handle)
    {
        vTaskDelete(svc->audio_input_task_handle);
        svc->audio_input_task_handle = NULL;
    }

    if (svc->audio_output_task_handle)
    {
        vTaskDelete(svc->audio_output_task_handle);
        svc->audio_output_task_handle = NULL;
    }

    if (svc->opus_codec_task_handle)
    {
        vTaskDelete(svc->opus_codec_task_handle);
        svc->opus_codec_task_handle = NULL;
    }

    lock_queues(svc);
    svc->encode_count = svc->decode_count = svc->playback_count = svc->send_count = 0;
    unlock_queues(svc);
}

// Enable or disable voice processing (VAD/AEC)
void audio_service_enable_voice_processing(audio_service_t *svc, bool enable)
{
    if (!svc || !svc->audio_processor)
    {
        return;
    }

    if (enable)
    {
        if (!svc->audio_processor_initialized)
        {
            svc->audio_processor->itf->initialize(svc->audio_processor, svc->codec, OPUS_FRAME_DURATION_MS, svc->models_list);
            svc->audio_processor->itf->set_output_callback(svc->audio_processor, audio_processor_output_cb, svc);
            svc->audio_processor->itf->set_vad_callback(svc->audio_processor, audio_processor_vad_cb, svc);
            svc->audio_processor_initialized = true;
        }

        if (svc->opus_decoder)
        {
            opus_decoder_wrapper_reset(svc->opus_decoder);
        }

        svc->audio_input_need_warmup = true;
        svc->audio_processor->itf->start(svc->audio_processor);
        xEventGroupSetBits(svc->event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    }
    else
    {
        svc->audio_processor->itf->stop(svc->audio_processor);
        xEventGroupClearBits(svc->event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    }
}

// Enable or disable device AEC
void audio_service_enable_device_aec(audio_service_t *svc, bool enable)
{
    if (!svc || !svc->audio_processor)
    {
        return;
    }

    if (!svc->audio_processor_initialized)
    {
        svc->audio_processor->itf->initialize(svc->audio_processor, svc->codec, OPUS_FRAME_DURATION_MS, svc->models_list);
        svc->audio_processor_initialized = true;
    }

    svc->audio_processor->itf->enable_device_aec(svc->audio_processor, enable);
}

// Check if voice is currently detected
bool audio_service_is_voice_detected(const audio_service_t *svc)
{
    return svc && svc->voice_detected;
}

// Check if audio service is idle (no pending tasks)
bool audio_service_is_idle(audio_service_t *svc)
{
    if (!svc)
    {
        return true;
    }

    bool idle;
    lock_queues(svc);
    idle = (svc->encode_count == 0 && svc->decode_count == 0 && svc->playback_count == 0 && svc->send_count == 0);
    unlock_queues(svc);

    return idle;
}

// Push packet to decode queue
bool audio_service_push_packet_to_decode_queue(audio_service_t *svc, audio_stream_packet_t *packet, bool wait)
{
    if (!svc || !packet)
    {
        return false;
    }

    bool pushed = false;

    while (1)
    {
        lock_queues(svc);
        pushed = push_decode_packet(svc, packet);
        unlock_queues(svc);

        if (pushed || !wait)
        {
            break;
        }

        xSemaphoreTake(svc->queue_sem, portMAX_DELAY);
    }

    if (pushed)
    {
        queue_signal(svc);
    }

    return pushed;
}

audio_stream_packet_t *audio_service_pop_packet_from_send_queue(audio_service_t *svc)
{
    if (!svc)
    {
        return NULL;
    }

    lock_queues(svc);
    audio_stream_packet_t *pkt = pop_send_packet(svc);
    unlock_queues(svc);

    if (pkt)
    {
        queue_signal(svc);
    }

    return pkt;
}

// Play sound from OGG data
void audio_service_play_sound(audio_service_t *svc, const uint8_t *ogg, size_t size)
{
    if (!svc || !ogg || size < 4)
    {
        return;
    }

    if (!svc->codec->output_enabled)
    {
        esp_timer_stop(svc->audio_power_timer);
        esp_timer_start_periodic(svc->audio_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
        if (svc->codec->func && svc->codec->func->EnableOutput)
        {
            svc->codec->func->EnableOutput(svc->codec, true);
        }
    }

    const uint8_t *buf = ogg;
    size_t offset = 0;

    auto find_page = [&](size_t start) -> size_t
    {
        for (size_t i = start; i + 4 <= size; ++i)
        {
            if (buf[i] == 'O' && buf[i + 1] == 'g' && buf[i + 2] == 'g' && buf[i + 3] == 'S')
            {
                return i;
            }
        }
        return (size_t)-1;
    };

    bool seen_head = false;
    bool seen_tags = false;
    int sample_rate = 16000;

    while (1)
    {
        size_t pos = find_page(offset);
        if (pos == (size_t)-1)
        {
            break;
        }

        offset = pos;
        if (offset + 27 > size)
        {
            break;
        }

        const uint8_t *page = buf + offset;
        uint8_t page_segments = page[26];
        size_t seg_table_off = offset + 27;
        if (seg_table_off + page_segments > size)
        {
            break;
        }

        size_t body_size = 0;
        for (size_t i = 0; i < page_segments; ++i)
        {
            body_size += page[27 + i];
        }

        size_t body_off = seg_table_off + page_segments;
        if (body_off + body_size > size)
        {
            break;
        }

        size_t cur = body_off;
        size_t seg_idx = 0;
        while (seg_idx < page_segments)
        {
            size_t pkt_len = 0;
            size_t pkt_start = cur;
            bool continued = false;
            do
            {
                uint8_t l = page[27 + seg_idx++];
                pkt_len += l;
                cur += l;
                continued = (l == 255);
            } while (continued && seg_idx < page_segments);

            if (pkt_len == 0)
            {
                continue;
            }

            const uint8_t *pkt_ptr = buf + pkt_start;

            if (!seen_head)
            {
                if (pkt_len >= 19 && memcmp(pkt_ptr, "OpusHead", 8) == 0)
                {
                    seen_head = true;
                    if (pkt_len >= 16)
                    {
                        sample_rate = pkt_ptr[12] | (pkt_ptr[13] << 8) | (pkt_ptr[14] << 16) | (pkt_ptr[15] << 24);
                    }
                }
                continue;
            }
            if (!seen_tags)
            {
                if (pkt_len >= 8 && memcmp(pkt_ptr, "OpusTags", 8) == 0)
                {
                    seen_tags = true;
                }
                continue;
            }

            /* Audio packet */
            audio_stream_packet_t *packet = (audio_stream_packet_t *)calloc(1, sizeof(audio_stream_packet_t));
            if (!packet)
            {
                return;
            }

            packet->sample_rate = sample_rate;
            packet->frame_duration_ms = OPUS_FRAME_DURATION_MS;
            packet->payload_len = pkt_len;
            packet->payload = (uint8_t *)malloc(pkt_len);
            if (!packet->payload)
            {
                free(packet);
                return;
            }
            memcpy(packet->payload, pkt_ptr, pkt_len);

            audio_service_push_packet_to_decode_queue(svc, packet, true);
        }

        offset = body_off + body_size;
    }
}

// Audio input task
static void audio_input_task(void *arg)
{
    audio_service_t *svc = (audio_service_t *)arg;

    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(svc->event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING, pdFALSE, pdFALSE, portMAX_DELAY);

        if (svc->service_stopped)
        {
            break;
        }

        if (svc->audio_input_need_warmup)
        {
            svc->audio_input_need_warmup = false;
            vTaskDelay(pdMS_TO_TICKS(120));
            continue;
        }

        if (bits & AS_EVENT_AUDIO_PROCESSOR_RUNNING)
        {
            if (!svc->audio_processor)
            {
                continue;
            }

            size_t feed_samples = svc->audio_processor->itf->get_feed_size(svc->audio_processor);
            if (feed_samples > 0)
            {
                int16_t *data = NULL;
                size_t samples = 0;
                if (audio_service_read_audio_data(svc, 16000, (int)feed_samples, &data, &samples))
                {
                    svc->audio_processor->itf->feed(svc->audio_processor, data, samples);
                    free(data);
                    continue;
                }
            }
        }
    }

    vTaskDelete(NULL);
}

// Audio output task
static void audio_output_task(void *arg)
{
    audio_service_t *svc = (audio_service_t *)arg;

    while (1)
    {
        if (svc->service_stopped)
        {
            break;
        }

        xSemaphoreTake(svc->queue_sem, portMAX_DELAY);

        if (svc->service_stopped)
        {
            break;
        }

        lock_queues(svc);
        audio_task_t *task = pop_playback_task(svc);
        unlock_queues(svc);

        if (!task)
        {
            continue;
        }

        if (!svc->codec->output_enabled)
        {
            esp_timer_stop(svc->audio_power_timer);
            esp_timer_start_periodic(svc->audio_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
            if (svc->codec->func && svc->codec->func->EnableOutput)
            {
                svc->codec->func->EnableOutput(svc->codec, true);
            }
        }

        if (svc->codec->func && svc->codec->func->OutputData)
        {
            svc->codec->func->OutputData(svc->codec, task->pcm, (int)task->pcm_samples);
        }

        svc->last_output_time_ms = audio_service_get_time_ms();
        svc->stats.playback_count++;

        free(task->pcm);
        free(task);
    }

    vTaskDelete(NULL);
}

// Opus codec task
static void opus_codec_task(void *arg)
{
    audio_service_t *svc = (audio_service_t *)arg;

    while (1)
    {
        if (svc->service_stopped)
        {
            break;
        }

        xSemaphoreTake(svc->queue_sem, portMAX_DELAY);

        if (svc->service_stopped)
        {
            break;
        }

        lock_queues(svc);
        audio_stream_packet_t *dec_pkt = pop_decode_packet(svc);
        unlock_queues(svc);

        if (dec_pkt)
        {
            audio_task_t *task = (audio_task_t *)calloc(1, sizeof(audio_task_t));
            if (!task)
            {
                free(dec_pkt->payload);
                free(dec_pkt);
            }
            else
            {
                task->type = AUDIO_TASK_TYPE_DECODE_TO_PLAYBACK_QUEUE;
                task->timestamp = dec_pkt->timestamp;

                int frame_samples = svc->opus_decoder->frame_size;
                int16_t *pcm = (int16_t *)malloc(frame_samples * sizeof(int16_t));
                if (!pcm)
                {
                    free(task);
                    free(dec_pkt->payload);
                    free(dec_pkt);
                }
                else
                {
                    int ret = opus_decoder_wrapper_decode(svc->opus_decoder, dec_pkt->payload, dec_pkt->payload_len, pcm, frame_samples);
                    if (ret > 0)
                    {
                        task->pcm = pcm;
                        task->pcm_samples = (size_t)ret;

                        if (svc->codec->output_sample_rate != dec_pkt->sample_rate)
                        {
                            if (!svc->output_resampler_inited)
                            {
                                svc->output_resampler = *opus_resampler_create();
                                opus_resampler_configure(&svc->output_resampler, dec_pkt->sample_rate, svc->codec->output_sample_rate);
                                svc->output_resampler_inited = true;
                            }
                            int out_samples = opus_resampler_get_output_samples(&svc->output_resampler, ret);
                            int16_t *resampled = (int16_t *)malloc(out_samples * sizeof(int16_t));
                            if (resampled && opus_resampler_process(&svc->output_resampler, pcm, ret, resampled))
                            {
                                free(task->pcm);
                                task->pcm = resampled;
                                task->pcm_samples = (size_t)out_samples;
                            }
                        }

                        lock_queues(svc);
                        if (!push_playback_task(svc, task))
                        {
                            free(task->pcm);
                            free(task);
                        }
                        else
                        {
                            svc->stats.decode_count++;
                        }
                        unlock_queues(svc);
                        queue_signal(svc);
                    }
                    else
                    {
                        free(pcm);
                        free(task);
                    }
                }

                free(dec_pkt->payload);
                free(dec_pkt);
            }
        }

        lock_queues(svc);
        audio_task_t *enc_task = pop_encode_task(svc);
        unlock_queues(svc);

        if (enc_task)
        {
            audio_stream_packet_t *packet = (audio_stream_packet_t *)calloc(1, sizeof(audio_stream_packet_t));
            if (!packet)
            {
                free(enc_task->pcm);
                free(enc_task);
            }
            else
            {
                packet->frame_duration_ms = OPUS_FRAME_DURATION_MS;
                packet->sample_rate = 16000;
                packet->timestamp = enc_task->timestamp;

                uint8_t buf[MAX_OPUS_PACKET_SIZE];
                int ret = opus_encoder_wrapper_encode_frame(svc->opus_encoder, enc_task->pcm, enc_task->pcm_samples, buf, sizeof(buf));
                if (ret > 0)
                {
                    packet->payload_len = (size_t)ret;
                    packet->payload = (uint8_t *)malloc(packet->payload_len);
                    if (packet->payload)
                    {
                        memcpy(packet->payload, buf, packet->payload_len);

                        lock_queues(svc);
                        if (!push_send_packet(svc, packet))
                        {
                            free(packet->payload);
                            free(packet);
                        }
                        else
                        {
                            svc->stats.encode_count++;
                            if (svc->callbacks.on_send_queue_available)
                            {
                                svc->callbacks.on_send_queue_available(svc->callbacks.user_ctx);
                            }
                        }
                        unlock_queues(svc);
                    }
                    else
                    {
                        free(packet);
                    }
                }
                else
                {
                    free(packet);
                }

                free(enc_task->pcm);
                free(enc_task);
            }
        }
    }

    vTaskDelete(NULL);
}

// Destroy audio service
void audio_service_destroy(audio_service_t *svc)
{
    if (!svc)
    {
        return;
    }

    audio_service_stop(svc);

    if (svc->audio_power_timer)
    {
        esp_timer_stop(svc->audio_power_timer);
        esp_timer_delete(svc->audio_power_timer);
    }

    if (svc->audio_processor)
    {
        afe_audio_processor_destroy(svc->audio_processor);
        svc->audio_processor = NULL;
    }

    if (svc->opus_encoder)
    {
        opus_encoder_wrapper_destroy(svc->opus_encoder);
    }

    if (svc->opus_decoder)
    {
        opus_decoder_wrapper_destroy(svc->opus_decoder);
    }

    if (svc->event_group)
    {
        vEventGroupDelete(svc->event_group);
    }

    if (svc->queue_mutex)
    {
        vSemaphoreDelete(svc->queue_mutex);
    }

    if (svc->queue_sem)
    {
        vSemaphoreDelete(svc->queue_sem);
    }

    free(svc);
}