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
#include "realtime_basic.h"

// Define log tag
#define TAG "[client:components:realtime:basic]"

// Constructor
RealtimeBasic::RealtimeBasic()
{
    // Create FreeRTOS event group
    event_group = xEventGroupCreate();
}

// Destructor
RealtimeBasic::~RealtimeBasic()
{
    // Delete event group
    if (event_group)
    {
        // Delete event group
        vEventGroupDelete(event_group);
    }
}

// Realtime connect method
void RealtimeBasic::RealtimeConnect(void)
{
    // Get access token
    response_access_token_t token_response = RealtimeAuthorize::Instance().Request();
    if (strlen(token_response.access_token) > 0)
    {
        // Convert access token to string
        std::string token_str(token_response.access_token);
        std::string masked = UtilsBasic::MaskSection(token_str, 20, token_str.size() - 30);

        // Log access token info
        ESP_LOGI(TAG, "AccessToken %s, Time: %d", masked.c_str(), token_response.time);

        // Set system time
        SystemTime::Instance().SetTimeSec(token_response.time);

        // Start signaling connection
        auto &signaling = SignalingBasic::Instance();

        // Define signaling callbacks
        SignalingCallbacks signaling_callbacks;

        // Set connected callback
        signaling_callbacks.on_connected_callback = [this]()
        {
            // Invoke callback
            if (callbacks.on_signaling_calledback)
            {
                // Notify signaling connected event
                callbacks.on_signaling_calledback("signaling:connected", "");
            }
        };

        // Set data callback
        signaling_callbacks.on_data_callback = [this](const char *data, size_t len, bool binary)
        {
            cJSON *root = cJSON_Parse(data);
            if (root)
            {
                // Get event item
                cJSON *event = cJSON_GetObjectItem(root, "event");

                // Get data item
                cJSON *data_obj = cJSON_GetObjectItem(root, "data");

                // Invoke callback
                if (callbacks.on_signaling_calledback)
                {
                    // Notify signaling disconnected event
                    callbacks.on_signaling_calledback(event->valuestring, data);
                }

                // Handle signaling connected event
                if (strcmp(event->valuestring, "signaling:connected") == 0)
                {
                    // Extract STUN and TURN server info from the message
                    std::vector<std::string> stun_urls;
                    std::vector<std::string> turn_urls;
                    std::string turn_username;
                    std::string turn_credential;

                    // Check if data_obj is an object
                    if (cJSON_IsObject(data_obj))
                    {
                        // Extract STUN URLs
                        cJSON *stuns = cJSON_GetObjectItem(data_obj, "stuns");
                        if (cJSON_IsObject(stuns))
                        {
                            // Extract URLs array
                            cJSON *urls = cJSON_GetObjectItem(stuns, "urls");
                            if (cJSON_IsArray(urls))
                            {
                                cJSON *item = nullptr;
                                cJSON_ArrayForEach(item, urls)
                                {
                                    if (cJSON_IsString(item) && item->valuestring)
                                    {
                                        stun_urls.emplace_back(item->valuestring);
                                    }
                                }
                            }
                        }
                    }

                    // Notify PeerBasic of signaling connection
                    WebRTCBasic::Instance().OnSignalingConnected(stun_urls, turn_urls, turn_username, turn_credential);

                    // Set event group bit for signaling connected
                    if (event_group)
                    {
                        // Set the signaling connected event bit
                        xEventGroupSetBits(event_group, REALTIME_EVENT_SIGNALING_CONNECTED);
                    }
                }

                // Handle signaling answer and candidate events
                if (strcmp(event->valuestring, "signaling:answer") == 0)
                {
                    // Check if data_obj is an object
                    if (!cJSON_IsObject(data_obj))
                    {
                        // Delete JSON root and return
                        cJSON_Delete(root);
                        return;
                    }

                    // Serialize answer JSON
                    char *answer_json = cJSON_PrintUnformatted(data_obj);
                    if (!answer_json)
                    {
                        // Delete JSON root and return
                        cJSON_Delete(root);
                        return;
                    }

                    // Notify PeerBasic of signaling answer
                    WebRTCBasic::Instance().OnSignalingAnswer(std::string(answer_json));

                    // Free serialized answer JSON
                    free(answer_json);

                    // Set event group bit for signaling answer
                    if (event_group)
                    {
                        // Set the signaling answer event bit
                        xEventGroupSetBits(event_group, REALTIME_EVENT_SIGNALING_ANSWER);
                    }
                }

                // Handle signaling candidate event
                if (strcmp(event->valuestring, "signaling:candidate") == 0)
                {
                    // Check  data_obj is an object
                    if (!cJSON_IsObject(data_obj))
                    {
                        // Delete JSON root and return
                        cJSON_Delete(root);
                        return;
                    }

                    // Serialize candidate JSON
                    char *candidate_json = cJSON_PrintUnformatted(data_obj);
                    if (!candidate_json)
                    {
                        // Delete JSON root and return
                        cJSON_Delete(root);
                        return;
                    }

                    // Notify PeerBasic of signaling candidate
                    WebRTCBasic::Instance().OnSignalingCandidate(std::string(candidate_json));

                    // Free serialized candidate JSON
                    free(candidate_json);

                    // Set event group bit for signaling candidate
                    if (event_group)
                    {
                        // Set the signaling candidate event bit
                        xEventGroupSetBits(event_group, REALTIME_EVENT_SIGNALING_CANDIDATE);
                    }
                }

                // Delete JSON root
                cJSON_Delete(root);
            }
        };

        // Set disconnected and error callbacks
        signaling_callbacks.on_disconnected_callback = [this]()
        {
            // Invoke callback
            if (callbacks.on_signaling_calledback)
            {
                // Notify signaling disconnected event
                callbacks.on_signaling_calledback("signaling:disconnected", "");
            }
        };

        // Set error callback
        signaling_callbacks.on_error_callback = [this](int error_code)
        {
            // Invoke callback
            if (callbacks.on_signaling_calledback)
            {
                // Notify signaling disconnected event
                callbacks.on_signaling_calledback("signaling:error", "");
            }
        };

        // Assign callbacks to signaling instance
        signaling.SetCallbacks(signaling_callbacks);

        // Assign callbacks to signaling instance
        signaling.Connection(token_str);

        // Create heartbeat task
        auto heartbeat = [](void *arg)
        {
            // Get WebSocket instance
            auto socket = SignalingBasic::Instance().GetSocket();

            // Heartbeat loop
            while (true)
            {
                // Check if socket is connected
                if (!socket->IsConnected())
                {
                    // Invoke callback
                    if (RealtimeBasic::Instance().callbacks.on_signaling_calledback)
                    {
                        // Notify signaling disconnected event
                        RealtimeBasic::Instance().callbacks.on_signaling_calledback("signaling:heartbeat:stopped", "");
                    }
                    break;
                }

                // Send signaling message
                SignalingBasic::Instance().Send("client:signaling:heartbeat", "heartbeat");

                // Wait for next heartbeat
                vTaskDelay(pdMS_TO_TICKS(15000));
            }

            // Delete task
            vTaskDelete(NULL);
        };

        // Create heartbeat task
        xTaskCreate(heartbeat, "realtime_signaling_heartbeat_task", 4096, NULL, 4, NULL);
    }
}

// Realtime reconnect method
void RealtimeBasic::RealtimeReconnect(void)
{
    // Stop current realtime connection
    RealtimeStop();

    // Start a new realtime connection
    RealtimeConnect();
}

// Realtime stop method
void RealtimeBasic::RealtimeStop(void)
{
    // Get WebSocket instance
    auto socket = SignalingBasic::Instance().GetSocket();

    // Check if socket is valid and connected
    if (socket && socket->IsConnected())
    {
        // Close WebSocket connection
        socket->Close();
    }
}

// Set Realtime basic callbacks
void RealtimeBasic::SetCallbacks(RealtimeCallbacks &cb)
{
    // Update callbacks
    callbacks = cb;
}