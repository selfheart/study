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

#include "../include/abst_config.h"
#include "abst.h"
#include "emoota_utility.h"
#include "spil_crypt/crypt.h"
#include "spil_file/abq_files.h"
#include "spil_file/spil_file.h"
#include <stdint.h>
#include <string.h>

static void
image_file_name(byte_t *filename, const targetAndCustom_t *target, const changeEvent_t *ce, bool_t encrypted) {
    // platform dependent way to store the image, could be identified by either the reportId or the filename (from target)
    sprintf(filename, "%s%s/%s%s", UPTANE_BASE, UPT_PACKAGE, target->target.fileName, encrypted ? ".encrypted" : "");
}

abst_err_t abst_getImageFileName(char* out, const targetAndCustom_t *target, const changeEvent_t *ce, bool_t encrypted){
    abst_err_t err = ABST_NO_ERROR;

    if (out != NULL) {
        image_file_name(out, target, ce, encrypted);
    } else {
        err = ABST_ERR_INVALID_PARAMETER;
    }
    return err;
}

abst_err_t
abst_DownloadImage(const char *url, const targetAndCustom_t *target,
                    const changeEvent_t *ce, bool_t encrypted, cstr_t rootca) {
    abst_err_t err = ABST_NO_ERROR;

    char filepath[1024] = {0};  // big enough
    image_file_name(filepath, target, ce, encrypted);

    char filename[256] = {0};
    sf_filename_of(filepath, filename, sizeof(filename));

    char dir[512] = {0};
    char *tmp = strstr(filepath, filename);
    int length = (char *) tmp - (char *) filepath;
    strncpy(dir, filepath, length);

    uint32_t http_code = 0;
    err = abst_GETfromCDN((cstr_t) url, NULL, dir, filename, &http_code, rootca);

    return err;
}

abst_err_t abst_VerifyFile(const uint8_t *const digest, cstr_t const filepath, const size_t filesize) {
    abst_err_t err = ABST_NO_ERROR;

    size_t bytes_read = 0U;
    uint32_t size = filesize;
    byte_t buffer[4096];// the bigger the more efficient
    CRYPT_HASH_CTX hash = NULL;
    byte_t sha256[SHA256_LENGTH] = {0};
    sf_info_t sfInfo = {0};

    sf_file_t file = {.mode = SF_MODE_UNKNOWN,
                      .handle = NULL};
    if (0 > sf_open_file(filepath, SF_MODE_READ, &file)) {
        err = ABST_ERR_NO_RESOURCE;
        goto error;
    }

    if (0 > sf_get_file_info(filepath, &sfInfo)) {
        err = ABST_ERR_NO_RESOURCE;
        goto error;
    }

    if (sfInfo.size != filesize) {
        err = ABST_ERR_VERIFICATION_FAILED;
        goto error;
    }


    abst_SHA256Init(&hash);


    while (size > 0) {
        if (0 > sf_read_buffer(&file, (uint8_t *) buffer, sizeof(buffer), &bytes_read)) {
            err = ABST_ERR_VERIFICATION_FAILED;
            break;
        }
        abst_SHA256Update(hash, buffer, bytes_read);
        size -= bytes_read;
    }

    if (0 != size) {
        err = ABST_ERR_VERIFICATION_FAILED;
    }

    if (ABST_NO_ERROR == err) {
        abst_SHA256Finalize(hash, (unsigned char *) sha256);
    }
    sf_close(&file);

    if (0 != memcmp(digest, sha256, SHA256_LENGTH)) {
        err = ABST_ERR_VERIFICATION_FAILED;
    }

error:
    return err;
}

abst_err_t abst_VerifyImage(const targetAndCustom_t *target, const changeEvent_t *ce, bool_t encrypted) {
    abst_err_t err = ABST_NO_ERROR;

    const target_t *tgt = encrypted ? &target->custom.encryptedTarget : &target->target;

    char filename[1024] = {0};// big enough
    image_file_name(filename, target, ce, encrypted);

    err = abst_VerifyFile(tgt->hashes[0].digest, filename, tgt->length);

    return err;
}

