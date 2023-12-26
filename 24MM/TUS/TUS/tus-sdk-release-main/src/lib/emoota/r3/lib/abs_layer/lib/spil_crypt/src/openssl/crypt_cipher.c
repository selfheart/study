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
#include <ontrac/ontrac/status_codes.h>
#include <spil_crypt/crypt.h>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include "crypt_ctx.h"

static int32_t get_openssl_padding(crypt_padding_t padding) {
    int32_t rvalue = -1;
    switch(padding) {
    case CRYPT_PADDING_PKCS7:
        rvalue = 1;  //OpenSSL v1.0.2 only supports ON/OFF for padding
        break;
    case CRYPT_PADDING_NONE:
        rvalue = 0;
        break;
    default:
        SLOG_E("Unimplemented padding scheme");
        break;
    }
    return rvalue;
}

static const EVP_CIPHER *get_openssl_cipher(crypt_cipher_algorithm_t alg,
		crypt_cipher_mode_t mode, size_t key_len) {
    const EVP_CIPHER *rvalue = NULL;

    if ((CRYPT_CIPHER_ALG_AES == alg) && (CRYPT_CIPHER_MODE_ECB == mode) && (16U == key_len)) {
        rvalue = EVP_aes_128_ecb();
    } else if ((CRYPT_CIPHER_ALG_AES == alg) && (CRYPT_CIPHER_MODE_CBC == mode) && (16U == key_len)) {
        rvalue = EVP_aes_128_cbc();
    } else if ((CRYPT_CIPHER_ALG_AES == alg) && (CRYPT_CIPHER_MODE_ECB == mode) && (24U == key_len)) {
        rvalue = EVP_aes_192_ecb();
    } else if ((CRYPT_CIPHER_ALG_AES == alg) && (CRYPT_CIPHER_MODE_CBC == mode) && (24U == key_len)) {
        rvalue = EVP_aes_192_cbc();
    } else if ((CRYPT_CIPHER_ALG_AES == alg) && (CRYPT_CIPHER_MODE_ECB == mode) && (32U == key_len)) {
        rvalue = EVP_aes_256_ecb();
    } else if ((CRYPT_CIPHER_ALG_AES == alg) && (CRYPT_CIPHER_MODE_CBC == mode) && (32U == key_len)) {
        rvalue = EVP_aes_256_cbc();
    } else {
        SLOG_E("Unimplemented algorithm support");
    }

    if (NULL == rvalue) {
        SLOG_E("Failed to find OpenSSL cipher operation");
    }
    return rvalue;
}

err_t crypt_cipher_start(CRYPT_CIPHER_CTX *ctx, const uint8_t *key, size_t key_len, // parasoft-suppress CODSTA-05-3 CODSTA-89-3 "Pointer to pointer expected by definition"
        crypt_cipher_action_t action,  // parasoft-suppress CODSTA-86-3 "enum type doesn't need validataion"
		crypt_cipher_algorithm_t alg,  // parasoft-suppress CODSTA-86-3 "enum type doesn't need validataion"
		crypt_cipher_mode_t mode,  // parasoft-suppress CODSTA-86-3 "enum type doesn't need validataion"
		crypt_padding_t padding,  // parasoft-suppress CODSTA-86-3 "enum type doesn't need validataion"
		const uint8_t *iv) {
    crypt_cipher_ctx_t *cipher_ctx = NULL;
    const EVP_CIPHER *cipher = NULL;
    int32_t opadding = get_openssl_padding(padding);
    err_t err = EXIT_SUCCESS;

    if ((NULL == ctx) || (NULL == key) || (0 >= opadding) || (0U == key_len)) {
        err = EINVAL;
    } else {
        cipher = get_openssl_cipher(alg, mode, key_len);
        if (NULL == cipher) {
            err = EINVAL;
        }
    }
    if (EXIT_SUCCESS == err) {
        cipher_ctx = (crypt_cipher_ctx_t *) abq_malloc(sizeof(crypt_cipher_ctx_t)); // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0020. Assigning memory to object from pre-allocated memory pool."
        if (NULL == cipher_ctx) {
            err = ENOMEM;
        } else {
            cipher_ctx->cipher = EVP_CIPHER_CTX_new();
            cipher_ctx->action = action;
            if (NULL == cipher_ctx->cipher) {
                err = ENOMEM;
            }
        }
    }
    if (EXIT_SUCCESS == err) {
        if (CRYPT_CIPHER_DECRYPT == action) {
            if (1 != EVP_DecryptInit_ex(cipher_ctx->cipher, cipher, NULL, key, iv)) {
                err = EINVAL;
            }
        } else {
            if (1 != EVP_EncryptInit_ex(cipher_ctx->cipher, cipher, NULL, key, iv)) {
                err = EINVAL;
            }
        }
    }
    if ((EXIT_SUCCESS != err) && (NULL != cipher_ctx)) {
        if (NULL != cipher_ctx->cipher) { EVP_CIPHER_CTX_free(cipher_ctx->cipher); }
        abq_free(cipher_ctx);
        cipher_ctx = NULL;
    }
    if (EXIT_SUCCESS == err) {
        // EVP_CIPHER_CTX_set_padding() is defined as 'always retun 1',
        // and no recovery method is provided for unexpected results.
        (void)EVP_CIPHER_CTX_set_padding(cipher_ctx->cipher, opadding);
        *ctx = cipher_ctx;
    }
    return err;
}

