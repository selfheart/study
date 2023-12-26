/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* system header */
#include <mbedtls/asn1.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha1.h>
#include <mbedtls/version.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if MBEDTLS_VERSION_MAJOR >= 3
#include <mbedtls/constant_time.h>
#else
#include <mbedtls/ssl_internal.h>
#endif

/* user header */
#include "logger.h"
#include "tup_crypt.h"
#include "tup_parser_error_code.h"

/* object macro */
#define TUP_KEYINFO_TUPSIGN \
    {                       \
        1, 0                \
    }

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */
struct pkeyinfo {
    uint8_t keyinfo[64];
    char *filename;
};

/* static variable */
static const struct pkeyinfo pkeyinfo[] = {
    { TUP_KEYINFO_TUPSIGN, "pubkey.pem" } // tentative
};

/* static function */
static const mbedtls_md_info_t *
tup_get_md(uint32_t algorithm)
{
    switch (algorithm) {
    case TUP_HMAC_SHA256_128:
        return mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    default:
        break;
    }
    return NULL;
}

static int
tup_import_pkey(const uint8_t keyinfo[64], mbedtls_pk_context *pkey)
{
    int ret;

    if (NULL == keyinfo) {
        LOG_ERROR("Invalid argument: keyinfo=%p", keyinfo);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == pkey) {
        LOG_ERROR("Invalid argument: pkey=%p", pkey);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    const char *filename = NULL;
    for (size_t i = 0; i < sizeof(pkeyinfo) / sizeof(pkeyinfo[0]); ++i) {
        if (0 ==
          memcmp(keyinfo, pkeyinfo[i].keyinfo, sizeof(pkeyinfo[0].keyinfo))) {
            filename = pkeyinfo[i].filename;
            break;
        }
    }
    if (NULL == filename) {
        char str[128 + 1];
        for (size_t i = 0, j = 0; i < 64; ++i, j += 2)
            snprintf(&str[j], 3, "%02hhx", keyinfo[i]);
        LOG_ERROR("Not found: keyinfo=0x%s", str);
        return TUP_PARSER_ERROR_INVALID_KEY_INFORMATION;
    }

    mbedtls_pk_init(pkey);
    ret = mbedtls_pk_parse_public_keyfile(pkey, filename);
    if (0 != ret) {
        LOG_ERROR("mbedtls_pk_parse_public_keyfile(): 0x%08x", ret);
        return TUP_PARSER_ERROR_IMPORT_KEY;
    }

    return 0;
}

static void
tup_release_pkey(mbedtls_pk_context *ppkey)
{
    (void)ppkey;
}

/* variable */

/* function */
int
tup_verify_ecdsa(const uint8_t keyinfo[64], const uint8_t *data,
  size_t data_len, const uint8_t *signature, size_t sign_len)
{
    int ret;

    /* convert ASN.1 to DER */
    mbedtls_mpi r, s;
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    size_t buflen = sign_len / 2;
    ret = mbedtls_mpi_read_binary(&r, signature, buflen);
    if (0 != ret) {
        LOG_ERROR("mbedtls_mpi_read_binary(): 0x%08x", ret);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }
    ret = mbedtls_mpi_read_binary(&s, signature + buflen, buflen);
    if (0 != ret) {
        LOG_ERROR("mbedtls_mpi_read_binary(): 0x%08x", ret);
        mbedtls_mpi_free(&r);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }
    unsigned char sig[104] = {};
    unsigned char *cur = sig + sizeof(sig);
    size_t len = 0;
    len += mbedtls_asn1_write_mpi(&cur, sig, &s);
    len += mbedtls_asn1_write_mpi(&cur, sig, &r);
    len += mbedtls_asn1_write_len(&cur, sig, len);
    len += mbedtls_asn1_write_tag(
      &cur, sig, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);

    unsigned char hash[20];
#if MBEDTLS_VERSION_MAJOR >= 3
    ret = mbedtls_sha1(data, data_len, hash);
#else
    ret = mbedtls_sha1_ret(data, data_len, hash);
#endif
    if (0 != ret) {
        LOG_ERROR("mbedtls_sha1(): 0x%08x", ret);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    mbedtls_pk_context pkey;
    ret = tup_import_pkey(keyinfo, &pkey);
    if (0 != ret) {
        LOG_ERROR("tup_import_pkey(): 0x%08x", ret);
        return ret;
    }
    ret = mbedtls_pk_verify(
      &pkey, MBEDTLS_MD_SHA1, hash, sizeof(hash), cur, len);
    tup_release_pkey(&pkey);
    if (0 != ret) {
        LOG_ERROR("mbedtls_pk_verify(): 0x%08x", ret);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    return 0;
}

int
tup_generate_hmac(uint32_t algorithm, const uint8_t *key, size_t key_len,
  const uint8_t *data, size_t data_len, uint8_t *icv, size_t *icv_len)
{
    int ret;
    mbedtls_md_context_t ctx;

    const mbedtls_md_info_t *info = tup_get_md(algorithm);
    if (NULL == info) {
        LOG_ERROR("tup_get_md(): %p", info);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    mbedtls_md_init(&ctx);
    ret = mbedtls_md_setup(&ctx, info, /* hamc */ 1);
    if (0 != ret) {
        LOG_ERROR("mbedtls_md_setup(): 0x%08x", ret);
        return TUP_PARSER_ERROR_GENERATE_ICV;
    }

    ret = mbedtls_md_hmac_starts(&ctx, key, key_len);
    if (0 != ret) {
        LOG_ERROR("mbedtls_md_hmac_starts(): 0x%08x", ret);
        return TUP_PARSER_ERROR_GENERATE_ICV;
    }

    ret = mbedtls_md_hmac_update(&ctx, data, data_len);
    if (0 != ret) {
        LOG_ERROR("mbedtls_md_hmac_update(): 0x%08x", ret);
        return TUP_PARSER_ERROR_GENERATE_ICV;
    }

    ret = mbedtls_md_hmac_finish(&ctx, icv);
    if (0 != ret) {
        LOG_ERROR("mbedtls_md_hmac_finish(): 0x%08x", ret);
        return TUP_PARSER_ERROR_GENERATE_ICV;
    }

    if (icv_len)
        *icv_len = (size_t)mbedtls_md_get_size(info);

    mbedtls_md_free(&ctx);

    return 0;
}

int
tup_constant_time_memcmp(const void *s1, const void *s2, size_t n)
{
#if MBEDTLS_VERSION_MAJOR >= 3
    return mbedtls_ct_memcmp(s1, s2, n);
#else
    return mbedtls_ssl_safer_memcmp(s1, s2, n);
#endif
}
