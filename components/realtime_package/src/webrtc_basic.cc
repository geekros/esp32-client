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
#include "webrtc_basic.h"

// Define log tag
#define TAG "[client:components:realtime:webrtc]"

// Constructor
WebRTCBasic::WebRTCBasic()
{
    // Create the event group
    event_group = xEventGroupCreate();
}

// Destructor
WebRTCBasic::~WebRTCBasic()
{
    // Delete the event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
        event_group = nullptr;
    }
}

// Handle signaling connected
void WebRTCBasic::OnSignalingConnected(const std::vector<std::string> &stun_urls, const std::vector<std::string> &turn_urls, const std::string &turn_username, const std::string &turn_credential)
{
    ESP_LOGI(TAG, "Signaling Connected - STUN URLs: %d, TURN URLs: %d", stun_urls.size(), turn_urls.size());

    // Initialize WebRTC
    esp_err_t err = Init(stun_urls, turn_urls, turn_username, turn_credential);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "WebRTCBasic Init failed");
    }
}

// Handle signaling answer
void WebRTCBasic::OnSignalingAnswer(const std::string &answer_json)
{
    ESP_LOGI(TAG, "OnSignalingAnswer: %s", answer_json.c_str());

    if (peer_ == nullptr)
    {
        ESP_LOGE(TAG, "OnSignalingAnswer: peer_ is null");
        return;
    }

    cJSON *root = cJSON_Parse(answer_json.c_str());
    if (!root)
    {
        ESP_LOGE(TAG, "OnSignalingAnswer: failed to parse JSON");
        return;
    }

    cJSON *sdp_item = cJSON_GetObjectItem(root, "sdp");
    if (!cJSON_IsString(sdp_item) || !sdp_item->valuestring)
    {
        ESP_LOGE(TAG, "OnSignalingAnswer: 'sdp' field not found or invalid");
        cJSON_Delete(root);
        return;
    }

    const char *sdp_str = sdp_item->valuestring;
    int sdp_len = (int)strlen(sdp_str);
    if (sdp_len <= 0)
    {
        ESP_LOGE(TAG, "OnSignalingAnswer: empty sdp string");
        cJSON_Delete(root);
        return;
    }

    uint8_t *buf = (uint8_t *)malloc(sdp_len);
    if (!buf)
    {
        ESP_LOGE(TAG, "OnSignalingAnswer: malloc failed");
        cJSON_Delete(root);
        return;
    }
    memcpy(buf, sdp_str, sdp_len);

    esp_peer_msg_t msg = {};
    msg.type = ESP_PEER_MSG_TYPE_SDP;
    msg.data = buf;
    msg.size = sdp_len;

    int ret = esp_peer_send_msg(peer_, &msg);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "OnSignalingAnswer: esp_peer_send_msg failed, ret=%d", ret);
    }
    else
    {
        ESP_LOGI(TAG, "OnSignalingAnswer: esp_peer_send_msg success (SDP Answer)");
    }

    free(buf);
    cJSON_Delete(root);
}

// Handle signaling candidate
void WebRTCBasic::OnSignalingCandidate(const std::string &candidate_json)
{
    ESP_LOGI(TAG, "OnSignalingCandidate: %s", candidate_json.c_str());

    if (peer_ == nullptr)
    {
        ESP_LOGE(TAG, "OnSignalingCandidate: peer_ is null");
        return;
    }

    cJSON *root = cJSON_Parse(candidate_json.c_str());
    if (!root)
    {
        ESP_LOGE(TAG, "OnSignalingCandidate: failed to parse JSON");
        return;
    }

    cJSON *cand_item = cJSON_GetObjectItem(root, "candidate");
    if (!cJSON_IsString(cand_item) || !cand_item->valuestring)
    {
        ESP_LOGE(TAG, "OnSignalingCandidate: 'candidate' field not found or invalid");
        cJSON_Delete(root);
        return;
    }

    const char *cand_str = cand_item->valuestring;
    int cand_len = (int)strlen(cand_str);
    if (cand_len <= 0)
    {
        ESP_LOGE(TAG, "OnSignalingCandidate: empty candidate string");
        cJSON_Delete(root);
        return;
    }

    // 拷贝一份 candidate 数据
    uint8_t *buf = (uint8_t *)malloc(cand_len);
    if (!buf)
    {
        ESP_LOGE(TAG, "OnSignalingCandidate: malloc failed");
        cJSON_Delete(root);
        return;
    }
    memcpy(buf, cand_str, cand_len);

    esp_peer_msg_t msg = {};
    msg.type = ESP_PEER_MSG_TYPE_CANDIDATE;
    msg.data = buf;
    msg.size = cand_len;

    int ret = esp_peer_send_msg(peer_, &msg);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "OnSignalingCandidate: esp_peer_send_msg failed, ret=%d", ret);
    }
    else
    {
        ESP_LOGI(TAG, "OnSignalingCandidate: esp_peer_send_msg success (ICE Candidate)");
    }

    free(buf);
    cJSON_Delete(root);
}

