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
#include "service_basic.h"

// Define log tag
#define TAG "[client:components:audio:service:basic]"

// Constructor
AudioService::AudioService()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
AudioService::~AudioService()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Initialize audio service with codec
void AudioService::Initialize(AudioCodec *codec_data)
{
    // Store codec reference
    codec = codec_data;
    codec->Start();

    // Initialize opus encoder and decoder
    opus_decoder = std::make_unique<OpusDecoderWrapper>(codec->GetOutputSampleRate(), 1, OPUS_FRAME_DURATION_MS);
    opus_encoder = std::make_unique<OpusEncoderWrapper>(16000, 1, OPUS_FRAME_DURATION_MS);
    opus_encoder->SetComplexity(0);

    // Configure resamplers if needed
    if (codec->GetInputSampleRate() != 16000)
    {
        input_resampler.Configure(codec->GetInputSampleRate(), 16000);
        reference_resampler.Configure(codec->GetInputSampleRate(), 16000);
    }

    // Set audio processor to AFE processor
    audio_processor = std::make_unique<AfeAudioProcessor>();

    // Initialize audio processor
    auto output_callback = [this](std::vector<int16_t> &&data)
    {
        PushTaskToEncodeQueue(AudioTaskTypeEncodeToSendQueue, std::move(data));
    };

    // Set audio processor output callback to push encoded data to send queue
    audio_processor->OnOutput(output_callback);

    // Initialize vad state change callback
    auto vad_state_change_callback = [this](bool speaking)
    {
        voice_detected = speaking;
        if (callbacks.on_vad_change)
        {
            callbacks.on_vad_change(speaking);
        }
    };

    // Set VAD state change callback
    audio_processor->OnVadStateChange(vad_state_change_callback);

    // Initialize audio power management timer
    esp_timer_create_args_t audio_service_power_timer_args = {
        .callback = [](void *arg)
        {
            AudioService *audio_service = (AudioService *)arg;
            audio_service->CheckAndUpdateAudioPowerState();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "audio_service_power_timer",
        .skip_unhandled_events = true,
    };

    // Create the timer
    ESP_ERROR_CHECK(esp_timer_create(&audio_service_power_timer_args, &audio_service_power_timer));
}

// Start audio service
void AudioService::Start()
{
    // Set service stopped flag to false
    service_stopped = false;

    // Start audio power management timer
    esp_timer_start_periodic(audio_service_power_timer, 1000000);

    // Define audio input task lambda
    auto audio_input_task = [](void *arg)
    {
        AudioService *audio_service = (AudioService *)arg;
        audio_service->AudioInputTask();
        vTaskDelete(nullptr);
    };

    // Create audio input task
    xTaskCreatePinnedToCore(audio_input_task, "audio_input_task", 2048 * 2, this, 8, &audio_input_task_handle, 0);

    // Define audio output task lambda
    auto audio_output_task = [](void *arg)
    {
        AudioService *audio_service = (AudioService *)arg;
        audio_service->AudioOutputTask();
        vTaskDelete(nullptr);
    };

    // Create audio output task
    xTaskCreate(audio_output_task, "audio_output_task", 2048, this, 4, &audio_output_task_handle);

    // Define opus codec task lambda
    auto audio_opus_codec_task = [](void *arg)
    {
        AudioService *audio_service = (AudioService *)arg;
        audio_service->OpusCodecTask();
        vTaskDelete(nullptr);
    };

    // Create opus codec task
    xTaskCreate(audio_opus_codec_task, "audio_opus_codec_task", 2048 * 13, this, 2, &opus_codec_task_handle);
}

// Stop audio service
void AudioService::Stop()
{
    // Set service stopped flag to true
    esp_timer_stop(audio_service_power_timer);
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
    if (!codec->GetInputEnabled())
    {
        // Disable audio power timer to permit codec input
        esp_timer_stop(audio_service_power_timer);

        // Permit codec input after stopping the timer
        esp_timer_start_periodic(audio_service_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);

        // Enable codec input
        codec->EnableInput(true);
    }

    // Resample if needed
    if (codec->GetInputSampleRate() != sample_rate)
    {
        // Resize data buffer and read input data
        data.resize(samples * codec->GetInputSampleRate() / sample_rate * codec->GetInputChannels());
        if (!codec->InputData(data))
        {
            // Return false if input failed
            return false;
        }

        // Perform resampling
        if (codec->GetInputChannels() == 2)
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
        data.resize(samples * codec->GetInputChannels());
        if (!codec->InputData(data))
        {
            // Return false if input failed
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
    // Audio input task loop
    while (true)
    {
        // Wait until audio processor is running
        EventBits_t bits = xEventGroupWaitBits(event_group, AS_EVENT_AUDIO_PROCESSOR_RUNNING, pdFALSE, pdFALSE, portMAX_DELAY);

        // Check if service is stopped
        if (service_stopped)
        {
            break;
        }

        // Check if audio input need warmup
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
                if (ReadAudioData(data, 16000, samples))
                {
                    audio_processor->Feed(std::move(data));
                    continue;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
    }
}

// Audio output task
void AudioService::AudioOutputTask()
{
    // Audio output task loop
    while (true)
    {
        // Lock audio queue mutex
        std::unique_lock<std::mutex> lock(audio_queue_mutex);

        // Initialize wait condition
        auto wait_condition = [this]()
        {
            return !audio_playback_queue.empty() || service_stopped;
        };

        // Wait for playback queue to have data or service to stop
        audio_queue_cv.wait(lock, wait_condition);

        // Check for service stopped
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
        if (!codec->GetOutputEnabled())
        {
            esp_timer_stop(audio_service_power_timer);
            esp_timer_start_periodic(audio_service_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
            codec->EnableOutput(true);
        }

        // Output audio data
        codec->OutputData(task->pcm);

        // Update last output time
        last_output_time = std::chrono::steady_clock::now();
    }
}

// Opus codec task
void AudioService::OpusCodecTask()
{
    // Opus codec task loop
    while (true)
    {
        // Lock audio queue mutex
        std::unique_lock<std::mutex> lock(audio_queue_mutex);

        // Define wait condition
        auto wait_condition = [this]()
        {
            return service_stopped || (!audio_encode_queue.empty() && audio_send_queue.size() < MAX_SEND_PACKETS_IN_QUEUE) || (!audio_decode_queue.empty() && audio_playback_queue.size() < MAX_PLAYBACK_TASKS_IN_QUEUE);
        };

        // Wait for encode/decode tasks or service to stop
        audio_queue_cv.wait(lock, wait_condition);

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
            auto task = std::make_unique<AudioServiceTask>();
            task->type = AudioTaskTypeDecodeToPlaybackQueue;
            task->timestamp = packet->timestamp;

            // Set decode sample rate if needed
            SetDecodeSampleRate(packet->sample_rate, packet->frame_duration);

            // Decode opus data
            if (opus_decoder->Decode(std::move(packet->payload), task->pcm))
            {
                // If resampling is needed
                if (opus_decoder->SampleRate() != codec->GetOutputSampleRate())
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
            auto packet = std::make_unique<AudioServiceStreamPacket>();
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
    // Check if current decoder matches requested sample rate and frame duration
    if (opus_decoder->SampleRate() == sample_rate && opus_decoder->DurationMS() == frame_duration)
    {
        // No need to reconfigure
        return;
    }

    // Recreate opus decoder with new sample rate and frame duration
    opus_decoder.reset();
    opus_decoder = std::make_unique<OpusDecoderWrapper>(sample_rate, 1, frame_duration);

    // Reconfigure output resampler
    if (codec)
    {
        // Check if resampling is needed
        if (opus_decoder->SampleRate() != codec->GetOutputSampleRate())
        {
            output_resampler.Configure(opus_decoder->SampleRate(), codec->GetOutputSampleRate());
        }
    }
}

// Push task to encode queue
void AudioService::PushTaskToEncodeQueue(AudioServiceTaskType type, std::vector<int16_t> &&pcm)
{
    auto task = std::make_unique<AudioServiceTask>();
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
            // Assign the oldest timestamp
            task->timestamp = timestamp_queue.front();
        }

        // Pop used timestamp
        timestamp_queue.pop_front();
    }

    // Initialize wait condition
    auto wait_condition = [this]()
    {
        return audio_encode_queue.size() < MAX_ENCODE_TASKS_IN_QUEUE;
    };

    // Wait until there is space in the encode queue
    audio_queue_cv.wait(lock, wait_condition);

    // Push task to encode queue
    audio_encode_queue.push_back(std::move(task));

    // Notify waiting tasks
    audio_queue_cv.notify_all();
}

// Push packet to decode queue
bool AudioService::PushPacketToDecodeQueue(std::unique_ptr<AudioServiceStreamPacket> packet, bool wait)
{
    // Lock audio queue mutex
    std::unique_lock<std::mutex> lock(audio_queue_mutex);

    // If audio decode queue is full
    if (audio_decode_queue.size() >= MAX_DECODE_PACKETS_IN_QUEUE)
    {
        if (wait)
        {
            // Initialize wait condition
            auto wait_condition = [this]()
            {
                return audio_decode_queue.size() < MAX_DECODE_PACKETS_IN_QUEUE;
            };

            // Wait until there is space in the decode queue
            audio_queue_cv.wait(lock, wait_condition);
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
std::unique_ptr<AudioServiceStreamPacket> AudioService::PopPacketFromSendQueue()
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
    // Enable or disable audio processor
    if (enable)
    {
        // Initialize audio processor if not already done
        if (!audio_processor_initialized)
        {
            audio_processor->Initialize(codec, OPUS_FRAME_DURATION_MS);
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

// Play sound from OGG data
void AudioService::PlaySound(const std::string_view &ogg)
{
    // If codec output is not enabled, enable it
    if (!codec->GetOutputEnabled())
    {
        esp_timer_stop(audio_service_power_timer);
        esp_timer_start_periodic(audio_service_power_timer, AUDIO_POWER_CHECK_INTERVAL_MS * 1000);
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
            {
                return i;
            }
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

            auto packet = std::make_unique<AudioServiceStreamPacket>();
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

// Check and update audio power state
void AudioService::CheckAndUpdateAudioPowerState()
{
    // Get current time
    auto now = std::chrono::steady_clock::now();

    // Calculate elapsed time since last input and output
    auto input_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_input_time).count();
    auto output_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_output_time).count();

    // Disable codec input if idle
    if (input_elapsed > AUDIO_POWER_TIMEOUT_MS && codec->GetInputEnabled())
    {
        // Disable codec input
        codec->EnableInput(false);
    }

    // Disable codec output if idle
    if (output_elapsed > AUDIO_POWER_TIMEOUT_MS && codec->GetOutputEnabled())
    {
        // Disable codec output
        codec->EnableOutput(false);
    }

    // Stop the timer if both input and output are disabled
    if (!codec->GetInputEnabled() && !codec->GetOutputEnabled())
    {
        // Stop the audio power timer
        esp_timer_stop(audio_service_power_timer);
    }
}

// Set audio callbacks
void AudioService::SetCallbacks(AudioServiceCallbacks &cb)
{
    // Set the callbacks
    callbacks = cb;
}