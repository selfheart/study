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

static uptaneErr_t repo_init(upt_context_t *ctx, repo_index_t repo);

static void repo_cleanup(upt_context_t *ctx, repo_index_t repo);

static uptaneErr_t targetVerifyAndDecrypt(upt_context_t *ctx, const targetAndCustom_t *target, const changeEvent_t *ce);

uptaneErr_t uptane_start(void **uptane_ctx, cstr_t primary_serial, util_json_t *mem, uint16_t qty) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked
    upt_context_t *ctx = (upt_context_t *) calloc(1, sizeof(upt_context_t));
    if (NULL == ctx) {
        err = UPTANE_ERR_RESOURCE;
    }

    if (UPTANE_NO_ERROR == err) {
        if (ABST_NO_ERROR != abst_get_primary_serialno(ctx->primarySerialNum, primary_serial)) {
            err = UPTANE_ERR_RESOURCE;
        }
    }
    if (UPTANE_NO_ERROR == err) {
        ctx->json_mem = mem;
        ctx->json_qty = qty;
        err = repo_init(ctx, REPODIR);
    }

    if (UPTANE_NO_ERROR == err) {
        // Image repo is not used in all config match cycles.
        // But as sample implementation, allocate memory here for simplicity
        err = repo_init(ctx, REPOIMG);
    }

    if (UPTANE_NO_ERROR == err) {
        *uptane_ctx = ctx;
    } else {
        uptane_cleanup(ctx);
    }

    return err;
}

uptaneErr_t load_director_repo_cached_meta(void *ctx) {
    // internal call, assume parameters already checked
    return load_cached_metadata((upt_context_t *) ctx, REPODIR);
}

uptaneErr_t load_image_repo_cached_meta(void *ctx) {
    // internal call, assume parameters already checked
    return load_cached_metadata((upt_context_t *) ctx , REPOIMG);
}

void uptane_commit(void *ctx) {
    // internal call, assume parameters already checked

    //printf("[ RUN      ] abq_notify_007\n");
    //printf("[ RUN      ] abq_notify_008\n");

    // all targets in one config match has the same release counter
    abst_set_curr_release_counter(
            ((upt_context_t *) ctx)->metadata[REPODIR].dld_targets->targetsMetaBody.targets[0].custom.releaseCounter);
}

void uptane_cleanup(void *uptane_ctx) {
    upt_context_t *ctx = (upt_context_t *) uptane_ctx;
    if (NULL != ctx) {
        repo_cleanup(ctx, REPODIR);
        repo_cleanup(ctx, REPOIMG);

        free(ctx);
    }
}

static uptaneErr_t repo_init(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    metadata_set_t *m = &ctx->metadata[repo];

    m->previous_root = (rootMeta_t *) calloc(1, sizeof(rootMeta_t));
    // latest root and previous root are the same until the first cached root is loaded
    m->latest_root = m->previous_root;

    /*
     * Non-root metadata memory could be allocated statically at build time, but to be consistent
     * with how root is handled, allocate them at runtime here.
     */
    m->cached_timestamp = (timeMeta_t *) calloc(1, sizeof(timeMeta_t));
    m->dld_timestamp = (timeMeta_t *) calloc(1, sizeof(timeMeta_t));
    m->cached_snapshot = (snapshotMeta_t *) calloc(1, sizeof(snapshotMeta_t));
    m->dld_snapshot = (snapshotMeta_t *) calloc(1, sizeof(snapshotMeta_t));
    m->cached_targets = (targetsMeta_t *) calloc(1, sizeof(targetsMeta_t));
    m->dld_targets = (targetsMeta_t *) calloc(1, sizeof(targetsMeta_t));

    if (REPODIR == repo) {
        m->cached_aug = (augMeta_t *) calloc(1, sizeof(augMeta_t));
        m->dld_aug = (augMeta_t *) calloc(1, sizeof(augMeta_t));
    }

    if ((NULL == m->previous_root) || (NULL == m->cached_timestamp) || (NULL == m->dld_timestamp) ||
        (NULL == m->cached_snapshot) || (NULL == m->dld_snapshot) ||
        (NULL == m->cached_targets) || (NULL == m->dld_targets)) {
        err = UPTANE_ERR_RESOURCE;
    }
    if ((REPODIR == repo) &&
        ((NULL == m->cached_aug) || (NULL == m->dld_aug))) {
        err = UPTANE_ERR_RESOURCE;
    }
    return err;
}

static void repo_cleanup(upt_context_t *ctx, repo_index_t repo) {
    metadata_set_t *m = &ctx->metadata[repo];

    if (m->latest_root->version != m->previous_root->version) {
        free(m->latest_root);
    }
    free(m->previous_root);
    free(m->cached_timestamp);
    free(m->dld_timestamp);
    free(m->cached_snapshot);
    free(m->dld_snapshot);
    free(m->cached_targets);
    free(m->dld_targets);

    free(m->cached_aug);
    free(m->dld_aug);
}

