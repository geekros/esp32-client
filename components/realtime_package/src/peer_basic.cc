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

// Include header
#include "peer_basic.h"

// Define log tag
#define TAG "[client:components:realtime:peer]"

// Constructor
PeerBasic::PeerBasic()
{
    // Create the event group
    event_group = xEventGroupCreate();
}

// Destructor
PeerBasic::~PeerBasic()
{
    // Delete the event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
        event_group = nullptr;
    }
}

// Peer state handler
int PeerBasic::OnStateHandler(esp_peer_state_t state, void *ctx)
{
    // Cast context to PeerBasic instance
    PeerBasic *self = static_cast<PeerBasic *>(ctx);
    if (self == nullptr)
    {
        // Return success (no operation)
        return ESP_OK;
    }

    // Handle connection
    if (state == ESP_PEER_STATE_CONNECTED)
    {
        // Create data channels
        self->CreatePeerDataChannels();

        // Start audio sending task if not already running
        if (self->peer_send_audio_task_handle == nullptr)
        {
            xTaskCreatePinnedToCore(self->PeerSendAudioTask, "peer_send_audio_task", 4096, self, 5, &self->peer_send_audio_task_handle, 0);
        }

        // Update peer connected state
        self->UpdatePeerConnectedState(true);
    }

    // Handle disconnection
    if (state == ESP_PEER_STATE_DISCONNECTED)
    {
        // Update peer connected state
        self->UpdatePeerConnectedState(false);
    }

    // Return success
    return ESP_OK;
}

// Peer message handler
int PeerBasic::OnMessageHandler(esp_peer_msg_t *msg, void *ctx)
{
    // Cast context to PeerBasic instance
    PeerBasic *self = static_cast<PeerBasic *>(ctx);
    if (self == nullptr)
    {
        // Return success (no operation)
        return ESP_OK;
    }

    // Handle SDP message
    if (msg->type == ESP_PEER_MSG_TYPE_SDP)
    {
        // Extract message data
        int len = msg->size;
        char buf[len + 1];

        // Check length and data
        if (len > 0 && msg->data != nullptr)
        {
            // Copy message data to buffer
            memcpy(buf, msg->data, len);

            // Null-terminate the buffer
            buf[len] = '\0';

            // Parse JSON from buffer
            cJSON *root = cJSON_CreateObject();
            if (root)
            {
                // Add type and sdp to JSON object
                cJSON_AddStringToObject(root, "type", "offer");
                cJSON_AddStringToObject(root, "sdp", buf);

                // Print unformatted JSON
                char *offer_json = cJSON_PrintUnformatted(root);
                if (offer_json)
                {
                    // Send offer_json to signaling server
                    if (self->callbacks.on_offer_calledback)
                    {
                        // Invoke offer callback
                        self->callbacks.on_offer_calledback(std::string(offer_json));
                    }

                    // Free the offer JSON
                    free(offer_json);
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to serialize offer JSON");
                }

                // Delete JSON object
                cJSON_Delete(root);
            }
        }
    }

    // Handle ICE candidate message
    if (msg->type == ESP_PEER_MSG_TYPE_CANDIDATE)
    {
        // Extract message data
        int len = msg->size;
        char buf[len + 1];

        // Check length and data
        if (len > 0 && msg->data != nullptr)
        {
            // Copy message data to buffer
            memcpy(buf, msg->data, len);

            // Null-terminate the buffer
            buf[len] = '\0';

            // Parse JSON from buffer
            cJSON *root = cJSON_CreateObject();
            if (root)
            {
                // Add candidate info to JSON object
                cJSON_AddStringToObject(root, "candidate", buf);
                cJSON_AddStringToObject(root, "sdpMid", "0");
                cJSON_AddNumberToObject(root, "sdpMLineIndex", 0);

                // Print unformatted JSON
                char *candidate_json = cJSON_PrintUnformatted(root);
                if (candidate_json)
                {
                    // Send candidate_json to signaling server
                    if (self->callbacks.on_candidate_calledback)
                    {
                        // Invoke candidate callback
                        self->callbacks.on_candidate_calledback(std::string(candidate_json));
                    }

                    // Free the candidate JSON
                    free(candidate_json);
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to serialize candidate JSON");
                }
            }

            // Delete JSON object
            cJSON_Delete(root);
        }
    }

    // Return success
    return ESP_OK;
}

// Video info handler
int PeerBasic::OnVideoInfoHandler(esp_peer_video_stream_info_t *info, void *ctx)
{
    // Return success
    return ESP_OK;
}

// Video data handler
int PeerBasic::OnVideoDataHandler(esp_peer_video_frame_t *frame, void *ctx)
{
    // Return success
    return ESP_OK;
}

