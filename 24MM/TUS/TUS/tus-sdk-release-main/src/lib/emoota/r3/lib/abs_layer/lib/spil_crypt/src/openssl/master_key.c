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

/*
 * ECDSA or RSA master key related functions implemented with OpenSSL
 * define MASTER_KEY_USE_RSA to use RSA, otherwise use ECDSA
 */
#include <platform/platformconfig.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/util/byte_buffer.h>
#include <ontrac/ontrac/status_codes.h>
#include <spil_file/ontrac/files.h>
#include <spil_crypt/crypt.h>
#include <spil_crypt/aq_master_key.h>

#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>

#include "crypt_ctx.h"
#include "../crypt_plat.h"

#define MASTER_KEY_USE_RSA

#ifdef MASTER_KEY_USE_RSA
static const uint8_t sign_method[] = "SHA256withRSA";
#define DEFAULT_RSA_KEY_SIZE 2048
#else
static const uint8_t sign_method[] = "SHA256withECDSA";
#endif

#define MASTER_KEY_SIGNING_HASH_METHOD CRYPT_HASH_MODE_SHA256
#define MASTER_KEY_SIGNING_HASH_SIZE 32

typedef struct {
    size_t max_sig_length;
    uint8_t *s_keyid;
    size_t s_keyid_size;
    EVP_PKEY *master_key_priv;
} master_key_data_t;

static err_t aq_load_private_pem(void);
static err_t aq_load_pubkey_id(void);

static master_key_data_t *master_key_data(void) {
    static master_key_data_t mkey_data = {0};
    return &mkey_data;
}

err_t crypt_plat_init_master_key(void) {
    err_t err = EXIT_SUCCESS;

    if (0 == aq_load_private_pem()) {
        SLOG_I("Private key of the Master Key loaded");
    } else {
        SLOG_W("Failed to load Private key of the Master Key");
        err = EINVAL;
    }

    if (EXIT_SUCCESS == err) {
        if (0 == aq_load_pubkey_id()) {
            SLOG_I("Public key of the Master Key loaded");
        } else {
            SLOG_W("Failed to load Public key of the Master Key");
            err = EINVAL;
        }
    }

    return err;
}

void crypt_plat_release_master_key(void) {
    master_key_data_t *mkey = master_key_data();
    if (NULL != mkey->s_keyid) {
        abq_free(mkey->s_keyid);
        mkey->s_keyid = NULL;
        mkey->s_keyid_size = 0;
    }
    if (NULL != mkey->master_key_priv) {
        EVP_PKEY_free(mkey->master_key_priv);
        mkey->master_key_priv = NULL;
    }
}

#ifdef MASTER_KEY_USE_RSA
#define PRIV_PEM_FILENAME "uptane/rsa_priv.pem"
#else
#define PRIV_PEM_FILE "/uptane/ecdsa_priv.pem"
#endif
#define PUB_DER_FILE "/uptane/master_pubkey.der"
#define PUB_ID_FILE "/uptane/keyid"
static err_t aq_load_private_pem(void) {
    err_t err = EXIT_SUCCESS;
    master_key_data_t *mkey = master_key_data();
    if (NULL != mkey->master_key_priv) {
        EVP_PKEY_free(mkey->master_key_priv);
        mkey->master_key_priv = NULL;
    }

    cstr_t pem_fullpath = abq_path_join(OTAMATIC_CONFIG_DIR, PRIV_PEM_FILENAME, abq_file_sys_path);
    BIO *cbio = BIO_new_file(pem_fullpath, "r");
    if (NULL ==cbio) {
        err = ENOMEM;
    } else {
        mkey->master_key_priv = PEM_read_bio_PrivateKey(cbio, NULL, NULL, NULL);
        (void)BIO_free(cbio);
        if (NULL == mkey->master_key_priv) {
            err = EINVAL;
        } else {
            mkey->max_sig_length = (size_t)EVP_PKEY_size(mkey->master_key_priv);
        }
    }
    (void) obj_release_self(pem_fullpath);
    return err;
}
static err_t aq_load_pubkey_id(void) {
    err_t err = EXIT_SUCCESS;
    master_key_data_t *mkey = master_key_data();

    if (NULL != mkey->s_keyid) {
        abq_free(mkey->s_keyid);
        mkey->s_keyid = NULL;
        mkey->s_keyid_size = 0;
    }
    sf_info_t info = {0};
    sf_file_t fp = {0};
#ifdef MASTER_KEY_PUBDER_AS_KEYID
    cstr_t id_fullpath = abq_path_join(OTAMATIC_CONFIG_DIR, PUB_DER_FILE, abq_file_sys_path);
#else
    cstr_t id_fullpath = abq_path_join(OTAMATIC_CONFIG_DIR, PUB_ID_FILE, abq_file_sys_path);
#endif
    if (0 != sf_get_file_info(id_fullpath, &info)) {
        err = EIO;
    }
    if (EXIT_SUCCESS == err) {
        mkey->s_keyid = (uint8_t*) abq_malloc_ex(info.size, NULL);
        if (NULL == mkey->s_keyid) {
            err = ENOMEM;
        } else {
            mkey->s_keyid_size = info.size;
        }
    }
    if (EXIT_SUCCESS == err) {
        size_t readlen = 0;
        if (0 != sf_open_file(id_fullpath, SF_MODE_READ, &fp)) {
            err = EIO;
            SLOG_E("could not open key ID file for reading\n");
        }

        if ((EXIT_SUCCESS == err) && (sf_read_buffer(&fp, mkey->s_keyid, mkey->s_keyid_size, &readlen) < 0)) {
            err = EIO;
        }
        (void)sf_close(&fp);
        if ((EXIT_SUCCESS != err) || (readlen != mkey->s_keyid_size)) {
            err = EIO;
            abq_free(mkey->s_keyid);
            mkey->s_keyid = NULL;
            mkey->s_keyid_size = 0;
        }
    }
    (void) obj_release_self(id_fullpath);
    return err;
}