static uptaneErr_t do_sync_check(const syncCheck_t *syncCheckInfo, byte_t *response, size_t rsp_size,
                                cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;
    byte_t signed_buf[MAX_CFG_MATCH_REQ_LEN] = {0};
    byte_t sig_buf[MAX_CFG_MATCH_REQ_LEN] = {0};

    printf("Do SyncCheck.\n");
    //printf("[ RUN      ] abq_sync_check_req_003\n");
    if (UTIL_NO_ERROR != util_CreateSyncCheckBody(syncCheckInfo, signed_buf, MAX_CFG_MATCH_REQ_LEN)) {
        err = UPTANE_ERR_RESOURCE;
    }
    //printf("[ RUN      ] abq_sync_check_req_004\n");

    if (UPTANE_NO_ERROR == err) {
        err = uptane_create_signature_block(signed_buf, sig_buf, MAX_CFG_MATCH_REQ_LEN);
    }
    if (UPTANE_NO_ERROR == err) {
        abst_err_t absterr = abst_do_sync_check(sig_buf, signed_buf, response, rsp_size, sdp_url, sdp_port, rootca);
        if (ABST_NO_ERROR != absterr) {
            if (ABST_ERR_TRANSACTION_FAILED == absterr) {
                err = UPTANE_ERR_NETWORK;
            } else {
                err = UPTANE_ERR_RESOURCE;
            }
        }
    }
    return err;
}

static uptaneErr_t do_cfg_match(const cfgMatch_t *cfgMatchInfo, byte_t *response, size_t rsp_size,
                                cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;
    byte_t signed_buf[MAX_CFG_MATCH_REQ_LEN] = {0};
    byte_t sig_buf[MAX_CFG_MATCH_REQ_LEN] = {0};

    printf("Do ConfigMatch\n");
    // Hardcoded vehicle configuration information is used on this sample.
    //printf("[ RUN      ] abq_config_match_001\n");
    // Vehicle configuration information is already putted into cfgMatchInfo.
    //printf("[ RUN      ] abq_config_match_002\n");
    //printf("[ RUN      ] abq_config_match_003\n");
    if (UTIL_NO_ERROR != util_CreateConfigMatchBody(cfgMatchInfo, signed_buf, MAX_CFG_MATCH_REQ_LEN)) {
        err = UPTANE_ERR_RESOURCE;
    }
    //printf("[ RUN      ] abq_config_match_004\n");

    if (UPTANE_NO_ERROR == err) {
        err = uptane_create_signature_block(signed_buf, sig_buf, MAX_CFG_MATCH_REQ_LEN);
    }
    if (UPTANE_NO_ERROR == err) {
        abst_err_t absterr = abst_do_cfg_match(sig_buf, signed_buf, response, rsp_size,
                                                sdp_url, sdp_port, rootca);
        if (ABST_NO_ERROR != absterr) {
            if (ABST_ERR_TRANSACTION_FAILED == absterr) {
                err = UPTANE_ERR_NETWORK;
            } else {
                err = UPTANE_ERR_RESOURCE;
            }
        }
    }
    return err;
}

