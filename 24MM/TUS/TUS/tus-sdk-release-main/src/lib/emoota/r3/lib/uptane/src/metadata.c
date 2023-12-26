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

#include <stdio.h>
#include "upt_internal.h"

#define HTTP_NOT_FOUND (404U)

uptaneErr_t metadata_parse_signed(upt_context_t *ctx, byte_t *json_str, int32_t json_len,
                                  void *parsed_signed, metaType_t meta_type, roleType_t *role_type) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    switch (meta_type) {
        case META_ROOT:
            //printf("[ RUN      ] abq_upt_meta_parse_024\n");
            if (UTIL_NO_ERROR != util_ParseUptRootMetadata(json_str, json_len,
                                                           ctx->json_mem, ctx->json_qty, (rootMeta_t *)parsed_signed)) {
                // either invalid json or too long a json is treated as invalid metadata
                err = UPTANE_ERR_INVALID_METADATA;
            } else {
                *role_type = ((rootMeta_t *)parsed_signed)->type;
            }
            //printf("[ RUN      ] abq_upt_meta_parse_025\n");
            break;
        case META_TIMESTAMP:
            //printf("[ RUN      ] abq_upt_meta_parse_009\n");
            if (UTIL_NO_ERROR != util_ParseUptTimeMetadata(json_str, json_len,
                                                           ctx->json_mem, ctx->json_qty, (timeMeta_t *)parsed_signed)) {
                // either invalid json or too long a json is treated as invalid metadata
                err = UPTANE_ERR_INVALID_METADATA;
            } else {
                *role_type = ((timeMeta_t *)parsed_signed)->type;
            }
            //printf("[ RUN      ] abq_upt_meta_parse_010\n");
            break;
        case META_SNAPSHOT:
            //printf("[ RUN      ] abq_upt_meta_parse_011\n");
            if (UTIL_NO_ERROR != util_ParseUptSnapshotMetadata(json_str, json_len,
                                                               ctx->json_mem, ctx->json_qty, (snapshotMeta_t *)parsed_signed)) {
                // either invalid json or too long a json is treated as invalid metadata
                err = UPTANE_ERR_INVALID_METADATA;
            } else {
                *role_type = ((snapshotMeta_t *)parsed_signed)->type;
            }
            //printf("[ RUN      ] abq_upt_meta_parse_012\n");
            break;
        case META_TARGETS:
            //printf("[ RUN      ] abq_upt_meta_parse_013\n");
            if (UTIL_NO_ERROR != util_ParseUptTargetsMetadata(json_str, json_len,
                                                              ctx->json_mem, ctx->json_qty, (targetsMeta_t *)parsed_signed)) {
                // either invalid json or too long a json is treated as invalid metadata
                err = UPTANE_ERR_INVALID_METADATA;
            } else {
                *role_type = ((targetsMeta_t *)parsed_signed)->type;
            }
            //printf("[ RUN      ] abq_upt_meta_parse_014\n");
            break;
        case META_AUGMENTED:
            //printf("[ RUN      ] abq_upt_meta_parse_015\n");
            if (UTIL_NO_ERROR != util_ParseUptAugmentedMetadata(json_str, json_len,
                                                                ctx->json_mem, ctx->json_qty, (augMeta_t *)parsed_signed)) {
                // either invalid json or too long a json is treated as invalid metadata
                err = UPTANE_ERR_INVALID_METADATA;
            } else {
                *role_type = ((augMeta_t *)parsed_signed)->type;
            }
            //printf("[ RUN      ] abq_upt_meta_parse_016\n");
            break;
        case META_GET_OTA_CMD:
            //printf("[ RUN      ] abq_upt_meta_parse_026\n");
            if (UTIL_NO_ERROR != util_ParseGetOtaCmdRes(json_str, json_len,
                                                        ctx->json_mem, ctx->json_qty, (getOtaCmdRes_t *)parsed_signed)) {
                // either invalid json or too long a json is treated as invalid metadata
                err = UPTANE_ERR_INVALID_METADATA;
            } else {
                *role_type = ((getOtaCmdRes_t *)parsed_signed)->type;
            }
            //printf("[ RUN      ] abq_upt_meta_parse_027\n");
            break;
        default:
            break;
    }

    if (UPTANE_NO_ERROR == err) {
        if ((RT_ROOT > *role_type) || (NUM_ROLES <= *role_type)) {
            err = UPTANE_ERR_INVALID_METADATA;
        }
    }
    return err;
}

