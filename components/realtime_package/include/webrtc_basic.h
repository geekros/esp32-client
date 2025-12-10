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

#ifndef REALTIME_WEBRTC_H
#define REALTIME_WEBRTC_H

// Include standard headers
#include <string>
#include <vector>
#include <functional>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include cJSON headers
#include "cJSON.h"

// Include ESP Peer headers
#include "esp_peer.h"
#include "esp_peer_default.h"

// Include headers
#include "signaling_basic.h"

// WebRTCBasic class definition
class WebRTCBasic
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Peer handle
    esp_peer_handle_t peer_ = nullptr;

    // Peer state variables
    bool peer_running_ = false;
    bool peer_stopped_ = false;
    TaskHandle_t peer_task_handle_ = nullptr;

    TaskHandle_t audio_task_handle_ = nullptr;
    static void AudioSendTask(void *arg);

    // STUN and TURN server info
    std::vector<std::string> stun_urls_;
    std::vector<std::string> turn_urls_;
    std::string turn_username_;
    std::string turn_credential_;

    // Local SDP sent flag
    bool local_sdp_sent_ = false;

    // Static handler methods
    static int PeerStateHandler(esp_peer_state_t state, void *ctx);
    static int PeerMsgHandler(esp_peer_msg_t *msg, void *ctx);
    static int PeerVideoInfoHandler(esp_peer_video_stream_info_t *info, void *ctx);
    static int PeerAudioInfoHandler(esp_peer_audio_stream_info_t *info, void *ctx);
    static int PeerVideoDataHandler(esp_peer_video_frame_t *frame, void *ctx);
    static int PeerAudioDataHandler(esp_peer_audio_frame_t *frame, void *ctx);
    static int PeerDataHandler(esp_peer_data_frame_t *frame, void *ctx);

    // Peer main task
    static void PeerMainTask(void *arg);

    // Start connection
    void StartConnection();

    // Create data channels
    void CreateDataChannels();

public:
    // Constructor and destructor
    WebRTCBasic();
    ~WebRTCBasic();

    // Get the singleton instance of the WebRTCBasic class
    static WebRTCBasic &Instance()
    {
        static WebRTCBasic instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    WebRTCBasic(const WebRTCBasic &) = delete;
    WebRTCBasic &operator=(const WebRTCBasic &) = delete;

    // Handle signaling connected
    void OnSignalingConnected(const std::vector<std::string> &stun_urls, const std::vector<std::string> &turn_urls, const std::string &turn_username, const std::string &turn_credential);

    // Handle signaling answer
    void OnSignalingAnswer(const std::string &answer_json);

    // Handle signaling candidate
    void OnSignalingCandidate(const std::string &candidate_json);

    // Initialization method
    esp_err_t Init(const std::vector<std::string> &stun_urls, const std::vector<std::string> &turn_urls, const std::string &turn_username, const std::string &turn_credential);
};

#endif