uptaneErr_t uptane_do_config_match(void *uptctx, augMetaBody_t **result,
                                   cstr_t rootca, const byte_t *upd,
                                   const ecuInfo_t *ecusInfo, const uint8_t numEcuInfos,
                                   const byte_t *lastCmpId, const byte_t *rxswinInfo,
                                   cstr_t campaign_vin,
                                   config_match_seq_cb_t cfg_match_seq_func,
                                   cstr_t sdp_url, cstr_t sdp_port,
                                   cstr_t cdn_url, cstr_t cdn_port) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;
    upt_context_t *ctx = (upt_context_t *) uptctx;

    cfgMatch_t cfgMatchInfo = {0};
    syncCheck_t syncCheckInfo = {0};
    byte_t response[MAX_CFG_MATCH_RESP_LEN] = {0};

    bool_t skip_cfg_match = false;
    bool_t sync_first = false;

    *result = NULL;

    if (UPTANE_NO_ERROR == err) {
        // Hardcoded vehicle configuration infomation is used on this sample.
        //printf("[ RUN      ] abq_sync_check_req_001\n");
        //printf("[ RUN      ] abq_sync_check_req_002\n");
        err = uptane_create_cfg_match_req(uptctx, &cfgMatchInfo, &syncCheckInfo,
                                          upd, ecusInfo, numEcuInfos, lastCmpId,
                                          rxswinInfo, campaign_vin);
    }

    if (UPTANE_NO_ERROR == err) {
        cfg_match_seq_func(&skip_cfg_match, &sync_first, &cfgMatchInfo);

        if (skip_cfg_match) {
            if (sync_first) {
                err = upt_get_root_up_to_data(ctx, REPODIR, cdn_url, cdn_port, rootca);

                if (UPTANE_NO_ERROR == err) {
                    err = do_sync_check(&syncCheckInfo, response, sizeof(response), sdp_url, sdp_port, rootca);
                }
                if (UPTANE_NO_ERROR == err) {
                    err = parse_cfg_match_resp(ctx, response);
                }
            }
        } else if (sync_first) {
            err = upt_get_root_up_to_data(ctx, REPODIR, cdn_url, cdn_port, rootca);

            if (UPTANE_NO_ERROR == err) {
                err = do_sync_check(&syncCheckInfo, response, sizeof(response), sdp_url, sdp_port, rootca);
            }
            if (UPTANE_NO_ERROR == err) {
                err = parse_cfg_match_resp(ctx, response);
                //printf("[ RUN      ] abq_upt_sync_rsp_017\n");
                if ((UPTANE_NO_ERROR == err) &&
                    (CONFIG_CHECK_STATUS_RE_SYNC == UPT_OTA_STATUS(ctx))) { // re-sync
                    printf("INFO: OTA Status is RE_SYNC.\n");
                    err = do_cfg_match(&cfgMatchInfo, response, sizeof(response), sdp_url, sdp_port, rootca);
                    if (UPTANE_NO_ERROR == err) {
                        err = parse_cfg_match_resp(ctx, response);
                    }
                }
            }
        } else {
            err = upt_get_root_up_to_data(ctx, REPODIR, cdn_url, cdn_port, rootca);

            if (UPTANE_NO_ERROR == err) {
                err = do_cfg_match(&cfgMatchInfo, response, sizeof(response), sdp_url, sdp_port, rootca);
            }
            if (UPTANE_NO_ERROR == err) {
                err = parse_cfg_match_resp(ctx, response);
            }
        }
    }

    if (UPTANE_NO_ERROR == err) {
        if ((CONFIG_CHECK_STATUS_UPDATE == UPT_OTA_STATUS(ctx)) && (UPT_TARGET_PENDING(ctx))) {
            //printf("[ RUN      ] abq_upt_sync_rsp_017\n");
            printf("INFO: OTA Status is UPDATE.\n");
            //printf("[ RUN      ] abq_upt_sync_rsp_018\n");
            int32_t ota_cmp_num = 0;
            // Check Campaign Type
            for (int32_t i = 0; i < ctx->metadata[REPODIR].dld_aug->augMetaBody.campaignsNum; i++) {
                if (CMP_TYPE_OTA == ctx->metadata[REPODIR].dld_aug->augMetaBody.campaigns[i].campaignType) {
                    ota_cmp_num++;
                }
            }

            if ( 1 <= ota_cmp_num) {
                printf("INFO: OTA campaign.\n");
                // Image repo metadata
                err = dld_and_verify_image_repo_meta(ctx, cdn_url, cdn_port, rootca);

                // disable upt_cross_check_targets()

                if (UPTANE_NO_ERROR == err) {
                    // cache downloaded metadata files after verifying
                    err = save_dir_cached_metadata(ctx);
                    if (UPTANE_NO_ERROR == err) {
                        err = save_img_cached_metadata(ctx);
                    }
                }
            } else {
                printf("INFO: Informational campaign only.\n");
                // No need to download Image Metadata
                // Cache the metadata from director repo
                err = save_dir_cached_metadata(ctx);
            }
        } else if (CONFIG_CHECK_STATUS_NO_UPDATE == UPT_OTA_STATUS(ctx)){
            //printf("[ RUN      ] abq_upt_sync_rsp_017\n");
            printf("INFO: OTA Status is NO_UPDATE.\n");
            // No update pending. Cache the metadata from director repo
            err = save_dir_cached_metadata(ctx);
        }
    }

    if (UPTANE_NO_ERROR == err) {
        *result = &ctx->metadata[REPODIR].dld_aug->augMetaBody;
    }

    return err;
}

uptaneErr_t check_OtaCmd_response(void *uptctx, byte_t *response, size_t rsp_size, getOtaCmdRes_t *otaCmdRes) {
    upt_context_t *ctx = (upt_context_t *) uptctx;
    uptaneErr_t err = UPTANE_NO_ERROR;

    err = metadata_parse_and_verify_sigs(ctx, ctx->metadata->latest_root,
                                         (str_t) response, rsp_size, (void *) otaCmdRes, META_GET_OTA_CMD);
    return err;
}