uptaneErr_t parse_next_root(upt_context_t *ctx, repo_index_t repo,
                            byte_t *json_str, size_t json_len) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked

    metadata_set_t *meta = &ctx->metadata[repo];

    rootMeta_t *root = (rootMeta_t *) calloc(1, sizeof(rootMeta_t));
    if (NULL == root) {
        err = UPTANE_ERR_RESOURCE;
    } else {
        err = metadata_parse_and_verify_sigs(ctx, meta->latest_root, json_str, json_len,
                                             (void *) root, META_ROOT);
    }

    if (UPTANE_NO_ERROR == err) {
        // Uptane-1.0.0 5.4.4.3 => 2 => 4
        if (meta->latest_root->version > root->version) {
            err = UPTANE_ERR_ROLLBACK_ATTACK;
        }
    }
    if (UPTANE_NO_ERROR == err) {
        if (meta->latest_root->version != meta->previous_root->version) {
            free(ctx->metadata[repo].previous_root);
        }

        meta->previous_root = meta->latest_root;
        // Uptane-1.0.0 5.4.4.3 => 2 => 5
        meta->latest_root = root;
    } else {
        free(root);
    }

    return err;
}

uptaneErr_t upt_get_root_up_to_data(upt_context_t *ctx, repo_index_t repo,
                                    cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked
    bool_t new_root_loaded = false;

    printf("Try to get the latest Root Metadata\n");
    do {
        rootMeta_t *prev_root = ctx->metadata[repo].latest_root;
        // Uptane-1.0.0 5.4.4.3 => 2 => 1
        //printf("[ RUN      ] abq_check_root_meta_001\n");
        uint32_t next_version = prev_root->version + 1U;

        byte_t json_str[MAX_METADATA_SZ];
        size_t json_len = 0;
        uint32_t http_status_code = 0U;

        // Uptane-1.0.0 5.4.4.3 => 2 => 2
        abst_err_t absterr = ABST_NO_ERROR;
        //printf("[ RUN      ] abq_check_root_meta_002\n");
        //printf("[ RUN      ] abq_check_root_meta_003\n");
        if (REPODIR == repo) {
            absterr = abst_DownloadRootDirMetadata(json_str, MAX_METADATA_SZ, &json_len,
                                                   next_version, &http_status_code,
                                                   cdn_url, cdn_port, rootca);
        } else {
            absterr = abst_DownloadRootImgMetadata(json_str, MAX_METADATA_SZ, &json_len,
                                                   next_version, &http_status_code,
                                                   cdn_url, cdn_port, rootca);
        }

        if (ABST_ERR_NO_RESOURCE == absterr) {
            err = UPTANE_ERR_RESOURCE;
        } else if (ABST_ERR_NOT_AVAILABLE == absterr) {
            if (http_status_code != HTTP_NOT_FOUND) {
                err = UPTANE_ERR_NETWORK;
            }
            printf("INFO: No new version of root metadata.\n");
            //printf("[ RUN      ] abq_check_root_meta_008\n");
            //printf("[ RUN      ] abq_check_root_meta_009\n");
            break;// make sure to break if (http_status_code == 404)
        } else {
            printf("INFO: New version of root metadata is available.\n");
            //printf("[ RUN      ] abq_check_root_meta_004\n");
            //printf("[ RUN      ] abq_check_root_meta_005\n");
            memcpy(ctx->metadata[repo].json_root, json_str, json_len);
            // Unlike other metadata files, there could be more than one root.
            // So this buffer may get re-used. Need to make sure the string ends with '\0'
            ctx->metadata[repo].json_root[json_len] = '\0';

            err = parse_next_root(ctx, repo, json_str, json_len);

            //printf("[ RUN      ] abq_check_root_meta_007\n");
            if (UPTANE_NO_ERROR == err) {
                // cache newly downloaded root after signature checking
                if (REPODIR == repo) {
                    absterr = (abst_err_t) abst_SaveCachedRootDirMetadata(ctx->metadata[repo].json_root, next_version);
                } else {
                    absterr = (abst_err_t) abst_SaveCachedRootImgMetadata(ctx->metadata[repo].json_root, next_version);
                }
            }
        }
        // Uptane-1.0.0 5.4.4.3 => 2 => 6;
    } while (UPTANE_NO_ERROR == err);

    if (UPTANE_NO_ERROR == err) {
        err = upt_verify_root_metadata(ctx, repo);
    }

    return err;
}

