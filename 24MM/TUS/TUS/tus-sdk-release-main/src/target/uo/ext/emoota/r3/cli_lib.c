/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <stdio.h>

#include "abst.h"
#include "uptane/uptane.h"
#include "upt_internal.h"

#include "t1_layer/t1_layer.h"
#include "cli_lib.h"


int sdpv3_client_init(const char *rootca, size_t rootca_len)
{
    abst_err_t abst_err;

    abst_err = abst_crypt_init(rootca, rootca_len);
    if (ABST_NO_ERROR != abst_err) {
        printf("abst_crypt_init(): failed (%d)\n", abst_err);
        return -1;
    }

    return 0;
}

int sdpv3_check_available_campaigns(t1_ctx_t *t1_ctx,
                                    const sdpv3_cfg_t *sdpv3_cfg, void **upt_ctx,
                                    util_json_t *mem, uint16_t qty)
{
    int ret = 0;
    sdpv3_error_t err;
    uptaneErr_t upterr;

    /* Start Uptane */
    upterr = uptane_start(upt_ctx, sdpv3_cfg->primary_serial, mem, qty);
    if (UPTANE_NO_ERROR != upterr) {
        printf("uptane_start(): failed (%d)\n", upterr);
        return -1;
    }

    /* Load cached Director metadata */
    printf("=============================\n");
    printf("Load cached director metadata\n");
    upterr = load_director_repo_cached_meta(*upt_ctx);
    if (UPTANE_NO_ERROR != upterr) {
        printf("load_director_repo_cached_meta(): failed (%d)\n", upterr);
        ret = -1;
        goto cleanup;
    }

    /* Load cached Image metadata */
    printf("=============================\n");
    printf("Load cached image metadata\n");
    upterr = load_image_repo_cached_meta(*upt_ctx);
    if (UPTANE_NO_ERROR != upterr) {
        printf("load_image_repo_cached_meta(): failed (%d)\n", upterr);
        ret = -1;
        goto cleanup;
    }

    // Get OTA command
    printf("=============================\n");
    printf("Get OTA command\n");
    err = run_get_cmd(t1_ctx, sdpv3_cfg, *upt_ctx);
    if (SDPV3_ERROR_NONE != err) {
        printf("run_get_cmd(): failed (%d)\n", err);
        ret = -1;
        goto cleanup;
    }

    /* Get Campaign Info */
    printf("=============================\n");
    printf("Sync Check\n");
    upterr = uptane_do_config_match(*upt_ctx,
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
    if (UPTANE_NO_ERROR != upterr) {
        printf("uptane_do_config_match(): failed (%d)\n", upterr);
        ret = -1;
        goto cleanup;
    }

    t1_ctx->selected_campaign =
        ((upt_context_t *)*upt_ctx)->metadata[REPODIR].dld_aug->augMetaBody.campaigns;

    ret = ((upt_context_t *)*upt_ctx)->metadata[REPODIR].dld_aug->augMetaBody.campaignsNum;

 cleanup:
    if (ret < 1) {  // if error or no campaign, resource is released.
        if (NULL != *upt_ctx) {
            uptane_cleanup(*upt_ctx);
        }
    }

    return ret;
}

int sdpv3_download_tup(t1_ctx_t *t1_ctx,
                        const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx)
{
    sdpv3_error_t err;

    /* Download */
    printf("=============================\n");
    printf("Download Campaign\n");
    err = run_download(t1_ctx, sdpv3_cfg, upt_ctx);
    if (SDPV3_ERROR_NONE != err) {
        printf("run_download(): failed (%d)\n", err);
        return -1;
    }

    return 0;
}

int sdpv3_notify_update_result(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg,
                                void *upt_ctx, int phase)
{
    int ret = 0;
    sdpv3_error_t err;

    if (phase == PHASE_DOWNLOADED) {
        err = notify_pkg_download_completion(t1_ctx, sdpv3_cfg, upt_ctx);
        if (SDPV3_ERROR_NONE != err) {
            printf("notify_pkg_download_completion(): failed (%d)\n", err);
            return -1;
        }

    // notify update completion
    } else if (phase == PHASE_COMPLETED || phase == PHASE_ABORTED) {
        if (CMP_TYPE_OTA == t1_ctx->selected_campaign->campaignType) {
            /* Activate */
            printf("=============================\n");
            printf("Activate\n");
            err = run_activate(t1_ctx, sdpv3_cfg);
            if (SDPV3_ERROR_NONE != err) {
                printf("run_activate(): failed (%d)\n", err);
                ret = -1;
                goto cleanup;
            }

            /* Software Update Completion */
            printf("=============================\n");
            printf("Software update completion\n");
            err = run_sw_update_comp(t1_ctx, sdpv3_cfg, upt_ctx);
            if (SDPV3_ERROR_NONE != err) {
                printf("run_sw_update_comp(): failed (%d)\n", err);
                ret = -1;
                goto cleanup;
            }
        }

        // Persist trust root
        printf("=============================\n");
        printf("Store trust root metadata\n");
        err = run_store_trust_root(upt_ctx);
        if (SDPV3_ERROR_NONE != err) {
            printf("run_store_trust_root(): failed (%d)\n", err);
            ret = -1;
            goto cleanup;
        }

 cleanup:
        if (NULL != upt_ctx) {
            uptane_cleanup(upt_ctx);
        }
    }

    return ret;
}

