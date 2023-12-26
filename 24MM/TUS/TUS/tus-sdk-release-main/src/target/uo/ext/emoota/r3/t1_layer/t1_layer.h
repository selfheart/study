/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifndef _T1_LAYER_H_
#define _T1_LAYER_H_

#include <sys/param.h>

#include "emoota_utility.h"

#define T1_BUF_SZ (16<<10)


typedef struct {
    char sdp_url[MAX_DWNLD_URL_SZ];
    char sdp_port[8]; // ":NNNNN"
    char cdn_url[MAX_DWNLD_URL_SZ];
    char cdn_port[8]; // ":NNNNN"
    char primary_serial[MAX_SERIAL_NUM_SZ];
    char campaign_vin[MAX_VIN_SZ + 1]; // store 17 chars
    const char *rootca;
    ecuInfo_t *ecusInfo;
    uint8_t numEcusInfo;
    byte_t rxswins[MAX_RXSWIN_SZ];
    eventStatus_t nty_err_code;
} sdpv3_cfg_t;

typedef struct {
    augmentedManifest_t augmentedManifest;
    augMetaBody_t *cfg_match_result;
    campaign_t *selected_campaign;
    changeEvent_t *ce;
    uint64_t lastUid;
    uint64_t cmdId;
    getOtaCmdType_t otaCmdType;
    mpuInitResponse_t* mpuInitResponse;
    ntf_t ntf;
    byte_t* requestBuffer;
    size_t requestBuffer_sz;
    byte_t* responseBuffer;
    size_t responseBuffer_sz;
    char lastCmpId[MAX_CAMPAIGN_ID_SZ];
    str_t rxswinInfo;
    str_t upd;
    byte_t dld_pkg_path[MAXPATHLEN];
} t1_ctx_t;

typedef enum {
    SDPV3_ERROR_NONE = 0,
    SDPV3_ERROR_UNKNOWN,
    SDPV3_ERROR_NO_MEMORY,
    SDPV3_ERROR_INVALID_ARGUMENT,
    SDPV3_ERROR_TRANSACTION,
} sdpv3_error_t;


#define EVENT_MEM_NOT_CLEAR 0
#define EVENT_MEM_CLEAR 1
sdpv3_error_t create_base_event(event_t *ntf_event, int mem_clear,
                                byte_t *campaign_id, uint16_t *num_events,
                                unionFlag_t union_flag, eventMode_t event_mode,
                                eventType_t event_type, eventStatus_t status);
sdpv3_error_t send_event(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg);
void config_match_seq_cb(bool_t *skip_config_match,
                            bool_t *sync_first, const cfgMatch_t *cfgMatchInfo);

sdpv3_error_t
run_config_match(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx);
sdpv3_error_t run_config_match_with_updated_config(t1_ctx_t *t1_ctx,
                                                   const sdpv3_cfg_t *sdpv3_cfg,
                                                   void *upt_ctx);
sdpv3_error_t
run_get_cmd(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx);
sdpv3_error_t
run_download(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx);
sdpv3_error_t notify_pkg_download_completion(t1_ctx_t *t1_ctx,
                                            const sdpv3_cfg_t *sdpv3_cfg,
                                            void *upt_ctx);
sdpv3_error_t run_activate(t1_ctx_t *t1_ctx, const sdpv3_cfg_t *sdpv3_cfg);
sdpv3_error_t run_sw_update_comp(t1_ctx_t *t1_ctx,
                                const sdpv3_cfg_t *sdpv3_cfg, void *upt_ctx);
sdpv3_error_t run_store_trust_root(const void *upt_ctx);

#endif  // _T1_LAYER_H_