uptaneErr_t parse_cfg_match_resp(upt_context_t *ctx, cstr_t response) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    metaInfo_t metaInfo = {0};
    // internal call, assume parameters already checked
    metadata_set_t *metaset = &ctx->metadata[REPODIR];
    printf("Parse ConfigMatch/SyncCheck response.\n");
    //printf("[ RUN      ] abq_upt_sync_rsp_001\n");

    // Uptane-1.0.0 5.4.4.4 => 1 (director)
    // Uptane-1.0.0 5.4.4.6 => 1 (director repo)
    //printf("[ RUN      ] abq_upt_sync_rsp_002\n");
    if (UTIL_NO_ERROR != util_FindBundleSections(response, strlen(response), &metaInfo)) {
        err = UPTANE_ERR_INVALID_METADATA;
    } else {
        memcpy(metaset->json_timestamp, metaInfo.headTimestamp, metaInfo.TimestampLength);
        memcpy(metaset->json_snapshot, metaInfo.headSnapshot, metaInfo.SnapshotLength);
        memcpy(metaset->json_targets, metaInfo.headTargets, metaInfo.TargetsLength);
        memcpy(metaset->json_aug, metaInfo.headAugment, metaInfo.AugmentLength);
    }
    //printf("[ RUN      ] abq_upt_sync_rsp_003\n");

    if (UPTANE_NO_ERROR == err) {
        err = metadata_parse_and_verify_sigs(ctx, metaset->latest_root,
                                             metaInfo.headTimestamp, metaInfo.TimestampLength,
                                             (void *) metaset->dld_timestamp, META_TIMESTAMP);
        if (UPTANE_NO_ERROR == err) {
            err = upt_verify_timestamp_metadata(ctx, REPODIR);
        }
    }
    if (UPTANE_NO_ERROR == err) {
        // Uptane-1.0.0 5.4.4.5 => 1 (director repo)
        if (metaInfo.SnapshotLength != metaset->dld_timestamp->timeMetaBody.length) {
            err = UPTANE_ERR_INVALID_METADATA;
        }
        if (UPTANE_NO_ERROR == err) {
	    //printf("[ RUN      ] abq_upt_sync_rsp_004\n");
            err = check_snapshot_digest(ctx, REPODIR, metaInfo.headSnapshot, metaInfo.SnapshotLength);
	    //printf("[ RUN      ] abq_upt_sync_rsp_005\n");
	    //printf("[ RUN      ] abq_upt_sync_rsp_015\n");
        }
        if (UPTANE_NO_ERROR == err) {
            err = metadata_parse_and_verify_sigs(ctx, metaset->latest_root,
                                                 metaInfo.headSnapshot, metaInfo.SnapshotLength,
                                                 (void *) metaset->dld_snapshot, META_SNAPSHOT);
        }
        if (UPTANE_NO_ERROR == err) {
            //printf("[ RUN      ] abq_upt_sync_rsp_014\n");
            err = upt_verify_snapshot_metadata(ctx, REPODIR);
        }
    }
    if (UPTANE_NO_ERROR == err) {
        err = metadata_parse_and_verify_sigs(ctx, metaset->latest_root,
                                             metaInfo.headTargets, metaInfo.TargetsLength,
                                             (void *) metaset->dld_targets, META_TARGETS);
        if (UPTANE_NO_ERROR == err) {
            //printf("[ RUN      ] abq_upt_sync_rsp_016\n");
            err = upt_verify_targets_metadata(ctx, REPODIR);
        }
    }
    if (UPTANE_NO_ERROR == err) {
        err = metadata_parse_and_verify_sigs(ctx, metaset->latest_root,
                                             metaInfo.headAugment, metaInfo.AugmentLength,
                                             (void *) metaset->dld_aug, META_AUGMENTED);
        if (UPTANE_NO_ERROR == err) {
            err = upt_verify_augmented_metadata(ctx);
        }
    }
    if (UPTANE_NO_ERROR == err) {
        if ((CONFIG_CHECK_STATUS_UPDATE != UPT_OTA_STATUS(ctx)) && UPT_TARGET_PENDING(ctx)) {
            err = UPTANE_ERR_INVALID_METADATA;
        }
    }
    return err;
}