uptaneErr_t uptane_download_change_event(void *uptctx, changeEvent_t *change_event, event_t *ntf_event, cstr_t rootca)
{
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;

    upt_context_t *ctx = (upt_context_t *) uptctx;
    targetAndCustom_t *downloadMeta = NULL;
    targetAndCustom_t *packageTarget = NULL;
    targetsMetaBody_t *tgts_meta = &ctx->metadata[REPODIR].dld_targets->targetsMetaBody;
    char decoded_filepath[1024];

    // find downloadMetadata target for respective change event
    for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
        if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {

            if (0 == strncmp(tgts_meta->targets[i].custom.ecuIdentifier, "DownloadMetadata", MAX_SERIAL_NUM_SZ)) {
                downloadMeta = &tgts_meta->targets[i];
                break;
            }
        }
    }

    // use found target to re-create the file path/name where it was downloaded in a previous step
    char dlMetaPath[1024] = {0};
    (void) abst_getImageFileName(dlMetaPath, downloadMeta, change_event, false);

    // As reference app, always download the image, even if the image has already been downloaded before
    char relativepath[ABST_MAX_RELATIVE_PATH_NUM][ABST_MAX_RELATIVE_PATH_SZ] = {0};
    char url[MAX_DWNLD_URL_SZ + MAX_QUERY_STRING_SZ + sizeof(relativepath)] = {0};

    //printf("[ RUN      ] abq_dwnld_meta_file_001\n");

    uint16_t package_num = abst_ParseDownloadMetadataFile(dlMetaPath, relativepath, false);
    //printf("[ RUN      ] abq_dwnld_meta_file_007\n");
    // Clear any previous notifications
    memset(ntf_event, 0x00U, sizeof(event_t) * MAX_NOTIFICATIONS);
    // download each package referenced in downloadMetadata file
    for (int32_t x = 0; x < package_num; ++x) {
        //printf("[ RUN      ] abq_dwnld_meta_file_008\n");
        memset(decoded_filepath,0,1024);
        abst_base64_decode(relativepath[x],strlen(relativepath[x]),decoded_filepath,1024);
        // find target for respective package
        //printf("[ RUN      ] abq_dwnld_meta_file_009\n");
        for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
            if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {

                if (0 == strncmp(tgts_meta->targets[i].target.fileName, decoded_filepath, MAX_FILE_NAME_SZ)) {
                    packageTarget = &tgts_meta->targets[i];
                    break;
                }
            }
        }
        if (NULL != packageTarget) {
            // build complete URL for current package
            //printf("[ RUN      ] abq_dwnld_meta_file_010\n");
            snprintf(url, sizeof(url), "%s%s?%s", change_event->rootUrl, decoded_filepath, change_event->queryString);
            //printf("[ RUN      ] abq_dwnld_meta_file_011\n");
            //printf("[ RUN      ] abq_dwnld_meta_file_012\n");
            abst_err_t absterr = abst_DownloadImage(url, packageTarget, change_event, false, rootca);
            if (ABST_ERR_NO_RESOURCE == absterr) {
                err = UPTANE_ERR_RESOURCE;
            } else if (ABST_ERR_TRANSACTION_FAILED == absterr) {
                err = UPTANE_ERR_NETWORK;
            } else {
                // Do nothing, MISRA requires `else` after `else if`
            }
            //printf("[ RUN      ] abq_dwnld_meta_file_013\n");
            //printf("[ RUN      ] abq_dwnld_meta_file_014\n");
            strncpy(ntf_event->eventExtras.trackingExtras.fileId,change_event->rootUrl, strlen(change_event->rootUrl));
            ntf_event->eventExtras.trackingExtras.fileSize += packageTarget->target.length;
        }
        if (err != UPTANE_NO_ERROR) {
            break; // error out and return
        }
    }
    return err;
}

uptaneErr_t uptane_download_repro_metadata(void *uptctx, changeEvent_t *change_event, event_t *ntf_event, cstr_t rootca)
{
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;

    upt_context_t *ctx = (upt_context_t *) uptctx;
    targetAndCustom_t *reproPolicy = NULL;
    targetsMetaBody_t *tgts_meta = &ctx->metadata[REPODIR].dld_targets->targetsMetaBody;

    //printf("[ RUN      ] abq_dl_metadata_009\n");
    for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
        if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {

            if (0 == strncmp(tgts_meta->targets[i].custom.ecuIdentifier, "ReproPolicyMetadata", MAX_SERIAL_NUM_SZ)) {
                reproPolicy = &tgts_meta->targets[i];
            } else {
            }
        }
    }

    str_t url = NULL;
    if (reproPolicy != NULL) {
        bool_t encrypted = false;
        url = reproPolicy->target.fileDownloadUrl;
        if (reproPolicy->custom.encryptedTarget.length != 0) {
            //printf("[ RUN      ] abq_dl_metadata_010\n");
            encrypted = true;
            url = reproPolicy->custom.encryptedTarget.fileDownloadUrl;
        }
        //printf("[ RUN      ] abq_dl_metadata_011\n");
        //printf("[ RUN      ] abq_dl_metadata_012\n");
        abst_err_t abst_err = abst_DownloadImage((cstr_t) url,
                                                 reproPolicy, change_event, encrypted, rootca);
        //printf("[ RUN      ] abq_dl_metadata_013\n");
        //printf("[ RUN      ] abq_dl_metadata_014\n");
        
        // Clear any previous notifications
        memset(ntf_event, 0x00U, sizeof(event_t) * MAX_NOTIFICATIONS);
        strncpy(ntf_event->eventExtras.trackingExtras.fileId,url, strlen(url));
        ntf_event->eventExtras.trackingExtras.fileSize = reproPolicy->custom.encryptedTarget.length;
    }
    return err;
}

