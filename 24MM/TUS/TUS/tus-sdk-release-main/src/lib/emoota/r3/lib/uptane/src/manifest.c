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

#include "upt_internal.h"

uptaneErr_t uptane_create_signature_block(cstr_t input, byte_t *output, size_t outbuf_len) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;
    sigInfo_t sig = {0};

    printf("Create signature block\n");
    //printf("[ RUN      ] abq_sign_body_001\n");
    //printf("[ RUN      ] abq_sign_body_002\n");

    byte_t sha256[SHA256_LENGTH] = {0};
    abst_CalculateDigestSHA256((unsigned char *) sha256, input, strlen(input));

    //printf("[ RUN      ] abq_sign_body_003\n");
    //printf("[ RUN      ] abq_sign_body_004\n");

    abst_GenerateSignatureSHA256((unsigned char *) sha256, SHA256_LENGTH, &sig);

    //printf("[ RUN      ] abq_sign_body_005\n");
    //printf("[ RUN      ] abq_sign_body_006\n");
    // Signature information is already stored into sig.
    //printf("[ RUN      ] abq_sign_body_007\n");

    if (UTIL_NO_ERROR != util_CreateSignatureBlock(&sig, 1U, output, outbuf_len)) {
        err = UPTANE_ERR_RESOURCE;
    }
    //printf("[ RUN      ] abq_sign_body_008\n");
    
    return err;
}

static void hash_string(void *hashctx, cstr_t str, size_t maxlen) {
    abst_SHA256Update(hashctx, str, strnlen(str, maxlen));
}

static uptaneErr_t hash_req_info(cfgMatch_t *cfg) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;
    void *hashctx = NULL;

    abst_SHA256Init(&hashctx);
    if (NULL == hashctx) {
        err = UPTANE_ERR_RESOURCE;
    }
    if (UPTANE_NO_ERROR == err) {
        hash_string(hashctx, cfg->augmentedManifest.dcmInfo.mobileNumber, MAX_MOBILE_NUMBER_SZ);
        hash_string(hashctx, cfg->vin, MAX_VIN_SZ);
        hash_string(hashctx, cfg->augmentedManifest.rxswinInfo, MAX_RXSWIN_SZ);
        hash_string(hashctx, cfg->augmentedManifest.clientInfo.version, MAX_VERSION_SZ);
        hash_string(hashctx, (char *) cfg->augmentedManifest.clientInfo.codehash, SHA256_LENGTH);

        abst_SHA256Update(hashctx, (cbyte_t *) cfg->augmentedManifest.licenseInfo,
                          sizeof(licenseInfo_t) * MAX_TOTAL_LICENSE_INFO);

        abst_SHA256Update(hashctx, (cbyte_t *) cfg->ecusInfo, sizeof(ecuInfo_t) * cfg->ecuInfoNum);
    }
    if (UPTANE_NO_ERROR == err) {
        abst_SHA256Finalize(hashctx, cfg->cfgDgst);
    }
    return err;
}

uptaneErr_t uptane_create_cfg_match_req(void *uptctx,
                                        cfgMatch_t *cfgMatchInfo, syncCheck_t *syncCheckInfo,
                                        const byte_t *upd, const ecuInfo_t *ecusInfo, const uint8_t numEcuInfos,
                                        const byte_t *lastCmpId, const byte_t *rxswinInfo, cstr_t campaign_vin) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;
    bool_t sync_first = false;
    upt_context_t *ctx = (upt_context_t *) uptctx;

    uint64_t time_ms = 0UL;
    if (ABST_NO_ERROR != abst_get_trusted_time(&time_ms)) {
        err = UPTANE_ERR_RESOURCE;
    } else {
        cfgMatchInfo->augmentedManifest.timestamp = time_ms / 1000UL;
    }

    if (UPTANE_NO_ERROR == err) {
        if (ABST_NO_ERROR != abst_get_packageStorage(&cfgMatchInfo->augmentedManifest.packageStorage)) {
            err = UPTANE_ERR_RESOURCE;
        }
    }

    if (UPTANE_NO_ERROR == err) {
        memcpy(cfgMatchInfo->primarySerialNum, ctx->primarySerialNum, MAX_SERIAL_NUM_SZ);

        if ((ABST_NO_ERROR != abst_get_vin(cfgMatchInfo->vin, campaign_vin)) ||
            (ABST_NO_ERROR != abst_get_dcm_info(&cfgMatchInfo->augmentedManifest.dcmInfo)) ||
            (ABST_NO_ERROR != abst_get_client_info(&cfgMatchInfo->augmentedManifest.clientInfo)) ||
            (ABST_NO_ERROR != abst_get_license_info(&cfgMatchInfo->augmentedManifest.licenseInfo[0]))) {
            err = UPTANE_ERR_RESOURCE;
        }
    }

    if (UPTANE_NO_ERROR == err) {
        strncpy(cfgMatchInfo->augmentedManifest.upd, upd, MAX_UPLOAD_REASON_SZ);
        strncpy(cfgMatchInfo->augmentedManifest.rxswinInfo, rxswinInfo, MAX_RXSWIN_SZ);
        strncpy(cfgMatchInfo->augmentedManifest.lastCmpId, lastCmpId, MAX_CAMPAIGN_ID_SZ);
        memcpy(cfgMatchInfo->ecusInfo, ecusInfo, sizeof(ecuInfo_t) * numEcuInfos);
        cfgMatchInfo->ecuInfoNum = numEcuInfos;
    }

    if (UPTANE_NO_ERROR == err) {
        err = hash_req_info(cfgMatchInfo);
    }

    if (UPTANE_NO_ERROR == err) {
        strncpy(syncCheckInfo->upd, cfgMatchInfo->augmentedManifest.upd, MAX_UPLOAD_REASON_SZ);
        memcpy(syncCheckInfo->vin, cfgMatchInfo->vin, MAX_VIN_SZ);
        memcpy(syncCheckInfo->primarySerialNum, cfgMatchInfo->primarySerialNum, MAX_SERIAL_NUM_SZ);
        memcpy(syncCheckInfo->cfgDgst, cfgMatchInfo->cfgDgst, SHA256_LENGTH);
        memcpy(&syncCheckInfo->packageStorage, &cfgMatchInfo->augmentedManifest.packageStorage,
               sizeof(packageStorage_t));
        syncCheckInfo->timestamp = cfgMatchInfo->augmentedManifest.timestamp;
    }

    abst_get_security_attack(cfgMatchInfo->securityAttack);
    return err;
}

#ifdef __cplusplus
}
#endif
