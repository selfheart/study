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

#include <ontrac/ontrac/mem_usage.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/util/byte_buffer.h>
#include <ontrac/ontrac/status_codes.h>
#include <spil_crypt/crypt.h>
#include <spil_crypt/crypt_tls.h>

#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#include "crypt_ctx.h"
#include "trustchain.h"
#include "../crypt_cmn.h"

#ifndef MAX_TLS_NIOLETS
#define MAX_TLS_NIOLETS (4U)
#endif /* MAX_TLS_NIOLETS */

#ifndef OPENSSL_VERSION_NUMBER
// To avoid some build error
#define OPENSSL_VERSION_NUMBER 0
#endif

#if (OPENSSL_VERSION_NUMBER >= 0x10101000L)
#define HAVE_KEYLOG_CALLBACK
#endif

#ifdef DEBUG
#define ENABLE_SSLKEYLOG
#endif

extern trustchain_ctx_t trust_ctx;
struct for_crypt_tls_ctx {
    SSL* ssl;
    BIO *bio;
    int32_t ssl_ret;

    crypt_tls_send_t fsend; // parasoft-suppress CODSTA-88-3 "Pointer to callback function desired"
    crypt_tls_recv_t frecv; // parasoft-suppress CODSTA-88-3 "Pointer to callback function desired"
    void *rs_ctx;
};

static void crypt_tls_ctx_delete(cvar_t old_tls_ctx);
DEFINE_CLASS(crypt_tls_ctx_class, crypt_tls_ctx_t, MAX_TLS_NIOLETS, NULL, NULL, NULL, NULL, crypt_tls_ctx_delete, static);

static crypt_tls_ctx_t* crypt_tls_ctx_resolve(cvar_t item) {
    crypt_tls_ctx_t* retval = NULL;
    CLASS_RESOLVE(crypt_tls_ctx_class, crypt_tls_ctx_t, retval, item);
    return retval;
}

static err_t crypt_tls_error_check(const crypt_tls_ctx_t *tls_ctx){
    err_t retval = EXIT_SUCCESS;

    VITAL_NOT_NULL(tls_ctx);
    int32_t ssl_error = SSL_get_error(tls_ctx->ssl, tls_ctx->ssl_ret);
    switch (ssl_error) {
    case SSL_ERROR_WANT_WRITE:
        retval = EINPROGRESS;
        break;
    case SSL_ERROR_WANT_READ:
        retval = EINPROGRESS;
        break;
    case SSL_ERROR_ZERO_RETURN:
        ABQ_ERROR_MSG("SSL disconnected by peer\n");
        retval = ECANCELED;
        break;
    case SSL_ERROR_SYSCALL:
        retval = abq_status_take(EIO);
        break;
    default:
        {
            uint64_t error = ERR_get_error();
            do {
            	const char *error_string = ERR_error_string(error, NULL);
            	ABQ_ERROR_MSG(error_string);
            	error = ERR_get_error();
            } while((uint64_t)EXIT_SUCCESS != error);
        }
        retval = EIO;
        break;
    }
    return retval;
}

err_t crypt_tls_handshake(crypt_tls_ctx_t *tls_ctx) {
    err_t rvalue = EXIT_SUCCESS;
    VITAL_NOT_NULL(tls_ctx);
    tls_ctx->ssl_ret = SSL_connect(tls_ctx->ssl);
    if (tls_ctx->ssl_ret > 0) {
        int64_t ret = SSL_get_verify_result(tls_ctx->ssl);
        if (X509_V_OK != ret) {
            ABQ_ERROR_MSG(X509_verify_cert_error_string(ret));
            rvalue = EIO;
        } else {
            ABQ_INFO_MSG("TLS connection established");
        }
    } else {
        rvalue = crypt_tls_error_check(tls_ctx);
    }
#ifdef TIME_SERVER_OVER_TLS
    if (EINPROGRESS != rvalue) {
        X509_VERIFY_PARAM *param = SSL_get0_param(tls_ctx->ssl);
        if (NULL != param) {
            cstr_t host = X509_VERIFY_PARAM_get0_peername(param);
            if (NULL != host) {
                tls_handshake_done(host);
            }
        }
    }
#endif
    return rvalue;
}

