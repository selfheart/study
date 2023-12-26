/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifndef SDPV3_CLIENT_LIB_H
#define SDPV3_CLIENT_LIB_H

#define PHASE_DOWNLOADED 1
#define PHASE_COMPLETED 2
#define PHASE_ABORTED 3

int sdpv3_client_init(const char *rootca, size_t rootca_len);
int sdpv3_check_available_campaigns(t1_ctx_t *t1_ctx,
                                    const sdpv3_cfg_t *sdpv3_cfg, void **upt_ctx,
                                    util_json_t *mem, uint16_t qty);
int sdpv3_download_tup(t1_ctx_t *t1_ctx,
                        const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx);
int sdpv3_notify_update_result(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg,
                                void *upt_ctx, int phase);

#endif