// Audio info handler
int PeerBasic::OnAudioInfoHandler(esp_peer_audio_stream_info_t *info, void *ctx)
{
    // Return success
    return ESP_OK;
}

// Audio data handler
int PeerBasic::OnAudioDataHandler(esp_peer_audio_frame_t *frame, void *ctx)
{
    // Return success
    return ESP_OK;
}

// Data channel open handler
int PeerBasic::OnDataChannelOpenHandler(esp_peer_data_channel_info_t *ch, void *ctx)
{
    // Return success
    return ESP_OK;
}

// Data channel data handler
int PeerBasic::OnDataChannelHandler(esp_peer_data_frame_t *frame, void *ctx)
{
    // Return success
    return ESP_OK;
}

// Data channel close handler
int PeerBasic::OnDataChannelCloseHandler(esp_peer_data_channel_info_t *ch, void *ctx)
{
    // Return success
    return ESP_OK;
}

// Peer task
void PeerBasic::PeerTask(void *param)
{
    // Cast parameter to PeerBasic instance
    PeerBasic *self = static_cast<PeerBasic *>(param);
    if (self == nullptr)
    {
        // Delete task
        vTaskDelete(nullptr);

        // Set peer task running flag to false
        self->peer_task_running = false;

        // Return
        return;
    }

    // Set peer task running flag to true
    self->peer_task_running = true;

    // Peer task loop
    while (self->peer_task_running)
    {
        // Check if peer handle is valid
        if (!self->client_peer)
        {
            ESP_LOGW(TAG, "Peer handle is invalid, exiting peer task");
            break;
        }

        // Call the main loop function
        esp_peer_main_loop(self->client_peer);

        // Delay for a short period
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Set peer task running flag to false
    self->peer_task_running = false;

    // Delete task
    vTaskDelete(nullptr);
}

// Peer send audio task
void PeerBasic::PeerSendAudioTask(void *param)
{
    // Cast parameter to PeerBasic instance
    PeerBasic *self = static_cast<PeerBasic *>(param);
    if (self == nullptr)
    {
        // Delete task
        vTaskDelete(nullptr);

        // Set peer task running flag to false
        self->peer_send_audio_task_running = false;

        // Return
        return;
    }

    // Set peer send audio task running flag to true
    self->peer_send_audio_task_running = true;

    // Define a simple Opus silence frame
    static const uint8_t opus_silence_frame[3] = {0xF8, 0xFF, 0xFE};
    esp_peer_audio_frame_t frame = {};
    frame.data = (uint8_t *)opus_silence_frame;
    frame.size = 3;

    // Peer send audio task loop
    while (self->peer_send_audio_task_running)
    {
        // Check if peer task is running and peer handle is valid
        if (!self->peer_task_running || self->client_peer == nullptr)
        {
            ESP_LOGW(TAG, "Peer task not running or peer handle invalid, exiting audio send task");
            break;
        }

        // Send audio frame
        esp_peer_send_audio(self->client_peer, &frame);

        // Delay for a short period
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Set peer send audio task running flag to false
    self->peer_send_audio_task_running = false;

    // Delete task
    vTaskDelete(nullptr);
}

// Peer send video task
void PeerBasic::PeerSendVideoTask(void *param)
{
    // Cast parameter to PeerBasic instance
    PeerBasic *self = static_cast<PeerBasic *>(param);
    if (self == nullptr)
    {
        // Delete task
        vTaskDelete(nullptr);

        // Set peer task running flag to false
        self->peer_send_video_task_running = false;

        // Return
        return;
    }

    // Set peer send video task running flag to true
    self->peer_send_video_task_running = true;

    // Define video frame
    esp_peer_video_frame_t frame = {};
    frame.data = nullptr;
    frame.size = 0;

    // Peer send video task loop
    while (self->peer_send_video_task_running)
    {
        // Check if peer task is running and peer handle is valid
        if (!self->peer_task_running || self->client_peer == nullptr)
        {
            ESP_LOGW(TAG, "Peer task not running or peer handle invalid, exiting video send task");
            break;
        }

        // Send video frame (nullptr for testing)
        esp_peer_send_video(self->client_peer, &frame);

        // Delay for a short period
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Set peer send video task running flag to false
    self->peer_send_video_task_running = false;

    // Delete task
    vTaskDelete(nullptr);
}

// Create peer method
esp_err_t PeerBasic::CreatePeer(const std::vector<std::string> &stun_urls)
{
    // Check if already initialized
    if (client_peer != nullptr)
    {
        ESP_LOGW(TAG, "Peer is already initialized");

        // Return success
        return ESP_OK;
    }

    // Define peer extra configuration
    esp_peer_default_cfg_t peer_extra_config = {0};

    // Set default values for peer extra configuration
    peer_extra_config.agent_recv_timeout = 100;
    peer_extra_config.data_ch_cfg.recv_cache_size = 1536;
    peer_extra_config.data_ch_cfg.send_cache_size = 1536;
    peer_extra_config.rtp_cfg.audio_recv_jitter.cache_size = 1024;
    peer_extra_config.rtp_cfg.send_pool_size = 1024;
    peer_extra_config.rtp_cfg.send_queue_num = 10;

    // Define peer configuration
    esp_peer_cfg_t peer_config = {};

    // Set peer audio configuration
    peer_config.audio_dir = ESP_PEER_MEDIA_DIR_SEND_RECV;
    peer_config.audio_info.codec = ESP_PEER_AUDIO_CODEC_OPUS;
    peer_config.audio_info.sample_rate = 16000;
    peer_config.audio_info.channel = 1;

    // Set peer data channel configuration
    peer_config.enable_data_channel = true;

    // Set manual data channel creation
    peer_config.manual_ch_create = true;

    // Set peer role
    peer_config.role = ESP_PEER_ROLE_CONTROLLING;

    // Set callbacks
    peer_config.on_state = OnStateHandler;
    peer_config.on_msg = OnMessageHandler;
    peer_config.on_video_info = OnVideoInfoHandler;
    peer_config.on_video_data = OnVideoDataHandler;
    peer_config.on_audio_info = OnAudioInfoHandler;
    peer_config.on_audio_data = OnAudioDataHandler;
    peer_config.on_channel_open = OnDataChannelOpenHandler;
    peer_config.on_data = OnDataChannelHandler;
    peer_config.on_channel_close = OnDataChannelCloseHandler;

    // Set context
    peer_config.ctx = this;

    // Set extra configuration
    peer_config.extra_cfg = &peer_extra_config;
    peer_config.extra_size = sizeof(esp_peer_default_cfg_t);

    // Create the peer connection
    int ret = esp_peer_open(&peer_config, esp_peer_get_default_impl(), &client_peer);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "Failed to open peer, ret=%d", ret);

        // Clear peer handle
        client_peer = nullptr;

        // Return failure
        return ESP_FAIL;
    }

    // Create peer task
    if (xTaskCreatePinnedToCore(PeerTask, "peer_task", 10 * 1024, this, 5, &peer_task_handle, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create peer task");

        // Set peer task running flag to false
        peer_task_running = false;

        // Close peer
        esp_peer_close(client_peer);

        // Clear peer handle
        client_peer = nullptr;

        // Return failure
        return ESP_FAIL;
    }

    // Connect the peer
    PeerConnect(stun_urls);

    // Return success
    return ESP_OK;
}

// Peer connect method
void PeerBasic::PeerConnect(const std::vector<std::string> &stun_urls)
{
    // Check if peer is initialized
    if (client_peer == nullptr)
    {
        ESP_LOGE(TAG, "Peer is not initialized");

        // Return
        return;
    }

    // Define ICE server configuration
    std::vector<esp_peer_ice_server_cfg_t> ice_servers;
    for (const auto &url : stun_urls)
    {
        esp_peer_ice_server_cfg_t server = {};
        server.stun_url = (char *)url.c_str();
        server.user = nullptr;
        server.psw = nullptr;
        ice_servers.push_back(server);
    }

    // Set ICE servers if available
    if (!ice_servers.empty())
    {
        // Update ICE server information
        int ret = esp_peer_update_ice_info(client_peer, ESP_PEER_ROLE_CONTROLLING, ice_servers.data(), (int)ice_servers.size());
        if (ret != ESP_PEER_ERR_NONE)
        {
            ESP_LOGE(TAG, "Failed to update ICE server info, ret=%d", ret);
        }
    }

    // Create new peer connection
    int ret = esp_peer_new_connection(client_peer);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "Failed to create new peer connection, ret=%d", ret);
    }
}

