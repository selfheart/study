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
 *  Copyright (c) 2021 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifdef __cplusplus
extern "C" {
#endif


#include <string.h>
#include <spil_file/ontrac/files.h>
#include "emoota_utility.h"
#include "../include/abst_config.h"
#include "../include/abst.h"


#include "spil_crypt/crypt.h"
#include "spil_crypt/aq_master_key.h"



#ifndef CRYPT_LOAD_ROOTCA_FROM_FILE
abst_err_t abst_crypt_init(cstr_t root_ca, size_t root_ca_len) {
#else
abst_err_t abst_crypt_init(void) {
#endif
    abst_err_t err = ABST_NO_ERROR;

    abq_mutex_t *mutex = ontrac_mutex_get();
    err_t status = abq_files_init(CONFIG_BASE, PERSIST_BASE, 1024*1024*100,
                                  mutex->lock, mutex->unlock, (cvar_t) mutex->mutex);

#ifndef CRYPT_LOAD_ROOTCA_FROM_FILE
    if (EXIT_SUCCESS != crypt_init(root_ca, root_ca_len)) {
#else
    if (EXIT_SUCCESS != crypt_init()) {
#endif
        err = ABST_ERR_NO_RESOURCE;
    }
    return err;
}

abst_err_t abst_get_trusted_time(uint64_t *time_ms) {
    abst_err_t err = ABST_NO_ERROR;
    if (EXIT_SUCCESS != crypt_get_time(time_ms)) {
        err = ABST_ERR_NO_RESOURCE;
    }
    return err;
}


abst_err_t abst_get_uuid(char *uuid, size_t buf_size) {
    abst_err_t err = ABST_NO_ERROR;
    if (EXIT_SUCCESS != crypt_generate_uuid(uuid, buf_size)) {
        err = ABST_ERR_NO_RESOURCE;
    }
    return err;
}

void abst_SHA256Init(void **ctx) {
    // internal call, assume parameters already checked
    EXPECT_IS_OK(crypt_hash_init((CRYPT_HASH_CTX *)ctx, CRYPT_HASH_MODE_SHA256));
}

void abst_SHA256Update(void *ctx, cbyte_t *data, size_t len) {
    // internal call, assume parameters already checked
    EXPECT_IS_OK(crypt_hash_update((CRYPT_HASH_CTX)ctx, (const uint8_t *)data, len));
}

void abst_SHA256Finalize(void *ctx, unsigned char *md_value) {
    // internal call, assume parameters already checked
    size_t hashlen;
    EXPECT_IS_OK(crypt_hash_finalize((CRYPT_HASH_CTX)ctx, NULL, 0, md_value, SHA256_LENGTH, &hashlen));
    ABQ_EXPECT(hashlen == SHA256_LENGTH);
}

void abst_CalculateDigestSHA256(unsigned char *md_value, const char *str, const unsigned long str_len ) {
    // internal call, assume parameters already checked
    CRYPT_HASH_CTX hash_ctx;
    size_t hashlen;

    EXPECT_IS_OK(crypt_hash_init(&hash_ctx, CRYPT_HASH_MODE_SHA256));
    EXPECT_IS_OK(crypt_hash_finalize(hash_ctx, (const uint8_t *)str, str_len, md_value, SHA256_LENGTH, &hashlen));
    ABQ_EXPECT(hashlen == SHA256_LENGTH);
}

static void abst_signature_parse_type(methodType_t type,
        crypt_hash_mode_t *hashmode, crypt_crypt_mode_t *mode) {
    // internal call, assume parameters already checked
    switch (type) {
    case MT_SHA256withRSA:
        *hashmode = CRYPT_HASH_MODE_SHA256;
        *mode = CRYPT_METHOD_RSA;
        break;
    case MT_SHA256withECDSA:
        *hashmode = CRYPT_HASH_MODE_SHA256;
        *mode = CRYPT_METHOD_ECDSA;
        break;
    default:
        // do nothing, should not happen, utility should already made sure the value is in valid range
        break;
    }
}

abst_err_t abst_VerifySignature(const keyInfo_t *key, const sigInfo_t *sig, const unsigned char md_value[EVP_MAX_MD_SIZE]) {
    // internal call, assume parameters already checked
    abst_err_t err = ABST_NO_ERROR;
    crypt_hash_mode_t hashmode;
    crypt_crypt_mode_t mode;

    abst_signature_parse_type(sig->methType, &hashmode, &mode);

    if (EXIT_SUCCESS != crypt_pk_verify_signature(hashmode, mode,
            key->value, key->value_len,
            sig->hash.digest, SHA256_LENGTH,
            sig->value, sig->value_len)) {
        err = ABST_ERR_VERIFICATION_FAILED;
    }
    return err;
}

void abst_GenerateSignatureSHA256(unsigned char *md_value, size_t md_len, sigInfo_t *sig) {
    // internal call, assume parameters already checked
    size_t siglen = 0;
    size_t idlen = 0;
    size_t method_len = 0;
    byte_t method[128] = {0}; // big enough for method string

    EXPECT_IS_OK(crypt_get_master_key_info(&siglen, &idlen, &method_len));
    ABQ_EXPECT(SHA256_LENGTH == idlen);
    ABQ_EXPECT(MAX_SIG_LENGTH >= siglen);
    ABQ_EXPECT(sizeof(method) >= method_len);

    /*
     * keyid directly copied into buffer by lower layer. This means the "keyid" file used by
     * spil_crypt should contain the binary format of the keyid, instead of hex encoded string
     */

    EXPECT_IS_OK(crypt_sign_with_master_key(md_value, md_len,
            sig->value, MAX_SIG_LENGTH, (size_t*)&sig->value_len, sig->keyid, SHA256_LENGTH, &idlen,
                                            (uint8_t*)method, method_len));

    sig->methType = MT_SHA256withRSA;
    if (0 == strncmp(method, "SHA256withECDSA", method_len)) {
        sig->methType = MT_SHA256withECDSA;
    }

    memcpy(sig->hash.digest, md_value, md_len);
    sig->hash.funcType = FT_SHA256;
}

size_t abst_base64_decode(cstr_t source, size_t source_len, byte_t *dest, size_t dest_len) {
    return b64_decode(source, (int32_t) source_len, dest, (int32_t) dest_len);
}

size_t abst_base64_encode(cstr_t source, size_t source_len, byte_t *dest, size_t dest_len) {
    return b64_encode(source, (int32_t) source_len, dest, (int32_t) dest_len, false);
}

abst_err_t abst_decrypt_dek(const byte_t *encrypted, size_t ilen,
                            uint8_t *output, size_t buflen, size_t *olen, padding_t padding) {
    // internal call, assume parameters already checked
    abst_err_t err = ABST_NO_ERROR;
    crypt_padding_t padding_mode = CRYPT_PADDING_PKCS7;
    crypt_hash_mode_t padding_md = CRYPT_HASH_MODE_NONE;

    switch (padding) {
        case OAEP_SHA1_MGF1:
            padding_mode = CRYPT_RSA_PKCS_V21;
            padding_md = CRYPT_HASH_MODE_SHA1;
            break;
        case OAEP_SHA256_MGF1:
            padding_mode = CRYPT_RSA_PKCS_V21;
            padding_md = CRYPT_HASH_MODE_SHA256;
            break;
        case OAEP_SHA384_MGF1:
            padding_mode = CRYPT_RSA_PKCS_V21;
            padding_md = CRYPT_HASH_MODE_SHA384;
            break;
        default:
            // Should never get here. Utility should made sure the padding in valid range.
            break;
    }

    if (EXIT_SUCCESS != crypt_decrypt_with_master_key((const uint8_t*)encrypted, ilen, output, buflen, olen, padding_mode, padding_md)) {
        err = ABST_ERR_TRANSACTION_FAILED;
    }
    return err;
}



#ifdef __cplusplus
}
#endif
