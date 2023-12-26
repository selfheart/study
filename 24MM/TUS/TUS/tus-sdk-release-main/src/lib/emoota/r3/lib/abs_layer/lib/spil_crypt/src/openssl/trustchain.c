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

#include <ontrac/ontrac/primitives.h>
#include <ontrac/ontrac/status_codes.h>
#include <spil_crypt/crypt.h>

#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "crypt_ctx.h"
#include "trustchain.h"
#define CRYPTO_ALWAYS_RETURN_SUCCESS

trustchain_ctx_t trust_ctx = {0};

err_t trustchain_init(trustchain_ctx_t *ctx, cstr_t trusted_cas, size_t cert_len, cstr_t clientcert, size_t clicrtlen)
{
    err_t err = EXIT_SUCCESS;
	uint32_t i = 0;
    STACK_OF(X509_INFO) *certstack = NULL;
#ifdef NIO_DEBUG
    char buf[B512_SIZE]= "";
#endif /* NIO_DEBUG */
    if (NULL == ctx) {
        err = EINVAL;
    }
    if ((EXIT_SUCCESS == err) && (NULL != clientcert) && (0U != clicrtlen)) {
        BIO *cbio = BIO_new_mem_buf((const void *) clientcert, (int32_t) clicrtlen);
        if (NULL ==cbio) {
            err = ENOMEM;
        } else {
            trust_ctx.client_cert_stack = PEM_X509_INFO_read_bio(cbio, NULL,  NULL, NULL);
            (void)BIO_free(cbio);
            if (NULL == trust_ctx.client_cert_stack) {
                SLOG_E("Failed to initiate client cert stack");
                err = EINVAL;
            }
        }
    }
    if ((EXIT_SUCCESS == err) && (NULL != clientcert) && (0U != clicrtlen)) {
        // The private key could be either in front of the certs or after
        // so create a new BIO to read it.
        BIO *cbio = BIO_new_mem_buf((const void *) clientcert, (int32_t) clicrtlen);
        if (NULL ==cbio) {
            err = ENOMEM;
        } else {
            trust_ctx.client_pkey = PEM_read_bio_PrivateKey(cbio, NULL, NULL, NULL);
            (void)BIO_free(cbio);
            if (NULL == trust_ctx.client_pkey) {
                SLOG_E("Failed to initiate client private key");
                err = EINVAL;
            }
        }
    }

    if (trusted_cas == NULL) {
        return err;
    }
    // if Root CA is not specified, the following processes will not be executed

    if (EXIT_SUCCESS == err) {
        ctx->trusted_certs = X509_STORE_new();
        if (NULL == ctx->trusted_certs) {
            err = ENOMEM;
        }
    }
    if (EXIT_SUCCESS == err) {
        BIO *cbio = BIO_new_mem_buf((const void *) trusted_cas, (int32_t) cert_len);
        if (NULL ==cbio) {
            err = ENOMEM;
        } else {
            certstack = PEM_X509_INFO_read_bio(cbio, NULL,  NULL, NULL);
            (void)BIO_free(cbio);
            if (NULL == certstack) {
                SLOG_E("Failed to initiate trust anchors");
                err = EINVAL;
            }
        }
    }
    if (EXIT_SUCCESS == err) {
        X509_INFO *stack_item = NULL;
        for (i = 0; i < sk_X509_INFO_num(certstack); i++) {
            stack_item = sk_X509_INFO_value(certstack, i);
            if (1 != X509_STORE_add_cert(ctx->trusted_certs, stack_item->x509)) {
                err = EINVAL;
                break;
            }
#ifdef NIO_DEBUG
            else {
                X509_NAME_oneline(X509_get_issuer_name(stack_item->x509),
                        buf, (int32_t)sizeof(buf));
                ABQ_INFO_MSG_X(buf, i);
            }
#endif /* NIO_DEBUG */
        }
    }
    if (NULL != certstack) {
        sk_X509_INFO_pop_free(certstack, X509_INFO_free);
    }
    if ((EXIT_SUCCESS != err) && (NULL != ctx) && (NULL != ctx->trusted_certs)) {
        X509_STORE_free(ctx->trusted_certs);
        ctx->trusted_certs = NULL;
    }
	return err;
}

void trustchain_cleanup(trustchain_ctx_t *ctx)
{
    if (NULL != ctx) {
        if (NULL != ctx->trusted_certs) {
            X509_STORE_free(ctx->trusted_certs);
            ctx->trusted_certs = NULL;
        }
        if (NULL != ctx->client_cert_stack) {
            sk_X509_INFO_pop_free(ctx->client_cert_stack, X509_INFO_free);
            ctx->client_cert_stack = NULL;
        }
        if (NULL != ctx->client_pkey) {
            EVP_PKEY_free(ctx->client_pkey);
            ctx->client_pkey = NULL;
        }
    }
}