uptaneErr_t dld_and_verify_image_repo_meta(upt_context_t *ctx, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    byte_t json_str[MAX_METADATA_SZ];
    size_t json_len = 0;
    metadata_set_t *metaset = &ctx->metadata[REPOIMG];

    err = upt_get_root_up_to_data(ctx, REPOIMG, cdn_url, cdn_port, rootca);

    abst_err_t absterr = ABST_NO_ERROR;
    if (UPTANE_NO_ERROR == err) {
        // Uptane-1.0.0 5.4.4.4 => 1 (image)
        //printf("[ RUN      ] abq_dwnld_img_meta_001\n");
        absterr = abst_DownloadTimestampImgMetadata(json_str, MAX_METADATA_SZ, &json_len, cdn_url, cdn_port, rootca);
        if (ABST_ERR_NO_RESOURCE == absterr) {
            err = UPTANE_ERR_RESOURCE;
        } else if (ABST_ERR_TRANSACTION_FAILED == absterr) {
            err = UPTANE_ERR_NETWORK;
        } else {
            //printf("[ RUN      ] abq_dwnld_img_meta_004\n");
            memcpy(metaset->json_timestamp, json_str, json_len);
            err = metadata_parse_and_verify_sigs(ctx, metaset->latest_root,
                                                 json_str, json_len, (void *) metaset->dld_timestamp, META_TIMESTAMP);
        }

        if (UPTANE_NO_ERROR == err) {
            err = upt_verify_timestamp_metadata(ctx, REPOIMG);
        }
    }

    if (UPTANE_NO_ERROR == err) {
        char filename[MAX_FILE_NAME_SZ + 12U]; // big enough
        // Uptane-1.0.0 5.4.4.5 => 1 (image repo)
        //printf("[ RUN      ] abq_dwnld_img_meta_007\n");
        //printf("[ RUN      ] abq_dwnld_img_meta_008\n");
        snprintf(filename, sizeof(filename), "%u.%s", metaset->dld_timestamp->timeMetaBody.version,
                 metaset->dld_timestamp->timeMetaBody.fileName);
        //printf("[ RUN      ] abq_dwnld_img_meta_009\n");
        absterr = abst_DownloadSnapshotImgMetadata(json_str,
                                                   MAX_METADATA_SZ,
                                                   &json_len, filename,
                                                   cdn_url, cdn_port, rootca);
        if (ABST_ERR_NO_RESOURCE == absterr) {
            err = UPTANE_ERR_RESOURCE;
        } else if (ABST_ERR_TRANSACTION_FAILED == absterr) {
            err = UPTANE_ERR_NETWORK;
        } else {
            //printf("[ RUN      ] abq_dwnld_img_meta_012\n");

            memcpy(metaset->json_snapshot, json_str, json_len);
            //printf("[ RUN      ] abq_dwnld_img_meta_013\n");
            //printf("[ RUN      ] abq_dwnld_img_meta_014\n");
            //printf("[ RUN      ] abq_dwnld_img_meta_018\n");
            // check_snapshot_digest function calculates digest, and compare calculated digest and timestamp metadata.
            err = check_snapshot_digest(ctx, REPOIMG, json_str, metaset->dld_timestamp->timeMetaBody.length);

            if (UPTANE_NO_ERROR == err) {
                err = metadata_parse_and_verify_sigs(ctx, metaset->latest_root,
                                                     json_str, json_len, (void *) metaset->dld_snapshot, META_SNAPSHOT);
            }
        }

        if (UPTANE_NO_ERROR == err) {
            //printf("[ RUN      ] abq_dwnld_img_meta_017\n");
            err = upt_verify_snapshot_metadata(ctx, REPOIMG);
        }
    }

    // suppress downloading of image/targets.json

    return err;
}

