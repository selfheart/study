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

#ifndef CRYPT_CTX_H
#define CRYPT_CTX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/opensslv.h>

typedef EVP_MD_CTX *crypt_md_ctx_t;

typedef struct for_crypt_cipher_ctx crypt_cipher_ctx_t;

struct for_crypt_cipher_ctx {
    EVP_CIPHER_CTX *cipher;
    crypt_cipher_action_t action;
};

const EVP_MD *openssl_md_type(crypt_hash_mode_t mode, size_t *hashlen);

int32_t openssl_rsa_padding_type(crypt_padding_t padding);

#ifdef __cplusplus
}
#endif

#endif /* CRYPT_CTX_H */

