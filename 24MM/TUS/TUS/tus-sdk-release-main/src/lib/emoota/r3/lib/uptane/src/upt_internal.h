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

// This file defines Uptane internal used stuff

#ifndef SDK_UPTANE_INTERNAL_H
#define SDK_UPTANE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "uptane/uptane.h"

#include "emoota_utility.h"
#include "abst.h"
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if BUILD_TYPE == DEBUG
#include "stdio.h"
#define __UPT_LOG(...) printf(__VA_ARGS__)
#else
#define __UPT_LOG(...)
#endif

typedef enum {
    REPODIR = 0,
    REPOIMG,
    NUM_REPO
} repo_index_t;

typedef enum {
    META_ROOT = 0,
    META_TIMESTAMP,
    META_SNAPSHOT,
    META_TARGETS,
    META_AUGMENTED,
    META_GET_OTA_CMD,
    MAX_META_NUM
} metaType_t;

typedef struct {
    /*
     * latest root loaded (version M) to verify version M+1 root during root metadata loading
     * and to verify non-root metadata files when there is no newer root
     */
    rootMeta_t *latest_root;
    /*
     * keep a record of initial trusted root version for comparison later,
     * to determing if trusted root should be updated or not
     */
    uint32_t trusted_root_ver;
    /*
     * keep a record of previous root (version M-1)
     * it is used to update the trusted root later if needed
     */
    rootMeta_t *previous_root;
    /*
     * keep a copy of starting root (version trusted_root_ver+1)
     * This is to check if if timestamp/snapshot key rotated, after root up-to-date
     */
    rootMeta_t starting_root;

    timeMeta_t *cached_timestamp;
    snapshotMeta_t *cached_snapshot;
    targetsMeta_t *cached_targets;
    timeMeta_t *dld_timestamp;
    snapshotMeta_t *dld_snapshot;
    targetsMeta_t *dld_targets;

    // Only used for director repo
    augMeta_t *cached_aug;
    augMeta_t *dld_aug;

    bool_t prev_timestamp_snapshot_del;

    // Keep a copy of original downloaded metadata files, before committing it.
    // memory consumption is not a concern, as a reference app.
    byte_t json_timestamp[MAX_METADATA_SZ];
    byte_t json_snapshot[MAX_METADATA_SZ];
    byte_t json_targets[MAX_METADATA_SZ];
    byte_t json_aug[MAX_METADATA_SZ];
    byte_t json_root[MAX_METADATA_SZ];
} metadata_set_t;

#define UPT_OTA_STATUS(ctx) (ctx->metadata[REPODIR].dld_aug->augMetaBody.status)
#define UPT_TARGET_PENDING(ctx) ('\0' != ctx->metadata[REPODIR].dld_targets->targetsMetaBody.targets[0].target.fileName[0])  // at least one image pending

typedef struct {
    metadata_set_t metadata[NUM_REPO];

    // device information used in various Uptane scenarios. So keep a record here.
    byte_t primarySerialNum[MAX_SERIAL_NUM_SZ];

    // handle to the memory T1 allocated for Utility to use
    util_json_t *json_mem;
    uint16_t json_qty;
} upt_context_t;

#include "metadata.h"
#include "manifest.h"

#ifdef __cplusplus
}
#endif

#endif /* SDK_UPTANE_INTERNAL_H */
