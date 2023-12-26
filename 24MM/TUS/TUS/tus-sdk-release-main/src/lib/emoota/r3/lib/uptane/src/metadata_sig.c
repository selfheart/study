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
#ifdef __cplusplus
extern "C" {
#endif

#include "upt_internal.h"

#define MAX_SIG_COUNT (100U) // T1 sample app is not memory constraint, reserve more than enough room for signatures

static uptaneErr_t verify_one_sig(rootMeta_t *root,
        sigInfo_t *sig, byte_t *digest, roleType_t roletype);
static uptaneErr_t verify_sigs_with_root(rootMeta_t *root, sigInfo_t *sigs,
        byte_t *digest, roleType_t role_type);

uptaneErr_t metadata_parse_and_verify_sigs(upt_context_t *ctx, rootMeta_t *root,
        byte_t *json_str, size_t json_len, void *parsed_signed, metaType_t meta_type) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    roleType_t role_type = RT_UNKNOWN;
    secInfo_t sec = {0};

    printf("Uptane Metadata Parsing and Verification:");
    switch(meta_type){
        case     META_ROOT:
            printf(" Root\n");
            break;
        case     META_TIMESTAMP:
            printf(" Timestamp\n");
            break;
        case     META_SNAPSHOT:
            printf(" Snapshot\n");
            break;
        case     META_TARGETS:
            printf(" Targets\n");
            break;
        case     META_AUGMENTED:
            printf(" Augumented\n");
            break;
        case     META_GET_OTA_CMD:
            printf(" Get OTA CMD Response\n");
            break;
        default:
            printf(" Unknown\n");
            break;
    }
    //printf("[ RUN      ] abq_upt_meta_parse_001\n");

    if (UTIL_NO_ERROR != util_FindSignatureSignedBlock(json_str, json_len, &sec)) {
        err = UPTANE_ERR_INVALID_METADATA;
    }
    //printf("[ RUN      ] abq_upt_meta_parse_002\n");

    uint16_t maxSigsNum = MAX_SIG_COUNT;
    sigInfo_t *sigs = (sigInfo_t *)calloc(1, sizeof(sigInfo_t) * maxSigsNum);
//    memset(sigs, 0x00U, sizeof(sigInfo_t) * maxSigsNum);

    if (NULL == sigs) {
        err = UPTANE_ERR_RESOURCE;
    }

    if (UPTANE_NO_ERROR == err) {
        uint16_t validSigNum = 0;
    //printf("[ RUN      ] abq_upt_meta_parse_003\n");
        if (UTIL_NO_ERROR != util_ParseSignature(sec.headSignatures, sec.signaturesLength,
                                                 ctx->json_mem, ctx->json_qty, sigs, maxSigsNum, &validSigNum)) {
            err = UPTANE_ERR_INVALID_METADATA;
        }
    }
    //printf("[ RUN      ] abq_upt_meta_parse_004\n");

    byte_t sha256[SHA256_LENGTH] = {0};
    if (UPTANE_NO_ERROR == err) {
    // Hash function on the current solution is only sha256.
    //printf("[ RUN      ] abq_upt_meta_parse_006\n");
        // Calculate digest of all hash functions used in the signatures. Currently only SHA256 is defined in ICD.
        abst_CalculateDigestSHA256((unsigned char *)sha256, sec.headSigned, sec.signedLength);
    }
    //printf("[ RUN      ] abq_upt_meta_parse_007\n");

    if (UPTANE_NO_ERROR == err) {
        err = metadata_parse_signed(ctx, sec.headSigned, sec.signedLength, parsed_signed, meta_type, &role_type);
    }

    // Uptane-1.0.0 5.4.4.3 => 2 => 3
    if (UPTANE_NO_ERROR == err) {
        // Uptane-1.0.0 5.4.4.3 => 2 => 3 => (1)
        // Uptane-1.0.0 5.4.4.4 => 2
        // Uptane-1.0.0 5.4.4.5 => 3
        // Uptane-1.0.0 5.4.4.6 => 3
        printf("Verify metadata with latest root key.\n");
        err = verify_sigs_with_root(root, sigs, sha256, role_type);
        if ((UPTANE_NO_ERROR == err) && (META_ROOT == meta_type)) {
            // Uptane-1.0.0 5.4.4.3 => 2 => 3 => (2);
            // root must be signed by itself as well
            printf("Verify root metadata with key of itself.\n");
            err = verify_sigs_with_root((rootMeta_t *)parsed_signed, sigs, sha256, role_type);
        }
    }

    free(sigs);
    return err;
}

