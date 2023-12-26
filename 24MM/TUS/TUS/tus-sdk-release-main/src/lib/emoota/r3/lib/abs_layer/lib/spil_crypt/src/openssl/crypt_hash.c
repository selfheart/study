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
#include <ontrac/ontrac/mem_usage.h>
#include <ontrac/ontrac/status_codes.h>
#include <spil_crypt/crypt.h>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "crypt_ctx.h"

#ifndef OPENSSL_VERSION_NUMBER
// To avoid some build error
#define OPENSSL_VERSION_NUMBER 0
#endif

const EVP_MD *openssl_md_type(crypt_hash_mode_t mode, size_t *hashlen) {
    const EVP_MD *rvalue = NULL;
    switch(mode) {
    case CRYPT_HASH_MODE_MD5:
        rvalue = EVP_md5();
        break;
    case CRYPT_HASH_MODE_SHA1:
        rvalue = EVP_sha1();
        break;
    case CRYPT_HASH_MODE_SHA224:
        rvalue = EVP_sha224();
        break;
    case CRYPT_HASH_MODE_SHA256:
        rvalue = EVP_sha256();
        break;
    case CRYPT_HASH_MODE_SHA384:
        rvalue = EVP_sha384();
        break;
    case CRYPT_HASH_MODE_SHA512:
        rvalue = EVP_sha512();
        break;
    default:
        ABQ_ERROR_MSG("Unsupported hash mode");
        break;
    }

    if ((NULL != rvalue) && (NULL != hashlen)) {
        *hashlen = (size_t)EVP_MD_size(rvalue);
    }
    return rvalue;
}

/* parasoft-begin-suppress CODSTA-05-3 CODSTA-89-3 "Pointer to pointer expected by definition" */
/* parasoft-begin-suppress CODSTA-86-3 "enum type doesn't need validataion" */
err_t crypt_hash_init(CRYPT_HASH_CTX *ctx, crypt_hash_mode_t mode)
/* parasoft-end-suppress CODSTA-86-3 */
/* parasoft-end-suppress CODSTA-05-3 CODSTA-89-3 */
{
    err_t err = EXIT_SUCCESS;
    EVP_MD_CTX *mdctx = NULL;

    VITAL_NOT_NULL(ctx);

    const EVP_MD *mdtype = openssl_md_type(mode, NULL);

    if (NULL == mdtype) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        mdctx = EVP_MD_CTX_create();
        if (NULL == mdctx) {
            err = ENOMEM;
        }
    }

    if ((EXIT_SUCCESS == err) && (1 != EVP_DigestInit_ex(mdctx, mdtype, NULL))) {
        err = EINVAL;
    }

    if (EXIT_SUCCESS == err) {
        *ctx = mdctx;
    } else {
        if (NULL != mdctx) {
            EVP_MD_CTX_destroy(mdctx);
        }
    }
    return err;
}

/* parasoft-begin-suppress CODSTA-05-3 CODSTA-89-3 "Pointer to pointer expected by definition" */
/* parasoft-begin-suppress CODSTA-86-3 "enum type doesn't need validataion" */
err_t crypt_hmac_init(CRYPT_HASH_CTX *ctx, const void *key, size_t key_len, crypt_hash_mode_t mode)
/* parasoft-end-suppress CODSTA-86-3 */
/* parasoft-end-suppress CODSTA-05-3 CODSTA-89-3 */
{
    err_t err = EXIT_SUCCESS;

    VITAL_NOT_NULL(ctx);

    HMAC_CTX *hmac = NULL;
    const EVP_MD *mdtype = openssl_md_type(mode, NULL);

    if (NULL == mdtype) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
#if OPENSSL_VERSION_NUMBER >= 0x10100000 // OpenSSL >= 1.1.0
        hmac = HMAC_CTX_new();
        if (NULL == hmac) {
            err = ENOMEM;
        }
#else
        hmac = (HMAC_CTX *) abq_calloc(sizeof(HMAC_CTX), 1); // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0023. Assigning memory to object from pre-allocated memory pool."
        if (NULL == hmac) {
            err = ENOMEM;
        } else {
            HMAC_CTX_init(hmac);
        }
#endif
    }

    if (1 != HMAC_Init_ex(hmac, key, (int32_t)key_len, mdtype, NULL)) {
        ABQ_ERROR_MSG("Failed to init HMAC");
        err = EINVAL;
    }

    if (EXIT_SUCCESS == err) {
        *ctx = hmac;
    }else{
        if (NULL != hmac) {
#if OPENSSL_VERSION_NUMBER >= 0x10100000 // OpenSSL >= 1.1.0
            HMAC_CTX_free(hmac);
#else
            HMAC_CTX_cleanup(hmac);
            abq_free(hmac);
#endif
        }
    }
    return err;
}