static plat_int_t crypt_tls_ctx_recv(BIO *bio, char *buf, plat_int_t len) {
    int32_t rvalue = -EWOULDBLOCK;
    size_t read = 0;
    err_t err = EXIT_SUCCESS;
    VITAL_NOT_NULL(bio);
    VITAL_NOT_NULL(buf);
    ABQ_VITAL(0U != len);

    crypt_tls_ctx_t *tls_ctx = (crypt_tls_ctx_t *)BIO_get_app_data(bio);

    if ((NULL != tls_ctx->frecv) && (NULL != tls_ctx->rs_ctx)) {
        err = tls_ctx->frecv(tls_ctx->rs_ctx, (uint8_t*)buf, (size_t)len, &read);
        if (EXIT_SUCCESS == err) {
            rvalue = (int32_t)read;
            BIO_clear_retry_flags(tls_ctx->bio);
        } else if(CHECK_WOULDBLOCK(err)) {
            BIO_set_retry_read(tls_ctx->bio);
        } else {
            abq_status_set(err, true);
            BIO_clear_retry_flags(tls_ctx->bio);
        }
    }
    return rvalue;
}

static plat_int_t crypt_tls_ctx_send(BIO *bio, const char *buf, plat_int_t len) {
    int32_t rvalue = -EWOULDBLOCK;
    size_t written = 0;
    err_t err = EXIT_SUCCESS;
    VITAL_NOT_NULL(bio);
    VITAL_NOT_NULL(buf);
    ABQ_VITAL(0U != len);
    crypt_tls_ctx_t *tls_ctx = (crypt_tls_ctx_t *)BIO_get_app_data(bio);
    if ((NULL != tls_ctx->fsend) && (NULL != tls_ctx->rs_ctx)) {
        err = tls_ctx->fsend(tls_ctx->rs_ctx, (const uint8_t*)buf, (size_t)len, &written);
        if (EXIT_SUCCESS == err) {
            rvalue = (int32_t)written;
            BIO_clear_retry_flags(tls_ctx->bio);
        } else if(CHECK_WOULDBLOCK(err)) {
            BIO_set_retry_write(tls_ctx->bio);
        } else {
            abq_status_set(err, true);
            BIO_clear_retry_flags(tls_ctx->bio);
        }
    }
    return rvalue;
}

err_t crypt_tls_set_bio(crypt_tls_ctx_t *tls_ctx, void * ctx,
		crypt_tls_send_t fsend, crypt_tls_recv_t frecv) { // parasoft-suppress CODSTA-88-3 "Pointer to callback function desired"
    err_t rvalue = EXIT_SUCCESS;
    VITAL_NOT_NULL(tls_ctx);
    VITAL_NOT_NULL(ctx);
    VITAL_NOT_NULL(fsend);
    VITAL_NOT_NULL(frecv);

    tls_ctx->frecv = frecv;
    tls_ctx->fsend = fsend;
    tls_ctx->rs_ctx = ctx;
#if OPENSSL_VERSION_NUMBER >= 0x10100000 // OpenSSL >= 1.1.0
    BIO_set_init(tls_ctx->bio, 1);
#else
    tls_ctx->bio->init = 1;
#endif
    return rvalue;
}

err_t crypt_tls_read(crypt_tls_ctx_t *tls_ctx, uint8_t *buf,
		size_t buf_size, size_t *bytes_read) {
    err_t rvalue = EXIT_SUCCESS;
    VITAL_NOT_NULL(tls_ctx);
    VITAL_NOT_NULL(buf);
    ABQ_VITAL(0 != buf_size);
    VITAL_NOT_NULL(bytes_read);

    tls_ctx->ssl_ret = SSL_read(tls_ctx->ssl, buf, (plat_int_t) buf_size);
    if (tls_ctx->ssl_ret <= 0) {
        rvalue = crypt_tls_error_check(tls_ctx);
    } else {
        *bytes_read = (size_t)tls_ctx->ssl_ret;
        rvalue = EXIT_SUCCESS;
    }
    return rvalue;
}