static uptaneErr_t verify_sigs_with_root(rootMeta_t *root, sigInfo_t *sigs,
        byte_t *digest, roleType_t role_type) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    roleInfo_t *role = NULL;
    uint32_t verified = 0U;

    role = get_role_from_root(root, role_type);
    //printf("[ RUN      ] abq_upt_meta_parse_017\n");
    if (NULL == role) {
        err = UPTANE_ERR_INVALID_METADATA;
    } else {
        for (int32_t i = 0; ((i < MAX_SIG_COUNT) && (0 != sigs[i].value_len)); i++) {// use value_len to indicate end of the list
            err = verify_one_sig(root, &sigs[i], digest, role_type);
            if (UPTANE_NO_ERROR == err) {
    //printf("[ RUN      ] abq_upt_meta_parse_023\n");
                verified++;
            }
            if (verified >= role->threshold) {
                break;
            }
        }
    }

    if (verified < role->threshold) {
    printf("ERR: too few valid signatures\n");
        err = UPTANE_ERR_NOT_ENOUGH_SIGS;
    } else {
        // log message signature verification succeed
    }
    return err;
}

roleInfo_t *get_role_from_root(rootMeta_t *root, roleType_t roletype) {
    roleInfo_t *rvalue = NULL;
    for (uint32_t i = 0U; i < NUM_ROLES; i++) {
        roleInfo_t *r = &root->rootMetaBody.roles[i];
        if ((roletype == r->role) && (0 < r->threshold)) {
            rvalue = r;
            break;
        }
    }
    return rvalue;
}

static uptaneErr_t verify_one_sig(rootMeta_t *root,
        sigInfo_t *sig, byte_t *digest, roleType_t roletype) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    keyInfo_t *key = NULL;
    for (int32_t i = 0;
            (i < MAX_TOTAL_KEY_NUM) && (0 != root->rootMetaBody.keys[i].value_len);// use value_len to indicate end of the list
            i++) {
        keyInfo_t *k = &root->rootMetaBody.keys[i];
        if (UPT_KEYID_EQUAL(k->id, sig->keyid)) {
            key = k;
            break;
        }
    }
    //printf("[ RUN      ] abq_upt_meta_parse_019\n");
    if (NULL != key) {
        bool_t key_in_role = false;
        roleInfo_t *role = get_role_from_root(root, roletype);
        if (NULL != role) {
            for (int32_t ik = 0; ik < MAX_KEY_NUM_PER_ROLE; ik++) {
                if (UPT_KEYID_EQUAL(key->id, role->ids[ik])) {
                    key_in_role = true;
                    break;
                }
            }
        }
        if (!key_in_role) {
            key = NULL;
        }

    } else {
        err = UPTANE_ERR_INVALID_SIG;
    }
    //printf("[ RUN      ] abq_upt_meta_parse_020\n");

    if (NULL != key) {
        //printf("[ RUN      ] abq_upt_meta_parse_005\n");
        // Possible hash function is SHA256 only now.
        //printf("[ RUN      ] abq_upt_meta_parse_008\n");
        if (!UPT_SHA256_EQUAL(digest, sig->hash.digest)) {
            err = UPTANE_ERR_INVALID_SIG;
        }

        //printf("[ RUN      ] abq_upt_meta_parse_021\n");
        if (UPTANE_NO_ERROR == err) {
            if (0 != abst_VerifySignature(key, sig, (unsigned char *)digest)) {
                err = UPTANE_ERR_INVALID_SIG;
            }
        }
        //printf("[ RUN      ] abq_upt_meta_parse_022\n");
    }

    return err;
}

#ifdef __cplusplus
}
#endif
