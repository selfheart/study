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
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

#ifndef NONTEE_TRUSTCHAIN_H
#define NONTEE_TRUSTCHAIN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    STACK_OF(X509_INFO) *client_cert_stack;
    EVP_PKEY *client_pkey;
    X509_STORE *trusted_certs;
    // TODO mbedtls_x509_crl trusted_crl;
    bool_t crl_initialized;
} trustchain_ctx_t;

extern trustchain_ctx_t trust_ctx;

err_t trustchain_init(trustchain_ctx_t *ctx, cstr_t trusted_cas, size_t cert_len, cstr_t clientcert, size_t clicrtlen);

void trustchain_cleanup(trustchain_ctx_t *ctx);

err_t trustchain_update_crl(trustchain_ctx_t *ctx, const char *crl, size_t size);

X509 *trustchain_verify_cert(trustchain_ctx_t *ctx, const char *cert, size_t size, bool_t skip_crl);

err_t trustchain_append_cert(trustchain_ctx_t *ctx, const char *cert, size_t size, bool_t skip_crl);

#ifdef __cplusplus
}
#endif

#endif /* NONTEE_TRUSTCHAIN_H */