err_t crypt_cipher_update(CRYPT_CIPHER_CTX ctx, const uint8_t *input, size_t input_len,
		uint8_t *output, size_t output_buf_len, size_t *result_len)
{
    VITAL_NOT_NULL(ctx);
    err_t err = EXIT_SUCCESS;
    crypt_cipher_ctx_t *cipher_ctx = (crypt_cipher_ctx_t *)ctx; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0021. Casting from known void pointer used within the same static function."

    if ((NULL == input) || (NULL == output) || (NULL == result_len) ||
            (output_buf_len < (input_len + (size_t)EVP_CIPHER_CTX_block_size(cipher_ctx->cipher)))) {
        err = EINVAL;
    }

    if (EXIT_SUCCESS == err) {
        plat_int_t olen = 0;
        if (CRYPT_CIPHER_DECRYPT == cipher_ctx->action) {
            if (1 != EVP_DecryptUpdate(cipher_ctx->cipher, output, &olen, input, (plat_int_t) input_len)) {
                err = EINVAL;
            }
        } else {
            if (1 != EVP_EncryptUpdate(cipher_ctx->cipher, output, &olen, input, (plat_int_t) input_len)) {
                err = EINVAL;
            }
        }
        *result_len = (size_t)olen;
    }
    if (EXIT_SUCCESS != err) {
        if (NULL != cipher_ctx->cipher) { EVP_CIPHER_CTX_free(cipher_ctx->cipher); }
        abq_free(cipher_ctx);
    }
    return err;
}

err_t crypt_cipher_finalize(CRYPT_CIPHER_CTX ctx,
		uint8_t *output, size_t output_buf_len, size_t *result_len)
{
    VITAL_NOT_NULL(ctx);
    err_t err = EXIT_SUCCESS;
    crypt_cipher_ctx_t *cipher_ctx = (crypt_cipher_ctx_t *)ctx; // parasoft-suppress MISRAC2012-RULE_11_5-a-4 "m0022. Casting from known void pointer used within the same static function."
    plat_int_t olen = 0;

    if ((NULL == output) || (NULL == result_len) ||
            (output_buf_len < (size_t)EVP_CIPHER_CTX_block_size(cipher_ctx->cipher))) {
        err = EINVAL;
    } else {
        if (CRYPT_CIPHER_DECRYPT == cipher_ctx->action) {
            if (1 != EVP_DecryptFinal_ex(cipher_ctx->cipher, output, &olen)) {
                err = EINVAL;
            }
        } else {
            if (1 != EVP_EncryptFinal_ex(cipher_ctx->cipher, output, &olen)) {
                err = EINVAL;
            }
        }
        *result_len = (size_t)olen;
    }
    if (NULL != cipher_ctx->cipher) { EVP_CIPHER_CTX_free(cipher_ctx->cipher); }
    abq_free(cipher_ctx);
    return err;
}
