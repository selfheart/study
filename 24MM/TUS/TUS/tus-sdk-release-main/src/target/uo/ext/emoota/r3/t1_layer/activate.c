/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>

#include "uptane/uptane.h"

#include "t1_layer.h"


static sdpv3_error_t wait_activation(t1_ctx_t *t1_ctx)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_RMSTATE, EVENT_MODE_ACTIVE,
                            EVENT_RMSTATE, EVENT_STATUS_SUCCESS);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.rmStateExtras.stateScope = EVENT_STATE_SCOPE_VEHICLE;
    ntf_event->eventExtras.rmStateExtras.rmVehState =
                                    EVENT_VEHICLE_STATE_WAIT_ACTIV_USER_ACCPT;

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t activation_approval(t1_ctx_t *t1_ctx)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_NO_EXTRAS, EVENT_MODE_ACTIVE,
                            EVENT_ACTIVATE, EVENT_STATUS_ACCEPTED);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t
sync_complete(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    if (NULL == selected_campaign) {
        return SDPV3_ERROR_NONE;
    }

    for (unsigned int ice = 0U;
            ((ice < MAX_CHANGE_EVENTS) && (ice < selected_campaign->ceNum));
            ice++) {
        t1_ctx->ce = &selected_campaign->changeEvents[ice];

        ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                selected_campaign->campaignId,
                &t1_ctx->ntf.eventsNum,
                UNION_FLAG_NO_EXTRAS, EVENT_MODE_ACTIVE,
                EVENT_SYNC_COMPLETE, EVENT_STATUS_SUCCESS);
        if (ret != SDPV3_ERROR_NONE) {
            printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t
ecu_consistency_complete(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    if (NULL == selected_campaign) {
        return SDPV3_ERROR_NONE;
    }

    for (unsigned int ice = 0U;
            ((ice < MAX_CHANGE_EVENTS) && (ice < selected_campaign->ceNum));
            ice++) {
        t1_ctx->ce = &selected_campaign->changeEvents[ice];

        ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                                selected_campaign->campaignId,
                                &t1_ctx->ntf.eventsNum,
                                UNION_FLAG_RMSTATE, EVENT_MODE_ACTIVE,
                                EVENT_RMSTATE, EVENT_STATUS_SUCCESS);
        if (ret != SDPV3_ERROR_NONE) {
            printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        ntf_event->eventExtras.rmStateExtras.stateScope =
                                    EVENT_STATE_SCOPE_VEHICLE;
        ntf_event->eventExtras.rmStateExtras.rmVehState =
                                    EVENT_VEHICLE_STATE_SYSTEM_SYNC_CHK_COMP;
        strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t
ota_process_after_reprog(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    if (NULL == selected_campaign) {
        return SDPV3_ERROR_NONE;
    }

    for (unsigned int ice = 0U;
            ((ice < MAX_CHANGE_EVENTS) && (ice < selected_campaign->ceNum));
            ice++) {
        t1_ctx->ce = &selected_campaign->changeEvents[ice];

        ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                                selected_campaign->campaignId,
                                &t1_ctx->ntf.eventsNum,
                                UNION_FLAG_RMSTATE, EVENT_MODE_ACTIVE,
                                EVENT_RMSTATE, EVENT_STATUS_SUCCESS);
        if (ret != SDPV3_ERROR_NONE) {
            printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        ntf_event->eventExtras.rmStateExtras.stateScope =
                                                EVENT_STATE_SCOPE_VEHICLE;
        ntf_event->eventExtras.rmStateExtras.rmVehState =
                                                EVENT_VEHICLE_STATE_POST_PROG;
        strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t
ota_complete(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    if (NULL == selected_campaign) {
        return SDPV3_ERROR_NONE;
    }

    // Clear any previous notifications
    //memset(t1_ctx->ntf.events, 0x00U, sizeof(event_t) * MAX_NOTIFICATIONS);
    // FIXME: currently t1_ctx->ntf.eventsNum = 1
    memset(ntf_event, 0x00U, sizeof(event_t));

    for (unsigned int ice = 0U;
            ((ice < MAX_CHANGE_EVENTS) && (ice < selected_campaign->ceNum));
            ice++) {
        t1_ctx->ce = &selected_campaign->changeEvents[ice];
        // On this application, elementId is used as ecuSoftwareId.
        // For production, please put appropriate ecuSoftwareId.
        strncpy(ntf_event->eventExtras.actvtExtras.eventEcuInfo[ice].ecuSoftwareId,
                t1_ctx->ce->elementId, MAX_ECU_SW_ID_SZ);
        // On this application, reportId is used as targetId.
        // For production, please put appropriate targetId.
        strncpy(ntf_event->eventExtras.actvtExtras.eventEcuInfo[ice].targetId,
                t1_ctx->ce->reportId, MAX_TARGET_ID_SZ);
        ntf_event->eventExtras.actvtExtras.eventEcuInfo[ice].rewriteBank =
                                        sdpv3_cfg->ecusInfo[0].active_bank.bank;
        if (sdpv3_cfg->nty_err_code == EVENT_STATUS_SUCCESS) {
            ntf_event->eventExtras.actvtExtras.eventEcuInfo[ice].updateStatus =
                                                    EVENT_UPDATE_STATUS_SUCCESS;
        } else if (sdpv3_cfg->nty_err_code == EVENT_STATUS_FAILURE) {
            ntf_event->eventExtras.actvtExtras.eventEcuInfo[ice].updateStatus =
                                                    EVENT_UPDATE_STATUS_FAILURE;
        } else {
            printf("unknown status code: %s:%d\n", __func__, __LINE__);
            return SDPV3_ERROR_INVALID_ARGUMENT;
        }
    }

    ret = create_base_event(ntf_event, EVENT_MEM_NOT_CLEAR, // not clear
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_ACTVT, EVENT_MODE_ACTIVE,
                            EVENT_ACK_INSTALL, EVENT_STATUS_ACCEPTED);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.actvtExtras.ecuInfoNum = selected_campaign->ceNum;
    if (sdpv3_cfg->nty_err_code == EVENT_STATUS_SUCCESS) {
        ntf_event->eventExtras.actvtExtras.updateStatus =
                                                    EVENT_UPDATE_STATUS_SUCCESS;
    } else if (sdpv3_cfg->nty_err_code == EVENT_STATUS_FAILURE) {
        ntf_event->eventExtras.actvtExtras.updateStatus =
                                                    EVENT_UPDATE_STATUS_FAILURE;
    } else {
        printf("unknown status code: %s:%d\n", __func__, __LINE__);
        return SDPV3_ERROR_INVALID_ARGUMENT;
    }
    {
        // Note: if original 'reportId' was set as-is, SDPv3 server
        //   treats the report "updated (unknown path)",
        //   not expected "updated (OTA success)" (for some unknown reasons).
        // Try to extract 'cid=XX' from "cid=XX,ct=YY,ce=ZZ" for now.
        const char *head = strstr(t1_ctx->ce->reportId, "cid=");
        if (head) {
            const char *tail = strchr(head, ',');
            if (tail) {
                if (tail - head < (ptrdiff_t)sizeof(t1_ctx->ntf.events[0U].reportId)) {
                    memcpy(t1_ctx->ntf.events[0U].reportId, head, tail - head);
                } else {
                    // invalid reportId?
                    // FIXME: error handling is not implemented at all?
                }
            } else {
                strncpy(t1_ctx->ntf.events[0U].reportId, head,
                        sizeof(t1_ctx->ntf.events[0U].reportId) - 1);
            }
        }
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t
activation_complete(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    if (NULL == selected_campaign) {
        return SDPV3_ERROR_NONE;
    }

    for (unsigned int ice = 0U;
            ((ice < MAX_CHANGE_EVENTS) && (ice < selected_campaign->ceNum));
            ice++) {
        t1_ctx->ce = &selected_campaign->changeEvents[ice];

        ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                                selected_campaign->campaignId,
                                &t1_ctx->ntf.eventsNum,
                                UNION_FLAG_ACTVT, EVENT_MODE_ACTIVE,
                                EVENT_ACTIVATE, sdpv3_cfg->nty_err_code);
        if (ret != SDPV3_ERROR_NONE) {
            printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        ntf_event->eventExtras.actvtExtras.ecuInfoNum = 1U;
        // On this application, elementId is used as ecuSoftwareId.
        // For production, please put appropriate ecuSoftwareId.
        strncpy(ntf_event->eventExtras.actvtExtras.eventEcuInfo[0U].ecuSoftwareId,
                t1_ctx->ce->elementId, MAX_ECU_SW_ID_SZ);
        // On this application, reportId is used as targetId.
        // For production, please put appropriate targetId.
        strncpy(ntf_event->eventExtras.actvtExtras.eventEcuInfo[0U].targetId,
                t1_ctx->ce->reportId, MAX_TARGET_ID_SZ);
        ntf_event->eventExtras.actvtExtras.eventEcuInfo[0U].rewriteBank =
                                        sdpv3_cfg->ecusInfo[0].active_bank.bank;
        if (sdpv3_cfg->nty_err_code == EVENT_STATUS_SUCCESS) {
            ntf_event->eventExtras.actvtExtras.eventEcuInfo[0U].updateStatus =
                                                    EVENT_UPDATE_STATUS_SUCCESS;
            ntf_event->eventExtras.actvtExtras.updateStatus =
                                                    EVENT_UPDATE_STATUS_SUCCESS;
        } else if (sdpv3_cfg->nty_err_code == EVENT_STATUS_FAILURE) {
            ntf_event->eventExtras.actvtExtras.eventEcuInfo[0U].updateStatus =
                                                    EVENT_UPDATE_STATUS_FAILURE;
            ntf_event->eventExtras.actvtExtras.updateStatus =
                                                    EVENT_UPDATE_STATUS_FAILURE;
        } else {
            printf("unknown status code: %s:%d\n", __func__, __LINE__);
            return SDPV3_ERROR_INVALID_ARGUMENT;
        }
        strncpy(ntf_event->reportId,
                t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

sdpv3_error_t run_activate(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;

    if (t1_ctx->selected_campaign->campaignType == CMP_TYPE_OTA) {
        // Send Event: Wait Activate User Acceptance
        if ((ret = wait_activation(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("wait_activation() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: Completion notice (activation approval)
        if ((ret = activation_approval(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("activation_approval() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: Notice Activation Completion
        if ((ret = activation_complete(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE){
            printf("activation_complete() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: Notice Target ECU sync Completion
        if ((ret = sync_complete(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("sync_complete() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: Notice of State (Post Programming)
        ret = ota_process_after_reprog(t1_ctx, sdpv3_cfg);
        if (ret != SDPV3_ERROR_NONE) {
            printf("ota_process_after_reprog() failed: %s:%d\n",
                    __func__, __LINE__);
            return ret;
        }
        // Send Event: Notice of State (System sync check complete)
        ret = ecu_consistency_complete(t1_ctx, sdpv3_cfg);
        if (ret != SDPV3_ERROR_NONE) {
            printf("ecu_consistency_complete() failed: %s:%d\n",
                    __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

sdpv3_error_t run_sw_update_comp(t1_ctx_t *t1_ctx,
                                const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx)
{
    sdpv3_error_t ret;

    if (t1_ctx->selected_campaign->campaignType == CMP_TYPE_OTA) {
        // Send Event: Notice Software update complete Acceptance result request
        if ((ret = ota_complete(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("ota_complete() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
	    uptane_commit(upt_ctx);

        // Update lastCmpId with applied campaignId
        _Static_assert(sizeof(t1_ctx->lastCmpId) ==
                        sizeof(t1_ctx->selected_campaign->campaignId),
                       "campaignId size mismatch");
        memcpy(t1_ctx->lastCmpId, t1_ctx->selected_campaign->campaignId,
                sizeof(t1_ctx->lastCmpId));
    }

    return SDPV3_ERROR_NONE;
}