err_t crypt_hash_update(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len)
{
    VITAL_NOT_NULL(ctx);

    err_t err = EXIT_SUCCESS;
    EVP_MD_CTX *mdctx = (EVP_MD_CTX *)ctx; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0024. Casting from known void pointer."

    if ((input == NULL) || (input_len == 0U)) {
        err = EINVAL;
    }

    if ((EXIT_SUCCESS == err) && (1 != EVP_DigestUpdate(mdctx, input, input_len))) {
        err = EINVAL;
    }
    return err;
}

err_t crypt_hash_finalize(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len,
		uint8_t *hash, size_t hash_buf_len, size_t *hash_len)
{
    VITAL_NOT_NULL(ctx);

    int32_t err = EXIT_SUCCESS;
    EVP_MD_CTX *mdctx = (EVP_MD_CTX *)ctx; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0025. Casting from known void pointer."
    plat_uint_t olen = 0U;

    if ((hash == NULL) || (hash_buf_len == 0U) || (hash_len == NULL)) {
        err = EINVAL;
    }

    if ((input != NULL) && (input_len != 0U) && (EXIT_SUCCESS != crypt_hash_update(mdctx, input, input_len))) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        if(EVP_MD_CTX_size(mdctx) > hash_buf_len){
            err = EOVERFLOW;
        } else if(1 != EVP_DigestFinal_ex(mdctx, hash, &olen)) {
            err = EINVAL;
        } else {
            /* MISRA2012-RULE-15_7-2 requires an 'else' after 'else if' */
        }
        *hash_len = (size_t)olen;
    }

    EVP_MD_CTX_destroy(mdctx);
    return err;
}

err_t crypt_hmac_update(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len) // parasoft-suppress MISRAC2012-RULE_8_7-a-4 "m0026. Keeping externally available for future use"
{
    VITAL_NOT_NULL(ctx);

    err_t err = EXIT_SUCCESS;
    HMAC_CTX *mctx = (HMAC_CTX *)ctx; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0027. Casting from known void pointer."

    if ((input == NULL) || (input_len == 0U)) {
        err = EINVAL;
    }
    if ((EXIT_SUCCESS == err) && (1 != HMAC_Update(mctx, input, input_len))) {
        err = EINVAL;
    }

    return err;
}

err_t crypt_hmac_finalize(CRYPT_HASH_CTX ctx, const uint8_t *input, size_t input_len,
		uint8_t *hash, size_t hash_buf_len, size_t *hash_len)
{
    VITAL_NOT_NULL(ctx);

    int32_t err = EXIT_SUCCESS;
    HMAC_CTX *mctx = (HMAC_CTX *)ctx; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0028. Casting from known void pointer."
    plat_uint_t olen = 0U;

    if ((hash == NULL) || (hash_buf_len == 0U) || (hash_len == NULL)) {
        err = EINVAL;
    }

    if ((input != NULL) && (input_len != 0U) && (EXIT_SUCCESS != crypt_hmac_update(mctx, input, input_len))) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS == err) {
        if(HMAC_size(mctx) > hash_buf_len){
            err = EOVERFLOW;
        } else if(1 != HMAC_Final(mctx, hash, &olen)) {
            err = EINVAL;
        } else {
            /* MISRA2012-RULE-15_7-2 requires an 'else' after 'else if' */
        }
        *hash_len = (size_t)olen;
    }

#if OPENSSL_VERSION_NUMBER >= 0x10100000 // OpenSSL >= 1.1.0
    HMAC_CTX_free(mctx);
#else
    HMAC_CTX_cleanup(mctx);
    abq_free(mctx);
#endif
    return err;
}