uptaneErr_t uptane_download_download_metadata(void *uptctx, changeEvent_t *change_event, event_t *ntf_event, cstr_t rootca)
{
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;

    upt_context_t *ctx = (upt_context_t *) uptctx;
    targetAndCustom_t *downloadMeta = NULL;
    targetsMetaBody_t *tgts_meta = &ctx->metadata[REPODIR].dld_targets->targetsMetaBody;

    //printf("[ RUN      ] abq_dl_metadata_022\n");
    for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
        if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {

            if (0 ==
                       strncmp(tgts_meta->targets[i].custom.ecuIdentifier, "DownloadMetadata", MAX_SERIAL_NUM_SZ)) {
                downloadMeta = &tgts_meta->targets[i];
            } else {
            }
        }
    }

    str_t url = NULL;
    if (downloadMeta != NULL) {
        bool_t encrypted = false;
        url = downloadMeta->target.fileDownloadUrl;
        if (downloadMeta->custom.encryptedTarget.length != 0U) {
            //printf("[ RUN      ] abq_dl_metadata_023\n");
            encrypted = true;
            url = downloadMeta->custom.encryptedTarget.fileDownloadUrl;
        }
        abst_err_t abst_err = abst_DownloadImage((cstr_t) url,
                                                 downloadMeta, change_event, encrypted, rootca);
        //printf("[ RUN      ] abq_dl_metadata_024\n");
        //printf("[ RUN      ] abq_dl_metadata_025\n");
        //printf("[ RUN      ] abq_dl_metadata_026\n");
        //printf("[ RUN      ] abq_dl_metadata_027\n");
        // Clear any previous notifications
        memset(ntf_event, 0x00U, sizeof(event_t) * MAX_NOTIFICATIONS);
        strncpy(ntf_event->eventExtras.trackingExtras.fileId,url, strlen(url));
        ntf_event->eventExtras.trackingExtras.fileSize = downloadMeta->custom.encryptedTarget.length;
    }
    return err;
}

#include "abst_config.h"

uptaneErr_t uptane_download_hmi_msg(void *uptctx, campaign_t *campaign, event_t *ntf_event, cstr_t rootca)
{
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;

    // As reference app, always download the messages, even if the messages have already been downloaded before
    uint32_t http_code = 0;
    //printf("[ RUN      ] abq_dl_metadata_001\n");
    // campaign->hmiMessages.url is URL for HMI message file
    //printf("[ RUN      ] abq_dl_metadata_002\n");
    //printf("[ RUN      ] abq_dl_metadata_003\n");
    abst_err_t absterr = abst_GETfromCDN(campaign->hmiMessages.url, NULL, PERSIST_BASE, NULL, &http_code, rootca);
    //printf("[ RUN      ] abq_dl_metadata_004\n");
    //printf("[ RUN      ] abq_dl_metadata_005\n");
    if (ABST_ERR_NO_RESOURCE == absterr) {
        err = UPTANE_ERR_RESOURCE;
    } else if (ABST_ERR_TRANSACTION_FAILED == absterr) {
        err = UPTANE_ERR_NETWORK;
    } else {
        // Do nothing, MISRA requires `else` after `else if`
    }
    // Clear any previous notifications
    memset(ntf_event, 0x00U, sizeof(event_t) * MAX_NOTIFICATIONS);
    if(strlen(campaign->hmiMessages.url) > MAX_EVENT_FILE_ID_SZ)
    {
        printf("ERR:MAX_EVENT_FILE_ID_SZ is not enough.\n");
        err = UPTANE_ERR_RESOURCE;
    } else {
        strncpy(ntf_event->eventExtras.trackingExtras.fileId,campaign->hmiMessages.url,strlen(campaign->hmiMessages.url));
        ntf_event->eventExtras.trackingExtras.fileSize=campaign->hmiMessages.length;
    }

    return err;
}