// Initialization method
esp_err_t WebRTCBasic::Init(const std::vector<std::string> &stun_urls, const std::vector<std::string> &turn_urls, const std::string &turn_username, const std::string &turn_credential)
{
    // Check if already initialized
    if (peer_ != nullptr)
    {
        ESP_LOGW(TAG, "WebRTCBasic already has a peer, skip Init");
        return ESP_OK;
    }

    // Store STUN and TURN server info
    stun_urls_ = stun_urls;
    turn_urls_ = turn_urls;
    turn_username_ = turn_username;
    turn_credential_ = turn_credential;

    // Initialization logic here
    ESP_LOGI(TAG, "Initializing WebRTCBasic...");

    // Define peer extra configuration
    esp_peer_default_cfg_t peer_extra_cfg = {0};

    // Set default values for peer extra configuration
    peer_extra_cfg.agent_recv_timeout = 100;
    peer_extra_cfg.data_ch_cfg.recv_cache_size = 1536;
    peer_extra_cfg.data_ch_cfg.send_cache_size = 1536;
    peer_extra_cfg.rtp_cfg.audio_recv_jitter.cache_size = 1024;
    peer_extra_cfg.rtp_cfg.send_pool_size = 1024;
    peer_extra_cfg.rtp_cfg.send_queue_num = 10;

    // Define peer configuration
    esp_peer_cfg_t cfg = {};

    // Set peer audio configuration
    cfg.audio_dir = ESP_PEER_MEDIA_DIR_SEND_RECV;
    cfg.audio_info.codec = ESP_PEER_AUDIO_CODEC_OPUS;
    cfg.audio_info.sample_rate = 16000;
    cfg.audio_info.channel = 1;

    // Set peer data channel configuration
    cfg.enable_data_channel = true;

    // Set manual data channel creation
    cfg.manual_ch_create = true;

    // Set peer role
    cfg.role = ESP_PEER_ROLE_CONTROLLING;

    // Set callback handlers
    cfg.on_state = PeerStateHandler;
    cfg.on_msg = PeerMsgHandler;
    cfg.on_video_info = PeerVideoInfoHandler;
    cfg.on_audio_info = PeerAudioInfoHandler;
    cfg.on_video_data = PeerVideoDataHandler;
    cfg.on_audio_data = PeerAudioDataHandler;
    cfg.on_data = PeerDataHandler;

    // Set context and extra configuration
    cfg.ctx = this;

    // Set extra configuration
    cfg.extra_cfg = &peer_extra_cfg;
    cfg.extra_size = sizeof(esp_peer_default_cfg_t);

    // Create the peer connection
    int ret = esp_peer_open(&cfg, esp_peer_get_default_impl(), &peer_);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "Failed to create PeerConnection, ret=%d", ret);

        // Clear peer handle
        peer_ = nullptr;

        // Return failure
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "PeerConnection created successfully, handle=%p", peer_);

    // Update peer state variables
    peer_running_ = true;
    peer_stopped_ = false;

    // Create the peer main task
    BaseType_t task_ret = xTaskCreatePinnedToCore(PeerMainTask, "webrtc_peer_task", 10 * 1024, this, 5, &peer_task_handle_, 0);
    if (task_ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create peer main task");

        // Update peer state variables
        peer_running_ = false;
        peer_task_handle_ = nullptr;

        // Close the peer connection
        esp_peer_close(peer_);

        // Clear peer handle
        peer_ = nullptr;

        // Return failure
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WebRTCBasic initialized successfully");

    StartConnection();

    // Return success
    return ESP_OK;
}

