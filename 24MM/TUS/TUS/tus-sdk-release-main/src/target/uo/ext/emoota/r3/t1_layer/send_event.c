/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abst.h"
#include "uptane/uptane.h"

#include "t1_layer.h"


sdpv3_error_t create_base_event(event_t *ntf_event, int mem_clear,
                                byte_t *campaign_id, uint16_t *num_events,
                                unionFlag_t union_flag, eventMode_t event_mode,
                                eventType_t event_type, eventStatus_t status)
{
    // clear any previous notifications
    if (mem_clear) {
        //memset(ntf_event, 0x00U, sizeof(event_t) * MAX_NOTIFICATIONS);
        // FIXME: currently *num_events=1
        memset(ntf_event, 0x00U, sizeof(event_t));
    }
    *num_events = 1U;   // t1_ctx->ntf.eventsNum

    // Get UUID
    abst_err_t err;
    err = abst_get_uuid(ntf_event->eventUuid, UUID_LEN);
    if (ABST_NO_ERROR != err) {
        printf("abst_get_uuid() failed: %s:%d\n", __func__, __LINE__);
        printf("UUID cannot be generated.\n");
        return (err == ABST_ERR_NO_RESOURCE)
                    ? SDPV3_ERROR_NO_MEMORY : SDPV3_ERROR_UNKNOWN;
    }
    strncpy(ntf_event->campaignId, campaign_id, MAX_CAMPAIGN_ID_SZ);
    ntf_event->unionFlag = union_flag;
    ntf_event->eventMode = event_mode;
    ntf_event->eventType = event_type;
    ntf_event->status = status;

    uint64_t time_ms = 0UL;
    err = abst_get_trusted_time(&time_ms);
    if (ABST_NO_ERROR == err) {
        ntf_event->currentTime.time = time_ms;
    } else {
        printf("abst_get_trusted_time() failed: %s:%d\n", __func__, __LINE__);
        return (err == ABST_ERR_NO_RESOURCE)
                    ? SDPV3_ERROR_NO_MEMORY : SDPV3_ERROR_UNKNOWN;
    }

    return SDPV3_ERROR_NONE;
}

sdpv3_error_t send_event(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    byte_t signed_buf[2<<10] = {};
    byte_t sig_buf[2<<10] = {};

    utilErr_t util_err;
    util_err = util_CreateNotificationBody(&t1_ctx->ntf,
                                            signed_buf, sizeof(signed_buf));
    if (util_err != UTIL_NO_ERROR) {
        printf("util_CreateNotificationBody() failed:%d %s:%d\n",
                util_err, __func__, __LINE__);
        return (util_err == UTIL_ERR_INVALID_PARAMETER)
                        ? SDPV3_ERROR_INVALID_ARGUMENT : SDPV3_ERROR_UNKNOWN;
    }

    uptaneErr_t upt_err;
    upt_err = uptane_create_signature_block(signed_buf, sig_buf, 2<<10);
    if (UPTANE_NO_ERROR != upt_err) {
        printf("uptane_create_signature_block() failed: %s:%d\n",
                __func__, __LINE__);
        return (upt_err == UPTANE_ERR_RESOURCE)
                        ? SDPV3_ERROR_NO_MEMORY : SDPV3_ERROR_UNKNOWN;
    }

    // 100U: plenty of extra space
    size_t full_req_len = strlen(sig_buf) + strlen(signed_buf) + 100U;

    // Optional, if underline networking lib supports sending it in blocks,
    // then no need for this
    memset(t1_ctx->requestBuffer, 0x00U, T1_BUF_SZ);
    snprintf(t1_ctx->requestBuffer, full_req_len,
            "{\"signatures\":%s, \"signed\":%s}", sig_buf, signed_buf);

    //printf("Notification: \n%s\n", t1_ctx->requestBuffer);
    uint32_t http_code = 0;
    memset(t1_ctx->responseBuffer, 0x00U, T1_BUF_SZ);
    t1_ctx->responseBuffer_sz = T1_BUF_SZ;

    abst_err_t abst_err;
    abst_err = abst_SLC_transaction(sdp_api_notification,
                                    t1_ctx->requestBuffer,
                                    strlen(t1_ctx->requestBuffer),
                                    t1_ctx->responseBuffer,
                                    &t1_ctx->responseBuffer_sz,
                                    t1_ctx->ntf.events[0U].eventUuid,
                                    &http_code,
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
