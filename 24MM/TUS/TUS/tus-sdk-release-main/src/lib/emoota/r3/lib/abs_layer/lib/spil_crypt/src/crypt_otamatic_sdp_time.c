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

#include <spil_os/aqSpil.h>
#include <ontrac/ontrac/primitives.h>
#include <ontrac/ontrac/traverser.h>
#include <ontrac/rest/item_readers.h>
#include <ontrac/ontrac/status_codes.h>
#include <spil_crypt/crypt.h>

#include "crypt_plat.h"
#include "crypt_cmn.h"

typedef struct for_trusted_time_response trusted_time_response_t;
struct for_trusted_time_response {
    /** a trusted timestamp from choreo */
    cstr_t time;
    /** original nonce sent to the backend */
    cstr_t nonce;
    /** base64 encoded signature of: str(time) + nonce */
    cstr_t signature;
    /** ordered list of cstr_t (PEM encoded certificates) used to sign the response*/
    vlist_t *pemCertChain;
    /** deserialized properties not mapped to an above field are stored here */
    ptree_t *internals;
};

DEFINE_DEFAULT_GLOBAL_PROPERTY(trust_time_response_time, trusted_time_response_t,
        time, cstr_t, &string_class, NULL);
DEFINE_DEFAULT_GLOBAL_PROPERTY(trust_time_response_nonce, trusted_time_response_t,
        nonce, cstr_t, &string_class, &trust_time_response_time);
DEFINE_DEFAULT_GLOBAL_PROPERTY(trust_time_response_signature, trusted_time_response_t,
        signature, cstr_t, &string_class, &trust_time_response_nonce);
DEFINE_DEFAULT_GLOBAL_PROPERTY(trust_time_response_pemCertChain, trusted_time_response_t,
        pemCertChain, vlist_t*, &list_of_string_class, &trust_time_response_signature);

static LLIST_HEAD_DEF(property_t, trust_time_response_globals) = &trust_time_response_pemCertChain;

DEFINE_SERIALIZABLE_CLASS(trusted_time_response_class, // parasoft-suppress MISRAC2012-RULE_8_7-a-4 "Reserved as global for future use"
        trusted_time_response_t, (1),
        NULL, NULL, NULL,
        trust_time_response_globals);


static uint8_t last_nonce[256];
static err_t crypt_get_time_nonce(byte_t* nonce, size_t buf_size) {
    err_t err = EXIT_SUCCESS;
    VITAL_NOT_NULL(nonce);
    ABQ_VITAL(0U != buf_size);

    err = crypt_generate_uuid(nonce, buf_size);
    if (EXIT_SUCCESS == err) {
        if (0 == utf8_write_bytes((byte_t*)last_nonce, nonce, -1)) {
            err = EINVAL;
        }
    }
    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}

void crypt_cmn_set_trusted_time_ms(uint64_t truested);
err_t crypt_cmn_get_trusted_time_ms(uint64_t *time_ms);

