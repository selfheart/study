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

// Based on Uptane spec 1.0.0: https://uptane.github.io/papers/ieee-isto-6100.1.0.0.uptane-standard.html

static uptaneErr_t check_freeze_attack(uint64_t expires);
static bool_t key_rotated(metadata_set_t *metaset, roleType_t role_type);

uptaneErr_t upt_verify_root_metadata(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked
    metadata_set_t *metaset = &ctx->metadata[repo];
    rootMeta_t *root = metaset->latest_root;

    // Uptane-1.0.0 5.4.4.3 => 3
    err = check_freeze_attack(root->expires);

    // Uptane-1.0.0 5.4.4.3 => 4;
    if (UPTANE_NO_ERROR == err) {
        if (key_rotated(metaset, RT_TIMESTAMP) || key_rotated(metaset, RT_SNAPSHOT)) {
            // Mark them as "soft deleted", instead of removing the file physically
            metaset->prev_timestamp_snapshot_del = true;
        }
    }
    return err;
}

uptaneErr_t upt_verify_augmented_metadata(upt_context_t *ctx) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked
    metadata_set_t *metaset = &ctx->metadata[REPODIR];

    if (metaset->cached_aug->version > metaset->dld_aug->version) {
        err = UPTANE_ERR_ROLLBACK_ATTACK;
    }
    if (UPTANE_NO_ERROR == err) {
        err = check_freeze_attack(metaset->dld_aug->expires);
    }
    return err;
}

uptaneErr_t upt_verify_timestamp_metadata(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked
    metadata_set_t *metaset = &ctx->metadata[repo];

    // Uptane-1.0.0 5.4.4.4 => 3
    if (!metaset->prev_timestamp_snapshot_del) {
        if (metaset->cached_timestamp->version > metaset->dld_timestamp->version) {
            err = UPTANE_ERR_ROLLBACK_ATTACK;
        }
    }
    if (UPTANE_NO_ERROR == err) {
        // Uptane-1.0.0 5.4.4.4 => 4;
        err = check_freeze_attack(metaset->dld_timestamp->expires);
    }
    return err;
}

uptaneErr_t check_snapshot_digest(upt_context_t *ctx, repo_index_t repo,
        cstr_t snapshot_str, size_t snapshot_len) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked
    metadata_set_t *metaset = &ctx->metadata[repo];

    byte_t sha256[SHA256_LENGTH] = {0};
    // Calculate digest of all hash functions used in the signatures. Currently only SHA256 is defined.
    abst_CalculateDigestSHA256((unsigned char *)sha256, snapshot_str, snapshot_len);    

    // Since only SHA256 supported, so there should be only one hash
    // Uptane-1.0.0 5.4.4.5 => 2 (1/2)
    if (!UPT_SHA256_EQUAL(sha256, metaset->dld_timestamp->timeMetaBody.hashes[0].digest)) {
        err = UPTANE_ERR_MIX_AND_MATCH_ATTACK;
    }

    return err;
}