uptaneErr_t uptane_verify_hmi_msg(void *uptctx, campaign_t *campaign)
{
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;
    char filename[1024] = {0};// big enough
    strcat(filename, PERSIST_BASE);
    abst_get_filename_of(campaign->hmiMessages.url, &filename[strlen(filename)], sizeof(filename));

    //printf("[ RUN      ] abq_dl_metadata_006\n");
    //printf("[ RUN      ] abq_dl_metadata_007\n");
    //printf("[ RUN      ] abq_dl_metadata_008\n");
    // abst_VerifyFile function calculates digest and compares calculated digest with populated digest.
    abst_err_t absterr = abst_VerifyFile(campaign->hmiMessages.digest, filename, campaign->hmiMessages.length);
    if (ABST_ERR_NO_RESOURCE == absterr) {
        err = UPTANE_ERR_RESOURCE;
    } else if (ABST_ERR_TRANSACTION_FAILED == absterr) {
        err = UPTANE_ERR_NETWORK;
    } else {
        // Do nothing, MISRA requires `else` after `else if`
    }

    return err;
}
uptaneErr_t uptane_vrfy_and_decrypt_image(void *uptctx, changeEvent_t *change_event) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;

    upt_context_t *ctx = (upt_context_t *) uptctx;
    targetAndCustom_t *downloadMeta = NULL;
    targetAndCustom_t *packageTarget = NULL;
    targetsMetaBody_t *tgts_meta = &ctx->metadata[REPODIR].dld_targets->targetsMetaBody;
    char decoded_filepath[1024];

    // find downloadMetadata target for respective change event
    for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
        if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {

            if (0 == strncmp(tgts_meta->targets[i].custom.ecuIdentifier, "DownloadMetadata", MAX_SERIAL_NUM_SZ)) {
                downloadMeta = &tgts_meta->targets[i];
                break;
            }
        }
    }

    // use found target to re-create the file path/name where it was downloaded in a previous step
    char dlMetaPath[1024] = {0};
    (void) abst_getImageFileName(dlMetaPath, downloadMeta, change_event, false);

    // As reference app, always download the image, even if the image has already been downloaded before
    char relativepath[ABST_MAX_RELATIVE_PATH_NUM][ABST_MAX_RELATIVE_PATH_SZ] = {0};
    char url[MAX_DWNLD_URL_SZ + MAX_QUERY_STRING_SZ + sizeof(relativepath)] = {0};
    uint16_t package_num = abst_ParseDownloadMetadataFile(dlMetaPath, relativepath,true);

    // verify each package referenced in downloadMetadata file
    for (int32_t x = 0; x < package_num; ++x) {
        memset(decoded_filepath,0,1024);
        abst_base64_decode(relativepath[x],strlen(relativepath[x]),decoded_filepath,1024);
        // find target for respective package
        for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
            if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {

                if (0 == strncmp(tgts_meta->targets[i].target.fileName, decoded_filepath, MAX_FILE_NAME_SZ)) {
                    packageTarget = &tgts_meta->targets[i];
                    break;
                }
            }
        }
        if (NULL != packageTarget) {

            if (packageTarget->custom.encryptedTarget.length != 0U) {
                // As reference app, always decrypt the image, even if the image has already been decrypted before
                //printf("[ RUN      ] abq_dwnld_meta_file_015\n");
                // Only sha256 is available on current solution.
                //printf("[ RUN      ] abq_dwnld_meta_file_016\n");
                // abst_VerifyImage function calculates digest and compare calculated digest with populated digest.
                abst_err_t absterr = abst_VerifyImage(packageTarget, change_event, true);
                //printf("[ RUN      ] abq_dwnld_meta_file_017\n");
                //printf("[ RUN      ] abq_dwnld_meta_file_018\n");

                if (ABST_ERR_NO_RESOURCE == absterr) {
                    err = UPTANE_ERR_RESOURCE;
                } else if (ABST_ERR_VERIFICATION_FAILED == absterr) {
                    err = UPTANE_ERR_INVALID_IMAGE;
                } else {
                    // Do nothing, MISRA requires `else` after `else if`
                }

                byte_t dek[MAX_ECRYPTD_SYMKEY_VALUE_SZ] = {
                        0}; // Big enough. Decrypted DEK should be <= Encrypted/encoded DEK
                size_t deklen = 0U;
                byte_t iv[MAX_BASE64_IV_SZ] = {0}; // Big enough. Decrypted DEK should be <= Encrypted/encoded DEK
                size_t ivlen = 0U;
                if (UPTANE_NO_ERROR == err) {
                    err = get_data_encryption_key(ctx, packageTarget->custom.ecryptdSymKey.ecryptdSymKeyValue,
                                                  dek, sizeof(dek), &deklen);
                }
                if (UPTANE_NO_ERROR == err) {
                    size_t enc_iv_len = strlen(packageTarget->custom.ecryptdSymKey.iv);
                    if (0 < enc_iv_len) { // IV exists
                        //printf("[ RUN      ] abq_decrypt_file_009\n");
                        ivlen = abst_base64_decode(packageTarget->custom.ecryptdSymKey.iv,
                                                   enc_iv_len, iv, sizeof(iv));
                        //printf("[ RUN      ] abq_decrypt_file_010\n");
                        if (0 > ivlen) {
                            err = UPTANE_ERR_INVALID_METADATA;
                        }
                    }
                }
                if (UPTANE_NO_ERROR == err) {
                    //printf("[ RUN      ] abq_decrypt_file_011\n");
                    absterr = abst_DecryptImage(packageTarget, change_event, dek, deklen, iv, ivlen);
                    //printf("[ RUN      ] abq_decrypt_file_012\n");
                    if (ABST_ERR_NO_RESOURCE == absterr) {
                        err = UPTANE_ERR_RESOURCE;
                    } else if (ABST_ERR_TRANSACTION_FAILED == absterr) {
                        err = UPTANE_ERR_IMAGE_DECRYPT;
                    } else {
                        // Do nothing, MISRA requires `else` after `else if`
                    }
                }
            }
            if (UPTANE_NO_ERROR == err) {
                //printf("[ RUN      ] abq_dwnld_meta_file_015\n");
                // Only sha256 is available on current solution.
                //printf("[ RUN      ] abq_dwnld_meta_file_016\n");
                // abst_VerifyImage function calculates digest and compare calculated digest with populated digest.
                abst_err_t absterr = abst_VerifyImage(packageTarget, change_event, false);
                //printf("[ RUN      ] abq_dwnld_meta_file_017\n");
                //printf("[ RUN      ] abq_dwnld_meta_file_018\n");
                if (ABST_ERR_NO_RESOURCE == absterr) {
                    err = UPTANE_ERR_RESOURCE;
                } else if (ABST_ERR_VERIFICATION_FAILED == absterr) {
                    err = UPTANE_ERR_INVALID_IMAGE;
                } else {
                    // Do nothing, MISRA requires `else` after `else if`
                }
            }
        }
        if (err != UPTANE_NO_ERROR) {
            break; // error out and return
        }
    }
    return err;
}