// Peer main task
void WebRTCBasic::PeerMainTask(void *arg)
{
    // Get the instance pointer
    WebRTCBasic *self = static_cast<WebRTCBasic *>(arg);
    if (!self)
    {
        // Delete the task if instance is null
        vTaskDelete(NULL);

        // Return
        return;
    }

    ESP_LOGI(TAG, "Peer main loop task started");

    // Main loop
    while (self->peer_running_)
    {
        // Run the peer main loop
        if (self->peer_)
        {
            // Call the main loop function
            esp_peer_main_loop(self->peer_);
        }

        // Delay to yield CPU
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // Update peer stopped state
    self->peer_stopped_ = true;

    ESP_LOGI(TAG, "Peer main loop task stopped");

    // Delete the task
    vTaskDelete(NULL);
}

// Start connection
void WebRTCBasic::StartConnection()
{
    if (peer_ == nullptr)
    {
        ESP_LOGE(TAG, "StartConnection: peer_ is null");
        return;
    }

    std::vector<esp_peer_ice_server_cfg_t> servers;
    servers.reserve(stun_urls_.size() + turn_urls_.size());

    for (const auto &url : stun_urls_)
    {
        esp_peer_ice_server_cfg_t cfg = {};
        cfg.stun_url = const_cast<char *>(url.c_str());
        cfg.user = nullptr;
        cfg.psw = nullptr;
        servers.push_back(cfg);
    }

    for (const auto &url : turn_urls_)
    {
        esp_peer_ice_server_cfg_t cfg = {};
        cfg.stun_url = const_cast<char *>(url.c_str());
        cfg.user = turn_username_.empty() ? nullptr : const_cast<char *>(turn_username_.c_str());
        cfg.psw = turn_credential_.empty() ? nullptr : const_cast<char *>(turn_credential_.c_str());
        servers.push_back(cfg);
    }

    if (!servers.empty())
    {
        int ret = esp_peer_update_ice_info(peer_, ESP_PEER_ROLE_CONTROLLING, servers.data(), (int)servers.size());
        if (ret != ESP_PEER_ERR_NONE)
        {
            ESP_LOGE(TAG, "esp_peer_update_ice_info failed, ret=%d", ret);
        }
        else
        {
            ESP_LOGI(TAG, "esp_peer_update_ice_info success, server_num=%d", (int)servers.size());
        }
    }
    else
    {
        ESP_LOGW(TAG, "StartConnection: no ICE servers configured, skip esp_peer_update_ice_info");
    }

    int ret = esp_peer_new_connection(peer_);
    if (ret != ESP_PEER_ERR_NONE)
    {
        ESP_LOGE(TAG, "esp_peer_new_connection failed, ret=%d", ret);
    }
    else
    {
        ESP_LOGI(TAG, "esp_peer_new_connection success, waiting for local SDP in PeerMsgHandler");
    }
}

// Create data channels
void WebRTCBasic::CreateDataChannels()
{
    if (!peer_)
    {
        ESP_LOGE(TAG, "CreateDataChannels: peer_ is null");
        return;
    }

    ESP_LOGI(TAG, "Creating DataChannels...");

    const char *channels[] = {"chat", "event"};

    for (int i = 0; i < 2; i++)
    {
        esp_peer_data_channel_cfg_t cfg = {};
        cfg.type = ESP_PEER_DATA_CHANNEL_RELIABLE; // Reliable (same as browser default)
        cfg.ordered = true;                        // Ordered true (typical default)
        cfg.label = (char *)channels[i];           // Channel label

        int ret = esp_peer_create_data_channel(peer_, &cfg);
        if (ret != ESP_PEER_ERR_NONE)
        {
            ESP_LOGE(TAG, "Failed to create DataChannel '%s', ret=%d", channels[i], ret);
        }
        else
        {
            ESP_LOGI(TAG, "DataChannel '%s' created", channels[i]);
        }
    }
}

void WebRTCBasic::AudioSendTask(void *arg)
{
    WebRTCBasic *self = static_cast<WebRTCBasic *>(arg);

    static const uint8_t opus_silence_frame[3] = {0xF8, 0xFF, 0xFE};

    esp_peer_audio_frame_t frame = {};
    frame.data = (uint8_t *)opus_silence_frame;
    frame.size = 3;

    ESP_LOGI(TAG, "AudioSendTask started");

    while (true)
    {
        if (!self->peer_running_ || self->peer_ == nullptr)
        {
            ESP_LOGW(TAG, "AudioSendTask exiting");
            break;
        }

        esp_peer_send_audio(self->peer_, &frame);
        vTaskDelay(pdMS_TO_TICKS(20)); // 50 FPS = 20ms per frame
    }

    vTaskDelete(NULL);
}

// Peer state handler
int WebRTCBasic::PeerStateHandler(esp_peer_state_t state, void *ctx)
{
    WebRTCBasic *self = static_cast<WebRTCBasic *>(ctx);
    if (!self)
        return 0;

    switch (state)
    {
    case ESP_PEER_STATE_CONNECTED:
        ESP_LOGI(TAG, "Peer state: CONNECTED");
        self->CreateDataChannels();
        if (self->audio_task_handle_ == nullptr)
        {
            xTaskCreatePinnedToCore(WebRTCBasic::AudioSendTask, "webrtc_audio_sender", 4096, self, 5, &self->audio_task_handle_, 0);
        }
        break;
    case ESP_PEER_STATE_DISCONNECTED:
        ESP_LOGI(TAG, "Peer state: DISCONNECTED");
        break;
    default:
        ESP_LOGI(TAG, "Peer state: %d", (int)state);
        break;
    }
    return 0;
}

// Peer message handler
int WebRTCBasic::PeerMsgHandler(esp_peer_msg_t *msg, void *ctx)
{
    WebRTCBasic *self = static_cast<WebRTCBasic *>(ctx);
    if (!self || !msg)
        return 0;

    ESP_LOGI(TAG, "Peer msg handler: type=%d, size=%d", (int)msg->type, (int)msg->size);

    if (msg->type == ESP_PEER_MSG_TYPE_SDP)
    {
        int len = msg->size;
        if (len > 4095)
        {
            len = 4095;
        }

        char buf[4096];
        if (len > 0 && msg->data != nullptr)
        {
            memcpy(buf, msg->data, len);
            buf[len] = '\0';
            ESP_LOGI(TAG, "Local SDP from esp_peer:\n%s", buf);

            if (!self->local_sdp_sent_)
            {
                cJSON *root = cJSON_CreateObject();
                if (root)
                {
                    cJSON_AddStringToObject(root, "type", "offer");
                    cJSON_AddStringToObject(root, "sdp", buf);

                    char *offer_json = cJSON_PrintUnformatted(root);
                    if (offer_json)
                    {
                        // 通过 SignalingBasic 发送给服务器
                        // 对应 Web 端的：signaling.send("client:signaling:offer", callback.sdp);
                        SignalingBasic::Instance().Send("client:signaling:offer", offer_json);

                        ESP_LOGI(TAG, "Sent client:signaling:offer via signaling, len=%d", strlen(offer_json));

                        free(offer_json);
                        self->local_sdp_sent_ = true;
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Failed to serialize offer JSON");
                    }

                    cJSON_Delete(root);
                }
            }
        }
        else
        {
            ESP_LOGW(TAG, "ESP_PEER_MSG_TYPE_SDP but no data");
        }
    }

    if (msg->type == ESP_PEER_MSG_TYPE_CANDIDATE)
    {
        int len = msg->size;
        if (len > 512)
        {
            len = 512;
        }

        char buf[513];
        if (len > 0 && msg->data != nullptr)
        {
            memcpy(buf, msg->data, len);
            buf[len] = '\0';
            // ESP_LOGI(TAG, "Local ICE candidate from esp_peer:\n%s", buf);

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "candidate", buf);
            cJSON_AddStringToObject(root, "sdpMid", "0");
            cJSON_AddNumberToObject(root, "sdpMLineIndex", 0);

            char *json = cJSON_PrintUnformatted(root);
            if (json)
            {
                SignalingBasic::Instance().Send("client:signaling:icecandidate", json);
                ESP_LOGI(TAG, "Sent client:signaling:icecandidate, len=%d", strlen(json));
                free(json);
            }
            else
            {
                ESP_LOGE(TAG, "Failed to serialize candidate JSON");
            }

            cJSON_Delete(root);
        }
        else
        {
            ESP_LOGW(TAG, "ESP_PEER_MSG_TYPE_CANDIDATE but no data");
        }
    }

    return 0;
}

