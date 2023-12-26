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

// functions for metadata parsing/verification

#ifndef SDK_UPTANE_METADATA_H
#define SDK_UPTANE_METADATA_H
#ifdef __cplusplus
extern "C" {
#endif

#define UPT_SHA256_EQUAL(hash1, hash2) (0 == memcmp(hash1, hash2, SHA256_LENGTH))
#define UPT_KEYID_EQUAL UPT_SHA256_EQUAL

uptaneErr_t load_trusted_root_dir(upt_context_t *ctx);

/**
 * Parse metadata file and verify the signatures
 */
uptaneErr_t metadata_parse_and_verify_sigs(upt_context_t *ctx, rootMeta_t *root,
        byte_t *json_str, size_t json_len, void *parsed_signed, metaType_t meta_type);

/**
 * Parse signed part of metadata for different types. Also returns the type specified in the metadata, for finding the key to verify signature.
 */
uptaneErr_t metadata_parse_signed(upt_context_t *ctx, byte_t *json_str, int32_t json_len,
        void *parsed_signed, metaType_t meta_type, roleType_t *role_type);

roleInfo_t *get_role_from_root(rootMeta_t *root, roleType_t roletype);

uptaneErr_t parse_next_root(upt_context_t *ctx, repo_index_t repo,
        byte_t *json_str, size_t json_len);

uptaneErr_t load_cached_metadata(upt_context_t *ctx, repo_index_t repo);

uptaneErr_t parse_cfg_match_resp(upt_context_t *ctx, cstr_t response);

uptaneErr_t upt_get_root_up_to_data(upt_context_t *ctx, repo_index_t repo, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca);

uptaneErr_t dld_and_verify_image_repo_meta(upt_context_t *ctx, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca);

uptaneErr_t upt_verify_root_metadata(upt_context_t *ctx, repo_index_t repo);
uptaneErr_t upt_verify_augmented_metadata(upt_context_t *ctx);
uptaneErr_t upt_verify_timestamp_metadata(upt_context_t *ctx, repo_index_t repo);
uptaneErr_t check_snapshot_digest(upt_context_t *ctx, repo_index_t repo,
        cstr_t snapshot_str, size_t snapshot_len);
uptaneErr_t upt_verify_snapshot_metadata(upt_context_t *ctx, repo_index_t repo);
uptaneErr_t upt_verify_targets_metadata(upt_context_t *ctx, repo_index_t repo);

uptaneErr_t get_data_encryption_key(upt_context_t *ctx, cstr_t ecryptdSymKeyValue, byte_t *dek, size_t buflen, size_t *deklen);

uptaneErr_t save_dir_cached_metadata(upt_context_t *ctx);
uptaneErr_t save_img_cached_metadata(upt_context_t *ctx);
#ifdef __cplusplus
}
#endif

#endif
