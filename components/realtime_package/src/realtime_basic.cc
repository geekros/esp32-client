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
        vEventGroupDelete(event_group);
    }
}

// Realtime start method
void RealtimeBasic::RealtimeStart(void)
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
            ESP_LOGI(TAG, "Signaling Connected");
        };

        // Set data callback
        signaling_callbacks.on_data_callback = [this](const char *data, size_t len, bool binary)
        {
            cJSON *root = cJSON_Parse(data);
            if (root)
            {
                // Get event item
                cJSON *event = cJSON_GetObjectItem(root, "event");

                ESP_LOGI(TAG, "Signaling Event: %s", event->valuestring);

                // Get data item
                cJSON *data_obj = cJSON_GetObjectItem(root, "data");

                // Handle signaling connected event
                if (strcmp(event->valuestring, "signaling:connected") == 0)
                {
                    ESP_LOGI(TAG, "Signaling Connection Established");

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
                                        ESP_LOGI(TAG, "STUN URL: %s", item->valuestring);
                                    }
                                }
                            }
                        }

                        // Extract TURN URLs, username, and credential
                        // cJSON *turns = cJSON_GetObjectItem(data_obj, "turns");
                        // if (cJSON_IsObject(turns))
                        // {
                        //     // Extract URLs array
                        //     cJSON *urls = cJSON_GetObjectItem(turns, "urls");
                        //     if (cJSON_IsArray(urls))
                        //     {
                        //         cJSON *item = nullptr;
                        //         cJSON_ArrayForEach(item, urls)
                        //         {
                        //             if (cJSON_IsString(item) && item->valuestring)
                        //             {
                        //                 turn_urls.emplace_back(item->valuestring);
                        //                 ESP_LOGI(TAG, "TURN URL: %s", item->valuestring);
                        //             }
                        //         }
                        //     }
                        //     cJSON *username = cJSON_GetObjectItem(turns, "username");
                        //     if (cJSON_IsString(username) && username->valuestring)
                        //     {
                        //         turn_username = username->valuestring;
                        //         ESP_LOGI(TAG, "TURN Username: %s", username->valuestring);
                        //     }
                        //     cJSON *credential = cJSON_GetObjectItem(turns, "credential");
                        //     if (cJSON_IsString(credential) && credential->valuestring)
                        //     {
                        //         turn_credential = credential->valuestring;
                        //         ESP_LOGI(TAG, "TURN Credential: %s", credential->valuestring);
                        //     }
                        // }
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Signaling Connected data is not an object");
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
                    ESP_LOGI(TAG, "Signaling Answer Received");

                    // Check if data_obj is an object
                    if (!cJSON_IsObject(data_obj))
                    {
                        ESP_LOGE(TAG, "signaling:answer 'data' is not object");
                        cJSON_Delete(root);
                        return;
                    }

                    // Serialize answer JSON
                    char *answer_json = cJSON_PrintUnformatted(data_obj);
                    if (!answer_json)
                    {
                        ESP_LOGE(TAG, "Failed to serialize answer json");
                        cJSON_Delete(root);
                        return;
                    }

                    ESP_LOGI(TAG, "Remote SDP Answer: %s", answer_json);

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
                    ESP_LOGI(TAG, "Signaling Candidate Received");

                    // Check  data_obj is an object
                    if (!cJSON_IsObject(data_obj))
                    {
                        ESP_LOGE(TAG, "signaling:candidate 'data' is not object");
                        cJSON_Delete(root);
                        return;
                    }

                    // Serialize candidate JSON
                    char *candidate_json = cJSON_PrintUnformatted(data_obj);
                    if (!candidate_json)
                    {
                        ESP_LOGE(TAG, "Failed to serialize candidate json");
                        cJSON_Delete(root);
                        return;
                    }

                    ESP_LOGI(TAG, "Remote ICE Candidate: %s", candidate_json);

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
            ESP_LOGI(TAG, "Signaling Disconnected");
        };

        // Set error callback
        signaling_callbacks.on_error_callback = [this](int error_code)
        {
            ESP_LOGE(TAG, "Signaling Error: %d", error_code);
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
                if (!socket->IsConnected())
                {
                    ESP_LOGW(TAG, "Signaling WebSocket disconnected, stopping heartbeat");
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