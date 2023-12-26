/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>

#include "uptane/uptane.h"
#include "upt_internal.h"

#include "t1_layer.h"


void config_match_seq_cb(bool_t *skip_config_match,
                        bool_t *sync_first,
                        const cfgMatch_t *cfgMatchInfo) {
    // T1 should implement code to make the decision, using information in
    // cfgMatchInfo, along with previously config match digest sent to SDP
    *skip_config_match = false;
    *sync_first = true;
}

sdpv3_error_t run_config_match(t1_ctx_t *t1_ctx,
                                const sdpv3_cfg_t *sdpv3_cfg,
                                void *upt_ctx)
{
    uptaneErr_t upt_err;
    upt_err = uptane_do_config_match(upt_ctx,
                                    &t1_ctx->cfg_match_result,
                                    sdpv3_cfg->rootca,
                                    t1_ctx->upd,
                                    sdpv3_cfg->ecusInfo,
                                    sdpv3_cfg->numEcusInfo,
                                    t1_ctx->lastCmpId,
                                    t1_ctx->rxswinInfo,
                                    sdpv3_cfg->campaign_vin,
                                    config_match_seq_cb,
                                    sdpv3_cfg->sdp_url,
                                    sdpv3_cfg->sdp_port,
                                    sdpv3_cfg->cdn_url,
                                    sdpv3_cfg->cdn_port);
    if (UPTANE_NO_ERROR != upt_err) {
        printf("uptane_do_config_match() failed: %s:%d\n", __func__, __LINE__);
        if (upt_err == UPTANE_ERR_INVALID_METADATA) {
            return SDPV3_ERROR_INVALID_ARGUMENT;
        } else if (upt_err == UPTANE_ERR_RESOURCE) {
            return SDPV3_ERROR_NO_MEMORY;
        } else if (upt_err == UPTANE_ERR_NETWORK) {
            return SDPV3_ERROR_TRANSACTION;
        } else {
            return SDPV3_ERROR_UNKNOWN;
        }
    }

    t1_ctx->selected_campaign =
        ((upt_context_t *)upt_ctx)->metadata[REPODIR].dld_aug->augMetaBody.campaigns;

    return SDPV3_ERROR_NONE;
}

sdpv3_error_t run_config_match_with_updated_config(t1_ctx_t *t1_ctx,
                                                   const sdpv3_cfg_t *sdpv3_cfg,
                                                   void *upt_ctx)
{
    uptaneErr_t upt_err = UPTANE_NO_ERROR;
    upt_err = uptane_do_config_match(upt_ctx,
                                    &t1_ctx->cfg_match_result,
                                    sdpv3_cfg->rootca,
                                    t1_ctx->upd,
                                    sdpv3_cfg->ecusInfo,
                                    sdpv3_cfg->numEcusInfo,
                                    t1_ctx->selected_campaign->campaignId,
                                    t1_ctx->rxswinInfo,
                                    sdpv3_cfg->campaign_vin,
                                    config_match_seq_cb,
                                    sdpv3_cfg->sdp_url,
                                    sdpv3_cfg->sdp_port,
                                    sdpv3_cfg->cdn_url,
                                    sdpv3_cfg->cdn_port);

    if (UPTANE_NO_ERROR != upt_err) {
        printf("uptane_do_config_match() failed: %s:%d\n", __func__, __LINE__);
        if (upt_err == UPTANE_ERR_INVALID_METADATA) {
            return SDPV3_ERROR_INVALID_ARGUMENT;
        } else if (upt_err == UPTANE_ERR_RESOURCE) {
            return SDPV3_ERROR_NO_MEMORY;
        } else if (upt_err == UPTANE_ERR_NETWORK) {
            return SDPV3_ERROR_TRANSACTION;
        } else {
            return SDPV3_ERROR_UNKNOWN;
        }
    }

    t1_ctx->selected_campaign =
        ((upt_context_t *)upt_ctx)->metadata[REPODIR].dld_aug->augMetaBody.campaigns;
    if (CONFIG_CHECK_STATUS_NO_UPDATE == t1_ctx->cfg_match_result->status) {
        printf("No Available Campaign.\n");
    } else {
        printf("Available Campaign is found.\n");
    }

    return SDPV3_ERROR_NONE;
}
