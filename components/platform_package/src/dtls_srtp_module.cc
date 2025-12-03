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
#include "dtls_srtp_module.h"

// Define log tag
#define TAG "[client:components:webrtc:dtls_srtp]"

// Forward declaration of measurement functions
extern void measure_start(const char *tag);
extern void measure_stop(const char *tag);

// Define default SRTP profiles
static const mbedtls_ssl_srtp_profile default_profiles[] = {
    MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_80, MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_32,
    MBEDTLS_TLS_SRTP_NULL_HMAC_SHA1_80, MBEDTLS_TLS_SRTP_NULL_HMAC_SHA1_32,
    MBEDTLS_TLS_SRTP_UNSET};

static bool already_signed = false;
#ifdef DTLS_SIGN_ONCE
static mbedtls_ctr_drbg_context signed_ctr_drbg;
static mbedtls_x509_crt signed_cert;
static mbedtls_pk_context signed_pkey;
static mbedtls_entropy_context signed_entropy;
#endif

// Constructor
RTCDtlsSrtpModule::RTCDtlsSrtpModule()
{
    // Create event group
    event_group = xEventGroupCreate();
}

// Destructor
RTCDtlsSrtpModule::~RTCDtlsSrtpModule()
{
    // Delete event group
    if (event_group != nullptr)
    {
        vEventGroupDelete(event_group);
        event_group = nullptr;
    }
}

// Function to compute X.509 certificate fingerprint
void RTCDtlsSrtpModule::dtls_srtp_x509_digest(const mbedtls_x509_crt *crt, char *buf)
{
    // Compute SHA-256 digest of the certificate
    unsigned char digest[32];
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0);
    mbedtls_sha256_update(&sha256_ctx, crt->raw.p, crt->raw.len);
    mbedtls_sha256_finish(&sha256_ctx, (unsigned char *)digest);
    mbedtls_sha256_free(&sha256_ctx);

    // Convert digest to hexadecimal string
    for (int i = 0; i < sizeof(digest); i++)
    {
        snprintf(buf, 4, "%.2X:", digest[i]);
        buf += 3;
    }

    // Null-terminate the string
    *(--buf) = '\0';
}

// Function to check SRTP support
int RTCDtlsSrtpModule::check_srtp(bool init)
{
    // Static variable to track initialization count
    static int init_count = 0;

    // Initialize or shutdown SRTP based on the init parameter
    if (init)
    {
        if (init_count == 0)
        {
            srtp_err_status_t ret = srtp_init();
            if (ret != srtp_err_status_ok)
            {
                return -1;
            }

            init_count++;
            init_count++;
        }
    }
    else
    {
        if (init_count)
        {
            init_count--;
            if (init_count == 0)
            {
                srtp_shutdown();
            }
        }
    }
    return 0;
}

// Function to generate self-signed certificate
int RTCDtlsSrtpModule::dtls_srtp_selfsign_cert(dtls_srtp_t *dtls_srtp)
{
    // Variable declarations
    int ret;
    mbedtls_x509write_cert crt;

    // Allocate memory for certificate buffer
    unsigned char *cert_buf = (unsigned char *)malloc(RSA_KEY_LENGTH);
    if (cert_buf == NULL)
    {
        // Return failure
        return -1;
    }

    // Generate self-signed certificate
    const char *pers = "dtls_srtp";

    // Initialize structures
    mbedtls_x509write_crt_init(&crt);
    mbedtls_ctr_drbg_seed(&dtls_srtp->ctr_drbg, mbedtls_entropy_func, &dtls_srtp->entropy, (const unsigned char *)pers, strlen(pers));
    mbedtls_pk_setup(&dtls_srtp->pkey, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    mbedtls_rsa_gen_key(mbedtls_pk_rsa(dtls_srtp->pkey), mbedtls_ctr_drbg_random, &dtls_srtp->ctr_drbg, RSA_KEY_LENGTH, 65537);

    // Set certificate parameters
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_name(&crt, "CN=dtls_srtp");
    mbedtls_x509write_crt_set_issuer_name(&crt, "CN=dtls_srtp");

#if MBEDTLS_VERSION_MAJOR == 3 && MBEDTLS_VERSION_MINOR >= 4 || MBEDTLS_VERSION_MAJOR >= 4
    unsigned char *serial = (unsigned char *)"1";
    size_t serial_len = 1;
    ret = mbedtls_x509write_crt_set_serial_raw(&crt, serial, serial_len);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "mbedtls_x509write_crt_set_serial_raw failed: -0x%04X", -ret);
    }