err_t crypt_tls_write(crypt_tls_ctx_t *tls_ctx, const uint8_t *buf,
		size_t buf_size, size_t *bytes_written) {
    err_t rvalue = EXIT_SUCCESS;
    VITAL_NOT_NULL(tls_ctx);
    VITAL_NOT_NULL(bytes_written);
    *bytes_written = 0UL;
    if (0U == buf_size) {
        if (1 == BIO_flush(tls_ctx->bio)) {
            rvalue = EXIT_SUCCESS;
        } else if(BIO_should_retry(tls_ctx->bio)){
            rvalue = EWOULDBLOCK;
        } else {
            rvalue = crypt_tls_error_check(tls_ctx);
        }
    } else {
        VITAL_NOT_NULL(buf);
        tls_ctx->ssl_ret = SSL_write(tls_ctx->ssl, buf, (plat_int_t) buf_size);
        if (tls_ctx->ssl_ret <= 0) {
            rvalue = crypt_tls_error_check(tls_ctx);
        } else {
            *bytes_written = (size_t)tls_ctx->ssl_ret;
            rvalue = EXIT_SUCCESS;
        }
    }
    return rvalue;
}

bool_t crypt_tls_pending(const crypt_tls_ctx_t *tls_ctx) {
    VITAL_NOT_NULL(tls_ctx);
    return (0 < SSL_pending(tls_ctx->ssl));
}

static plat_long_t s_ctrl (BIO *bio, /* parasoft-suppress MISRAC2012-RULE_8_13-a-4 "m0030. Not using 'const' specifier to adhere to OpenSSL API callback type." */
                           plat_int_t cmd,
                           plat_long_t scmd,
                           void *param) { /* parasoft-suppress MISRAC2012-RULE_8_13-a-4 "m0031. Not using 'const' specifier to adhere to OpenSSL API callback type." */
    // Currently no special ctrl is handled
    (void) bio; // Unused parameter in this callback implementation
    (void) cmd; // Unused parameter in this callback implementation
    (void) scmd; // Unused parameter in this callback implementation
    (void) param; // Unused parameter in this callback implementation
    return (plat_long_t) 1;
}

static plat_int_t verify_callback(plat_int_t preverify_ok, X509_STORE_CTX *ctx) {
    char buf[B512_SIZE] ="";
    VITAL_NOT_NULL(ctx);
    plat_int_t retval = preverify_ok;

    plat_int_t err = X509_STORE_CTX_get_error(ctx);

    if (EXIT_SUCCESS != err) {
        bool_t skip = false;
        ABQ_ERROR_MSG_X(X509_verify_cert_error_string(err), err);
#ifdef TIME_SERVER_OVER_TLS
        // In this reference implementation of spil_crypt, when trusted time not valid yet,
        // it will always result in CERT_NOT_YET_VALID, and should not result in CERT_HAS_EXPIRED
        if (X509_V_ERR_CERT_NOT_YET_VALID == err) {
            SSL *ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
            cstr_t host = NULL;
            if (NULL != ssl) {
                host = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
            }
            if ((NULL != host) && tls_skip_valid_period_check(host)) {
                    skip = true;
            }
        }
#endif
        if (skip) {
            X509_STORE_CTX_set_error(ctx, 0);
            retval = 1; // overwrite preverify_ok and allow verification to continue
        } else {
            BIO *bio = BIO_new(BIO_s_mem());

            STACK_OF(X509) *chain = X509_STORE_CTX_get_chain(ctx);
            for (int32_t index = 0; index < sk_X509_num(chain); index++) {
                X509_NAME_oneline(X509_get_issuer_name(sk_X509_value(chain, index)), buf,
                        (int32_t)sizeof(buf));
                ABQ_INFO_MSG_X(buf, index);
                if ((index + 1) == sk_X509_num(chain))
                    PEM_write_bio_X509(bio, sk_X509_value(chain, index));
            }

            BUF_MEM *bptr = NULL;
            BIO_get_mem_ptr(bio, &bptr);
            ABQ_INFO_MSG_Y("x509: ", bptr->data, bptr->length);

            (void)BIO_free(bio);
        }
    }
    return retval;
}