static err_t crypt_set_time(cstr_t nonce, cstr_t time_str,
        cstr_t signature, cstr_t certs)
{
    err_t err = EXIT_SUCCESS;
    vlist_t *plist = vlist_create(&string_class);
    cstr_t fulltime_str = NULL;

    if ((NULL == nonce) || (NULL == time_str) || (NULL == signature) || (NULL == certs)) {
        err = EINVAL;
    }
    if ((EXIT_SUCCESS == err) && (NULL == plist)) {
        err = ENOMEM;
    }
    if (EXIT_SUCCESS == err) {
        err = vlist_add(plist, time_str);
    }
    if (EXIT_SUCCESS == err) {
        err = vlist_add(plist, nonce);
    }

    if (EXIT_SUCCESS == err) {
        fulltime_str = str_join(plist, "", false);
    }
    if (NULL == fulltime_str) {
        err = ENOMEM;
    }

    CRYPT_HASH_CTX ctx = NULL;
    uint8_t hash[32];
    size_t hashlen = 0;

    if (0 != utf8_compare_exact((byte_t*)last_nonce, nonce, utf8_byte_length(nonce, -1))) {
        err = EINVAL;
    }
    if (EXIT_SUCCESS != crypt_hash_init(&ctx, CRYPT_HASH_MODE_SHA256)) {
        err = EIO;
    }

    if ((EXIT_SUCCESS == err) && (EXIT_SUCCESS != crypt_hash_finalize(ctx, (uint8_t*)fulltime_str,
            (size_t)utf8_byte_length(fulltime_str, -1), hash, sizeof(hash), &hashlen))) {
        err = EINVAL;
    }


#ifdef TRUST_CHAIN_VERIFY_CERT
    if (EXIT_SUCCESS == err) {
        err = crypt_verify_signature_ex(certs, hash, hashlen, signature, true);
    }
#endif /* TRUST_CHAIN_VERIFY_CERT */

    if (EXIT_SUCCESS == err) {
        int64_t trusted = -1;
        if (0 > utf8_read_int(time_str, (size_t)utf8_byte_length(time_str, -1) + 1U, &trusted, 10U)) {
            err = EINVAL;
        }
        else {
            crypt_cmn_set_trusted_time_ms((uint64_t)trusted);
        }
    }
    (void)obj_release_self(fulltime_str);
    (void)obj_release_self(plist);

    if (EXIT_SUCCESS != err) {
        SLOG_E("Security Manager operation failed");
    }
    return err;
}

err_t crypt_time_server_request(char *request, size_t buf_size,
		char *query, size_t query_buf_size, char *url_path, size_t url_buf_size) {
    static const char CHOREO_TRUSTED_TIME_PATH[] = "/ca/api/1.0/time";
    static const char sdp_time_query[] = "nonce";
    err_t rvalue = EXIT_SUCCESS;

    if ((CRYPT_UUID_LEN >= buf_size) || (NULL == request) ||
            (NULL == query) || (sizeof(sdp_time_query) > query_buf_size) ||
            (NULL == url_path) || (sizeof(CHOREO_TRUSTED_TIME_PATH) > url_buf_size)) {
        SLOG_E("Invalid paramters");
        rvalue = EINVAL;
    }

    if (EXIT_SUCCESS == rvalue) {
        rvalue = crypt_get_time_nonce(request, buf_size);
    }

    if (EXIT_SUCCESS == rvalue) {
        if (0 > utf8_write_bytes((byte_t*)query, (cstr_t)sdp_time_query, -1)) {
            rvalue = EINVAL;
        }
    }

    if (EXIT_SUCCESS == rvalue) {
        if (0 > utf8_write_bytes((byte_t*)url_path, (cstr_t)CHOREO_TRUSTED_TIME_PATH, -1)) {
            rvalue = EINVAL;
        }
    }
    return rvalue;
}

err_t crypt_on_time_server_response(const char *time_response, uint64_t *timestamp_ms,
                                    const char *req) {
    VITAL_NOT_NULL(time_response);

    err_t rvalue = EXIT_SUCCESS;
    trusted_time_response_t *trusted_time = NULL;
    cvar_t parsed = str_parse_json(time_response, -1, &trusted_time_response_class);
    CLASS_RESOLVE(trusted_time_response_class, trusted_time_response_t, trusted_time, parsed);

    if ((NULL == time_response) || (NULL == trusted_time) || (NULL == timestamp_ms) || (NULL == req)) {
        rvalue = EINVAL;
    } else {
        cstr_t certs = str_join(trusted_time->pemCertChain, "", false);
        if(NULL == certs) {
            rvalue = EINVAL;
        }else{
            rvalue = crypt_set_time(trusted_time->nonce, trusted_time->time,
                    trusted_time->signature, certs);
            if (EXIT_SUCCESS == rvalue) {
                // Only reseed RNG if time server response is valid.
                // Otherwise attacker could replay the same time server response to reduce
                // randomness of the RNG

                // Signature contains some random data generated on server while signing,
                // so it would be a good source of RNG seed
                crypt_reseed_rng((const uint8_t *)trusted_time->signature,
                        (size_t)utf8_byte_length(trusted_time->signature, -1));
                rvalue = crypt_get_time(timestamp_ms);
            }
        }
        (void)obj_release_self(certs);
    }
    (void)obj_release_self(parsed);
    return rvalue;
}