// Set peer answer method
void PeerBasic::SetPeerAnswer(const std::string &answer_json)
{
    // Check if peer is initialized
    if (client_peer == nullptr)
    {
        ESP_LOGE(TAG, "Peer is not initialized");

        // Return
        return;
    }

    // Parse answer JSON
    cJSON *root = cJSON_Parse(answer_json.c_str());
    if (root == nullptr)
    {
        ESP_LOGE(TAG, "Failed to parse answer JSON");

        // Return
        return;
    }

    // Get SDP item
    cJSON *sdp_item = cJSON_GetObjectItem(root, "sdp");
    if (!cJSON_IsString(sdp_item) || !sdp_item->valuestring)
    {
        ESP_LOGE(TAG, "Invalid SDP in answer JSON");

        // Delete JSON root
        cJSON_Delete(root);

        // Return
        return;
    }

    // Extract SDP string
    const char *sdp_str = sdp_item->valuestring;
    int sdp_len = (int)strlen(sdp_str);
    if (sdp_len <= 0)
    {
        ESP_LOGE(TAG, "Empty SDP in answer JSON");

        // Delete JSON root
        cJSON_Delete(root);

        // Return
        return;
    }

    // Allocate buffer for SDP
    uint8_t *buf = (uint8_t *)malloc(sdp_len);
    if (!buf)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for SDP");

        // Delete JSON root
        cJSON_Delete(root);

        // Return
        return;
    }

    // Set SDP answer to peer
    memcpy(buf, sdp_str, sdp_len);

    // Create peer message
    esp_peer_msg_t msg = {};
    msg.type = ESP_PEER_MSG_TYPE_SDP;
    msg.data = buf;
    msg.size = sdp_len;

    // Send message to peer
    int ret = esp_peer_send_msg(client_peer, &msg);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "Failed to send SDP answer to peer, ret=%d", ret);
    }

    // Feed buffer
    free(buf);

    // Delete JSON root
    cJSON_Delete(root);
}

