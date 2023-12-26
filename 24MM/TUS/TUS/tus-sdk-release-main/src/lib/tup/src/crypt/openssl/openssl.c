/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* system header */
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/decoder.h>
#else
#include <openssl/pem.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
static const EVP_MD *
tup_get_md(uint32_t algorithm)
{
    switch (algorithm) {
    case TUP_HMAC_SHA256_128:
        return EVP_sha256();
    default:
        break;
    }
    return NULL;
}

static int
tup_import_pkey(const uint8_t keyinfo[64], EVP_PKEY **ppkey)
{
    if (NULL == keyinfo) {
        LOG_ERROR("Invalid argument: keyinfo=%p", keyinfo);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == ppkey) {
        LOG_ERROR("Invalid argument: ppkey=%p", ppkey);
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

    FILE *fp = fopen(filename, "rb");
    if (NULL == fp) {
        LOG_ERROR("fopen(): %p", fp);
        return TUP_PARSER_ERROR_IMPORT_KEY;
    } else {
#if OPENSSL_VERSION_MAJOR >= 3
        OSSL_DECODER_CTX *dctx = OSSL_DECODER_CTX_new_for_pkey(
          ppkey, "PEM", NULL, "EC", 0, NULL, NULL);
        if (NULL == dctx) {
            LOG_ERROR("OSSL_DECODER_CTX_new_for_pkey(): %p", dctx);
            (void)fclose(fp);
            return TUP_PARSER_ERROR_IMPORT_KEY;
        }
        if (!OSSL_DECODER_from_fp(dctx, fp)) {
            LOG_ERROR("OSSL_DECODER_from_fp(): 0x%08x", 0);
            OSSL_DECODER_CTX_free(dctx);
            (void)fclose(fp);
            return TUP_PARSER_ERROR_IMPORT_KEY;
        }
        OSSL_DECODER_CTX_free(dctx);
#else
        *ppkey = PEM_read_PUBKEY(fp, NULL, 0, NULL);
#endif
        (void)fclose(fp);
        if (NULL == *ppkey) {
            LOG_ERROR("*ppkey=%p", *ppkey);
            return TUP_PARSER_ERROR_IMPORT_KEY;
        }
    }
    return 0;
}

static void
tup_release_pkey(EVP_PKEY **ppkey)
{
    EVP_PKEY_free(*ppkey);
}

/* variable */

/* function */
int
tup_verify_ecdsa(const uint8_t keyinfo[64], const uint8_t *data,
  size_t data_len, const uint8_t *signature, size_t sign_len)
{
    int ret;

    ECDSA_SIG *ecdsa_sig = ECDSA_SIG_new();
    if (NULL == ecdsa_sig) {
        LOG_ERROR("ECDSA_SIG_new(): %p", ecdsa_sig);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    BIGNUM *r = BN_bin2bn(signature, sign_len / 2, NULL);
    if (NULL == r) {
        LOG_ERROR("BN_bin2bn(): %p", r);
        ECDSA_SIG_free(ecdsa_sig);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    BIGNUM *s = BN_bin2bn(&signature[sign_len / 2], sign_len / 2, NULL);
    if (NULL == s) {
        LOG_ERROR("BN_bin2bn(): %p", s);
        BN_free(r);
        ECDSA_SIG_free(ecdsa_sig);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    ret = ECDSA_SIG_set0(ecdsa_sig, r, s);
    if (1 != ret) {
        LOG_ERROR("ECDSA_SIG_set0(): 0x%08x", ret);
        BN_free(s);
        BN_free(r);
        ECDSA_SIG_free(ecdsa_sig);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    unsigned char sig_der[104];
    unsigned char *sig_ptr = sig_der;
    int sig_len = i2d_ECDSA_SIG(ecdsa_sig, &sig_ptr);
    ECDSA_SIG_free(ecdsa_sig);

    EVP_PKEY *pkey;
    ret = tup_import_pkey(keyinfo, &pkey);
    if (0 != ret) {
        LOG_ERROR("tup_import_pkey(): 0x%08x", ret);
        return ret;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (NULL == ctx) {
        LOG_ERROR("EVP_MD_CTX_new(): %p", ctx);
        tup_release_pkey(&pkey);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    ret = EVP_DigestVerifyInit(ctx, NULL, EVP_sha1(), NULL, pkey);
    if (1 != ret) {
        LOG_ERROR("EVP_DigestVerifyInit(): 0x%08x", ret);
        EVP_MD_CTX_free(ctx);
        tup_release_pkey(&pkey);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    ret = EVP_DigestVerifyUpdate(ctx, data, data_len);
    if (1 != ret) {
        LOG_ERROR("EVP_DigestVerifyUpdate(): 0x%08x", ret);
        EVP_MD_CTX_free(ctx);
        tup_release_pkey(&pkey);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    ret = EVP_DigestVerifyFinal(ctx, sig_der, sig_len);
    EVP_MD_CTX_free(ctx);
    tup_release_pkey(&pkey);
    if (1 != ret) {
        LOG_ERROR("EVP_DigestVerifyFinal(): 0x%08x", ret);
        return TUP_PARSER_ERROR_VERIFY_SIGNATURE;
    }

    return 0;
}

int
tup_generate_hmac(uint32_t algorithm, const uint8_t *key, size_t key_len,
  const uint8_t *data, size_t data_len, uint8_t *icv, size_t *icv_len)
{
    unsigned int len;
    unsigned char *result;

    const EVP_MD *evp_md = tup_get_md(algorithm);
    if (NULL == evp_md) {
        LOG_ERROR("tup_get_md(): %p", evp_md);
        return TUP_PARSER_ERROR_INVALID_ALGORITHM;
    }

    result = HMAC(evp_md, key, key_len, data, data_len, icv, &len);
    if (NULL == result) {
        LOG_ERROR("HMAC(): %p", result);
        return TUP_PARSER_ERROR_GENERATE_ICV;
    }

    if (icv_len)
        *icv_len = (size_t)len;

    return 0;
}

int
tup_constant_time_memcmp(const void *s1, const void *s2, size_t n)
{
    return CRYPTO_memcmp(s1, s2, n);
}