uptaneErr_t  get_data_encryption_key(upt_context_t *ctx, cstr_t ecryptdSymKeyValue, byte_t *dek, size_t buflen, size_t *deklen) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    size_t b64_len = strlen(ecryptdSymKeyValue);
    size_t dek_str_len = 0U;

    dataEncryptionKey_t dek_struct = {0};

    byte_t dek_str[MAX_ECRYPTD_SYMKEY_VALUE_SZ] = {0};

    //printf("[ RUN      ] abq_decrypt_file_001\n");
    dek_str_len = abst_base64_decode(ecryptdSymKeyValue, b64_len, dek_str, MAX_ECRYPTD_SYMKEY_VALUE_SZ);
    if (0 > dek_str_len) {
        err = UPTANE_ERR_INVALID_METADATA; // Something is wrong with the b64 encoded string.
    }
    //printf("[ RUN      ] abq_decrypt_file_002\n");

    if (UPTANE_NO_ERROR == err) {
        //printf("[ RUN      ] abq_decrypt_file_005\n");
        if (UTIL_NO_ERROR != util_ParseDataEncryptionKey(dek_str, dek_str_len, ctx->json_mem, ctx->json_qty, &dek_struct)) {
            err = UPTANE_ERR_INVALID_METADATA;
        }
        //printf("[ RUN      ] abq_decrypt_file_006\n");
    }
    if (UPTANE_NO_ERROR == err) {
        size_t encoded_key_len = strlen(dek_struct.value);
        size_t decoded_key_len = 0U;
        byte_t encrypted_key[MAX_BASE64_ENCRYPTED_DEK_SZ] = {0};

        //printf("[ RUN      ] abq_decrypt_file_003\n");
        decoded_key_len = abst_base64_decode(dek_struct.value, encoded_key_len, encrypted_key, MAX_BASE64_ENCRYPTED_DEK_SZ);
        //printf("[ RUN      ] abq_decrypt_file_004\n");
        if (0 > decoded_key_len) {
            err = UPTANE_ERR_INVALID_METADATA; // Something is wrong with the b64 encoded string.
        } else {
            //printf("[ RUN      ] abq_decrypt_file_007\n");
            if (ABST_NO_ERROR != abst_decrypt_dek(encrypted_key, decoded_key_len, (uint8_t *)dek, buflen, deklen, dek_struct.padding)) {
                err = UPTANE_ERR_INVALID_METADATA;
            }
            //printf("[ RUN      ] abq_decrypt_file_008\n");
        }
    }

    return err;
}


#ifdef __cplusplus
}
#endif
