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
#include <openssl/ssl.h>

#include "crypt_ctx.h"
#include "trustchain.h"

#include "../crypt_cmn.h"
#include "../crypt_plat.h"

err_t crypt_plat_init(cstr_t trusted_cas, size_t length, cstr_t clientcert, size_t clicrtlen)
{
    err_t err = EXIT_SUCCESS;

    SLOG_I("Security Manager init wit OpenSSL");

    if (1 != SSL_library_init()) {
        err = EINVAL;
    }

    if (EXIT_SUCCESS == err) {
        SSL_load_error_strings();
        err = trustchain_init(&trust_ctx, trusted_cas, length, clientcert, clicrtlen);
    }

#ifdef TRUST_CHAIN_VERIFY_CERT
    const char *crl = "dummy crl, not going to work, just place holder until we have more details about the security structure"; // TODO
    (void)crypt_update_crl(crl); // TODO
#endif

    return err;
}

void crypt_plat_cleanup(void) {
    trustchain_cleanup(&trust_ctx);
}

int32_t openssl_rsa_padding_type(crypt_padding_t padding) {
    int32_t rvalue = 0;

    switch (padding) {
    case CRYPT_RSA_PKCS_V15:
        rvalue = RSA_PKCS1_PADDING;
        break;
    case CRYPT_RSA_PKCS_V21:
        rvalue = RSA_PKCS1_OAEP_PADDING;
        break;
    default:
        SLOG_E("Padding mode not supported");
        break;
    }
    return rvalue;
}

#ifdef TRUST_CHAIN_VERIFY_CERT
err_t crypt_update_crl(const char *crl)  // parasoft-suppress MISRAC2012-RULE_8_7-a-4 "Keeping externally available for future use"
{
    err_t err = EXIT_SUCCESS;
    if (NULL == crl) {
        err = EINVAL;
    } else {
        err = trustchain_update_crl(&trust_ctx, crl, (size_t)utf8_byte_length(crl, -1) + 1U);
    }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}

err_t crypt_verify_cert(const char *cert)
{
    err_t err = EXIT_SUCCESS;
    X509 *x509 = NULL;

    if (NULL == cert) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        x509 = trustchain_verify_cert(&trust_ctx, cert, (size_t)utf8_byte_length(cert, -1) + 1U, false);
        if (NULL == x509) {
            err = EINVAL;
        } else {
            X509_free(x509);
        }
    }

    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}

err_t crypt_cert_append_trustchain(const char *cert)
{
    err_t err = EXIT_SUCCESS;

    if (NULL == cert) {
        err = EINVAL;
    } else {
        err = trustchain_append_cert(&trust_ctx, cert, (size_t)utf8_byte_length(cert, -1) + 1U, false);
    }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}

err_t crypt_verify_signature_ex(const char *cert,  // parasoft-suppress MISRAC2012-RULE_8_7-a-4 "Keeping externally available for future use"
		const uint8_t *digest, size_t digest_len,
		const char *signature, bool_t skip_crl) // parasoft-suppress CODSTA-86-3 "bool_t type doesn't need validataion"
{
    err_t err = EXIT_SUCCESS;
    uint8_t *sig = NULL;
    size_t siglen = 0;
    int32_t sigstrlen = 0;
    X509 *x509 = NULL;
    EVP_PKEY *evp_pubkey = NULL;
    EVP_PKEY_CTX *pkey_ctx = NULL;

    if ((NULL == cert) || (NULL == digest) || (0U == digest_len) || (NULL == signature))  {
        err = EINVAL;
    }

    if (EXIT_SUCCESS == err) {
        x509 = trustchain_verify_cert(&trust_ctx, cert, (size_t)utf8_byte_length(cert, -1) + 1U, skip_crl);
        if (NULL == x509) {
            err = EINVAL;
        }
    }
    if (EXIT_SUCCESS == err) {
        evp_pubkey = X509_get_pubkey(x509);
        if (NULL == evp_pubkey) {
            err = EINVAL;
        }
    }

    if (EXIT_SUCCESS == err) {
        sigstrlen = utf8_byte_length(signature, -1);
        siglen = ((size_t)sigstrlen * 3U) / 4U;
        sig = (uint8_t *)abq_malloc(siglen); // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "Casting from known void pointer."
        siglen = (size_t)b64_decode(signature, sigstrlen, (byte_t*)sig, (int32_t)siglen);
        if (0U >= siglen) {
            err = EINVAL;
        }
    }

    if (EXIT_SUCCESS == err) {
        pkey_ctx = EVP_PKEY_CTX_new(evp_pubkey, NULL);
        if (NULL == pkey_ctx) {
            err = EINVAL;
        }
    }
    if ((EXIT_SUCCESS == err) && (1 != EVP_PKEY_verify_init(pkey_ctx))) {
        err = EINVAL;
    }
    if ((EXIT_SUCCESS == err) && (EVP_PKEY_RSA == EVP_PKEY_base_id(evp_pubkey)) &&
            (1 != EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PADDING))) {
        err = EINVAL;
    }
    if ((EXIT_SUCCESS == err) && (1 != EVP_PKEY_CTX_set_signature_md(pkey_ctx, EVP_sha256()))) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        if (1 != EVP_PKEY_verify(pkey_ctx, sig, siglen, digest, digest_len)) {
            err = EINVAL;
            SLOG_E(ERR_error_string(ERR_get_error(), NULL));
        }
    }
    if (NULL != sig) { abq_free(sig); }
    if (NULL != x509) { X509_free(x509); }
    if (NULL != evp_pubkey) { EVP_PKEY_free(evp_pubkey); }
    if (NULL != pkey_ctx) { EVP_PKEY_CTX_free(pkey_ctx); }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}