// Peer video info handler
int WebRTCBasic::PeerVideoInfoHandler(esp_peer_video_stream_info_t *info, void *ctx)
{
    (void)ctx;
    ESP_LOGI(TAG, "Peer video info handler");
    return 0;
}

// Peer audio info handler
int WebRTCBasic::PeerAudioInfoHandler(esp_peer_audio_stream_info_t *info, void *ctx)
{
    (void)ctx;
    ESP_LOGI(TAG, "Peer audio info handler");
    return 0;
}

// Peer video data handler
int WebRTCBasic::PeerVideoDataHandler(esp_peer_video_frame_t *frame, void *ctx)
{
    (void)ctx;
    (void)frame;
    // 暂时不处理视频
    return 0;
}

// Peer audio data handler
int WebRTCBasic::PeerAudioDataHandler(esp_peer_audio_frame_t *frame, void *ctx)
{
    (void)ctx;
    (void)frame;
    // 后续可以在这里接收远端音频
    return 0;
}

// Peer data handler
int WebRTCBasic::PeerDataHandler(esp_peer_data_frame_t *frame, void *ctx)
{
    WebRTCBasic *self = static_cast<WebRTCBasic *>(ctx);
    if (!self || !frame)
    {
        return 0;
    }

    ESP_LOGI(TAG, "Peer data handler: type=%d, size=%d", (int)frame->type, (int)frame->size);
    // 后续这里会解析 chat/media/event 三个 DataChannel 的 JSON 协议

    return 0;
}
