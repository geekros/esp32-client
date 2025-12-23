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

#ifndef REALTIME_PEER_H
#define REALTIME_PEER_H

// Include standard headers
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

// Include cJSON headers
#include "cJSON.h"

// Include ESP Peer headers
#include "esp_peer.h"
#include "esp_peer_default.h"

// Include headers
#include "black_image.h"

// Define peer data channel meta structure
struct PeerDataChannelMeta
{
    uint16_t stream_id;
    std::string label;
};

// Define peer callbacks structure
struct PeerCallbacks
{
    std::function<void(std::string json_data)> on_offer_calledback;
    std::function<void(std::string json_data)> on_candidate_calledback;
    std::function<void(std::string label, std::string event, std::string data)> on_datachannel_calledback;
    std::function<void(std::string label, std::string event, esp_peer_audio_stream_info_t *info)> on_audio_info_calledback;
    std::function<void(std::string label, std::string event, esp_peer_video_stream_info_t *info)> on_video_info_calledback;
    std::function<void(std::string label, std::string event, const esp_peer_audio_frame_t *frame)> on_audio_frame_received;
    std::function<void(std::string label, std::string event, const esp_peer_video_frame_t *frame)> on_video_frame_received;
};

// PeerBasic class definition
class PeerBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Audio and Video transmit queue
    QueueHandle_t audio_tx_queue;
    QueueHandle_t video_tx_queue;

    // Send mutex
    SemaphoreHandle_t send_mutex = nullptr;

    // Peer callbacks
    PeerCallbacks callbacks;

    // Peer handle
    esp_peer_handle_t client_peer = nullptr;

    // Data channels map
    std::unordered_map<uint16_t, PeerDataChannelMeta> data_channels;
    std::mutex data_channels_mutex;

    // Peer connected flag
    bool peer_connected = false;

    // Enable camera flag
    bool enable_camera = false;

    // Camera FPS
    uint8_t camera_fps = 15;

    // Peer state handler
    static int OnStateHandler(esp_peer_state_t state, void *ctx);
    static int OnMessageHandler(esp_peer_msg_t *msg, void *ctx);
    static int OnVideoInfoHandler(esp_peer_video_stream_info_t *info, void *ctx);
    static int OnVideoDataHandler(esp_peer_video_frame_t *frame, void *ctx);
    static int OnAudioInfoHandler(esp_peer_audio_stream_info_t *info, void *ctx);
    static int OnAudioDataHandler(esp_peer_audio_frame_t *frame, void *ctx);
    static int OnDataChannelOpenHandler(esp_peer_data_channel_info_t *ch, void *ctx);
    static int OnDataChannelHandler(esp_peer_data_frame_t *frame, void *ctx);
    static int OnDataChannelCloseHandler(esp_peer_data_channel_info_t *ch, void *ctx);

    // Peer task
    bool peer_task_running = false;
    TaskHandle_t peer_task_handle = nullptr;
    static void PeerTask(void *param);

    // Peer send audio task
    bool peer_send_audio_task_running = false;
    TaskHandle_t peer_send_audio_task_handle = nullptr;
    static void PeerSendAudioTask(void *arg);

    // Peer send video task
    bool peer_send_video_task_running = false;
    TaskHandle_t peer_send_video_task_handle = nullptr;
    static void PeerSendVideoTask(void *arg);

public:
    // Constructor and Destructor
    PeerBasic();
    ~PeerBasic();

    // Get the singleton instance of the PeerBasic class
    static PeerBasic &Instance()
    {
        static PeerBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    PeerBasic(const PeerBasic &) = delete;
    PeerBasic &operator=(const PeerBasic &) = delete;

    // Create peer method
    esp_err_t CreatePeer(const std::vector<std::string> &stun_urls);

    // Peer connect method
    void PeerConnect(const std::vector<std::string> &stun_urls);

    // Set peer answer method
    void SetPeerAnswer(const std::string &answer_json);

    // Set candidate method
    void SetPeerCandidate(const std::string &candidate_json);

    // Create data channels method
    void CreatePeerDataChannels(void);

    // Update peer connected state
    void UpdatePeerConnectedState(bool connected);

    // Send video frame method
    esp_err_t SendVideoFrame(const esp_peer_video_frame_t *frame);

    // Send audio frame method
    esp_err_t SendAudioFrame(const esp_peer_audio_frame_t *frame);

    // Send data channel message method
    esp_err_t SendDataChannelMessage(esp_peer_data_channel_type_t type, std::string label, const uint8_t *data, int size);

    // Set peer callbacks
    void SetCallbacks(PeerCallbacks &cb);
};

#endif