#if defined(HAVE_KEYLOG_CALLBACK) && defined(ENABLE_SSLKEYLOG)
/**
 * Keylogging output handle.
 */
static BIO *keylog_bio = NULL;

/**
 * Trivial logging function that outputs to keylogging output handle.
 */
static void keylog_callback(const SSL *ssl, const char *line) {
  const char nl[] = "\n\0";
  if (NULL != keylog_bio) {
    BIO_puts(keylog_bio, line);
    BIO_puts(keylog_bio, &nl);
    BIO_flush(keylog_bio);
  }
}

/**
 * Initialize the keylogging output handle.  Does nothing if the keylogging
 * handle is already initialized.
 *
 * TODO: determine suitable return type.
 *
 * Return NULL on failure.
 */
static plat_int_t try_init_keylog_handle(const char *filename) {
  plat_int_t result = 0;
  if (NULL == keylog_bio) {
    keylog_bio = BIO_new_file(filename, "w");
    result = (plat_int_t) keylog_bio;
  }
  return result;
}
#endif

crypt_tls_ctx_t *crypt_tls_ctx_create(cstr_t fqdn)
{
    err_t errval = EXIT_SUCCESS;
    crypt_tls_ctx_t *tls_context = NULL;
    SSL_CTX *ssl_ctx = NULL;
    X509_VERIFY_PARAM *param = NULL;

    if (NULL == fqdn) {
        errval = EINVAL;
    }
    if (EXIT_SUCCESS == errval) {
        tls_context = CREATE_BASE_INSTANCE(crypt_tls_ctx_class, crypt_tls_ctx_t);
        if (NULL == tls_context) {
            errval = ENOMEM;
        }
    }
    if (EXIT_SUCCESS == errval) {
        *tls_context = (crypt_tls_ctx_t){
            .fsend = NULL,
            .frecv = NULL,
            .rs_ctx = NULL,
            .bio = NULL,
            .ssl = NULL,
            .ssl_ret = 0
        };
#if OPENSSL_VERSION_NUMBER >= 0x10100000 // OpenSSL >= 1.1.0
        ABQ_TRACE_MSG("TLS_client_method");
        ssl_ctx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);
#else
        ABQ_TRACE_MSG("TLS_client_method");
        ssl_ctx = SSL_CTX_new(TLSv1_2_method());
#endif
        if (NULL == ssl_ctx) {
#ifdef NIO_DEBUG
            ERR_print_errors_fp(stderr);
#endif
            errval = ENOMEM;
            ABQ_DUMP_ERROR(errval, "SSL_CTX_new");
        }
    }
    if (EXIT_SUCCESS != errval) {
    	// Return error code as is
    }else if(NULL == trust_ctx.trusted_certs) {
    	ABQ_ERROR_MSG("Crypt is not initialized!!!");
    	errval = EINVAL;
    }else{
        ABQ_EXPECT(1 == SSL_CTX_set1_verify_cert_store(ssl_ctx, trust_ctx.trusted_certs));
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, verify_callback);
    }
    if ((EXIT_SUCCESS == errval) && (NULL != trust_ctx.client_cert_stack)) {
        for (uint32_t i = 0; i < sk_X509_INFO_num(trust_ctx.client_cert_stack); i++) {
            X509_INFO *stack_item = sk_X509_INFO_value(trust_ctx.client_cert_stack, i);
            if (0 == i) {
                ABQ_EXPECT(1 == SSL_CTX_use_certificate(ssl_ctx, stack_item->x509));
                ABQ_EXPECT(1 == SSL_CTX_clear_extra_chain_certs(ssl_ctx));
            } else {
#if OPENSSL_VERSION_NUMBER >= 0x10100000 // OpenSSL >= 1.1.0
                ABQ_EXPECT(1 == X509_up_ref(stack_item->x509)); // So it won't be freed along with ssl_ctx
#else
                CRYPTO_add(&((stack_item->x509)->references), 1, CRYPTO_LOCK_X509);
#endif
                ABQ_EXPECT(1 == SSL_CTX_add_extra_chain_cert(ssl_ctx, stack_item->x509));
            }
        }
    }
    if ((EXIT_SUCCESS == errval) && (NULL != trust_ctx.client_pkey)) {
        ABQ_EXPECT(1 == SSL_CTX_use_PrivateKey(ssl_ctx, trust_ctx.client_pkey));
    }
    if (EXIT_SUCCESS == errval) {
        tls_context->ssl = SSL_new(ssl_ctx);
        if (NULL == tls_context->ssl) {
            errval = ENOMEM;
        }
    }
