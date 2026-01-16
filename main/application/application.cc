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
#include "application.h"

// Define log tag
#define TAG "[client:application]"

// Constructor
Application::Application()
{
    // Create event group
    event_group = xEventGroupCreate();

    // Define clock timer arguments
    esp_timer_create_args_t clock_timer_args = {
        .callback = [](void *arg)
        {
            Application *app = (Application *)arg;
            xEventGroupSetBits(app->event_group, MAIN_EVENT_CLOCK_TICK);
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "application_clock_timer",
        .skip_unhandled_events = true,
    };

    // Create clock timer
    esp_timer_create(&clock_timer_args, &clock_timer_handle);
}

// Destructor
Application::~Application()
{
    // Delete clock timer
    if (clock_timer_handle != nullptr)
    {
        esp_timer_stop(clock_timer_handle);
        esp_timer_delete(clock_timer_handle);
    }

    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Main application entry point
void Application::ApplicationMain()
{
    // Log the GeekROS version
    ESP_LOGI(TAG, "Client Version: %s", GEEKROS_VERSION);

    // Check if GeekROS service GRK and project token are configured
    if (GEEKROS_SERVICE_GRK == NULL || strlen(GEEKROS_SERVICE_GRK) == 0 || GEEKROS_SERVICE_PROJECT_TOKEN == NULL || strlen(GEEKROS_SERVICE_PROJECT_TOKEN) == 0)
    {
        ESP_LOGW(TAG, "Please configure GEEKROS_SERVICE_GRK and GEEKROS_SERVICE_PROJECT_TOKEN in menuconfig");
        return;
    }

    // Initialize basic runtime components
    RuntimeBasic::Instance().Init();

    // Initialize system components
    SystemBasic::Instance().Init(GEEKROS_SPIFFS_BASE_PATH, GEEKROS_SPIFFS_LABEL, GEEKROS_SPIFFS_MAX_FILE);

    // Initialize system settings
    SystemSettings::Instance().Initialize();

    // Initialize locale and language components
    LanguageBasic::Instance().Init();

    // Initialize board-specific components
    BoardBasic *board = CreateBoard();
    board->Initialization();

    // Initialize WiFi board
    auto &wifi_board = WifiBoard::Instance();

    // Get audio codec from board
    auto *audio_codec = board->GetAudioCodec();

    // Set WiFi board callbacks
    WifiCallbacks wifi_board_callbacks;
    wifi_board_callbacks.on_access_point = [this, board, audio_codec]()
    {
        ESP_LOGI(TAG, "Entered Access Point Mode");

        // Initialize audio service
        audio_service.Initialize(audio_codec);

        // Start audio service
        audio_service.Start();

        // Disable voice processing initially
        audio_service.EnableVoiceProcessing(false);

        // Play WiFi configuration sound
        audio_service.PlaySound(Lang::Sounds::OGG_WIFI_CONFIG);

        // Wait until audio service is idle
        while (!audio_service.IsIdle())
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    };
    wifi_board_callbacks.on_station = [this, board, audio_codec]()
    {
        ESP_LOGI(TAG, "Entered Station Mode");

        // Check network status
        NetworkBasic::Instance().CheckNetwork();

        // Set Realtime basic callbacks
        RealtimeCallbacks realtime_callbacks;
        realtime_callbacks.on_signaling_calledback = [this](std::string event, std::string data)
        {
            ESP_LOGI(TAG, "Realtime Signaling Event: %s %s", event.c_str(), data.c_str());
        };
        realtime_callbacks.on_peer_datachannel_calledback = [this, audio_codec](std::string label, std::string event, std::string data)
        {
            // ESP_LOGI(TAG, "Realtime Peer Data Channel Event: %s label=%s data=%s", event.c_str(), label.c_str(), data.c_str());

            if (event == "peer:datachannel:open" && label == "event")
            {
                // Initialize audio service
                audio_service.Initialize(audio_codec);

                // Start audio service
                audio_service.Start();

                // Disable voice processing initially
                audio_service.EnableVoiceProcessing(true);

                // Define audio callbacks
                AudioServiceCallbacks audio_service_callbacks;
                audio_service_callbacks.on_send_queue_available = [this]()
                {
                    xEventGroupSetBits(event_group, MAIN_EVENT_SEND_AUDIO);
                };
                audio_service_callbacks.on_vad_change = [this](bool speaking)
                {
                    xEventGroupSetBits(event_group, MAIN_EVENT_VAD_CHANGE);
                };
                audio_service.SetCallbacks(audio_service_callbacks);

                // Play WiFi configuration sound
                audio_service.PlaySound(Lang::Sounds::OGG_WIFI_SUCCESS);

                // Wait until audio service is idle
                while (!audio_service.IsIdle())
                {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }

                // Initialize button components
                ButtonBasic::Instance().ButtonInitialize(BOARD_BUTTON_GPIO, 0);
                ButtonCallbacks button_callbacks;
                button_callbacks.on_button_calledback = [this](std::string event)
                {
                    // Handle short press to unmute uplink audio
                    if (event == "button:short:press")
                    {
                        // Unmute uplink audio if muted
                        if (mute_uplink_audio)
                        {
                            // Create send data channel message
                            std::string message = "{\"event\":\"client:connection:interrupt\"}";

                            RealtimeBasic::Instance().GetPeerInstance()->SendDataChannelMessage(ESP_PEER_DATA_CHANNEL_STRING, "event", (const uint8_t *)message.c_str(), message.length());
                            // Reset decoder to clear any buffered audio
                            audio_service.ResetDecoder();

                            // Unmute uplink audio
                            mute_uplink_audio = false;

                            // Reset last audio time
                            last_audio_time_us = esp_timer_get_time();
                        }
                    }

                    // Handle long press to enter AP mode
                    if (event == "button:long:press")
                    {
                        SystemSettings::Instance().SetWifiAccessPointMode(true);
                        esp_restart();
                    }
                };
                ButtonBasic::Instance().SetCallbacks(button_callbacks);
            }

            // Handle wakeup status event
            if (event == "connection:wakeup:status" && label == "event")
            {
                ESP_LOGI(TAG, "Wakeup Status: %s", data.c_str());
            }

            // Handle speak status event
            if (event == "connection:speak:status" && label == "event")
            {
                ESP_LOGI(TAG, "Speak Status: %s", data.c_str());
            }

            // Handle wakeup status event
            if (event == "connection:chat:content" && label == "chat")
            {
                // ESP_LOGI(TAG, "Chat Content: %s", data.c_str());
            }
        };
        realtime_callbacks.on_peer_audio_info_calledback = [this](std::string label, std::string event, esp_peer_audio_stream_info_t *info)
        {
            ESP_LOGI(TAG, "Realtime Peer Audio Info Event: %s label=%s codec=%d, sample_rate=%d, channel=%d", event.c_str(), label.c_str(), info->codec, info->sample_rate, info->channel);
        };
        realtime_callbacks.on_peer_video_info_calledback = [this](std::string label, std::string event, esp_peer_video_stream_info_t *info)
        {
            ESP_LOGI(TAG, "Realtime Peer Video Info Event: %s label=%s codec=%d, width=%d, height=%d, fps=%d", event.c_str(), label.c_str(), info->codec, info->width, info->height, info->fps);
        };
        realtime_callbacks.on_peer_audio_calledback = [this](std::string label, std::string event, const esp_peer_audio_frame_t *frame)
        {
            // ESP_LOGI(TAG, "Realtime Peer Audio Data Event: %s label=%s pts=%u, size=%d", event.c_str(), label.c_str(), frame->pts, frame->size);

            // Check event and frame validity
            if (event != "peer:audio:frame" || !frame || frame->size == 0)
            {
                return;
            }

            mute_uplink_audio = true;

            // Update last audio time
            last_audio_time_us = esp_timer_get_time();

            // Create audio service stream packet
            auto packet = std::make_unique<AudioServiceStreamPacket>();
            packet->payload.assign(frame->data, frame->data + frame->size);

            // Set sample rate and timestamp
            packet->sample_rate = 16000;
            packet->frame_duration = OPUS_FRAME_DURATION_MS;
            packet->timestamp = frame->pts;

            // Push packet to decode queue without waiting
            audio_service.PushPacketToDecodeQueue(std::move(packet));
        };
        realtime_callbacks.on_peer_video_calledback = [this](std::string label, std::string event, const esp_peer_video_frame_t *frame)
        {
            ESP_LOGI(TAG, "Realtime Peer Video Data Event: %s label=%s pts=%u, size=%d", event.c_str(), label.c_str(), frame->pts, frame->size);
        };
        RealtimeBasic::Instance().SetCallbacks(realtime_callbacks);

        // Connect realtime service
        RealtimeBasic::Instance().RealtimeConnect();
    };
    wifi_board.SetCallbacks(wifi_board_callbacks);

    // Start network
    wifi_board.StartNetwork();

    // Create main event loop task
    auto application_loop_task = [](void *param)
    {
        ((Application *)param)->ApplicationLoop();
        vTaskDelete(nullptr);
    };

    // Create the task with a larger stack size
    xTaskCreate(application_loop_task, "application_loop", 4096, this, 3, &main_event_loop_task_handle);

    // Start clock timer with 1 second period
    esp_timer_start_periodic(clock_timer_handle, 1000000);
}

// Main application loop
void Application::ApplicationLoop()
{
    // Main application loop
    while (true)
    {
        // Wait for clock tick event
        auto bits = xEventGroupWaitBits(event_group, MAIN_EVENT_CLOCK_TICK | MAIN_EVENT_SEND_AUDIO | MAIN_EVENT_VAD_CHANGE, pdTRUE, pdFALSE, portMAX_DELAY);

        // Handle send audio event
        if (bits & MAIN_EVENT_SEND_AUDIO)
        {
            // Get peer instance
            auto *peer = RealtimeBasic::Instance().GetPeerInstance();
            if (peer)
            {
                // Send audio frames from audio service send queue
                while (auto packet = audio_service.PopPacketFromSendQueue())
                {
                    // Check if we need to send silence
                    if (mute_uplink_audio)
                    {
                        static std::vector<uint8_t> silence_opus;
                        silence_opus.assign(packet->payload.size(), 0);

                        // Prepare esp_peer_audio_frame_t
                        esp_peer_audio_frame_t frame = {};
                        frame.data = silence_opus.data();
                        frame.size = silence_opus.size();
                        frame.pts = packet->timestamp;

                        // Send audio frame via peer
                        if (!peer->SendAudioFrame(&frame))
                        {
                            break;
                        }
                    }
                    else
                    {
                        // Prepare esp_peer_audio_frame_t
                        esp_peer_audio_frame_t frame = {};
                        frame.data = packet->payload.data();
                        frame.size = packet->payload.size();
                        frame.pts = packet->timestamp;

                        // Send audio frame via peer
                        if (!peer->SendAudioFrame(&frame))
                        {
                            break;
                        }
                    }
                }
            }
        }

        // Handle VAD change event
        if (bits & MAIN_EVENT_VAD_CHANGE)
        {
            // Get current VAD state
            // bool speaking = audio_service.IsVoiceDetected();
        }

        // Handle clock tick event
        if (bits & MAIN_EVENT_CLOCK_TICK)
        {
            // Increment health check clock
            health_check_clock++;

            // Check if it's time for health check
            if (health_check_clock % 60 == 0)
            {
                // Perform system health check every 30 seconds
                SystemBasic::HealthCheck();
            }
        }

        // Handle uplink audio muting
        if (mute_uplink_audio)
        {
            // Check if we should unmute uplink audio
            int64_t now = esp_timer_get_time();

            // If no audio received for more than 200ms, unmute uplink audio
            if (audio_service.IsIdle() && (now - last_audio_time_us) > 200 * 1000)
            {
                // Reset decoder to clear any buffered audio
                audio_service.ResetDecoder();

                // Unmute uplink audio
                mute_uplink_audio = false;
            }
        }

        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}