uptaneErr_t upt_verify_snapshot_metadata(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked
    metadata_set_t *metaset = &ctx->metadata[repo];

    // Uptane-1.0.0 5.4.4.5 => 2 (2/2);
    if (metaset->dld_timestamp->timeMetaBody.version != metaset->dld_snapshot->version) {
        err = UPTANE_ERR_MIX_AND_MATCH_ATTACK;
    }

    if (!metaset->prev_timestamp_snapshot_del) {
        // Uptane-1.0.0 5.4.4.5 => 4
        if (metaset->cached_snapshot->version > metaset->dld_snapshot->version) {
            err = UPTANE_ERR_ROLLBACK_ATTACK;
        }

        // Uptane-1.0.0 5.4.4.5 => 5
        if (UPTANE_NO_ERROR == err) {
            // Assumption: only one targets metadata
            if (metaset->cached_snapshot->snapshotMetaBody.snapshotMetaFiles[0].version >
                    metaset->dld_snapshot->snapshotMetaBody.snapshotMetaFiles[0].version) {
                err = UPTANE_ERR_ROLLBACK_ATTACK;
            }
        }

        // Uptane-1.0.0 5.4.4.5 => 6
        if (UPTANE_NO_ERROR == err) {
            // Assumption: only one targets metadata, so the file names must match
            if (0 != strcmp(metaset->cached_snapshot->snapshotMetaBody.snapshotMetaFiles[0].fileName,
                    metaset->dld_snapshot->snapshotMetaBody.snapshotMetaFiles[0].fileName)) {
                err = UPTANE_ERR_ROLLBACK_ATTACK;
            }
        }
    }
    if (UPTANE_NO_ERROR == err) {
        // Uptane-1.0.0 5.4.4.5 => 7;
        err = check_freeze_attack(metaset->dld_snapshot->expires);
    }
    return err;
}

uptaneErr_t upt_verify_targets_metadata(upt_context_t *ctx, repo_index_t repo) {
    uptaneErr_t err = UPTANE_NO_ERROR;
    // internal call, assume parameters already checked
    metadata_set_t *metaset = &ctx->metadata[repo];

    // Uptane-1.0.0 5.4.4.6 => 2
    // Assumpion: only one targets metadata
    if (metaset->dld_snapshot->snapshotMetaBody.snapshotMetaFiles[0].version !=
            metaset->dld_targets->version) {
        err = UPTANE_ERR_MIX_AND_MATCH_ATTACK;
    }

    // Uptane-1.0.0 5.4.4.6 => 4
    if (metaset->cached_targets->version > metaset->dld_targets->version) {
        err = UPTANE_ERR_ROLLBACK_ATTACK;
    }

    if (UPTANE_NO_ERROR == err) {
        // Uptane-1.0.0 5.4.4.6 => 5
        err = check_freeze_attack(metaset->dld_targets->expires);
    }
    // Uptane-1.0.0 5.4.4.6 => 6 (Not apply)
    // Uptane-1.0.0 5.4.4.6 => 7; (Not apply)
    return err;
}

static uptaneErr_t check_freeze_attack(uint64_t expires) {
    uint64_t time_ms = 0UL;
    uptaneErr_t err = UPTANE_ERR_RESOURCE;

    if (ABST_NO_ERROR == abst_get_trusted_time(&time_ms)) {
        if ((time_ms / 1000UL) >= expires) {
            err = UPTANE_ERR_FREEZE_ATTACK;
        } else {
            err = UPTANE_NO_ERROR;
        }
    }
    return err;
}

static bool_t key_rotated(metadata_set_t *metaset, roleType_t role_type) {
    roleInfo_t *role = get_role_from_root(metaset->latest_root, role_type);
    roleInfo_t *prev_role = get_role_from_root(&metaset->starting_root, role_type);

    // Other code should have already make sure the role exists
    byte_t zero[SHA256_LENGTH] = {0};
    bool_t new_key = false;
    for (int32_t ir = 0; (ir < MAX_KEY_NUM_PER_ROLE) && !UPT_KEYID_EQUAL(role->ids[ir], zero); ir++) {
        for (int32_t ipr = 0; (ipr < MAX_KEY_NUM_PER_ROLE) && !UPT_KEYID_EQUAL(prev_role->ids[ipr], zero); ipr++) {
            if (UPT_KEYID_EQUAL(role->ids[ir], prev_role->ids[ipr])) {
                new_key = true;
                break;
            }
        }
        if (new_key) {
            break;
        }
    }
    /*
     * Note: it is never clear that if only revoking some old keys considered as "keys rotated"
     * However, this check is only for recovery from a "fast-forward" attack. So it should be OK
     * to require the server to be more explicit about "keys rotated", which means we do require
     * to see some new keys used
     */
    return new_key;
}

#ifdef __cplusplus
}
#endif