#else
    mbedtls_mpi serial;
    mbedtls_mpi_init(&serial);
    mbedtls_mpi_fill_random(&serial, 16, mbedtls_ctr_drbg_random, &dtls_srtp->ctr_drbg);
    mbedtls_x509write_crt_set_serial(&crt, &serial);
    mbedtls_mpi_free(&serial);
#endif

    // Set validity period
    mbedtls_x509write_crt_set_validity(&crt, "20230101000000", "20280101000000");
    mbedtls_x509write_crt_set_subject_key(&crt, &dtls_srtp->pkey);
    mbedtls_x509write_crt_set_issuer_key(&crt, &dtls_srtp->pkey);

    // Write certificate to PEM format
    ret = mbedtls_x509write_crt_pem(&crt, cert_buf, RSA_KEY_LENGTH, mbedtls_ctr_drbg_random, &dtls_srtp->ctr_drbg);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "mbedtls_x509write_crt_pem failed");
    }

    // Parse the generated certificate
    mbedtls_x509_crt_parse(&dtls_srtp->cert, cert_buf, RSA_KEY_LENGTH);
    mbedtls_x509write_crt_free(&crt);

    // Free allocated memory and return
    free(cert_buf);

    // Return success
    return ret;
}

// Function to try generating certificate
int RTCDtlsSrtpModule::dtls_srtp_try_gen_cert(dtls_srtp_t *dtls_srtp)
{
    // Variable declarations
    int ret = 0;

#ifdef DTLS_SIGN_ONCE
    if (already_signed)
    {
        dtls_srtp->ctr_drbg = signed_ctr_drbg;
        dtls_srtp->cert = signed_cert;
        dtls_srtp->pkey = signed_pkey;
        dtls_srtp->entropy = signed_entropy;
        // Return success
        return 0;
    }
#endif

    // Initialize DTLS-SRTP structures
    mbedtls_x509_crt_init(&dtls_srtp->cert);
    mbedtls_pk_init(&dtls_srtp->pkey);
    mbedtls_entropy_init(&dtls_srtp->entropy);
    mbedtls_ctr_drbg_init(&dtls_srtp->ctr_drbg);

    // Generate self-signed certificate
    ret = dtls_srtp_selfsign_cert(dtls_srtp);
    if (ret != 0)
    {
        // Return failure
        return ret;
    }

#ifdef DTLS_SIGN_ONCE
    already_signed = true;
    signed_ctr_drbg = dtls_srtp->ctr_drbg;
    signed_cert = dtls_srtp->cert;
    signed_pkey = dtls_srtp->pkey;
    signed_entropy = dtls_srtp->entropy;
#endif

    // Return success
    return 0;
}

// Function to generate self-signed certificate
int RTCDtlsSrtpModule::dtls_srtp_gen_cert(void)
{
#ifdef DTLS_SIGN_ONCE
    // Allocate memory for DTLS-SRTP context
    dtls_srtp_t *dtls_srtp = (dtls_srtp_t *)RTCMediaModule::Instance().media_lib_calloc(1, sizeof(dtls_srtp_t));
    if (dtls_srtp == NULL)
    {
        return -1;
    }

    // Generate certificate
    if (already_signed)
    {
        mbedtls_x509_crt_free(&signed_cert);
        mbedtls_pk_free(&signed_pkey);
        mbedtls_ctr_drbg_free(&signed_ctr_drbg);
        already_signed = false;
    }

    // Generate certificate
    int ret = dtls_srtp_try_gen_cert(dtls_srtp);

    // Deinitialize DTLS-SRTP context
    dtls_srtp_deinit(dtls_srtp);

    // Return result
    return ret;
#else
    // DTLS_SIGN_ONCE not defined, return failure
    return -1;
#endif
}