static void map_alg_parameters(const targetAndCustom_t *target, crypt_cipher_algorithm_t *alg,
                               crypt_cipher_mode_t *mode, crypt_padding_t *padding) {
    const ecryptdSymKey_t *key = &target->custom.ecryptdSymKey;
    switch (key->ecryptdSymKeyType) {
        case ECRYPTD_SYMKEY_TYPE_AES:
            *alg = CRYPT_CIPHER_ALG_AES;
            break;
        case ECRYPTD_SYMKEY_TYPE_DES:
            *alg = CRYPT_CIPHER_ALG_DES;
            break;
        default:
            // Should never get here
            break;
    }

    switch (key->ecryptdSymKeyAlgMode) {
        case ECRYPTD_SYMKEY_ALG_MODE_CBC:
            *mode = CRYPT_CIPHER_MODE_CBC;
            break;
        default:
            // Should never get here
            break;
    }

    switch (key->paddingScheme) {
        case PADDING_SCHEME_PKCS5:
            *padding = CRYPT_PADDING_PKCS7;
            break;
        default:
            // Should never get here
            break;
    }
}

abst_err_t abst_DecryptImage(const targetAndCustom_t *target, const changeEvent_t *ce, const byte_t *dek, size_t deklen,
                             const byte_t *iv, size_t ivlen) {
    abst_err_t err = ABST_NO_ERROR;

    char src_filename[4096] = {0};// big enough
    image_file_name(src_filename, target, ce, true);
    char dst_filename[4096] = {0};// big enough
    image_file_name(dst_filename, target, ce, false);

    const target_t *enc = &target->custom.encryptedTarget;
    const target_t *dec = &target->target;

    CRYPT_CIPHER_CTX cipher = NULL;

    sf_file_t src = {.mode = SF_MODE_UNKNOWN,
                     .handle = NULL};
    sf_file_t dst =  {.mode = SF_MODE_UNKNOWN,
            .handle = NULL};
    if ((0 > sf_open_file(src_filename, SF_MODE_READ, &src)) ||
        (0 > sf_open_file(dst_filename, SF_MODE_CREATE, &dst))) {
        err = ABST_ERR_NO_RESOURCE;
    }

    if (ABST_NO_ERROR == err) {
        crypt_cipher_algorithm_t alg = CRYPT_CIPHER_ALG_NONE;
        crypt_cipher_mode_t mode = CRYPT_CIPHER_MODE_NONE;
        crypt_padding_t padding = CRYPT_PADDING_PKCS7;

        map_alg_parameters(target, &alg, &mode, &padding);
        if (EXIT_SUCCESS != crypt_cipher_start(&cipher, (const uint8_t *)dek, deklen, CRYPT_CIPHER_DECRYPT,
                                               alg, mode, padding,
                                               (0U != ivlen) ? (const uint8_t *)iv : NULL)) {
            err = ABST_ERR_TRANSACTION_FAILED;
        }
    }
    if (ABST_NO_ERROR == err) {
        size_t bytes_read = 0U;
        uint32_t size = enc->length;
        byte_t buffer[4096];// the bigger the more efficient
        byte_t decbuf[4096];
        size_t declen = 0U;
        while (size > 0) {
            if ((0 > sf_read_buffer(&src, (uint8_t *)buffer, sizeof(buffer), &bytes_read)) ||
                (EXIT_SUCCESS != crypt_cipher_update(cipher, (uint8_t *)buffer, bytes_read, (uint8_t *)decbuf, sizeof(decbuf), &declen))) {
                err = ABST_ERR_TRANSACTION_FAILED;
                break;
            }
            if ((declen > 0) && (0 > sf_writeBuffer(&dst, (uint8_t *)decbuf, declen, &declen))) {
                err = ABST_ERR_TRANSACTION_FAILED;
                break;
            }

            size -= bytes_read;
        }
        if (ABST_NO_ERROR == err) {
            if (EXIT_SUCCESS != crypt_cipher_finalize(cipher, (uint8_t *)decbuf, sizeof(decbuf), &declen)) {
                err = ABST_ERR_TRANSACTION_FAILED;
            } else if ((declen > 0) && (0 > sf_writeBuffer(&dst, (uint8_t *)decbuf, declen, &declen))) {
                err = ABST_ERR_TRANSACTION_FAILED;
            }
        }
    }
    sf_close(&src);
    sf_close(&dst);
    return err;
}


#ifdef __cplusplus
}
#endif