#ifdef ALLOW_KEY_PROVISION
err_t crypt_on_master_certificate_signed(const uint8_t *cert_chain) {
    err_t ret = ENOSYS;
    // We are not using Choreo to sign the certificate in current architecture design
    SLOG_E("Function not implemented");
    return ret;
}

static err_t store_key_pair(EVP_PKEY *pkey) {
    err_t ret = 0;

    if (0 == ret) {
        BIO *cbio;
        cbio = BIO_new_file(PRIV_PEM_FILE, "wb");
        if (NULL == cbio) {
            ret = ENOMEM;
        } else {
            if (1 != PEM_write_bio_PrivateKey(cbio, pkey, NULL, NULL, 0, NULL, NULL)) {
                ret = EINVAL;
                SLOG_E("Failed to store PEM of private key");
            } else {
                SLOG_I("Private key PEM stored");
            }
        }
        BIO_free(cbio);
    }
    if (0 == ret) {
        BIO *cbio;
        cbio = BIO_new_file(PUB_DER_FILE, "wb");
        if (NULL == cbio) {
            ret = ENOMEM;
        } else {
            if (1 != i2d_PUBKEY_bio(cbio, pkey)) {
                ret = EINVAL;
                SLOG_E("Failed to store DER of public key");
            } else {
                SLOG_I("Public key DER stored");
            }
        }
        BIO_free(cbio);
    }
    if (0 == ret) {
        if ((0 != aq_load_private_pem()) ||
#ifdef MASTER_KEY_PUBDER_AS_KEYID  // TODO: write key id file
                (0 != aq_load_pubkey_id())
#else
                FALSE
#endif
                ) {
            SLOG_E("Failed to load the key pair back");
            ret = EINVAL;
        }
    }
    return ret;
}

err_t crypt_provision_master_key(const uint8_t *subj_name,
        uint8_t *csr, size_t buflen) {
    err_t ret = 0;
    EVP_PKEY *pkey = NULL;
#ifdef MASTER_KEY_USE_RSA
    int32_t keybits = DEFAULT_RSA_KEY_SIZE;
    RSA *rsa = RSA_new();
    BIGNUM *bn = BN_new();
    if ((NULL == bn) || (1 != BN_set_word(bn, RSA_F4))) {
        ret = EINVAL;
    }
    if ((0 == ret) && ((NULL == rsa) || (1 != RSA_generate_key_ex(rsa, keybits, bn, NULL)))) {
        ret = EINVAL;
    }

    if (0 == ret) {
        pkey = EVP_PKEY_new();
        if (NULL == pkey) {
            ret = ENOMEM;
        } else if (1 != EVP_PKEY_set1_RSA(pkey, rsa)) {
            ret = EINVAL;
        }
    }
    if (NULL != bn) { BN_free(bn); }
    if (NULL != rsa) { RSA_free(rsa); }
#else
    EC_KEY *eckey = NULL;
    // Setup the group and NID
    eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1); // Equivalent to SECP256R1
    if (NULL == eckey) {
        ret = EINVAL;
    } else {
        EC_KEY_set_asn1_flag(eckey, OPENSSL_EC_NAMED_CURVE);
        // Now generate the key
        if (1 != EC_KEY_generate_key(eckey)) {
            ret = EINVAL;
        }
    }

    if (0 == ret) {
        pkey = EVP_PKEY_new();
        if (NULL == pkey) {
            ret = ENOMEM;
        } else if (1 != EVP_PKEY_set1_EC_KEY(pkey, eckey)) {
            ret = EINVAL;
        }
    }
    if (NULL != eckey) { EC_KEY_free(eckey); }
