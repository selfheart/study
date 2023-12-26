/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/param.h>

#include "abst.h"
#include "abst_config.h"
#include "uptane/uptane.h"
#include "upt_internal.h"

#include "t1_layer.h"


static sdpv3_error_t hmi_tracking_event(t1_ctx_t *t1_ctx)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    ret = create_base_event(ntf_event, EVENT_MEM_NOT_CLEAR, // not clear
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_TRACKING, EVENT_MODE_ACTIVE,
                            EVENT_TRACKING, EVENT_STATUS_SUCCESS);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    strncpy(ntf_event->reportId,
            selected_campaign->hmiMessages.reportId, MAX_REPORT_ID_SZ);

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t downloadHMI(t1_ctx_t *t1_ctx,
                                const sdpv3_cfg_t *sdpv3_cfg,
                                void *upt_ctx)
{
    augMetaBody_t *cfg_match_result = t1_ctx->cfg_match_result;
    if (cfg_match_result == NULL) {
        return SDPV3_ERROR_NONE;
    }

    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;
    if (CONFIG_CHECK_STATUS_UPDATE == cfg_match_result->status) {
        for (unsigned int ic = 0U;
                (ic < MAX_CAMPAIGNS) &&
                    (cfg_match_result->campaigns[ic].campaignId[0U] != '\0');
                ic++) {
            uptaneErr_t upterr;
            t1_ctx->selected_campaign = &cfg_match_result->campaigns[ic];

            printf("Download HMI message file\n");
            upterr = uptane_download_hmi_msg(upt_ctx, selected_campaign,
                                            ntf_event, sdpv3_cfg->rootca);
            /* TODO: error(ABST_ERR_NO_RESOURCE) occurs in the
               abst_GETfromCDN() called after that */
            if (upterr != UPTANE_NO_ERROR) {
                //printf("uptane_download_hmi_msg() failed:%d %s:%d\n",
                //        upterr, __func__, __LINE__);
                return SDPV3_ERROR_UNKNOWN;
            }

            printf("Send Event: Tracking HMI Messages\n");
            if ((ret = hmi_tracking_event(t1_ctx)) != SDPV3_ERROR_NONE) {
                printf("hmi_tracking_event() failed: %s:%d\n",
                        __func__, __LINE__);
                return ret;
            }
            if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
                printf("send_event() failed: %s:%d\n", __func__, __LINE__);
                return ret;
            }
            printf("Verify HMI Messages\n");
            upterr = uptane_verify_hmi_msg(upt_ctx, selected_campaign);
            if (upterr != UPTANE_NO_ERROR) {
                printf("uptane_verify_hmi_msg() failed:%d %s:%d\n",
                        upterr, __func__, __LINE__);
                return SDPV3_ERROR_UNKNOWN;
            }
        }
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t ce_tracking_event(t1_ctx_t *t1_ctx)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    ret = create_base_event(ntf_event, EVENT_MEM_NOT_CLEAR, // not clear
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_TRACKING, EVENT_MODE_ACTIVE,
                            EVENT_TRACKING, EVENT_STATUS_SUCCESS);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.trackingExtras.dlTime.time =
                                                    ntf_event->currentTime.time;
    ntf_event->eventExtras.trackingExtras.dlTime.clockSource =
                                                        EVENT_CLKSRC_DEFAULT;
    strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t downloadCE_metadata(t1_ctx_t *t1_ctx,
                                        const sdpv3_cfg_t *sdpv3_cfg,
                                        void *upt_ctx)
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

        uptaneErr_t upterr;
        upterr = uptane_download_repro_metadata(upt_ctx,
                                        &selected_campaign->changeEvents[ice],
                                        ntf_event, sdpv3_cfg->rootca);
        if (UPTANE_NO_ERROR != upterr) {
            printf("uptane_download_repro_metadata() failed:"
                    "%d %s:%d\n", upterr, __func__, __LINE__);
            return SDPV3_ERROR_UNKNOWN;
        }

        // Send Event: Tracking Change Event
        if ((ret = ce_tracking_event(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("ce_tracking_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }

        upterr = uptane_vrfy_and_decrypt_image_repro_metadata(upt_ctx,
                                        &selected_campaign->changeEvents[ice]);
        if (UPTANE_NO_ERROR != upterr) {
            printf("uptane_vrfy_and_decrypt_image_repro_metadata() failed:"
                    "%d %s:%d\n", upterr, __func__, __LINE__);
            return SDPV3_ERROR_UNKNOWN;
        }

        upterr = uptane_download_download_metadata(upt_ctx,
                                        &selected_campaign->changeEvents[ice],
                                        ntf_event, sdpv3_cfg->rootca);
        if (UPTANE_NO_ERROR != upterr) {
            printf("uptane_download_download_metadata() failed:"
                    "%d %s:%d\n", upterr, __func__, __LINE__);
            return SDPV3_ERROR_UNKNOWN;
        }

        // Send Event: Tracking Change Event
        if ((ret = ce_tracking_event(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("ce_tracking_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }

        upterr = uptane_vrfy_and_decrypt_image_dl_metadata(upt_ctx,
                                        &selected_campaign->changeEvents[ice]);
        if (UPTANE_NO_ERROR != upterr) {
            printf("uptane_vrfy_and_decrypt_image_dl_metadata() failed:"
                    "%d %s:%d\n", upterr, __func__, __LINE__);
            return SDPV3_ERROR_UNKNOWN;
        }
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t wait_for_download_event(t1_ctx_t *t1_ctx)
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
    ntf_event->eventExtras.rmStateExtras.rmVehState =
                                    EVENT_VEHICLE_STATE_WAIT_PKG_DL_USER_ACCPT;
    ntf_event->eventExtras.rmStateExtras.stateScope = EVENT_STATE_SCOPE_VEHICLE;

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t info_camp_comp_nfy(t1_ctx_t *t1_ctx)
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
    ntf_event->eventExtras.rmStateExtras.rmVehState =
                                            EVENT_VEHICLE_STATE_INFO_CMP_COMP;
    ntf_event->eventExtras.rmStateExtras.stateScope = EVENT_STATE_SCOPE_VEHICLE;

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t wait_for_camp_accept(t1_ctx_t *t1_ctx)
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
    ntf_event->eventExtras.rmStateExtras.rmVehState =
                                        EVENT_VEHICLE_STATE_WAIT_CMP_USER_ACCPT;
    ntf_event->eventExtras.rmStateExtras.stateScope = EVENT_STATE_SCOPE_VEHICLE;

    return SDPV3_ERROR_NONE;
}


static sdpv3_error_t ota_reprog_status_ntc(t1_ctx_t *t1_ctx)
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
    ntf_event->eventExtras.rmStateExtras.rmVehState =EVENT_VEHICLE_STATE_PKG_DL;
    ntf_event->eventExtras.rmStateExtras.stateScope = EVENT_STATE_SCOPE_VEHICLE;
    strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t dl_progress_event(t1_ctx_t *t1_ctx, void *upt_ctx)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    upt_context_t *ctx = (upt_context_t *) upt_ctx;
    targetAndCustom_t *downloadMeta = NULL;
    targetsMetaBody_t *tgts_meta =
                        &ctx->metadata[REPODIR].dld_targets->targetsMetaBody;
    changeEvent_t *change_event = t1_ctx->ce;
    char decoded_filepath[1<<10];

    // find downloadMetadata target for respective change event
    for (int32_t i = 0U; i < tgts_meta->targetsNum; i++) {
        if (0 == strncmp(tgts_meta->targets[i].custom.reportId,
                            change_event->reportId, MAX_REPORT_ID_SZ)) {

            if (0 == strncmp(tgts_meta->targets[i].custom.ecuIdentifier,
                                "DownloadMetadata", MAX_SERIAL_NUM_SZ)) {
                downloadMeta = &tgts_meta->targets[i];
                break;
            }
        }
    }

    // use found target to re-create the file path/name where it was downloaded
    // in a previous step
    char dlMetaPath[1<<10] = {};
    (void) abst_getImageFileName(dlMetaPath, downloadMeta, change_event, false);

    // always download the image, even if the image has already been downloaded
    // before
    char relativepath[ABST_MAX_RELATIVE_PATH_NUM][ABST_MAX_RELATIVE_PATH_SZ]={};

    uint16_t package_num = abst_ParseDownloadMetadataFile(dlMetaPath,
                                                            relativepath, true);

    // Clear any previous notifications
    //memset(t1_ctx->ntf.events, 0x00U, sizeof(event_t) * MAX_NOTIFICATIONS);
    // FIXME: currently t1_ctx->ntf.eventsNum = 1
    memset(ntf_event, 0x00U, sizeof(event_t));

   t1_ctx->ntf.events[0U].eventExtras.dwnldExtras.downloadSize = 0U;
    // download each package referenced in downloadMetadata file
    for (int32_t x = 0U; x < package_num; ++x) {
        // find target for respective package
        for (int32_t i = 0U; i < tgts_meta->targetsNum; i++) {
            if (0 == strncmp(tgts_meta->targets[i].custom.reportId,
                                change_event->reportId, MAX_REPORT_ID_SZ)) {
                memset(decoded_filepath, 0 , 1<<10);
                abst_base64_decode(relativepath[x], strlen(relativepath[x]),
                                    decoded_filepath, 1<<10);
                if (0 == strncmp(tgts_meta->targets[i].target.fileName,
                                    decoded_filepath, MAX_FILE_NAME_SZ)) {
                    snprintf(t1_ctx->dld_pkg_path, MAXPATHLEN,
                                UPTANE_BASE UPT_PACKAGE "/%s",
                                tgts_meta->targets[i].target.fileName);
                    printf("INFO: Target FileName: %s\n", t1_ctx->dld_pkg_path);
                    t1_ctx->ntf.events[0U].eventExtras.dwnldExtras.downloadSize
                                        += tgts_meta->targets[i].target.length;
                    break;
                }
            }
        }
    }

    ret = create_base_event(ntf_event, EVENT_MEM_NOT_CLEAR, // not clear
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_DWNLD, EVENT_MODE_ACTIVE,
                            EVENT_DOWNLOAD, EVENT_STATUS_INPROGRESS);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.dwnldExtras.ecuInfoNum = 0U;
    // progress notification is notified only one time before downloading.
    // Then, bytesDownloaded = 0U.
    ntf_event->eventExtras.dwnldExtras.bytesDownloaded = 0U;
    strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

    return SDPV3_ERROR_NONE;
}


static sdpv3_error_t
dl_compl_event(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;
    uint64_t downloadSize = ntf_event->eventExtras.trackingExtras.fileSize;


    ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_DWNLD, EVENT_MODE_ACTIVE,
                            EVENT_DOWNLOAD, sdpv3_cfg->nty_err_code);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.dwnldExtras.ecuInfoNum = 0U;
    ntf_event->eventExtras.dwnldExtras.bytesDownloaded = downloadSize;
    ntf_event->eventExtras.dwnldExtras.downloadSize = downloadSize;
    strncpy(ntf_event->reportId,
            t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

    return SDPV3_ERROR_NONE;
}


static sdpv3_error_t
comp_ntc_pkg_verify_comp(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;
    uint64_t downloadSize = ntf_event->eventExtras.dwnldExtras.downloadSize;

    eventStatus_t event_status;
    if (sdpv3_cfg->nty_err_code == EVENT_STATUS_SUCCESS) {
        event_status = EVENT_STATUS_VALIDATED;
    } else {
        event_status = EVENT_STATUS_FAILURE;
    }

    ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_DWNLD, EVENT_MODE_ACTIVE,
                            EVENT_DOWNLOAD, event_status);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.dwnldExtras.ecuInfoNum = 0U;
    ntf_event->eventExtras.dwnldExtras.bytesDownloaded = downloadSize;
    ntf_event->eventExtras.dwnldExtras.downloadSize = downloadSize;
    strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

    return SDPV3_ERROR_NONE;
}


static sdpv3_error_t
comp_ntc_data_xtract_comp(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;
    uint64_t downloadSize = ntf_event->eventExtras.dwnldExtras.downloadSize;

    eventStatus_t event_status;
    if (sdpv3_cfg->nty_err_code == EVENT_STATUS_SUCCESS) {
        event_status = EVENT_STATUS_EXTRACTED;
    } else {
        event_status = EVENT_STATUS_FAILURE;
    }

    ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_RMSTATE, EVENT_MODE_ACTIVE,
                            EVENT_RMSTATE, event_status);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.dwnldExtras.ecuInfoNum = 0U;
    ntf_event->eventExtras.dwnldExtras.bytesDownloaded = downloadSize;
    ntf_event->eventExtras.dwnldExtras.downloadSize = downloadSize;
    strncpy(ntf_event->reportId, t1_ctx->ce->reportId, MAX_REPORT_ID_SZ);

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t downloadCE_packages(t1_ctx_t *t1_ctx,
                                        const sdpv3_cfg_t *sdpv3_cfg,
                                        void *upt_ctx)
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

        uptaneErr_t upterr;

        // Send Event: OTA reprogramming status notice (package DL)
        if ((ret = ota_reprog_status_ntc(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("ota_reprog_status_ntc() failed: %s:%d\n",
                    __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }

        // Send Event: Download progress
        if ((ret = dl_progress_event(t1_ctx, upt_ctx)) != SDPV3_ERROR_NONE) {
            printf("dl_progress_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }

        upterr = uptane_download_change_event(upt_ctx,
                                        &selected_campaign->changeEvents[ice],
                                        ntf_event, sdpv3_cfg->rootca);
        if (UPTANE_NO_ERROR != upterr) {
            printf("uptane_download_change_event() failed:"
                    "%d %s:%d\n", upterr, __func__, __LINE__);
            return SDPV3_ERROR_UNKNOWN;
        }
        // Send Event: Tracking Change Event
        if ((ret = ce_tracking_event(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("ce_tracking_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

sdpv3_error_t notify_pkg_download_completion(t1_ctx_t *t1_ctx,
                                            const sdpv3_cfg_t *sdpv3_cfg,
                                            void *upt_ctx)
{
    sdpv3_error_t ret;
    campaign_t *selected_campaign = t1_ctx->selected_campaign;
    if (NULL == selected_campaign) {
        return SDPV3_ERROR_NONE;
    }

    for (unsigned int ice = 0U;
            ((ice < MAX_CHANGE_EVENTS) && (ice < selected_campaign->ceNum));
            ice++) {
        t1_ctx->ce = &selected_campaign->changeEvents[ice];

        uptaneErr_t upterr;

        // Send Event: Completion notice (download completion)
        if ((ret = dl_compl_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("dl_compl_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }

        upterr = uptane_vrfy_and_decrypt_image(upt_ctx,
                                        &selected_campaign->changeEvents[ice]);
        if (UPTANE_NO_ERROR != upterr) {
            printf("uptane_vrfy_and_decrypt_image() failed:"
                    "%d %s:%d\n", upterr, __func__, __LINE__);
            return SDPV3_ERROR_UNKNOWN;
        }
        // Send Event: Completion notice (package verification completion)
        ret = comp_ntc_pkg_verify_comp(t1_ctx, sdpv3_cfg);
        if (ret != SDPV3_ERROR_NONE) {
            printf("comp_ntc_pkg_verify_comp() failed: %s:%d\n",
                    __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: Completion notice (data extraction completion)
        ret = comp_ntc_data_xtract_comp(t1_ctx, sdpv3_cfg);
        if (ret != SDPV3_ERROR_NONE) {
            printf("comp_ntc_data_xtract_comp() failed: %s:%d\n",
                    __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t camp_accept_nfy(t1_ctx_t *t1_ctx)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_NO_EXTRAS, EVENT_MODE_ACTIVE,
                            EVENT_NOTIFICATION, EVENT_STATUS_ACCEPTED);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }

    return SDPV3_ERROR_NONE;
}

static sdpv3_error_t dl_accept_nfy(t1_ctx_t *t1_ctx)
{
    sdpv3_error_t ret;
    event_t *ntf_event = &t1_ctx->ntf.events[0U];
    campaign_t *selected_campaign = t1_ctx->selected_campaign;

    ret = create_base_event(ntf_event, EVENT_MEM_CLEAR,
                            selected_campaign->campaignId,
                            &t1_ctx->ntf.eventsNum,
                            UNION_FLAG_DWNLD, EVENT_MODE_ACTIVE,
                            EVENT_DOWNLOAD, EVENT_STATUS_ACCEPTED);
    if (ret != SDPV3_ERROR_NONE) {
        printf("create_base_event() failed: %s:%d\n", __func__, __LINE__);
        return ret;
    }
    ntf_event->eventExtras.dwnldExtras.ecuInfoNum = 0U;
    ntf_event->eventExtras.dwnldExtras.bytesDownloaded = 0U;
    ntf_event->eventExtras.dwnldExtras.downloadSize = 0U;

    return SDPV3_ERROR_NONE;
}

sdpv3_error_t run_download(t1_ctx_t *t1_ctx,
                            const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx)
{
    abst_err_t err;
    err = abst_get_vin(t1_ctx->ntf.vin, sdpv3_cfg->campaign_vin);
    if (err != ABST_NO_ERROR) {
        printf("abst_get_vin() failed: %s:%d\n", __func__, __LINE__);
        return SDPV3_ERROR_UNKNOWN;
    }
    err = abst_get_primary_serialno(t1_ctx->ntf.primarySerialNum,
                                    sdpv3_cfg->primary_serial);
    if (err != ABST_NO_ERROR) {
        printf("abst_get_primary_serialno() failed: %s:%d\n", __func__, __LINE__);
        return SDPV3_ERROR_UNKNOWN;
    }
    
    sdpv3_error_t ret;
    if (t1_ctx->selected_campaign->campaignType == CMP_TYPE_INFO) {
        // Download HMI MSG file
        ret = downloadHMI(t1_ctx, sdpv3_cfg, upt_ctx);
        if (ret != SDPV3_ERROR_NONE) {
            // FIXME: (WA) currently there is an error here
            //printf("downloadHMI() failed: %s:%d\n", __func__, __LINE__);
            //return ret;
        }
        // Send Event: OTA status wait for campaign acceptance
        if ((ret = wait_for_camp_accept(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("wait_for_camp_accept() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: Campaign User Acceptance
        if ((ret = camp_accept_nfy(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("camp_accept_nfy() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: Informational Campaign Completion
        if ((ret = info_camp_comp_nfy(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("info_camp_comp_nfy() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        uptane_commit(upt_ctx);
    } else {
        // Download HMI MSG file
        ret = downloadHMI(t1_ctx, sdpv3_cfg, upt_ctx);
        if (ret != SDPV3_ERROR_NONE) {
            // FIXME: (WA) currently there is an error here
            //printf("downloadHMI() failed: %s:%d\n", __func__, __LINE__);
            //return ret;
        }
        // Download Change Event Metadata
        ret = downloadCE_metadata(t1_ctx, sdpv3_cfg, upt_ctx);
        if (ret != SDPV3_ERROR_NONE) {
            printf("downloadCE_metadata() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: OTA status wait for campaign acceptance
        if ((ret = wait_for_camp_accept(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("wait_for_camp_accept() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: campaign acceptance
        if ((ret = camp_accept_nfy(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("camp_accept_nfy() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: OTA status wait for download
        if ((ret = wait_for_download_event(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("wait_for_download_event() failed: %s:%d\n",
                    __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Send Event: DL acceptance
        if ((ret = dl_accept_nfy(t1_ctx)) != SDPV3_ERROR_NONE) {
            printf("dl_accept_nfy() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        if ((ret = send_event(t1_ctx, sdpv3_cfg)) != SDPV3_ERROR_NONE) {
            printf("send_event() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
        // Download and verify Change Event Package
        ret = downloadCE_packages(t1_ctx, sdpv3_cfg, upt_ctx);
        if (ret != SDPV3_ERROR_NONE) {
            printf("downloadCE_packages() failed: %s:%d\n", __func__, __LINE__);
            return ret;
        }
    }

    return SDPV3_ERROR_NONE;
}

