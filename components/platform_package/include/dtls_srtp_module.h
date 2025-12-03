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

#ifndef DTLS_SRTP_MODULE_H
#define DTLS_SRTP_MODULE_H

// Include standard headers
#include <string>
#include <unistd.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/timing.h>
#include <srtp.h>
#include <arpa/inet.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// Include media module header
#include "media_module.h"

// Define DTLS-SRTP related constants
#define RSA_KEY_LENGTH 1024
#define SRTP_MASTER_KEY_LENGTH 16
#define SRTP_MASTER_SALT_LENGTH 14
#define DTLS_SRTP_KEY_MATERIAL_LENGTH 60
#define DTLS_SRTP_FINGERPRINT_LENGTH 160

// Define DTLS-SRTP macros
#define DTLS_SIGN_ONCE
#define DTLS_MTU_SIZE 1500

// Define macro for breaking on failure
#define BREAK_ON_FAIL(ret) \
    if (ret != 0)          \
    {                      \
        break;             \
    }

// Define DTLS-SRTP role enumeration
typedef enum
{
    DTLS_SRTP_ROLE_CLIENT,
    DTLS_SRTP_ROLE_SERVER
} dtls_srtp_role_t;

// Define DTLS-SRTP state enumeration
typedef enum
{
    DTLS_SRTP_STATE_NONE,
    DTLS_SRTP_STATE_INIT,
    DTLS_SRTP_STATE_HANDSHAKE,
    DTLS_SRTP_STATE_CONNECTED
} dtls_srtp_state_t;

// Define DTLS-SRTP context structure
typedef struct
{
    void *ctx;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_ssl_cookie_ctx cookie_ctx;
    mbedtls_x509_crt cert;
    mbedtls_pk_context pkey;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    dtls_srtp_role_t role;
    dtls_srtp_state_t state;
    srtp_policy_t remote_policy;
    srtp_policy_t local_policy;
    srtp_t srtp_in;
    srtp_t srtp_out;
    unsigned char remote_policy_key[SRTP_MASTER_KEY_LENGTH + SRTP_MASTER_SALT_LENGTH];
    unsigned char local_policy_key[SRTP_MASTER_KEY_LENGTH + SRTP_MASTER_SALT_LENGTH];
    char local_fingerprint[DTLS_SRTP_FINGERPRINT_LENGTH];
    char remote_fingerprint[DTLS_SRTP_FINGERPRINT_LENGTH];
    media_lib_mutex_handle_t lock;
    int (*udp_send)(void *ctx, const unsigned char *buf, size_t len);
    int (*udp_recv)(void *ctx, unsigned char *buf, size_t len);
} dtls_srtp_t;

// Define DTLS-SRTP configuration structure
typedef struct
{
    dtls_srtp_role_t role;
    int (*udp_send)(void *ctx, const unsigned char *buf, size_t len);
    int (*udp_recv)(void *ctx, unsigned char *buf, size_t len);
    void *ctx;
} dtls_srtp_cfg_t;

// RTCDtlsSrtpModule class definition
class RTCDtlsSrtpModule
{
private:
    // Event group handle
    EventGroupHandle_t event_group;

    // Function to compute X.509 certificate fingerprint
    static void dtls_srtp_x509_digest(const mbedtls_x509_crt *crt, char *buf);

    // Function to check SRTP support
    static int check_srtp(bool init);

    // Function to generate self-signed certificate
    static int dtls_srtp_selfsign_cert(dtls_srtp_t *dtls_srtp);

    // Function to try generating certificate
    static int dtls_srtp_try_gen_cert(dtls_srtp_t *dtls_srtp);

    // Key derivation function
    static void dtls_srtp_key_derivation(void *context, mbedtls_ssl_key_export_type secret_type, const unsigned char *secret, size_t secret_len, const unsigned char client_random[32], const unsigned char server_random[32], mbedtls_tls_prf_types tls_prf_type);

    // Function to perform DTLS-SRTP handshake
    static int dtls_srtp_do_handshake(dtls_srtp_t *dtls_srtp);

    // Server handshake function
    static int dtls_srtp_handshake_server(dtls_srtp_t *dtls_srtp);

    // Client handshake function
    static int dtls_srtp_handshake_client(dtls_srtp_t *dtls_srtp);

public:
    // Constructor and Destructor
    RTCDtlsSrtpModule();
    ~RTCDtlsSrtpModule();

    // Get the singleton instance of the RTCDtlsSrtpModule class
    static RTCDtlsSrtpModule &Instance()
    {
        static RTCDtlsSrtpModule instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    RTCDtlsSrtpModule(const RTCDtlsSrtpModule &) = delete;
    RTCDtlsSrtpModule &operator=(const RTCDtlsSrtpModule &) = delete;

    // Generate self-signed certificate
    int dtls_srtp_gen_cert(void);

    // Initialize DTLS-SRTP context
    dtls_srtp_t *dtls_srtp_init(dtls_srtp_cfg_t *cfg);

    // Get local fingerprint
    char *dtls_srtp_get_local_fingerprint(dtls_srtp_t *dtls_srtp);

    // handle DTLS-SRTP handshake
    int dtls_srtp_handshake(dtls_srtp_t *dtls_srtp);

    // Reset DTLS-SRTP session
    void dtls_srtp_reset_session(dtls_srtp_t *dtls_srtp, dtls_srtp_role_t role);

    // Get role of DTLS-SRTP context
    dtls_srtp_role_t dtls_srtp_get_role(dtls_srtp_t *dtls_srtp);

    // Write and read functions
    int dtls_srtp_write(dtls_srtp_t *dtls_srtp, const uint8_t *buf, size_t len);
    int dtls_srtp_read(dtls_srtp_t *dtls_srtp, uint8_t *buf, size_t len);

    // Probe function to check if packet is DTLS-SRTP
    bool dtls_srtp_probe(uint8_t *buf);

    // RTP/RTCP packet encryption and decryption functions
    void dtls_srtp_encrypt_rtp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int buf_size, int *bytes);

    // RTP/RTCP packet decryption functions
    int dtls_srtp_decrypt_rtp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int *bytes);

    // RTCP packet encryption function
    void dtls_srtp_encrypt_rctp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int buf_size, int *bytes);

    // RTCP packet decryption function
    int dtls_srtp_decrypt_rtcp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int *bytes);

    // Deinitialize DTLS-SRTP context
    void dtls_srtp_deinit(dtls_srtp_t *dtls_srtp);
};

#endif