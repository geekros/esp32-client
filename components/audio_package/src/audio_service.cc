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

// constructor
AudioService::AudioService()
{
    // Create FreeRTOS event group
    event_group = xEventGroupCreate();
}

// destructor
AudioService::~AudioService()
{
    if (event_group)
    {
        vEventGroupDelete(event_group);
    }
}

// Initialize audio service with codec
void AudioService::Initialize(AudioCodec *codec_data)
{
    // Store codec reference
    codec = codec_data;
    codec->Start();

    // Initialize opus encoder and decoder
    opus_decoder = std::make_unique<OpusDecoderWrapper>(codec->OutputSampleRate(), 1, OPUS_FRAME_DURATION_MS);
    opus_encoder = std::make_unique<OpusEncoderWrapper>(16000, 1, OPUS_FRAME_DURATION_MS);
    opus_encoder->SetComplexity(0);

    // Configure resamplers if needed
    if (codec->InputSampleRate() != 16000)
    {
        input_resampler.Configure(codec->InputSampleRate(), 16000);
        reference_resampler.Configure(codec->InputSampleRate(), 16000);
    }

    // Set audio processor to AFE processor
    audio_processor = std::make_unique<AfeAudioProcessor>();

    // Set audio processor output callback to push encoded data to send queue
    audio_processor->OnOutput([this](std::vector<int16_t> &&data)
                              { PushTaskToEncodeQueue(AudioTaskTypeEncodeToSendQueue, std::move(data)); });

    // Set VAD state change callback
    audio_processor->OnVadStateChange([this](bool speaking)
                                      {
        voice_detected = speaking;
        if (callbacks.on_vad_change) {
            callbacks.on_vad_change(speaking);
        } });

    // Initialize audio power management timer
    esp_timer_create_args_t audio_power_timer_args = {
        .callback = [](void *arg)
        {
            AudioService *audio_service = (AudioService *)arg;
            audio_service->CheckAndUpdateAudioPowerState();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "audio_power_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&audio_power_timer_args, &audio_power_timer));
}

// Start audio service
void AudioService::Start()
{
    // Set service stopped flag to false
    service_stopped = false;

    // Set audio processor running event bit
    xEventGroupClearBits(event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);

    // Start audio power management timer
    esp_timer_start_periodic(audio_power_timer, 1000000);

    // Create audio input task
    xTaskCreatePinnedToCore([](void *arg)
                            {
        AudioService* audio_service = (AudioService*)arg;
        audio_service->AudioInputTask();
        vTaskDelete(NULL); }, "audio_input", 2048 * 3, this, 8, &audio_input_task_handle, 0);

    // Create audio output task
    xTaskCreate([](void *arg)
                {
        AudioService* audio_service = (AudioService*)arg;
        audio_service->AudioOutputTask();
        vTaskDelete(NULL); }, "audio_output", 2048 * 2, this, 4, &audio_output_task_handle);

    // Create opus codec task
    xTaskCreate([](void *arg)
                {
        AudioService* audio_service = (AudioService*)arg;
        audio_service->OpusCodecTask();
        vTaskDelete(NULL); }, "opus_codec", 2048 * 13, this, 2, &opus_codec_task_handle);
}

// Start audio output task
void AudioService::StartAudioTask()
{
    // Set service stopped flag to false
    service_stopped = false;

    // Set audio processor running event bit
    xEventGroupClearBits(event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);

    // Start audio power management timer
    esp_timer_start_periodic(audio_power_timer, 1000000);

    // Create audio output task
    xTaskCreate([](void *arg)
                {
        AudioService* audio_service = (AudioService*)arg;
        audio_service->AudioOutputTask();
        vTaskDelete(NULL); }, "audio_output", 2048 * 2, this, 4, &audio_output_task_handle);

    // Create opus codec task
    xTaskCreate([](void *arg)
                {
        AudioService* audio_service = (AudioService*)arg;
        audio_service->OpusCodecTask();
        vTaskDelete(NULL); }, "opus_codec", 2048 * 13, this, 2, &opus_codec_task_handle);
}

// Stop audio service
void AudioService::Stop()
{
    // Set service stopped flag to true
    esp_timer_stop(audio_power_timer);
    service_stopped = true;

    // Set audio processor not running event bit
    xEventGroupSetBits(event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);

    // lock audio queue mutex and notify all waiting tasks
    std::lock_guard<std::mutex> lock(audio_queue_mutex);

    // Clear audio queues
    audio_encode_queue.clear();
    audio_decode_queue.clear();
    audio_playback_queue.clear();
    audio_queue_cv.notify_all();
}

// Read audio data from playback queue
bool AudioService::ReadAudioData(std::vector<int16_t> &data, int sample_rate, int samples)
{
    // Check if codec input is enabled
    if (!codec->InputEnabled())
    {
        esp_timer_stop(audio_power_timer);
        esp_timer_start_periodic(audio_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
        codec->EnableInput(true);
    }

    // Resample if needed
    if (codec->InputSampleRate() != sample_rate)
    {
        // Resize data buffer and read input data
        data.resize(samples * codec->InputSampleRate() / sample_rate * codec->InputChannels());
        if (!codec->InputData(data))
        {
            return false;
        }

        // Perform resampling
        if (codec->InputChannels() == 2)
        {
            auto mic_channel = std::vector<int16_t>(data.size() / 2);
            auto reference_channel = std::vector<int16_t>(data.size() / 2);
            for (size_t i = 0, j = 0; i < mic_channel.size(); ++i, j += 2)
            {
                mic_channel[i] = data[j];
                reference_channel[i] = data[j + 1];
            }
            auto resampled_mic = std::vector<int16_t>(input_resampler.GetOutputSamples(mic_channel.size()));
            auto resampled_reference = std::vector<int16_t>(reference_resampler.GetOutputSamples(reference_channel.size()));
            input_resampler.Process(mic_channel.data(), mic_channel.size(), resampled_mic.data());
            reference_resampler.Process(reference_channel.data(), reference_channel.size(), resampled_reference.data());
            data.resize(resampled_mic.size() + resampled_reference.size());
            for (size_t i = 0, j = 0; i < resampled_mic.size(); ++i, j += 2)
            {
                data[j] = resampled_mic[i];
                data[j + 1] = resampled_reference[i];
            }
        }
        else
        {
            auto resampled = std::vector<int16_t>(input_resampler.GetOutputSamples(data.size()));
            input_resampler.Process(data.data(), data.size(), resampled.data());
            data = std::move(resampled);
        }
    }
    else
    {
        // Read input data directly
        data.resize(samples * codec->InputChannels());
        if (!codec->InputData(data))
        {
            return false;
        }
    }

    // Update last input time
    last_input_time = std::chrono::steady_clock::now();

    // Return true on success
    return true;
}

// Audio input task
void AudioService::AudioInputTask()
{
    // Dummy implementation
    while (true)
    {
        EventBits_t bits = xEventGroupWaitBits(event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING, pdFALSE, pdFALSE, portMAX_DELAY);
        if (service_stopped)
        {
            break;
        }
        if (audio_input_need_warmup)
        {
            audio_input_need_warmup = false;
            vTaskDelay(pdMS_TO_TICKS(120));
            continue;
        }

        // Process audio if processor is running
        if (bits & AS_EVENT_AUDIO_PROCESSOR_RUNNING)
        {
            std::vector<int16_t> data;
            int samples = audio_processor->GetFeedSize();
            if (samples > 0)
            {
                // Read audio data
                if (ReadAudioData(data, 16000, samples))
                {
                    // Feed data to audio processor
                    audio_processor->Feed(std::move(data));

                    // Delay according to frame duration
                    int frame_ms = samples * 1000 / 16000;
                    if (frame_ms > 0)
                    {
                        vTaskDelay(pdMS_TO_TICKS(frame_ms));
                    }

                    continue;
                }
            }
            vTaskDelay(1);
            continue;
        }

        vTaskDelay(10);
        continue;
    }
}

// Audio output task
void AudioService::AudioOutputTask()
{
    // Dummy implementation
    while (true)
    {
        // Lock audio queue mutex
        std::unique_lock<std::mutex> lock(audio_queue_mutex);

        // Wait for playback queue to have data or service to stop
        audio_queue_cv.wait(lock, [this]()
                            { return !audio_playback_queue.empty() || service_stopped; });

        if (service_stopped)
        {
            break;
        }

        // Pop audio task from playback queue
        auto task = std::move(audio_playback_queue.front());
        audio_playback_queue.pop_front();
        audio_queue_cv.notify_all();
        lock.unlock();

        // Output audio data using codec
        if (!codec->OutputEnabled())
        {
            esp_timer_stop(audio_power_timer);
            esp_timer_start_periodic(audio_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
            codec->EnableOutput(true);
        }
        codec->OutputData(task->pcm);

        // Update last output time
        last_output_time = std::chrono::steady_clock::now();
    }
}

// Opus codec task
void AudioService::OpusCodecTask()
{
    // Dummy implementation
    while (true)
    {
        // Lock audio queue mutex
        std::unique_lock<std::mutex> lock(audio_queue_mutex);

        audio_queue_cv.wait(lock, [this]()
                            { return service_stopped ||
                                     (!audio_encode_queue.empty() && audio_send_queue.size() < MAX_SEND_PACKETS_IN_QUEUE) ||
                                     (!audio_decode_queue.empty() && audio_playback_queue.size() < MAX_PLAYBACK_TASKS_IN_QUEUE); });
        // Check for service stopped
        if (service_stopped)
        {
            break;
        }

        // If there are tasks to encode
        if (!audio_decode_queue.empty() && audio_playback_queue.size() < MAX_PLAYBACK_TASKS_IN_QUEUE)
        {
            // Pop packet from decode queue
            auto packet = std::move(audio_decode_queue.front());
            audio_decode_queue.pop_front();
            audio_queue_cv.notify_all();
            lock.unlock();

            // Create audio task for playback
            auto task = std::make_unique<AudioTask>();
            task->type = AudioTaskTypeDecodeToPlaybackQueue;
            task->timestamp = packet->timestamp;

            // Set decode sample rate if needed
            SetDecodeSampleRate(packet->sample_rate, packet->frame_duration);
            if (opus_decoder->Decode(std::move(packet->payload), task->pcm))
            {
                // If resampling is needed
                if (opus_decoder->SampleRate() != codec->OutputSampleRate())
                {
                    int target_size = output_resampler.GetOutputSamples(task->pcm.size());
                    std::vector<int16_t> resampled(target_size);
                    output_resampler.Process(task->pcm.data(), task->pcm.size(), resampled.data());
                    task->pcm = std::move(resampled);
                }

                // Push task to playback queue
                lock.lock();
                audio_playback_queue.push_back(std::move(task));
                audio_queue_cv.notify_all();
            }
            else
            {
                lock.lock();
            }
        }

        // If there are tasks to decode
        if (!audio_encode_queue.empty() && audio_send_queue.size() < MAX_SEND_PACKETS_IN_QUEUE)
        {
            // Pop task from encode queue
            auto task = std::move(audio_encode_queue.front());
            audio_encode_queue.pop_front();
            audio_queue_cv.notify_all();
            lock.unlock();

            // Create audio stream packet for sending
            auto packet = std::make_unique<AudioStreamPacket>();
            packet->frame_duration = OPUS_FRAME_DURATION_MS;
            packet->sample_rate = 16000;
            packet->timestamp = task->timestamp;
            if (!opus_encoder->Encode(std::move(task->pcm), packet->payload))
            {
                continue;
            }

            // Push packet to send queue
            if (task->type == AudioTaskTypeEncodeToSendQueue)
            {
                {
                    std::lock_guard<std::mutex> lock(audio_queue_mutex);
                    audio_send_queue.push_back(std::move(packet));
                }
                if (callbacks.on_send_queue_available)
                {
                    callbacks.on_send_queue_available();
                }
            }

            lock.lock();
        }
    }
}

// Set decode sample rate and frame duration
void AudioService::SetDecodeSampleRate(int sample_rate, int frame_duration)
{
    if (opus_decoder->SampleRate() == sample_rate && opus_decoder->DurationMS() == frame_duration)
    {
        return;
    }

    opus_decoder.reset();
    opus_decoder = std::make_unique<OpusDecoderWrapper>(sample_rate, 1, frame_duration);

    // Reconfigure output resampler
    if (codec)
    {
        if (opus_decoder->SampleRate() != codec->OutputSampleRate())
        {
            output_resampler.Configure(opus_decoder->SampleRate(), codec->OutputSampleRate());
        }
    }
}

// Push task to encode queue
void AudioService::PushTaskToEncodeQueue(AudioTaskType type, std::vector<int16_t> &&pcm)
{
    auto task = std::make_unique<AudioTask>();
    task->type = type;
    task->pcm = std::move(pcm);

    // Lock audio queue mutex
    std::unique_lock<std::mutex> lock(audio_queue_mutex);

    // If the encode queue is full
    if (type == AudioTaskTypeEncodeToSendQueue && !timestamp_queue.empty())
    {
        // Assign timestamp if available
        if (timestamp_queue.size() <= MAX_TIMESTAMPS_IN_QUEUE)
        {
            task->timestamp = timestamp_queue.front();
        }

        // Pop used timestamp
        timestamp_queue.pop_front();
    }

    audio_queue_cv.wait(lock, [this]()
                        { return audio_encode_queue.size() < MAX_ENCODE_TASKS_IN_QUEUE; });

    // Push task to encode queue
    audio_encode_queue.push_back(std::move(task));

    // Notify waiting tasks
    audio_queue_cv.notify_all();
}

// Push packet to decode queue
bool AudioService::PushPacketToDecodeQueue(std::unique_ptr<AudioStreamPacket> packet, bool wait)
{
    // Lock audio queue mutex
    std::unique_lock<std::mutex> lock(audio_queue_mutex);

    // If audio decode queue is full
    if (audio_decode_queue.size() >= MAX_DECODE_PACKETS_IN_QUEUE)
    {
        if (wait)
        {
            audio_queue_cv.wait(lock, [this]()
                                { return audio_decode_queue.size() < MAX_DECODE_PACKETS_IN_QUEUE; });
        }
        else
        {
            return false;
        }
    }

    // Push packet to decode queue
    audio_decode_queue.push_back(std::move(packet));

    // Notify waiting tasks
    audio_queue_cv.notify_all();

    // Return true on success
    return true;
}

// Pop packet from send queue
std::unique_ptr<AudioStreamPacket> AudioService::PopPacketFromSendQueue()
{
    // Lock audio queue mutex
    std::lock_guard<std::mutex> lock(audio_queue_mutex);

    // If send queue is empty, return nullptr
    if (audio_send_queue.empty())
    {
        return nullptr;
    }

    // Pop packet from send queue
    auto packet = std::move(audio_send_queue.front());

    // Remove packet from send queue
    audio_send_queue.pop_front();

    // Notify waiting tasks
    audio_queue_cv.notify_all();

    // Return the packet
    return packet;
}

// Enable or disable voice processing
void AudioService::EnableVoiceProcessing(bool enable)
{
    if (enable)
    {
        if (!audio_processor_initialized)
        {
            audio_processor->Initialize(codec, OPUS_FRAME_DURATION_MS, models_list);
            audio_processor_initialized = true;
        }

        ResetDecoder();
        audio_input_need_warmup = true;
        audio_processor->Start();
        xEventGroupSetBits(event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    }
    else
    {
        audio_processor->Stop();
        xEventGroupClearBits(event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    }
}

// Set audio callbacks
void AudioService::SetCallbacks(AudioCallbacks &cb)
{
    callbacks = cb;
}

// Play sound from OGG data
void AudioService::PlaySound(const std::string_view &ogg)
{
    // If codec output is not enabled, enable it
    if (!codec->OutputEnabled())
    {
        esp_timer_stop(audio_power_timer);
        esp_timer_start_periodic(audio_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
        codec->EnableOutput(true);
    }

    // Parse OGG data and play sound
    const uint8_t *buf = reinterpret_cast<const uint8_t *>(ogg.data());
    size_t size = ogg.size();
    size_t offset = 0;

    // Lambda function to find next OGG page
    auto find_page = [&](size_t start) -> size_t
    {
        for (size_t i = start; i + 4 <= size; ++i)
        {
            if (buf[i] == 'O' && buf[i + 1] == 'g' && buf[i + 2] == 'g' && buf[i + 3] == 'S')
                return i;
        }
        return static_cast<size_t>(-1);
    };

    // OGG decoding state variables
    bool seen_head = false;
    bool seen_tags = false;
    int sample_rate = 16000;

    // Loop through OGG pages
    while (true)
    {
        size_t pos = find_page(offset);
        if (pos == static_cast<size_t>(-1))
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
                if (pkt_len >= 19 && std::memcmp(pkt_ptr, "OpusHead", 8) == 0)
                {
                    seen_head = true;

                    if (pkt_len >= 12)
                    {
                        // uint8_t version = pkt_ptr[8];
                        // uint8_t channel_count = pkt_ptr[9];

                        if (pkt_len >= 16)
                        {
                            sample_rate = pkt_ptr[12] | (pkt_ptr[13] << 8) | (pkt_ptr[14] << 16) | (pkt_ptr[15] << 24);
                            // ESP_LOGI(TAG, "version=%d, channels=%d, sample_rate=%d", version, channel_count, sample_rate);
                        }
                    }
                }
                continue;
            }

            if (!seen_tags)
            {
                if (pkt_len >= 8 && std::memcmp(pkt_ptr, "OpusTags", 8) == 0)
                {
                    seen_tags = true;
                }
                continue;
            }

            auto packet = std::make_unique<AudioStreamPacket>();
            packet->sample_rate = sample_rate;
            packet->frame_duration = 60;
            packet->payload.resize(pkt_len);
            std::memcpy(packet->payload.data(), pkt_ptr, pkt_len);
            PushPacketToDecodeQueue(std::move(packet), true);
        }

        offset = body_off + body_size;
    }
}

// Check if audio service is idle
bool AudioService::IsIdle()
{
    // Check if all audio queues are empty
    std::lock_guard<std::mutex> lock(audio_queue_mutex);

    // Return true if all queues are empty
    return audio_encode_queue.empty() && audio_decode_queue.empty() && audio_playback_queue.empty();
}

void AudioService::ResetDecoder()
{
    // Lock audio queue mutex
    std::lock_guard<std::mutex> lock(audio_queue_mutex);

    // Reset opus decoder and clear queues
    opus_decoder->ResetState();
    timestamp_queue.clear();
    audio_decode_queue.clear();
    audio_playback_queue.clear();
    audio_queue_cv.notify_all();
}

// Set models list for audio processor
void AudioService::SetModelsList(srmodel_list_t *models_list_data)
{
    models_list = models_list_data;
}

// Check and update audio power state
void AudioService::CheckAndUpdateAudioPowerState()
{
    auto now = std::chrono::steady_clock::now();
    auto input_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_input_time).count();
    auto output_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_output_time).count();
    if (input_elapsed > AUDIO_POWER_TIMEOUT_MS && codec->InputEnabled())
    {
        codec->EnableInput(false);
    }
    if (output_elapsed > AUDIO_POWER_TIMEOUT_MS && codec->OutputEnabled())
    {
        codec->EnableOutput(false);
    }
    if (!codec->InputEnabled() && !codec->OutputEnabled())
    {
        esp_timer_stop(audio_power_timer);
    }
}