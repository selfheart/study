/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "abst.h"
#include "uptane/uptane.h"

#include "t1_layer.h"


static sdpv3_error_t
create_command(const t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    byte_t signed_buf[2<<10] = {};
    byte_t sig_buf[2<<10] = {};

    getOtaCmd_t otaCmdInfo = {
            .lastUid = t1_ctx->lastUid,
    };

    abst_err_t abst_err;
    abst_err = abst_get_vin(otaCmdInfo.vin, sdpv3_cfg->campaign_vin);
    if (abst_err != ABST_NO_ERROR) {
        printf("abst_get_vin() failed: %s:%d\n", __func__, __LINE__);
        return SDPV3_ERROR_UNKNOWN;
    }
    abst_err = abst_get_primary_serialno(otaCmdInfo.primarySerialNum,
                                            sdpv3_cfg->primary_serial);
    if (abst_err != ABST_NO_ERROR) {
        printf("abst_get_primary_serialno() failed: %s:%d\n", __func__, __LINE__);
        return SDPV3_ERROR_UNKNOWN;
    }

    utilErr_t util_err;
    util_err = util_CreateGetOtaCmdBody(&otaCmdInfo, signed_buf, 2<<10);
    if (util_err != UTIL_NO_ERROR) {
        printf("util_CreateGetOtaCmdBody() failed:%d %s:%d\n",
                util_err, __func__, __LINE__);
        return (util_err == UTIL_ERR_INVALID_PARAMETER)
                ? SDPV3_ERROR_INVALID_ARGUMENT : SDPV3_ERROR_UNKNOWN;
    }

    uptaneErr_t upt_err;
    upt_err = uptane_create_signature_block(signed_buf, sig_buf, 2<<10);
    if (upt_err != UPTANE_NO_ERROR) {
        printf("uptane_create_signature_block() failed: %s:%d\n",
                __func__, __LINE__);
        return (upt_err == UPTANE_ERR_RESOURCE) ?
                        SDPV3_ERROR_NO_MEMORY : SDPV3_ERROR_UNKNOWN;
    }

    // 100U: plenty of extra space
    size_t full_req_len = strlen(sig_buf) + strlen(signed_buf) + 100U;

    // Optional, if underline networking lib supports sending it in blocks,
    // then no need for this
    if (NULL == t1_ctx->requestBuffer) {
        printf("requestBuffer is NULL\n");
        return SDPV3_ERROR_UNKNOWN;
    }
    memset(t1_ctx->requestBuffer, 0x00U, T1_BUF_SZ);
    snprintf(t1_ctx->requestBuffer, full_req_len,
            "{\"signatures\":%s, \"signed\":%s}", sig_buf, signed_buf);

    //printf("getOtaCommandRequest: \n%s\n", t1_ctx->requestBuffer);

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t
send_command(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    uint32_t http_code = 0U;

    memset(t1_ctx->responseBuffer, 0x00U, T1_BUF_SZ);
    t1_ctx->responseBuffer_sz = T1_BUF_SZ;

    abst_err_t abst_err;
    abst_err = abst_SLC_transaction(sdp_api_getOtaCmd,
                                    t1_ctx->requestBuffer,
                                    strlen(t1_ctx->requestBuffer),
                                    t1_ctx->responseBuffer,
                                    &t1_ctx->responseBuffer_sz,
                                    NULL, &http_code,
                                    sdpv3_cfg->sdp_url,
                                    sdpv3_cfg->sdp_port,
                                    sdpv3_cfg->rootca);
    if (ABST_NO_ERROR != abst_err) {
        printf("abst_SLC_transaction() failed: %s:%d\n", __func__, __LINE__);
        return (abst_err == ABST_ERR_TRANSACTION_FAILED)
                        ? SDPV3_ERROR_TRANSACTION : SDPV3_ERROR_UNKNOWN;
    }
    if (200 != http_code) {
        printf("abst_SLC_transaction(): http status code != 200  %s:%d\n",
                __func__, __LINE__);
        return SDPV3_ERROR_TRANSACTION;
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t parse_command(t1_ctx_t *t1_ctx, void *upt_ctx)
{
    getOtaCmdRes_t otaCmdRes = {};

    uptaneErr_t upt_err;
    upt_err = check_OtaCmd_response(upt_ctx,
                                    t1_ctx->responseBuffer,
                                    t1_ctx->responseBuffer_sz,
                                    &otaCmdRes);
    if (upt_err != UPTANE_NO_ERROR) {
        printf("check_OtaCmd_response() failed:%d %s:%d\n",
                upt_err, __func__, __LINE__);
        if (upt_err == UPTANE_ERR_INVALID_METADATA) {
            return SDPV3_ERROR_INVALID_ARGUMENT;
        } else if (upt_err == UPTANE_ERR_RESOURCE) {
            return SDPV3_ERROR_NO_MEMORY;
        } else {
            return SDPV3_ERROR_UNKNOWN;
        }
    }

    t1_ctx->cmdId = otaCmdRes.getOtaCmdResBody.cmdId;
    t1_ctx->otaCmdType = otaCmdRes.getOtaCmdResBody.getOtaCmdType;

    switch (t1_ctx->otaCmdType) {
        case OTA_CMD_TYPE_NOT_APPLICABLE:
            // Nothing to do;
            printf("INFO: No OTA command.\n");
            break;
        case OTA_CMD_TYPE_CONFIG_MATCH:
            // Do Config Match.
            // This sample application omits to do action for OTA command.
            printf("INFO: Received Config Match command.\n");
            printf("INFO: This sample application omits to do action for OTA command.\n");
            break;
        case OTA_CMD_TYPE_UPLD_LOG:
            // Do file upload.
            // This sample application omits to do action for OTA command.
            printf("INFO: Received Upload File command.\n");
            printf("INFO: This sample application omits to do action for OTA command.\n");
            break;
        default:
            break;
    }

    return SDPV3_ERROR_NONE;
}

sdpv3_error_t
run_get_cmd(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx)
{
    sdpv3_error_t ret;
    // For production, receiving SMS message will trigger this process.

    uptaneErr_t upterr;
    upterr = upt_get_dir_root_up_to_data(upt_ctx,
                                        sdpv3_cfg->cdn_url,
                                        sdpv3_cfg->cdn_port,
                                        sdpv3_cfg->rootca);
    if (UPTANE_NO_ERROR == upterr) {
        printf("Create HTTP request for getting OTA command\n");
        if ((ret = create_command(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("create_command() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        printf("Send HTTP request for getting OTA command\n");
        if ((ret = send_command(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_command() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        printf("Parse HTTP response for getting OTA command\n");
        if ((ret = parse_command(t1_ctx, upt_ctx)) != SDPV3_ERROR_NONE) {
            printf("parse_command() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    } else {
        printf("upt_get_dir_root_up_to_data() failed: %s:%d\n",
                __func__, __LINE__);
        return SDPV3_ERROR_UNKNOWN;
    }

    return SDPV3_ERROR_NONE;
}

