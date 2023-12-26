/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>

#include "abst.h"
#include "uptane/uptane.h"
#include "upt_internal.h"

#include "t1_layer.h"


sdpv3_error_t run_store_trust_root(const void *upt_ctx)
{
    utilErr_t util_err = UTIL_NO_ERROR;
    byte_t dir_out[4<<10] = {};
    byte_t img_out[4<<10] = {};

    upt_context_t *ctx = (upt_context_t *)upt_ctx;
    metadata_set_t *dir_m = &ctx->metadata[REPODIR];
    metadata_set_t *img_m = &ctx->metadata[REPOIMG];

    util_err = util_CreateFromTrustRootMeta(dir_m->previous_root,
                                            dir_out, sizeof(dir_out));
    if (util_err != UTIL_NO_ERROR) {
        printf("util_CreateFromTrustRootMeta() failed: %s:%d\n",
                __func__, __LINE__);
        return (util_err == UTIL_ERR_INVALID_PARAMETER)
                ? SDPV3_ERROR_INVALID_ARGUMENT : SDPV3_ERROR_UNKNOWN;
    }

    util_err = util_CreateFromTrustRootMeta(img_m->previous_root,
                                            img_out, sizeof(img_out));
    if (util_err != UTIL_NO_ERROR) {
        printf("util_CreateFromTrustRootMeta() failed: %s:%d\n",
                __func__, __LINE__);
        return (util_err == UTIL_ERR_INVALID_PARAMETER)
                ? SDPV3_ERROR_INVALID_ARGUMENT : SDPV3_ERROR_UNKNOWN;
    }

    // Store latest Root Director & Image Metadata as trust metadata.
    if (abst_SaveTrustRootDirMetadata(dir_out) != 0) {
        printf("abst_SaveTrustRootDirMetadata() failed: %s:%d\n",
                __func__, __LINE__);
        return SDPV3_ERROR_UNKNOWN;
    }
    if (abst_SaveTrustRootImgMetadata(img_out) != 0) {
        printf("abst_SaveTrustRootImgMetadata() failed: %s:%d\n",
                __func__, __LINE__);
        return SDPV3_ERROR_UNKNOWN;
    }

    return SDPV3_ERROR_NONE;
}