uptaneErr_t uptane_vrfy_and_decrypt_image_dl_metadata(void *uptctx, changeEvent_t *change_event) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;

    upt_context_t *ctx = (upt_context_t *) uptctx;
    targetAndCustom_t *downloadMeta = NULL;
    targetsMetaBody_t *tgts_meta = &ctx->metadata[REPODIR].dld_targets->targetsMetaBody;
    abst_err_t absterr = ABST_NO_ERROR;

    for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
        if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {
            if (0 == strncmp(tgts_meta->targets[i].custom.ecuIdentifier, "DownloadMetadata", MAX_SERIAL_NUM_SZ)) {
                downloadMeta = &tgts_meta->targets[i];
            } else {
            }
        }
    }

    if (NULL == downloadMeta) {
        err = UPTANE_ERR_INVALID_METADATA;
    }

    if (UPTANE_NO_ERROR == err) {
        // Only sha256 is available on current solution.
        //printf("[ RUN      ] abq_dl_metadata_037\n");
        // targetVerifyAndDecrypt function calculates digest, verifies it and decrypt file.
        //printf("[ RUN      ] abq_dl_metadata_028\n");
        //printf("[ RUN      ] abq_dl_metadata_029\n");
        //printf("[ RUN      ] abq_dl_metadata_030\n");
        err = targetVerifyAndDecrypt(ctx, downloadMeta, change_event);
    }
    if (UPTANE_NO_ERROR == err) {
        // Only sha256 is available on current solution.
        //printf("[ RUN      ] abq_dl_metadata_038\n");
        // abst_VerifyImage function calculates digest and verifies it.
        //printf("[ RUN      ] abq_dl_metadata_032\n");
        //printf("[ RUN      ] abq_dl_metadata_033\n");
        //printf("[ RUN      ] abq_dl_metadata_034\n");
        absterr = abst_VerifyImage(downloadMeta, change_event, false);
        if (ABST_ERR_NO_RESOURCE == absterr) {
            err = UPTANE_ERR_RESOURCE;
        } else if (ABST_ERR_VERIFICATION_FAILED == absterr) {
            err = UPTANE_ERR_INVALID_IMAGE;
        } else {
            // Do nothing, MISRA requires `else` after `else if`
        }
    }
    return err;
}

