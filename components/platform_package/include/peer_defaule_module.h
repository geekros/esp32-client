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

#ifndef PEER_DEFAULE_MODULE_H
#define PEER_DEFAULE_MODULE_H

// Include standard headers
#include <string>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include peer module header
#include "peer_module.h"

// Define default data channel configuration structure
typedef struct
{
    uint16_t cache_timeout;   /*!< Data channel frame keep timeout (unit ms) default: 5000ms if set to 0 */
    uint32_t send_cache_size; /*!< Cache size for outgoing data channel packets (unit Bytes)
                                  default: 100kB if set to 0 */
    uint32_t recv_cache_size; /*!< Cache size for incoming data channel packets (unit Bytes)
                                  default: 100kB if set to 0 */
} esp_peer_default_data_ch_cfg_t;

// Define default jitter buffer configuration structure
typedef struct
{
    uint16_t cache_timeout; /*!< Maximum timeout to keep the received RTP packet (unit ms) default: 100ms if set to 0 */
    uint16_t resend_delay;  /*!< Not resend until resend delay reached (unit ms) default: 20ms if set to 0*/
    uint32_t cache_size;    /*!< Cache size for incoming data channel frame (unit Bytes)
                                For audio jitter buffer default: 100kB if set to 0
                                For video jitter buffer default: 400kB if set to 0 */
} esp_peer_default_jitter_cfg_t;

// Define default RTP configuration structure
typedef struct
{
    esp_peer_default_jitter_cfg_t audio_recv_jitter; /*!< Audio jitter buffer configuration */
    esp_peer_default_jitter_cfg_t video_recv_jitter; /*!< Video jitter buffer configuration */
    uint32_t send_pool_size;                         /*!< Send pool size for outgoing RTP packets (unit Bytes)
                                                          default: 400kB if set to 0 */
    uint32_t send_queue_num;                         /*!< Maximum queue number to hold outgoing RTP packet metadata info
                                                          default: 256 */
    uint16_t max_resend_count;                       /*!< Maximum resend count for one RTP packet
                                                          default: 3 times */
} esp_peer_default_rtp_cfg_t;

// Define default peer configuration structure
typedef struct
{
    uint16_t agent_recv_timeout;                /*!< ICE agent receive timeout setting (unit ms)
                                                     default: 100ms if set to 0
                                                     Some STUN/TURN server reply message slow increase this value */
    esp_peer_default_data_ch_cfg_t data_ch_cfg; /*!< Configuration of data channel */
    esp_peer_default_rtp_cfg_t rtp_cfg;         /*!< Configuration of RTP buffer */
} esp_peer_default_cfg_t;

/**
 * @brief  Get default peer connection implementation
 *
 * @return
 *       - NULL    No default implementation, or not enough memory
 *       - Others  Default implementation
 */
const esp_peer_ops_t *esp_peer_get_default_impl(void);

#endif