// Initialize DTLS-SRTP context
dtls_srtp_t *RTCDtlsSrtpModule::dtls_srtp_init(dtls_srtp_cfg_t *cfg)
{
    // Allocate memory for DTLS-SRTP context
    dtls_srtp_t *dtls_srtp = (dtls_srtp_t *)RTCMediaModule::Instance().media_lib_calloc(1, sizeof(dtls_srtp_t));
    if (dtls_srtp == NULL)
    {
        return NULL;
    }

    // Check srtp support
    int ret = check_srtp(true);

    // Do initialization steps
    do
    {
        // On failure, break
        BREAK_ON_FAIL(ret);

        // Media library mutex create
        RTCMediaModule::Instance().media_lib_mutex_create(&dtls_srtp->lock);

        // Set role and callbacks
        dtls_srtp->role = cfg->role;
        dtls_srtp->state = DTLS_SRTP_STATE_INIT;
        dtls_srtp->ctx = cfg->ctx;
        dtls_srtp->udp_send = cfg->udp_send;
        dtls_srtp->udp_recv = cfg->udp_recv;

        // Initialize mbedtls components
        mbedtls_ssl_config_init(&dtls_srtp->conf);
        mbedtls_ssl_init(&dtls_srtp->ssl);
        ret = dtls_srtp_try_gen_cert(dtls_srtp);
        BREAK_ON_FAIL(ret);

        // Configure DTLS-SRTP
        mbedtls_ssl_conf_ca_chain(&dtls_srtp->conf, &dtls_srtp->cert, NULL);
        ret = mbedtls_ssl_conf_own_cert(&dtls_srtp->conf, &dtls_srtp->cert, &dtls_srtp->pkey);
        mbedtls_ssl_conf_rng(&dtls_srtp->conf, mbedtls_ctr_drbg_random, &dtls_srtp->ctr_drbg);
        BREAK_ON_FAIL(ret);

        // Configure DTLS parameters
        mbedtls_ssl_conf_read_timeout(&dtls_srtp->conf, 1000);
        mbedtls_ssl_conf_handshake_timeout(&dtls_srtp->conf, 1000, 6000);
        // mbedtls_ssl_conf_dtls_replay_window(&dtls_srtp->conf, 1000);
        mbedtls_ssl_conf_dtls_anti_replay(&dtls_srtp->conf, MBEDTLS_SSL_ANTI_REPLAY_DISABLED);

        // Configure DTLS based on role
        if (dtls_srtp->role == DTLS_SRTP_ROLE_SERVER)
        {
            // Server configuration
            ret = mbedtls_ssl_config_defaults(&dtls_srtp->conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
            mbedtls_ssl_cookie_init(&dtls_srtp->cookie_ctx);
            mbedtls_ssl_cookie_setup(&dtls_srtp->cookie_ctx, mbedtls_ctr_drbg_random, &dtls_srtp->ctr_drbg);
            mbedtls_ssl_conf_dtls_cookies(&dtls_srtp->conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &dtls_srtp->cookie_ctx);
        }
        else
        {
            // Client configuration
            ret = mbedtls_ssl_config_defaults(&dtls_srtp->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
        }
        BREAK_ON_FAIL(ret);

        // Set up DTLS-SRTP parameters
        dtls_srtp_x509_digest(&dtls_srtp->cert, dtls_srtp->local_fingerprint);
        ret = mbedtls_ssl_conf_dtls_srtp_protection_profiles(&dtls_srtp->conf, default_profiles);
        BREAK_ON_FAIL(ret);

        // Set MKI value support to unsupported
        mbedtls_ssl_conf_srtp_mki_value_supported(&dtls_srtp->conf, MBEDTLS_SSL_DTLS_SRTP_MKI_UNSUPPORTED);
        ret = mbedtls_ssl_setup(&dtls_srtp->ssl, &dtls_srtp->conf);
        BREAK_ON_FAIL(ret);
        mbedtls_ssl_set_mtu(&dtls_srtp->ssl, DTLS_MTU_SIZE);
        return dtls_srtp;
    } while (0);

    // Deinitialize DTLS-SRTP context on failure
    dtls_srtp_deinit(dtls_srtp);

    // Return NULL on failure
    return NULL;
}

// Get local fingerprint
char *RTCDtlsSrtpModule::dtls_srtp_get_local_fingerprint(dtls_srtp_t *dtls_srtp)
{
    // Return local fingerprint
    return dtls_srtp->local_fingerprint;
}

// Deinitialize DTLS-SRTP context
void RTCDtlsSrtpModule::dtls_srtp_deinit(dtls_srtp_t *dtls_srtp)
{
    // Check for null context
    if (dtls_srtp->state == DTLS_SRTP_STATE_NONE)
    {
        return;
    }

    // Free SRTP sessions
    mbedtls_ssl_free(&dtls_srtp->ssl);
    mbedtls_ssl_config_free(&dtls_srtp->conf);

    // Free certificate and keys if not already signed
    if (already_signed == false)
    {
        mbedtls_entropy_free(&dtls_srtp->entropy);
        mbedtls_x509_crt_free(&dtls_srtp->cert);
        mbedtls_pk_free(&dtls_srtp->pkey);
        mbedtls_ctr_drbg_free(&dtls_srtp->ctr_drbg);
    }

    // Free cookie context if server
    if (dtls_srtp->role == DTLS_SRTP_ROLE_SERVER)
    {
        mbedtls_ssl_cookie_free(&dtls_srtp->cookie_ctx);
    }

    // Deallocate SRTP sessions
    if (dtls_srtp->srtp_in)
    {
        srtp_dealloc(dtls_srtp->srtp_in);
        dtls_srtp->srtp_in = NULL;
    }

    // Deallocate SRTP sessions
    if (dtls_srtp->srtp_out)
    {
        srtp_dealloc(dtls_srtp->srtp_out);
        dtls_srtp->srtp_out = NULL;
    }

    // Free mutex
    if (dtls_srtp->lock)
    {
        RTCMediaModule::Instance().media_lib_mutex_destroy(dtls_srtp->lock);
    }

    // Check srtp support
    check_srtp(false);

    // Set state to none
    dtls_srtp->state = DTLS_SRTP_STATE_NONE;
}

// Key derivation function
void RTCDtlsSrtpModule::dtls_srtp_key_derivation(void *context, mbedtls_ssl_key_export_type secret_type, const unsigned char *secret, size_t secret_len, const unsigned char client_random[32], const unsigned char server_random[32], mbedtls_tls_prf_types tls_prf_type)
{
    // Declare variables
    dtls_srtp_t *dtls_srtp = (dtls_srtp_t *)context;
    int ret;
    const char *dtls_srtp_label = "EXTRACTOR-dtls_srtp";
    unsigned char randbytes[64];
    uint8_t key_material[DTLS_SRTP_KEY_MATERIAL_LENGTH];

    // Prepare random bytes
    memcpy(randbytes, client_random, 32);
    memcpy(randbytes + 32, server_random, 32);

    // Derive key material using TLS PRF
    if ((ret = mbedtls_ssl_tls_prf(tls_prf_type, secret, secret_len, dtls_srtp_label, randbytes, sizeof(randbytes), key_material, sizeof(key_material))) != 0)
    {
        return;
    }

    // Set remote policy
    memset(&dtls_srtp->remote_policy, 0, sizeof(dtls_srtp->remote_policy));
    srtp_crypto_policy_set_rtp_default(&dtls_srtp->remote_policy.rtp);
    srtp_crypto_policy_set_rtcp_default(&dtls_srtp->remote_policy.rtcp);

    // Set remote policy key
    memcpy(dtls_srtp->remote_policy_key, key_material, SRTP_MASTER_KEY_LENGTH);
    memcpy(dtls_srtp->remote_policy_key + SRTP_MASTER_KEY_LENGTH, key_material + SRTP_MASTER_KEY_LENGTH + SRTP_MASTER_KEY_LENGTH, SRTP_MASTER_SALT_LENGTH);

    // Set remote policy
    dtls_srtp->remote_policy.ssrc.type = ssrc_any_inbound;
    dtls_srtp->remote_policy.key = dtls_srtp->remote_policy_key;
    dtls_srtp->remote_policy.next = NULL;
    srtp_t *send_session = (dtls_srtp->role == DTLS_SRTP_ROLE_SERVER) ? &dtls_srtp->srtp_in : &dtls_srtp->srtp_out;
    ret = srtp_create(send_session, &dtls_srtp->remote_policy);
    if (ret != srtp_err_status_ok)
    {
        return;
    }

    // Set local policy
    memset(&dtls_srtp->local_policy, 0, sizeof(dtls_srtp->local_policy));
    srtp_crypto_policy_set_rtp_default(&dtls_srtp->local_policy.rtp);
    srtp_crypto_policy_set_rtcp_default(&dtls_srtp->local_policy.rtcp);

    // Set local policy key
    memcpy(dtls_srtp->local_policy_key, key_material + SRTP_MASTER_KEY_LENGTH, SRTP_MASTER_KEY_LENGTH);
    memcpy(dtls_srtp->local_policy_key + SRTP_MASTER_KEY_LENGTH, key_material + SRTP_MASTER_KEY_LENGTH + SRTP_MASTER_KEY_LENGTH + SRTP_MASTER_SALT_LENGTH, SRTP_MASTER_SALT_LENGTH);

    // Set local policy
    dtls_srtp->local_policy.ssrc.type = ssrc_any_outbound;
    dtls_srtp->local_policy.key = dtls_srtp->local_policy_key;
    dtls_srtp->local_policy.next = NULL;
    srtp_t *recv_session = (dtls_srtp->role == DTLS_SRTP_ROLE_SERVER) ? &dtls_srtp->srtp_out : &dtls_srtp->srtp_in;
    ret = srtp_create(recv_session, &dtls_srtp->local_policy);
    if (ret != srtp_err_status_ok)
    {
        return;
    }

    // Set state to connected
    dtls_srtp->state = DTLS_SRTP_STATE_CONNECTED;
}

// Function to perform DTLS-SRTP handshake
int RTCDtlsSrtpModule::dtls_srtp_do_handshake(dtls_srtp_t *dtls_srtp)
{
    // Variable declarations
    int ret;
    static mbedtls_timing_delay_context timer;

    // Set timer and callbacks
    mbedtls_ssl_set_timer_cb(&dtls_srtp->ssl, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
    mbedtls_ssl_set_export_keys_cb(&dtls_srtp->ssl, dtls_srtp_key_derivation, dtls_srtp);
    mbedtls_ssl_set_bio(&dtls_srtp->ssl, dtls_srtp, dtls_srtp->udp_send, dtls_srtp->udp_recv, NULL);

    // Perform handshake
    do
    {
        // Perform handshake step
        ret = mbedtls_ssl_handshake(&dtls_srtp->ssl);
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    // Reset session on close notify
    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
        // Reset SSL session
        mbedtls_ssl_session_reset(&dtls_srtp->ssl);
    }

    // Return result
    return ret;
}

// Server handshake function
int RTCDtlsSrtpModule::dtls_srtp_handshake_server(dtls_srtp_t *dtls_srtp)
{
    // Variable declarations
    int ret;

    // Loop to handle HelloVerifyRequest
    while (1)
    {
        // Dummy client IP for cookie verification
        unsigned char client_ip[] = "test";

        // Reset SSL session
        mbedtls_ssl_session_reset(&dtls_srtp->ssl);
        mbedtls_ssl_set_client_transport_id(&dtls_srtp->ssl, client_ip, sizeof(client_ip));

        // Perform handshake
        ret = dtls_srtp_do_handshake(dtls_srtp);
        if (ret != MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED)
        {
            break;
        }
    }

    // Return result
    return ret;
}

// Client handshake function
int RTCDtlsSrtpModule::dtls_srtp_handshake_client(dtls_srtp_t *dtls_srtp)
{
    // Perform handshake
    int ret = dtls_srtp_do_handshake(dtls_srtp);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "CLient handshake fail ret -0x%.4x", (unsigned int)-ret);
        return -1;
    }

    // Verify peer certificate
    int flags;
    if ((flags = mbedtls_ssl_get_verify_result(&dtls_srtp->ssl)) != 0)
    {
#if !defined(MBEDTLS_X509_REMOVE_INFO)
        char vrfy_buf[512];
#endif
#if !defined(MBEDTLS_X509_REMOVE_INFO)
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
#endif
    }

    // Return result
    return ret;
}

// Function to perform DTLS-SRTP handshake based on role
int RTCDtlsSrtpModule::dtls_srtp_handshake(dtls_srtp_t *dtls_srtp)
{
    // Variable declarations
    int ret;

    // Perform handshake based on role
    if (dtls_srtp->role == DTLS_SRTP_ROLE_SERVER)
    {
        // Server handshake
        ret = dtls_srtp_handshake_server(dtls_srtp);
    }
    else
    {
        // Client handshake
        ret = dtls_srtp_handshake_client(dtls_srtp);
    }

    // Log handshake success
    if (ret == 0)
    {
        ESP_LOGI(TAG, "%s handshake success", dtls_srtp->role == DTLS_SRTP_ROLE_SERVER ? "Server" : "Client");
    }

    // Get DTLS-SRTP negotiation result
    mbedtls_dtls_srtp_info dtls_srtp_negotiation_result;
    mbedtls_ssl_get_dtls_srtp_negotiation_result(&dtls_srtp->ssl, &dtls_srtp_negotiation_result);

    // Log negotiated SRTP profile
    return ret;
}

// Function to reset DTLS-SRTP session
void RTCDtlsSrtpModule::dtls_srtp_reset_session(dtls_srtp_t *dtls_srtp, dtls_srtp_role_t role)
{
    // Check if session is connected
    if (dtls_srtp->state == DTLS_SRTP_STATE_CONNECTED)
    {
        srtp_dealloc(dtls_srtp->srtp_in);
        dtls_srtp->srtp_in = NULL;
        srtp_dealloc(dtls_srtp->srtp_out);
        dtls_srtp->srtp_out = NULL;
        mbedtls_ssl_session_reset(&dtls_srtp->ssl);
    }

    // Update role if different
    if (role != dtls_srtp->role)
    {
        // Check for server role
        if (dtls_srtp->role == DTLS_SRTP_ROLE_SERVER)
        {
            // Free cookie context
            mbedtls_ssl_cookie_free(&dtls_srtp->cookie_ctx);
        }

        // Check for new server role
        if (role == DTLS_SRTP_ROLE_SERVER)
        {
            // Server need cookie verify
            mbedtls_ssl_config_defaults(&dtls_srtp->conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);

            // Initialize cookie context
            mbedtls_ssl_cookie_init(&dtls_srtp->cookie_ctx);
            mbedtls_ssl_cookie_setup(&dtls_srtp->cookie_ctx, mbedtls_ctr_drbg_random, &dtls_srtp->ctr_drbg);
            mbedtls_ssl_conf_dtls_cookies(&dtls_srtp->conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &dtls_srtp->cookie_ctx);
        }
        else
        {
            // CLient need skip verify server CA verify fingerprint later
            mbedtls_ssl_config_defaults(&dtls_srtp->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
            mbedtls_ssl_conf_authmode(&dtls_srtp->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        }

        // Reconfigure DTLS-SRTP parameters
        mbedtls_ssl_conf_dtls_srtp_protection_profiles(&dtls_srtp->conf, default_profiles);
        mbedtls_ssl_conf_srtp_mki_value_supported(&dtls_srtp->conf, MBEDTLS_SSL_DTLS_SRTP_MKI_UNSUPPORTED);
        mbedtls_ssl_conf_dtls_anti_replay(&dtls_srtp->conf, MBEDTLS_SSL_ANTI_REPLAY_DISABLED);
        mbedtls_ssl_setup(&dtls_srtp->ssl, &dtls_srtp->conf);
        mbedtls_ssl_set_mtu(&dtls_srtp->ssl, DTLS_MTU_SIZE);
        dtls_srtp->role = role;
    }

    // Set state to init
    dtls_srtp->state = DTLS_SRTP_STATE_INIT;
}

// Function to get DTLS-SRTP role
dtls_srtp_role_t RTCDtlsSrtpModule::dtls_srtp_get_role(dtls_srtp_t *dtls_srtp)
{
    // Return current role
    return dtls_srtp->role;
}

// Function to write data over DTLS-SRTP
int RTCDtlsSrtpModule::dtls_srtp_write(dtls_srtp_t *dtls_srtp, const unsigned char *buf, size_t len)
{
    // Variable declarations
    int ret;
    int consume = 0;

    // Lock mutex
    RTCMediaModule::Instance().media_lib_mutex_lock(dtls_srtp->lock, MEDIA_LIB_MAX_LOCK_TIME);

    // Write data in a loop
    while (len)
    {
        measure_start("ssl_write");
        ret = mbedtls_ssl_write(&dtls_srtp->ssl, buf, len);
        measure_stop("ssl_write");
        if (ret > 0)
        {
            consume += ret;
            buf += ret;
            len -= ret;
        }
        else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            break;
        }
        else
        {
            consume = ret;
            break;
        }
    }

    // Unlock mutex
    RTCMediaModule::Instance().media_lib_mutex_unlock(dtls_srtp->lock);

    // Return number of bytes consumed or error code
    return consume;
}

// Function to read data over DTLS-SRTP
int RTCDtlsSrtpModule::dtls_srtp_read(dtls_srtp_t *dtls_srtp, unsigned char *buf, size_t len)
{
    // Variable declarations
    int ret = 0;
    int read_bytes = 0;

    // Lock mutex
    RTCMediaModule::Instance().media_lib_mutex_lock(dtls_srtp->lock, MEDIA_LIB_MAX_LOCK_TIME);

    // Read data in a loop
    while (read_bytes < len)
    {
        measure_start("ssl_read");
        ret = mbedtls_ssl_read(&dtls_srtp->ssl, buf + read_bytes, len - read_bytes);
        measure_stop("ssl_read");
        if (ret > 0)
        {
            read_bytes += ret;
            continue;
        }
        else if (ret == 0 || ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        {
            // Detected DTLS connection close ret
            ret = -1;
            break;
        }
        else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_TIMEOUT)
        {
            // No more data to read
            ret = 0;
            break;
        }
        else
        {
            // Other errors
            ret = 0;
            break;
        }
    }

    // Check if read buffer
    if (ret != -1 && read_bytes)
    {
        ret = read_bytes;
    }

    // Unlock mutex
    RTCMediaModule::Instance().media_lib_mutex_unlock(dtls_srtp->lock);

    // Return number of bytes read or error code
    return ret;
}

// Function to probe DTLS-SRTP packet
bool RTCDtlsSrtpModule::dtls_srtp_probe(uint8_t *buf)
{
    // Check for null buffer
    if (buf == NULL)
    {
        return false;
    }

    // Check if the first byte indicates a DTLS record
    return ((*buf > 19) && (*buf < 64));
}

// Function to encrypt RTP packet
int RTCDtlsSrtpModule::dtls_srtp_decrypt_rtp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int *bytes)
{
    // Decrypt RTP packet using SRTP
    size_t size = *bytes;
    int ret = srtp_unprotect(dtls_srtp->srtp_in, packet, size, packet, &size);
    *bytes = size;

    // Return result
    return ret;
}

// Function to encrypt RTCP packet
int RTCDtlsSrtpModule::dtls_srtp_decrypt_rtcp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int *bytes)
{
    // Decrypt RTCP packet using SRTP
    size_t size = *bytes;
    int ret = srtp_unprotect_rtcp(dtls_srtp->srtp_in, packet, size, packet, &size);
    *bytes = size;

    // Return result
    return ret;
}

// Function to encrypt RTP packet
void RTCDtlsSrtpModule::dtls_srtp_encrypt_rtp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int buf_size, int *bytes)
{
    // Encrypt RTP packet using SRTP
    size_t size = buf_size;
    srtp_protect(dtls_srtp->srtp_out, packet, *bytes, packet, &size, 0);
    *bytes = size;
}

// Function to encrypt RTCP packet
void RTCDtlsSrtpModule::dtls_srtp_encrypt_rctp_packet(dtls_srtp_t *dtls_srtp, uint8_t *packet, int buf_size, int *bytes)
{
    // Encrypt RTCP packet using SRTP
    size_t size = buf_size;
    srtp_protect_rtcp(dtls_srtp->srtp_out, packet, *bytes, packet, &size, 0);
    *bytes = size;
}