#ifdef TRUST_CHAIN_VERIFY_CERT
err_t trustchain_update_crl(trustchain_ctx_t *ctx, const char *crl, size_t size)
{
    err_t err = EXIT_SUCCESS;
    if ((NULL == ctx) || (NULL == crl) || (0U == size)) {
        err = EINVAL;
    } else {
        ctx->crl_initialized = true;
    }

    SLOG_W("CRL Function not implemented yet");
    return err;
}

X509 *trustchain_verify_cert(trustchain_ctx_t *ctx, const char *cert, size_t size, bool_t skip_crl)
{
    err_t err = EXIT_SUCCESS;
    X509 *rvalue = NULL;
    X509_STORE_CTX *store_ctx = NULL;
    STACK_OF(X509_INFO) *certstack = NULL;
    STACK_OF(X509) *newsk = NULL;

    if ((NULL == ctx) || (NULL == cert) || (0U == size)) {
        err = EINVAL;
    }
    if (skip_crl) {
        SLOG_W("CRL skipped");
    }
    if (EXIT_SUCCESS == err) {
        BIO *cbio = BIO_new_mem_buf((const void*)cert, -1);
        if (NULL ==cbio) {
            err = ENOMEM;
        } else {
            certstack = PEM_X509_INFO_read_bio(cbio, NULL,  NULL, NULL);
            (void)BIO_free(cbio);
            if (NULL == certstack) {
                err = EINVAL;
            }
        }
    }
    if (EXIT_SUCCESS == err) {
        newsk = sk_X509_new_null();
        if (NULL == newsk) {
            err = ENOMEM;
        } else {
            while (0 != sk_X509_INFO_num(certstack)) {
                X509_INFO *info = sk_X509_INFO_pop(certstack);
                rvalue = info->x509;
                if (0 != sk_X509_INFO_num(certstack)) {
                    // Not the end of cert chain yet
                    sk_X509_insert(newsk, info->x509, 0);
                }
                info->x509 = NULL; // Do not free the X509 in the following X509_INFO_free()
                X509_INFO_free(info);
            }
        }
    }
    if (EXIT_SUCCESS == err) {
        store_ctx = X509_STORE_CTX_new();
        if (NULL == store_ctx) {
            err = ENOMEM;
        } else {
            if (1 != X509_STORE_CTX_init(store_ctx, ctx->trusted_certs, rvalue, newsk)) {
                SLOG_E("Failed to initiate X509_STORE_CTX\n");
                err = EINVAL;
            }
        }
    }
    if (EXIT_SUCCESS == err) {
        if (1 != X509_verify_cert(store_ctx)) {
            SLOG_E("Failed to verify certificate");
            SLOG_E(X509_verify_cert_error_string(X509_STORE_CTX_get_error(store_ctx)));
            err = EINVAL;
        }
    }
#if 0
    if (EXIT_SUCCESS == err) {
	    if (!skip_crl) {
            if (!ctx->crl_initialized) {
                err = EINVAL;
            } else {
                crl = &ctx->trusted_crl;
            }
        }
    }
#endif
    // TODO verify cert

#ifdef CRYPTO_ALWAYS_RETURN_SUCCESS
    err = EXIT_SUCCESS;
#endif
    if ((EXIT_SUCCESS != err) && (NULL != rvalue)) {
        X509_free(rvalue);
        rvalue = NULL;
    }
    if (NULL != certstack) {
        sk_X509_INFO_pop_free(certstack, X509_INFO_free);
    }
    if (NULL != newsk) {
        sk_X509_pop_free(newsk, X509_free);
    }
    if (NULL != store_ctx) {
        X509_STORE_CTX_free(store_ctx);
    }
    return rvalue;
}

/* parasoft-begin-suppress MISRAC2012-RULE_2_7-a-4 "Keeping unused parameter for future reference" */
/* parasoft-begin-suppress MISRAC2012-RULE_8_13-a-4 "Keeping non-const parameter for future use" */
err_t trustchain_append_cert(trustchain_ctx_t *ctx, const char *cert, size_t size, bool_t skip_crl)
/* parasoft-end-suppress MISRAC2012-RULE_8_13-a-4 */
/* parasoft-end-suppress MISRAC2012-RULE_2_7-a-4 */
{
    err_t err = EXIT_SUCCESS;

    if (NULL == ctx) {
        err = EINVAL;
    }
    SLOG_W("CRL Function not implemented yet");
    err = ENOSYS;

#ifdef CRYPTO_ALWAYS_RETURN_SUCCESS
    err = EXIT_SUCCESS;
#endif

    return err;
}
#endif /* TRUST_CHAIN_VERIFY_CERT */
