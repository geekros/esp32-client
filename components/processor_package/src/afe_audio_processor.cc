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
#define TAG "[client:components:processor:afe]"

// Constructor
AfeAudioProcessor::AfeAudioProcessor()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
AfeAudioProcessor::~AfeAudioProcessor()
{
    // Destroy AFE data
    if (afe_data != nullptr)
    {
        // Call destroy method
        afe_iface->destroy(afe_data);
    }

    // Delete event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
        event_group = nullptr;
    }
}

// Initialize method
void AfeAudioProcessor::Initialize(AudioCodec *codec_data, int frame_duration_ms)
{
    // Store codec and frame samples
    codec = codec_data;
    frame_samples = frame_duration_ms * 16000 / 1000;

    // Reserve output buffer size
    output_buffer.reserve(frame_samples);

    // Get reference channel number
    int ref_num = codec->GetInputReference() ? 1 : 0;

    // Create input format string
    std::string input_format;
    for (int i = 0; i < codec->GetInputChannels() - ref_num; i++)
    {
        input_format.push_back('M');
    }
    for (int i = 0; i < ref_num; i++)
    {
        input_format.push_back('R');
    }

    // Initialize AFE models
    srmodel_list_t *models = ModelBasic::Instance().Load();

    ESP_LOGI(TAG, "AFE models loaded: %d", models->num);

    // Filter models
    char *ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);
    char *vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL);

    // Initialize AFE configuration
    afe_config_t *afe_config = afe_config_init(input_format.c_str(), NULL, AFE_TYPE_VC, AFE_MODE_HIGH_PERF);
    afe_config->aec_mode = AEC_MODE_VOIP_HIGH_PERF;
    afe_config->vad_mode = VAD_MODE_0;
    afe_config->vad_min_noise_ms = 100;

    // Set model names
    if (vad_model_name != nullptr)
    {
        // Set VAD model
        afe_config->vad_model_name = vad_model_name;
    }

    // Set NS model
    if (ns_model_name != nullptr)
    {
        // Enable NS
        afe_config->ns_init = true;
        afe_config->ns_model_name = ns_model_name;
        afe_config->afe_ns_mode = AFE_NS_MODE_NET;
    }
    else
    {
        // Disable NS
        afe_config->ns_init = false;
    }

    // Disable AGC
    afe_config->agc_init = false;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

#ifdef CONFIG_USE_DEVICE_AEC
    // Enable AEC, disable VAD
    afe_config->aec_init = true;
    afe_config->vad_init = false;
#else
    // Disable AEC, enable VAD
    afe_config->aec_init = false;
    afe_config->vad_init = true;
#endif

    // Create AFE interface and data
    afe_iface = const_cast<esp_afe_sr_iface_t *>(esp_afe_handle_from_config(afe_config));
    afe_data = afe_iface->create_from_config(afe_config);

    // Create audio processing task
    auto audio_communication_task = [](void *param)
    {
        auto this_ = (AfeAudioProcessor *)param;
        this_->AudioProcessorTask();
        vTaskDelete(nullptr);
    };

    // Start audio processing task
    xTaskCreate(audio_communication_task, "audio_communication_task", 4096, this, 3, NULL);
}

// Feed audio data to AFE processor
void AfeAudioProcessor::Feed(std::vector<int16_t> &&data)
{
    // Feed data to AFE if initialized
    if (afe_data == nullptr)
    {
        return;
    }

    // Feed data
    afe_iface->feed(afe_data, data.data());
}

// Start AFE audio processor
void AfeAudioProcessor::Start()
{
    xEventGroupSetBits(event_group, PROCESSOR_RUNNING);
}

// Stop AFE audio processor
void AfeAudioProcessor::Stop()
{
    // Clear running event bit
    xEventGroupClearBits(event_group, PROCESSOR_RUNNING);

    // Reset AFE buffer
    if (afe_data != nullptr)
    {
        afe_iface->reset_buffer(afe_data);
    }
}

// Get feed size
size_t AfeAudioProcessor::GetFeedSize()
{
    // Return feed chunk size
    if (afe_data == nullptr)
    {
        // Not initialized
        return 0;
    }

    // Return feed chunk size
    return afe_iface->get_feed_chunksize(afe_data);
}

// Check if AFE audio processor is running
bool AfeAudioProcessor::IsRunning()
{
    // Check running event bit
    return xEventGroupGetBits(event_group) & PROCESSOR_RUNNING;
}

// On output callback
void AfeAudioProcessor::OnOutput(std::function<void(std::vector<int16_t> &&data)> callback)
{
    // Store output callback
    output_callback = callback;
}

// On VAD state change callback
void AfeAudioProcessor::OnVadStateChange(std::function<void(bool speaking)> callback)
{
    // Store VAD state change callback
    vad_state_change_callback = callback;
}

// Audio processing task
void AfeAudioProcessor::AudioProcessorTask()
{
    // Get chunk sizes
    auto fetch_size = afe_iface->get_fetch_chunksize(afe_data);
    auto feed_size = afe_iface->get_feed_chunksize(afe_data);
    ESP_LOGI(TAG, "AFE fetch size: %d, feed size: %d", fetch_size, feed_size);

    // Processing loop
    while (true)
    {
        // Wait until running
        xEventGroupWaitBits(event_group, PROCESSOR_RUNNING, pdFALSE, pdTRUE, portMAX_DELAY);

        // Fetch processed data
        auto res = afe_iface->fetch_with_delay(afe_data, portMAX_DELAY);
        if ((xEventGroupGetBits(event_group) & PROCESSOR_RUNNING) == 0)
        {
            continue;
        }

        // Check result
        if (res == nullptr || res->ret_value == ESP_FAIL)
        {
            if (res != nullptr)
            {
                ESP_LOGI(TAG, "Error code: %d", res->ret_value);
            }
            continue;
        }

        // VAD state change handling
        if (vad_state_change_callback)
        {
            // Speech detection
            if (res->vad_state == VAD_SPEECH && !is_speaking)
            {
                is_speaking = true;
                vad_state_change_callback(true);
            }

            // Silence detection
            if (res->vad_state == VAD_SILENCE && is_speaking)
            {
                is_speaking = false;
                vad_state_change_callback(false);
            }
        }

        // Output callback handling
        if (output_callback)
        {
            // Determine number of samples
            size_t samples = res->data_size / sizeof(int16_t);

            // Add data to buffer
            output_buffer.insert(output_buffer.end(), res->data, res->data + samples);

            // Call output callback with frame-sized chunks
            while (output_buffer.size() >= frame_samples)
            {
                // Invoke callback
                if (output_buffer.size() == frame_samples)
                {
                    output_callback(std::move(output_buffer));
                    output_buffer.clear();
                    output_buffer.reserve(frame_samples);
                }
                else
                {
                    output_callback(std::vector<int16_t>(output_buffer.begin(), output_buffer.begin() + frame_samples));
                    output_buffer.erase(output_buffer.begin(), output_buffer.begin() + frame_samples);
                }
            }
        }
    }
}

// Enable or disable device AEC
void AfeAudioProcessor::EnableDeviceAec(bool enable)
{
    if (enable)
    {
#ifdef CONFIG_USE_DEVICE_AEC
        afe_iface_->disable_vad(afe_data);
        afe_iface_->enable_aec(afe_data);
#endif
    }
    else
    {
        // Disable device AEC, enable VAD
        afe_iface->disable_aec(afe_data);
        afe_iface->enable_vad(afe_data);
    }
}