// Set peer candidate method
void PeerBasic::SetPeerCandidate(const std::string &candidate_json)
{
    // Check if peer is initialized
    if (client_peer == nullptr)
    {
        ESP_LOGE(TAG, "Peer is not initialized");

        // Return
        return;
    }

    // Parse candidate JSON
    cJSON *root = cJSON_Parse(candidate_json.c_str());
    if (!root)
    {
        ESP_LOGE(TAG, "Failed to parse candidate JSON");

        // Return
        return;
    }

    // Get candidate item
    cJSON *candidate_item = cJSON_GetObjectItem(root, "candidate");
    if (!cJSON_IsString(candidate_item) || !candidate_item->valuestring)
    {
        ESP_LOGE(TAG, "Invalid candidate in candidate JSON");

        // Delete JSON root
        cJSON_Delete(root);

        // Return
        return;
    }

    // Extract candidate string
    const char *candidate_str = candidate_item->valuestring;
    int candidate_len = (int)strlen(candidate_str);
    if (candidate_len <= 0)
    {
        ESP_LOGE(TAG, "Empty candidate in candidate JSON");

        // Delete JSON root
        cJSON_Delete(root);

        // Return
        return;
    }

    // Allocate buffer for candidate
    uint8_t *buf = (uint8_t *)malloc(candidate_len);
    if (!buf)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for candidate");

        // Delete JSON root
        cJSON_Delete(root);

        // Return
        return;
    }

    // Set candidate to peer
    memcpy(buf, candidate_str, candidate_len);

    // Create peer message
    esp_peer_msg_t msg = {};
    msg.type = ESP_PEER_MSG_TYPE_CANDIDATE;
    msg.data = buf;
    msg.size = candidate_len;

    int ret = esp_peer_send_msg(client_peer, &msg);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "Failed to send candidate to peer, ret=%d", ret);
    }

    // Free buffer
    free(buf);

    // Delete JSON root
    cJSON_Delete(root);
}

// Create peer data channels
void PeerBasic::CreatePeerDataChannels()
{
    // Check if peer is initialized
    if (!client_peer)
    {
        ESP_LOGE(TAG, "Peer is not initialized");
        return;
    }

    // Define data channel names
    const char *channels[] = {"chat", "event"};

    // Create data channels
    for (const char *ch_name : channels)
    {
        // Define data channel configuration
        esp_peer_data_channel_cfg_t data_channel_config = {};
        data_channel_config.type = ESP_PEER_DATA_CHANNEL_RELIABLE; // Reliable (same as browser default)
        data_channel_config.ordered = true;                        // Ordered true (typical default)
        data_channel_config.label = (char *)ch_name;               // Channel label

        // Create data channel
        int ret = esp_peer_create_data_channel(client_peer, &data_channel_config);
        if (ret != ESP_PEER_ERR_NONE)
        {
            ESP_LOGE(TAG, "Failed to create data channel '%s', ret=%d", ch_name, ret);
        }
    }
}

// Update peer connected state
void PeerBasic::UpdatePeerConnectedState(bool connected)
{
    // Update peer connected flag
    peer_connected = connected;
}

// Set peer callbacks
void PeerBasic::SetCallbacks(PeerCallbacks &cb)
{
    // Update callbacks
    callbacks = cb;
}