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

static uptaneErr_t load_trusted_root(upt_context_t *ctx, repo_index_t repo);
static uptaneErr_t load_next_cached_root(upt_context_t *ctx, repo_index_t repo, bool_t *new_root_loaded);
static uptaneErr_t load_cached_timestamp(upt_context_t *ctx, repo_index_t repo);
static uptaneErr_t load_cached_snapshot(upt_context_t *ctx, repo_index_t repo);
static uptaneErr_t load_cached_targets(upt_context_t *ctx, repo_index_t repo);
static uptaneErr_t load_cached_augmented(upt_context_t *ctx);

static uptaneErr_t prepare_trust_root(metadata_set_t *metaset, byte_t *buf, size_t buflen);

uptaneErr_t save_dir_cached_metadata(upt_context_t *ctx) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    metadata_set_t *metaset = &ctx->metadata[REPODIR];
    byte_t trust_root_str[MAX_METADATA_SZ];

    if (metaset->previous_root->version != metaset->trusted_root_ver) {
        // Root has updated. Prepare trust root to be updated
        err = prepare_trust_root(metaset, trust_root_str, MAX_METADATA_SZ);
    }

    if (UPTANE_NO_ERROR == err) {
        /* NOTE: It is important that this block of code not being interrupted.
        * Otherwise the mis-matched version of cached metadata files could cause failure to verify
        * the cached metadata files upon next OTA request.
        *
        * Could consider using some technics to make it automic.
        */
        int32_t ret = 0;

        if (metaset->previous_root->version != metaset->trusted_root_ver) {
            ret = abst_SaveTrustRootDirMetadata(trust_root_str);
        }

        ret = abst_SaveLatestTimestampDirMetadata(metaset->json_timestamp);

        ret = abst_SaveLatestSnapshotDirMetadata(metaset->json_snapshot);

        ret = abst_SaveLatestTargetsDirMetadata(metaset->json_targets);

        ret = abst_SaveLatestAugmentedMetadata(metaset->json_aug);

        /* NOTE: end of code that should not be interrupted */
    }

    if (UPTANE_NO_ERROR == err) {
        // Now we can discard old root metadata files with version <= trust_root_version
        abst_DiscardObsoleteRootDir(metaset->previous_root->version);
    }
    return err;
}

uptaneErr_t save_img_cached_metadata(upt_context_t *ctx) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    metadata_set_t *metaset = &ctx->metadata[REPOIMG];
    byte_t trust_root_str[MAX_METADATA_SZ];

    if (metaset->previous_root->version != metaset->trusted_root_ver) {
        // Root has updated. Prepare trust root to be updated
        err = prepare_trust_root(metaset, trust_root_str, MAX_METADATA_SZ);
    }

    if (UPTANE_NO_ERROR == err) {
        /* NOTE: It is important that this block of code not being interrupted.
        * Otherwise the mis-matched version of cached metadata files could cause failure to verify
        * the cached metadata files upon next OTA request.
        *
        * Could consider using some technics to make it automic.
        */
        int32_t ret = 0;

        if (metaset->previous_root->version != metaset->trusted_root_ver) {
            ret = abst_SaveTrustRootImgMetadata(trust_root_str);
        }

        ret = abst_SaveLatestTimestampImgMetadata(metaset->json_timestamp);

        ret = abst_SaveLatestSnapshotImgMetadata(metaset->json_snapshot);

        // suppress saving of image/targets.json

        /* NOTE: end of code that should not be interrupted */
    }

    if (UPTANE_NO_ERROR == err) {
        // Now we can discard old root metadata files with version <= trust_root_version
        abst_DiscardObsoleteRootImg(metaset->previous_root->version);
    }
    return err;
}