#endif

    if (0 == ret) {
        ret = store_key_pair(pkey);
    }
    if (NULL != pkey) { EVP_PKEY_free(pkey); }
    return ret;
}
#endif /* ALLOW_KEY_PROVISION */

err_t crypt_get_master_key_info(size_t *max_siglen, size_t *max_keyid_len, size_t *method_len) {
    err_t err = EXIT_SUCCESS;
    master_key_data_t *mkey = master_key_data();

    if ((NULL == max_siglen) || (NULL == max_keyid_len) || (NULL == method_len)) {
        err = EINVAL;
    } else {
        *max_siglen = mkey->max_sig_length;
        *max_keyid_len = mkey->s_keyid_size;
        *method_len = sizeof(sign_method);
    }

    return err;
}

err_t crypt_sign_with_master_key(const uint8_t *data, size_t datalen,
        uint8_t *sig, size_t sigbuflen, size_t *siglen,
        uint8_t *keyid, size_t keyidbuflen, size_t *keyidlen,
        uint8_t *method, size_t method_buf_len) {
    err_t ret = 0;
    master_key_data_t *mkey = master_key_data();

    if ((NULL == method) || (method_buf_len < sizeof(sign_method)) ||
            (NULL == keyid) || (keyidbuflen < mkey->s_keyid_size) || (NULL == keyidlen) ||
            (NULL == mkey->s_keyid) || (NULL == siglen)) {
        ret = EINVAL;
    }
    CRYPT_HASH_CTX ctx = NULL;
    uint8_t hash[MASTER_KEY_SIGNING_HASH_SIZE] = {0};
    size_t hashlen = 0;
    const EVP_MD *mdtype = openssl_md_type(MASTER_KEY_SIGNING_HASH_METHOD, &hashlen);
    if ((0 == ret) && (EXIT_SUCCESS != crypt_hash_init(&ctx, MASTER_KEY_SIGNING_HASH_METHOD))) {
        ret = EIO;
    }

    if ((0 == ret) && (EXIT_SUCCESS != crypt_hash_finalize(ctx, data, datalen, hash, sizeof(hash), &hashlen))) {
        ret = EINVAL;
    }

    EVP_PKEY_CTX *pkey_ctx = NULL;
    if (0 == ret) {
        pkey_ctx = EVP_PKEY_CTX_new(mkey->master_key_priv, NULL);
        if (NULL == pkey_ctx) {
            ret = EINVAL;
        }
    }
    if ((0 == ret) && (1 != EVP_PKEY_sign_init(pkey_ctx))) {
        ret = EINVAL;
    }
    if ((0 == ret) && (1 != EVP_PKEY_CTX_set_signature_md(pkey_ctx, mdtype))) {
        ret = EINVAL;
    }
    size_t slen = sigbuflen;
    if (0 == ret) {
        if (1 != EVP_PKEY_sign(pkey_ctx, sig, &slen, hash, hashlen)) {
            ret = EINVAL;
        } else {
            *siglen = slen;
        }
    }
    if (NULL != pkey_ctx) { EVP_PKEY_CTX_free(pkey_ctx); }

    if (0 == ret) {
        bytes_copy(keyid, mkey->s_keyid, mkey->s_keyid_size);
        *keyidlen = mkey->s_keyid_size;
        bytes_copy(method, sign_method, sizeof(sign_method));
    }
    return ret;
}

err_t crypt_decrypt_with_master_key(const uint8_t *encrypted, size_t ilen,
        uint8_t *output, size_t buflen, size_t *olen,
        crypt_padding_t padding, crypt_hash_mode_t padding_md) {
    err_t ret = 0;
    master_key_data_t *mkey = master_key_data();
    (void) padding_md; // OpenSSL does not use this parameter but other crypto libs do
    if ((NULL == encrypted) || (0U == ilen) || (NULL == output) || (0U == buflen) ||
            (NULL == olen)) {
        ret = EINVAL;
    }

    EVP_PKEY_CTX *pkey_ctx = NULL;
    if (0 == ret) {
        pkey_ctx = EVP_PKEY_CTX_new(mkey->master_key_priv, NULL);
        if (NULL == pkey_ctx) {
            ret = EINVAL;
        }
    }
    if ((0 == ret) && (1 != EVP_PKEY_decrypt_init(pkey_ctx))) {
        ret = EINVAL;
    }
    if ((0 == ret) && (EVP_PKEY_RSA == EVP_PKEY_base_id(mkey->master_key_priv)) &&
            (1 != EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, openssl_rsa_padding_type(padding)))) {
        ret = EINVAL;
    }
    size_t outlen = buflen;
    if ((0 == ret) && (1 != EVP_PKEY_decrypt(pkey_ctx, output, &outlen, encrypted, ilen))) {
        ret = EINVAL;
    }
    if (0 == ret) {
        *olen = outlen;
    }

    if (NULL != pkey_ctx) { EVP_PKEY_CTX_free(pkey_ctx); }
    return ret;
}