uptaneErr_t uptane_vrfy_and_decrypt_image_repro_metadata(void *uptctx, changeEvent_t *change_event) {
    // internal call, assume parameters already checked
    uptaneErr_t err = UPTANE_NO_ERROR;

    upt_context_t *ctx = (upt_context_t *) uptctx;
    targetAndCustom_t *reproPolicy = NULL;
    targetsMetaBody_t *tgts_meta = &ctx->metadata[REPODIR].dld_targets->targetsMetaBody;
    abst_err_t absterr = ABST_NO_ERROR;

    for (int32_t i = 0; i < tgts_meta->targetsNum; i++) {
        if (0 == strncmp(tgts_meta->targets[i].custom.reportId, change_event->reportId, MAX_REPORT_ID_SZ)) {

            if (0 == strncmp(tgts_meta->targets[i].custom.ecuIdentifier, "ReproPolicyMetadata", MAX_SERIAL_NUM_SZ)) {
                reproPolicy = &tgts_meta->targets[i];
            } else {
            }
        }
    }

    if (NULL == reproPolicy) {
        err = UPTANE_ERR_INVALID_METADATA;
    }

    if (UPTANE_NO_ERROR == err) {
        // Only sha256 is available on current solution.
        //printf("[ RUN      ] abq_dl_metadata_035\n");
        // targetVerifyAndDecrypt function calculates digest, verifies it and decrypt file.
        //printf("[ RUN      ] abq_dl_metadata_015\n");
        //printf("[ RUN      ] abq_dl_metadata_016\n");
        //printf("[ RUN      ] abq_dl_metadata_017\n");
        err = targetVerifyAndDecrypt(ctx, reproPolicy, change_event);
    }
    if (UPTANE_NO_ERROR == err) {
        // Only sha256 is available on current solution.
        //printf("[ RUN      ] abq_dl_metadata_036\n");
        // abst_VerifyImage function calculates digest and verifies it.
        //printf("[ RUN      ] abq_dl_metadata_019\n");
        //printf("[ RUN      ] abq_dl_metadata_020\n");
        //printf("[ RUN      ] abq_dl_metadata_021\n");
        absterr = abst_VerifyImage(reproPolicy, change_event, false);
        if (ABST_ERR_NO_RESOURCE == absterr) {
            err = UPTANE_ERR_RESOURCE;
        } else if (ABST_ERR_VERIFICATION_FAILED == absterr) {
            err = UPTANE_ERR_INVALID_IMAGE;
        } else {
            // Do nothing, MISRA requires `else` after `else if`
        }
    }
    return err;
}

static uptaneErr_t
targetVerifyAndDecrypt(upt_context_t *ctx, const targetAndCustom_t *target, const changeEvent_t *ce) {
    abst_err_t absterr = ABST_NO_ERROR;
    uptaneErr_t err = UPTANE_NO_ERROR;
    if (target->custom.encryptedTarget.length != 0U) {
        // As reference app, always decrypt the image, even if the image has already been decrypted before
        absterr = abst_VerifyImage(target, ce, true);
        if (ABST_ERR_NO_RESOURCE == absterr) {
            err = UPTANE_ERR_RESOURCE;
        } else if (ABST_ERR_VERIFICATION_FAILED == absterr) {
            err = UPTANE_ERR_INVALID_IMAGE;
        } else {
            // Do nothing, MISRA requires `else` after `else if`
        }

        byte_t dek[MAX_ECRYPTD_SYMKEY_VALUE_SZ] = {
                0}; // Big enough. Decrypted DEK should be <= Encrypted/encoded DEK
        size_t deklen = 0U;
        byte_t iv[MAX_BASE64_IV_SZ] = {0}; // Big enough. Decrypted DEK should be <= Encrypted/encoded DEK
        size_t ivlen = 0U;
        if (UPTANE_NO_ERROR == err) {
            err = get_data_encryption_key(ctx, target->custom.ecryptdSymKey.ecryptdSymKeyValue,
                                          dek, sizeof(dek), &deklen);
        }
        if (UPTANE_NO_ERROR == err) {
            //printf("[ RUN      ] abq_decrypt_file_009\n");
            size_t enc_iv_len = strlen(target->custom.ecryptdSymKey.iv);
            if (0 < enc_iv_len) { // IV exists
                ivlen = abst_base64_decode(target->custom.ecryptdSymKey.iv,
                                           enc_iv_len, iv, sizeof(iv));
                //printf("[ RUN      ] abq_decrypt_file_010\n");
                if (0 > ivlen) {
                    err = UPTANE_ERR_INVALID_METADATA;
                }
            }
        }
        if (UPTANE_NO_ERROR == err) {
            //printf("[ RUN      ] abq_decrypt_file_011\n");
            absterr = abst_DecryptImage(target, ce, dek, deklen, iv, ivlen);
            //printf("[ RUN      ] abq_decrypt_file_012\n");
            if (ABST_ERR_NO_RESOURCE == absterr) {
                err = UPTANE_ERR_RESOURCE;
            } else if (ABST_ERR_TRANSACTION_FAILED == absterr) {
                err = UPTANE_ERR_IMAGE_DECRYPT;
            } else {
                // Do nothing, MISRA requires `else` after `else if`
            }
        }
    }
    return err;
}

uptaneErr_t upt_get_dir_root_up_to_data(void *uptctx, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca){
    return upt_get_root_up_to_data((upt_context_t *)uptctx, REPODIR, cdn_url, cdn_port, rootca);
}

#ifdef __cplusplus
}
#endif
