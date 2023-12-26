/* ****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

#ifndef CRYPT_MGR_IF_CRYPT_TLS_H
#define CRYPT_MGR_IF_CRYPT_TLS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file crypt_tls.h
 *
 * This file defines primitive functions to support TLS operations.
 *
 * Following requirements for the TLS implementation do not affect how OTAmatic Client core
 * components function. They are demanded to satisfy end-to-end architecture requirements.
 * - Only allow TLS v1.2+
 * - Support SNI
 * - Client Authentication
 */

typedef struct for_crypt_tls_ctx crypt_tls_ctx_t;

/**
 * @brief Create implementation specific TLS context
 * @public
 * @param [in] fqdn Full Qualified Domain Name of the server
 * @return Handle of TLS context created, NULL when error
 */
crypt_tls_ctx_t *crypt_tls_ctx_create(cstr_t fqdn);

/**
 * @brief low level IO read function
 * @public
 * @param [in] ctx BIO context
 * @param [in] buf buffer to get data
 * @param [in] len size of the buffer
 * @param [out] read buffer to return number of bytes read by the function
 * @return
 * 	- EXIT_SUCCESS for success
 * 	- otherwise when error
 */
typedef err_t (*crypt_tls_recv_t)(void *ctx, uint8_t *buf, size_t len, size_t *read);

/**
 * @brief low level IO write function
 * @public
 * @param [in] ctx BIO context
 * @param [in] buf buffer containing data to be sent
 * @param [in] len data size
 * @param [out] written buffer to return number of bytes written by the function
 * @return
 * 	- EXIT_SUCCESS for success
 * 	- otherwise when error
 */
typedef err_t (*crypt_tls_send_t)(void *ctx, const uint8_t *buf, size_t len, size_t *written);

/**
 * @brief TLS handshake
 *
 * Caller would call this function repeatedly when getting EINPROGRESS, until the function
 * returns EXIT_SUCCESS or other error code.
 *
 * @public
 * @param [in] tls_ctx TLS context from crypt_tls_ctx_create
 * @return
 * 	- EXIT_SUCCESS when handshake is success with the server.
 *  - EINPROGRESS when handshake is in progress.
 *  - otherwise when error
 */
err_t crypt_tls_handshake(crypt_tls_ctx_t *tls_ctx);

/**
 * @brief associate a set of BIO functions to the TLS context
 *
 * General calling flow:
 * \startuml
 *     participant "Caller" as caller
 *     participant "crypt_tls" as crypt_tls
 *     participant "I/O" as io
 *     participant "Underline TLS library\nsuch as OpenSSL" as tlslib
 *     caller -> crypt_tls: crypt_tls_read/write/handshake
 *     crypt_tls -> tlslib: TLS read/write/handshake
 *     tlslib -> crypt_tls: BIO read/write
 *     crypt_tls -> io: frecv()/fsend()
 *     io -> crypt_tls: frecv()/fsend() result
 *     crypt_tls -> tlslib: BIO read/write result
 *     tlslib -> crypt_tls: TLS read/write/handshake result
 *     crypt_tls -> caller
 * \enduml
 * @public
 * @param [in] tls_ctx TLS context from crypt_tls_ctx_create
 * @param [in] ctx BIO context to pass into BIO read/write function
 * @param [in] fsend BIO write function
 * @param [in] frecv BIO read function
 * @return EXIT_SUCCESS when no error. Otherwise when error.
 */
err_t crypt_tls_set_bio(crypt_tls_ctx_t *tls_ctx, void * ctx,
		crypt_tls_send_t fsend, crypt_tls_recv_t frecv);

/**
 * @brief Read decrypted data from the TLS channel
 * @public
 * @param [in] tls_ctx TLS context from crypt_tls_ctx_create
 * @param [in] buf buffer to get data
 * @param [in] buf_size size of the buffer
 * @param [out] bytes_read return number of bytes read from the channel
 * @return
 * 	- EXIT_SUCCESS success
 *  - EINPROGRESS more low level IO needed to get decrypted data
 *  - otherwise when error
 */
err_t crypt_tls_read(crypt_tls_ctx_t *tls_ctx, uint8_t *buf,
		size_t buf_size, size_t *bytes_read);

/**
 * @brief Write unencrypted data to the TLS channel
 * @public
 * @param [in] tls_ctx TLS context from crypt_tls_ctx_create
 * @param [in] buf buffer containing data
 * @param [in] buf_size size of the data
 * @param [out] bytes_written return number of bytes written to channel
 * @return
 * 	EXIT_SUCCESS success
 *  EINPROGRESS more low level IO needed to send unencrypted data
 *  otherwise when error
 */
err_t crypt_tls_write(crypt_tls_ctx_t *tls_ctx, const uint8_t *buf,
		size_t buf_size, size_t *bytes_written);

/**
 * @brief If there is decrypted data pending in the channel
 * @public
 * @param [in] tls_ctx TLS context from crypt_tls_ctx_create
 * @return
 * 	TRUE if there is data pending, FALSE is not
 */
bool_t crypt_tls_pending(const crypt_tls_ctx_t *tls_ctx);

/**
 * @brief temporarily disable validity period check of the certificate from server
 *  + This function is only required when TIME_SERVER_OVER_TLS, and preferred not to be implemented
 *    if TIME_SERVER_OVER_TLS is not defined
 *  + this function, as well as following skipping validity period check, should only take effect
 *    if trusted time is not ready (EXIT_SUCCESS != crypt_get_time())
 *  + if (NULL == fqdn), spil_crypt should stop skipping any cert validity period,
 *    until this function is called again.
 *  + spil_crypt would skip cert validity period check for the immediate next connection to proxy,
 *    if (NULL != proxy)
 *  + spil_crypt would skip cert validity period check for the immediate next connection to fqdn
 *  + once a connection attempt to fqdn or proxy finished, whether successful or not,
 *    spil_crypt should not skip cert validity period check again for that host,
 *    until this function is called again.
 *  + once a connection attempt to fqdn finished, whether successful or not,
 *    spil_crypt should not skip cert validity period check for the proxy,
 *    until this function is called again.
 * @param fqdn FQDN of remote host that spil_crypt should skip the validity check.
 * @param proxy FQDN of the proxy server, if the client connect to proxy server ver TLS.
 *              proxy can be NULL if no proxy is used, or non-TLS connection for proxy server.
 * @return EXIT_SUCCESS for success, either set or canceled the skipping of validity check
 *         EINVAL indicates invalid state, say trusted time has already set
 *         Other error code, such as ENOMEM
 */
err_t crypt_disable_valid_per_verify(cstr_t fqdn, cstr_t proxy);

#ifdef __cplusplus
}
#endif
#endif /* CRYPT_MGR_IF_CRYPT_TLS_H */