err_t crypt_verify_signature(const char *cert,
		const uint8_t *digest, size_t digest_len,
		const char *signature)
{
    err_t err = EXIT_SUCCESS;

    if ((NULL == cert) || (NULL == digest) || (0U == digest_len) || (NULL == signature))  {
        err = EINVAL;
    } else {
        err = crypt_verify_signature_ex(cert, digest, digest_len, signature, false);
    }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}
#endif /* TRUST_CHAIN_VERIFY_CERT */

err_t crypt_pk_verify_signature(crypt_hash_mode_t hashmode, crypt_crypt_mode_t method, // parasoft-suppress CODSTA-86-3 "enum type doesn't need validataion"
        const uint8_t *pubkey, size_t pubkey_len, const uint8_t *data, size_t datalen,
        const uint8_t *signature, size_t sig_len)
{
    err_t err = EXIT_SUCCESS;

    CRYPT_HASH_CTX ctx = NULL;
    uint8_t *hash = NULL;
    const_uint8_t *fin_hash = NULL;
    size_t hashlen = 0;
    EVP_PKEY *evp_pubkey = NULL;
    EVP_PKEY_CTX *pkey_ctx = NULL;
    const EVP_MD *mdtype = NULL;

    if ((data == NULL) || (datalen == 0U) || (signature == NULL) || (sig_len == 0U) ||
            (NULL == pubkey) || (0U == pubkey_len)) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        if (CRYPT_HASH_MODE_NONE != hashmode) {
            mdtype = openssl_md_type(hashmode, &hashlen);
            if ((mdtype == NULL) || (hashlen == 0U)) {
                err = EINVAL;
            } else {
                hash = (uint8_t *)abq_calloc(1, hashlen); // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0029. Assigning memory to object from pre-allocated memory pool."
                if (NULL == hash) {
                    err = ENOMEM;
                }
            }
            if ((EXIT_SUCCESS == err) && (EXIT_SUCCESS != crypt_hash_init(&ctx, hashmode))) {
                err = EIO;
            }

            if ((EXIT_SUCCESS == err) && (EXIT_SUCCESS != crypt_hash_finalize(ctx, data, datalen, hash, hashlen, &hashlen))) {
                err = EINVAL;
            }
            fin_hash = hash;
        } else {
            fin_hash = data;
            hashlen = datalen;
        }
    }
    if (EXIT_SUCCESS == err) {
        BIO *cbio = BIO_new_mem_buf(pubkey, (int32_t)pubkey_len);
        if (NULL ==cbio) {
            err = ENOMEM;
        } else {
            evp_pubkey = d2i_PUBKEY_bio(cbio, NULL);
            (void)BIO_free(cbio);
            if (NULL == evp_pubkey) {
                err = EINVAL;
            }
        }
    }

    if (EXIT_SUCCESS == err) {
        pkey_ctx = EVP_PKEY_CTX_new(evp_pubkey, NULL);
        if (NULL == pkey_ctx) {
            err = EINVAL;
        }
    }
    if ((EXIT_SUCCESS == err) && (1 != EVP_PKEY_verify_init(pkey_ctx))) {
        err = EINVAL;
    }
    if ((EXIT_SUCCESS == err) && (CRYPT_METHOD_RSA == method) &&
            (1 != EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PADDING))) {
        err = EINVAL;
    }
    if ((EXIT_SUCCESS == err) && (NULL != mdtype) &&
            (1 != EVP_PKEY_CTX_set_signature_md(pkey_ctx, mdtype))) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        if (1 != EVP_PKEY_verify(pkey_ctx, signature, sig_len, fin_hash, hashlen)) {
            err = EINVAL;
            SLOG_E(ERR_error_string(ERR_get_error(), NULL));
        }
    }
    if (NULL != evp_pubkey) { EVP_PKEY_free(evp_pubkey); }
    if (NULL != pkey_ctx) { EVP_PKEY_CTX_free(pkey_ctx); }
    if (CRYPT_HASH_MODE_NONE != hashmode) {
        abq_free(hash);
    }
    return err;
}

#ifdef EXTRACT_CERT
err_t crypt_extract_pubkey_from_cert(const char *pem_cert, uint8_t *der_pubkey, size_t buflen,
		size_t *derlen) {
    err_t err = EXIT_SUCCESS;
    X509 *cert = NULL;
    EVP_PKEY *pubkey = NULL;

    if ((NULL == pem_cert) || (NULL == der_pubkey) || (0U == buflen) || (NULL == derlen)) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        BIO *cbio = NULL;
        cbio = BIO_new_mem_buf((const void *)pem_cert, -1);
        cert = PEM_read_bio_X509(cbio, NULL, NULL, NULL);
        if (NULL == cert) {
            err = EINVAL;
        } else {
            pubkey = X509_get_pubkey(cert);
            if (NULL == pubkey) {
                err = EINVAL;
            }
        }
        (void)BIO_free(cbio);
    }
    if (EXIT_SUCCESS == err) {
        BIO *cbio = NULL;
        cbio = BIO_new(BIO_s_mem());
        if (NULL == cbio) {
            err = ENOMEM;
        } else {
            if (1 != i2d_PUBKEY_bio(cbio, pubkey)) {
                err = EINVAL;
            } else {
                int32_t r = BIO_read(cbio, der_pubkey, (plat_int_t) buflen);
                if (0 > r) {
                    err = EIO;
                } else {
                    *derlen = (size_t)r;
                }
            }
        }
        (void)BIO_free(cbio);
    }
    if (NULL != pubkey) { EVP_PKEY_free(pubkey); }
    if (NULL != cert) { X509_free(cert); }
    return err;
}
#endif /* EXTRACT_CERT */