uptaneErr_t load_cached_metadata(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    // load trusted root (version M)
    printf("Load trusted root metadata\n");
    err = load_trusted_root(ctx, repo);

    // Uptane-1.0.0 5.4.4.3 => 1
    if (UPTANE_NO_ERROR == err) {
        // Load cached root version M+1
        bool_t new_root_loaded = false;
        printf("Load cached root metadata\n");
        err = load_next_cached_root(ctx, repo, &new_root_loaded);
        if ((UPTANE_NO_ERROR == err) && (!new_root_loaded)) {
            err = UPTANE_ERR_INVALID_METADATA;
        } else {
            memcpy(&ctx->metadata[repo].starting_root, ctx->metadata[repo].latest_root, sizeof(rootMeta_t));
        }
    }

    // Use cached root version M+1 to verify signatures of cached non-root metadata
    if (UPTANE_NO_ERROR == err) {
        printf("Load cached timestamp metadata\n");
        err = load_cached_timestamp(ctx, repo);
    }
    if (UPTANE_NO_ERROR == err) {
        printf("Load cached snapshot metadata\n");
        err = load_cached_snapshot(ctx, repo);
    }
    // suppress loading of image/targets.json (repo=REPOIMG)
    if ((UPTANE_NO_ERROR == err) && (REPODIR == repo)) {
        printf("Load cached targets metadata\n");
        err = load_cached_targets(ctx, repo);
    }
    if ((UPTANE_NO_ERROR == err) && (REPODIR == repo)) {
        printf("Load cached augmented metadata\n");
        err = load_cached_augmented(ctx);
    }

    if (UPTANE_NO_ERROR == err) {
        /* Load next version of cached root, until there is no more.
         *
         * There is chance that newer root metadata file has been downloaded, but the client was not
         * ready to update the trusted root (version M and M+1), for example, if downloading
         * new non-root metadata failed after getting new version of root.
         * The newer root metadata can be cached here, so the client doesn't need to download again.
         */
        printf("Load next version of cached root, until there is no more\n");
        bool_t new_root_loaded = false;
        do {
            err = load_next_cached_root(ctx, repo, &new_root_loaded);
            if (UPTANE_NO_ERROR != err) {
                break;
            }
        } while (new_root_loaded);
    }
    return err;
}

static uptaneErr_t load_trusted_root(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked

    byte_t json_str[MAX_METADATA_SZ];
    (void) memset(json_str, 0x00U, sizeof(json_str));
    int32_t rval = 0;

    //printf("[ RUN      ] abq_upt_cache_verify_001\n");
    if (REPODIR == repo) {
        rval = abst_LoadTrustRootDirMetadata(json_str);
    } else {
        rval = abst_LoadTrustRootImgMetadata(json_str);
    }
    if (0 > rval) {
        err = UPTANE_ERR_RESOURCE;
    }
    //printf("[ RUN      ] abq_upt_cache_verify_002\n");

    if (UPTANE_NO_ERROR == err) {
        //printf("[ RUN      ] abq_upt_cache_verify_003\n");
        if (UTIL_NO_ERROR != util_ParseTrustRootMetadata(json_str, strlen(json_str),
                                                         ctx->json_mem, ctx->json_qty, (rootMeta_t *) ctx->metadata[repo].previous_root)) {
            err = UPTANE_ERR_INVALID_METADATA;
        }
        //printf("[ RUN      ] abq_upt_cache_verify_004\n");
    }
    if (UPTANE_NO_ERROR == err) {
        ctx->metadata[repo].trusted_root_ver = ctx->metadata[repo].previous_root->version;
    }

    return err;
}

static uptaneErr_t load_next_cached_root(upt_context_t *ctx, repo_index_t repo, bool_t *new_root_loaded) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked

    rootMeta_t *prev_root = ctx->metadata[repo].latest_root;
    // Check root version to load
    // version is hardcoded in this sample application.
    //printf("[ RUN      ] abq_upt_cache_verify_005\n");
    uint32_t next_version = prev_root->version + 1U;

    *new_root_loaded = false;

    byte_t json_str[MAX_METADATA_SZ];
    (void) memset(json_str, 0x00U, sizeof(json_str));
    int32_t rval = 0;

    //printf("[ RUN      ] abq_upt_cache_verify_006\n");
    if (REPODIR == repo) {
        rval = abst_LoadCachedRootDirMetadata(json_str, next_version);
    } else {
        rval = abst_LoadCachedRootImgMetadata(json_str, next_version);
    }
    //printf("[ RUN      ] abq_upt_cache_verify_007\n");

    // version is hardcoded in this sample application.    
    if (0 < strlen(json_str)) {
        err = parse_next_root(ctx, repo, json_str, strlen(json_str));
        if (UPTANE_NO_ERROR == err) {
            *new_root_loaded = true;
        }
    }

    return err;
}

static uptaneErr_t load_cached_timestamp(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked

    byte_t json_str[MAX_METADATA_SZ];
    (void) memset(json_str, 0x00U, sizeof(json_str));
    int32_t rval = 0;

    //printf("[ RUN      ] abq_upt_cache_verify_010\n");
    if (REPODIR == repo) {
        rval = abst_LoadLatestTimestampDirMetadata(json_str);
    } else {
        rval = abst_LoadLatestTimestampImgMetadata(json_str);
    }
    //printf("[ RUN      ] abq_upt_cache_verify_011\n");
    if (0 > rval) {
        err = UPTANE_ERR_RESOURCE;
    }
    if (0 < strlen(json_str)) {
        err = metadata_parse_and_verify_sigs(ctx, ctx->metadata[repo].latest_root,
                json_str, (size_t)strlen(json_str),
                (void *)ctx->metadata[repo].cached_timestamp, META_TIMESTAMP);
    }

    return err;
}

