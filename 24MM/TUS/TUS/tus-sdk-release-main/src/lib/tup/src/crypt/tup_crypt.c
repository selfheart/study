/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* system header */
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* user header */
#include "logger.h"
#include "tup_crypt.h"
#include "tup_parser.h"

/* object macro */
#define TUP_KEYINFO_TUPMETA 0x101
#define TUP_KEY_TUPMETA "TUPMETA"
#define TUP_KEYINFO_IPKG_ICV 0x201
#define TUP_KEY_IPKG_ICV "IPKG_ICV"

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */
struct rooticv_keyinfo {
    uint8_t keyinfo[64];
    char *key;
};

struct icv_keyinfo {
    uint32_t keyinfo;
    char *key;
};

/* static variable */
static struct rooticv_keyinfo rooticv_keyinfo[] = { { { 0x01, 0x01 },
  TUP_KEY_TUPMETA } };

static struct icv_keyinfo icv_keyinfo[] = { { TUP_KEYINFO_TUPMETA,
                                              TUP_KEY_TUPMETA },
    { TUP_KEYINFO_IPKG_ICV, TUP_KEY_IPKG_ICV } };

/* static function */

/* variable */

/* function */
bool
tup_check_icv_algorithm(uint32_t algorithm)
{
    switch (algorithm) {
    case TUP_HMAC_SHA256_128:
        return true;
    default:
        break;
    }
    return false;
}

int
tup_get_icv_size(uint32_t algorithm)
{
    switch (algorithm) {
    case TUP_HMAC_SHA256_128:
        return 16;
    default:
        break;
    }
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}

bool
tup_check_icv_key(uint64_t keyinfo)
{
    for (size_t i = 0; i < sizeof(icv_keyinfo) / sizeof(icv_keyinfo[0]); ++i) {
        if (icv_keyinfo[i].keyinfo == keyinfo)
            return true;
    }
    return false;
}

int
tup_get_icv_key_size(uint64_t keyinfo, size_t *psize)
{
    if (NULL == psize) {
        LOG_ERROR("Invalid argument: psize=%p", psize);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < sizeof(icv_keyinfo) / sizeof(icv_keyinfo[0]); ++i) {
        if (icv_keyinfo[i].keyinfo == keyinfo) {
            *psize = strlen(icv_keyinfo[i].key);
            return 0;
        }
    }
    LOG_ERROR("Not found: keyinfo=0x%016" PRIx64, keyinfo);
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}

int
tup_get_icv_key(uint64_t keyinfo, uint8_t *pkey)
{
    if (NULL == pkey) {
        LOG_ERROR("Invalid argument: pkey=%p", pkey);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < sizeof(icv_keyinfo) / sizeof(icv_keyinfo[0]); ++i) {
        if (icv_keyinfo[i].keyinfo == keyinfo) {
            memcpy(pkey, icv_keyinfo[i].key, strlen(icv_keyinfo[i].key));
            return 0;
        }
    }
    LOG_ERROR("Not found: keyinfo=0x%016" PRIx64, keyinfo);
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}

bool
tup_check_rooticv_key(const uint8_t keyinfo[64])
{
    if (NULL == keyinfo) {
        LOG_ERROR("Invalid argument: keyinfo=%p", keyinfo);
        return false;
    }

    for (size_t i = 0; i < sizeof(rooticv_keyinfo) / sizeof(rooticv_keyinfo[0]);
         ++i) {
        if (!memcmp(keyinfo, rooticv_keyinfo[i].keyinfo,
              sizeof(rooticv_keyinfo[0].keyinfo)))
            return true;
    }

    return false;
}

int
tup_get_rooticv_key_size(const uint8_t keyinfo[64], size_t *psize)
{
    if (NULL == psize) {
        LOG_ERROR("Invalid argument: psize=%p", psize);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < sizeof(rooticv_keyinfo) / sizeof(rooticv_keyinfo[0]);
         ++i) {
        if (!memcmp(keyinfo, rooticv_keyinfo[i].keyinfo,
              sizeof(rooticv_keyinfo[0].keyinfo))) {
            *psize = strlen(rooticv_keyinfo[i].key);
            return 0;
        }
    }

    char str[128 + 1];
    for (size_t i = 0, j = 0; i < 64; ++i, j += 2)
        snprintf(&str[j], 3, "%02hhx", keyinfo[i]);
    LOG_ERROR("Not found: keyinfo=0x%s", str);
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}

int
tup_get_rooticv_key(const uint8_t keyinfo[64], uint8_t *pkey)
{
    if (NULL == pkey) {
        LOG_ERROR("Invalid argument: pkey=%p", pkey);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < sizeof(rooticv_keyinfo) / sizeof(rooticv_keyinfo[0]);
         ++i) {
        if (!memcmp(keyinfo, rooticv_keyinfo[i].keyinfo,
              sizeof(rooticv_keyinfo[0].keyinfo))) {
            memcpy(
              pkey, rooticv_keyinfo[i].key, strlen(rooticv_keyinfo[i].key));
            return 0;
        }
    }

    char str[128 + 1];
    for (size_t i = 0, j = 0; i < 64; ++i, j += 2)
        snprintf(&str[j], 3, "%02hhx", keyinfo[i]);
    LOG_ERROR("Not found: keyinfo0x%s", str);
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}

int
tup_generate_icv(uint32_t algorithm, const uint8_t *key, size_t key_len,
  const uint8_t *data, size_t data_len, uint8_t *icv, size_t *icv_len)
{
    switch (algorithm) {
    case TUP_HMAC_SHA256_128:
        return tup_generate_hmac(
          algorithm, key, key_len, data, data_len, icv, icv_len);
    default:
        break;
    }
    LOG_ERROR("Not found: algorihtm=0x%08x", algorithm);
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}

bool
tup_check_sign_algorithm(uint32_t algorithm)
{
    switch (algorithm) {
    case TUP_ECDSA_NIST384P:
        return true;
    default:
        break;
    }
    return false;
}

int
tup_get_sign_len(uint32_t algorithm, size_t *plen)
{
    if (NULL == plen) {
        LOG_ERROR("Invalid argument: plen=%p", plen);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    switch (algorithm) {
    case TUP_ECDSA_NIST384P:
        *plen = 96;
        break;
    default:
        LOG_ERROR("Not found: algorithm=0x%08x", algorithm);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    return 0;
}

int
tup_verify_signature(uint32_t algorithm, uint32_t blocksize,
  const uint8_t keyinfo[64], const uint8_t *data, size_t data_len,
  const uint8_t *signature, size_t sign_len)
{
    (void)blocksize;

    if (!tup_check_sign_algorithm(algorithm)) {
        LOG_ERROR("tup_check_sign_algorith(): %s", "false");
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == keyinfo) {
        LOG_ERROR("Invalid argument: keyinfo=%p", keyinfo);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == data) {
        LOG_ERROR("Invalid argument: data=%p", data);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == signature) {
        LOG_ERROR("Invalid argument: signature=%p", signature);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    switch (algorithm) {
    case TUP_ECDSA_NIST384P:
        return tup_verify_ecdsa(keyinfo, data, data_len, signature, sign_len);
    default:
        break;
    }
    LOG_ERROR("Not found: algorithm=0x%08x", algorithm);
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}