#if defined(HAVE_KEYLOG_CALLBACK) && defined(ENABLE_SSLKEYLOG)
    if (EXIT_SUCCESS == errval) {
      if (NULL == SSL_CTX_get_keylog_callback(ssl_ctx)) {
	const char * keylogfile_name = getenv("SSLKEYLOG_FILE");
	if (NULL != try_init_keylog_handle(keylogfile_name)){
	  SSL_CTX_set_keylog_callback(ssl_ctx, keylog_callback);
	}
      }
    }
#endif
    if (EXIT_SUCCESS == errval) {
        param = SSL_get0_param(tls_context->ssl);
        uint64_t time_ms = 0U;
        (void) crypt_get_time(&time_ms);
        X509_VERIFY_PARAM_set_time(param, (time_t)(time_ms/1000U));
        X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
        if (1 != X509_VERIFY_PARAM_set1_host(param, fqdn, (size_t)utf8_byte_length(fqdn, -1))) {
            errval = EINVAL;
        }
        (void)SSL_set_tlsext_host_name(tls_context->ssl, fqdn);
    }
    if (NULL != ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
    }
    if (EXIT_SUCCESS == errval) {
#if OPENSSL_VERSION_NUMBER >= 0x10100000 // OpenSSL >= 1.1.0
        BIO_METHOD *method = BIO_meth_new(BIO_TYPE_SOURCE_SINK, "OpenSSLBIO");
        if (NULL != method &&
                1 == BIO_meth_set_write(method, crypt_tls_ctx_send) &&
                1 == BIO_meth_set_read(method, crypt_tls_ctx_recv) &&
                1 == BIO_meth_set_ctrl(method, s_ctrl)) {
            tls_context->bio = BIO_new(method);
        }
#else
        static BIO_METHOD method = {
            .type = BIO_TYPE_SOURCE_SINK,
            .name = "OpenSSLBIO",
            .bwrite = crypt_tls_ctx_send,
            .bread = crypt_tls_ctx_recv,
            .ctrl = s_ctrl,
        };
        tls_context->bio = BIO_new(&method);
#endif
        if (NULL != tls_context->bio) {
            BIO_set_app_data(tls_context->bio, tls_context);
            SSL_set_bio(tls_context->ssl, tls_context->bio, tls_context->bio);
        } else {
            errval = ENOMEM;
        }
    }
    if ((EXIT_SUCCESS != errval) && (NULL != tls_context)) {
    	(void) abq_status_set(errval, true);
        (void)obj_release_self(tls_context);
    	tls_context = NULL;
    }
    return tls_context;
}

static void crypt_tls_ctx_delete(cvar_t old_tls_ctx)
{
    crypt_tls_ctx_t * tls_ctx
        = crypt_tls_ctx_resolve(old_tls_ctx);
    VITAL_NOT_NULL(tls_ctx);
    SSL_free(tls_ctx->ssl);
}