static uptaneErr_t load_cached_snapshot(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked

    byte_t json_str[MAX_METADATA_SZ];
    (void) memset(json_str, 0x00U, sizeof(json_str));
    int32_t rval = 0;

    //printf("[ RUN      ] abq_upt_cache_verify_014\n");
    if (REPODIR == repo) {
        rval = abst_LoadLatestSnapshotDirMetadata(json_str);
    } else {
        rval = abst_LoadLatestSnapshotImgMetadata(json_str);
    }
    //printf("[ RUN      ] abq_upt_cache_verify_015\n");
    if (0 > rval) {
        err = UPTANE_ERR_RESOURCE;
    }
    if (0 < strlen(json_str)) {
        err = metadata_parse_and_verify_sigs(ctx, ctx->metadata[repo].latest_root,
                json_str, (size_t)strlen(json_str),
                (void *)ctx->metadata[repo].cached_snapshot, META_SNAPSHOT);
    }

    return err;
}

static uptaneErr_t load_cached_targets(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked

    byte_t json_str[MAX_METADATA_SZ];
    (void) memset(json_str, 0x00U, sizeof(json_str));
    int32_t rval = 0;

    //printf("[ RUN      ] abq_upt_cache_verify_018\n");
    if (REPODIR == repo) {
        rval = abst_LoadLatestTargetsDirMetadata(json_str);
    } else {
        rval = abst_LoadLatestTargetsImgMetadata(json_str);
    }
    //printf("[ RUN      ] abq_upt_cache_verify_019\n");

    if (0 > rval) {
        err = UPTANE_ERR_RESOURCE;
    }
    if (0 < strlen(json_str)) {
        err = metadata_parse_and_verify_sigs(ctx, ctx->metadata[repo].latest_root,
                json_str, (size_t)strlen(json_str),
                (void *)ctx->metadata[repo].cached_targets, META_TARGETS);
    }

    return err;
}

static uptaneErr_t load_cached_augmented(upt_context_t *ctx) {
    uptaneErr_t err = UPTANE_NO_ERROR;

    // internal call, assume parameters already checked

    byte_t json_str[MAX_METADATA_SZ];
    (void) memset(json_str, 0x00U, sizeof(json_str));
    int32_t rval = 0;

    rval = abst_LoadLatestAugmentedMetadata(json_str);

    if (0 > rval) {
        // It's OK if there is not cached director augmented metadata. Just skip to verify cached augmented metadata.
        printf("INFO:No cached director augmented metadata.\n");
    } else {
        if (0 < strlen(json_str)) {
            //printf("[ RUN      ] abq_upt_cache_verify_022\n");
            err = metadata_parse_and_verify_sigs(ctx, ctx->metadata[REPODIR].latest_root,
                                                 json_str, (size_t) strlen(json_str),
                                                 (void *) ctx->metadata[REPODIR].cached_aug, META_AUGMENTED);
            //printf("[ RUN      ] abq_upt_cache_verify_023\n");
        }
    }

    return err;
}

static uptaneErr_t prepare_trust_root(metadata_set_t *metaset, byte_t *buf, size_t buflen) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked

    rootMeta_t trust_root = {0};
    trust_root.version = metaset->previous_root->version;
    trust_root.type = metaset->previous_root->type;
    trust_root.expires = metaset->previous_root->expires; // "expires" not really important to trust root, but anyway

    // discard roles and keys other than root role
    for (uint8_t i = 0U; i < metaset->previous_root->rootMetaBody.rolesNum; i++) {
        if (RT_ROOT == metaset->previous_root->rootMetaBody.roles[i].role) {
            memcpy(&trust_root.rootMetaBody.roles[0],
                   &metaset->previous_root->rootMetaBody.roles[i],
                   sizeof(roleInfo_t));
            trust_root.rootMetaBody.rolesNum = 1U;
            break;
        }
    }

    for (uint8_t ik = 0U; ik < metaset->previous_root->rootMetaBody.keysNum; ik++) {
        for (uint8_t iid = 0U; iid < trust_root.rootMetaBody.roles[0].idsNum; iid++) {
            if (0 == memcmp(trust_root.rootMetaBody.roles[0].ids[iid],
                            metaset->previous_root->rootMetaBody.keys[ik].id,
                            SHA256_LENGTH)) {
                // This key belongs to root role
                memcpy(&trust_root.rootMetaBody.keys[trust_root.rootMetaBody.keysNum],
                       &metaset->previous_root->rootMetaBody.keys[ik],
                       sizeof(keyInfo_t));
                trust_root.rootMetaBody.keysNum++;
                break;
            }
        }
    }

    if (UTIL_NO_ERROR != util_CreateFromTrustRootMeta(&trust_root, buf, buflen)) {
        err = UPTANE_ERR_RESOURCE;
    }
    return err;
}

#ifdef __cplusplus
}
#endif
