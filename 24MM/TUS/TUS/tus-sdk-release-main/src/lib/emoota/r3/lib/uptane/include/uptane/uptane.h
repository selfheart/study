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

#ifndef SDK_UPTANE_H_
#define SDK_UPTANE_H_

#include "emoota_utility_types.h"

typedef enum {
    UPTANE_NO_ERROR = 0,  ///< no error
    UPTANE_CANCELED, ///< config match skipped
    UPTANE_ERR_RESOURCE, ///< memory, file, etc.
    UPTANE_ERR_NETWORK, ///< for failure to talk to backend
    UPTANE_ERR_INVALID_METADATA, ///< Invalid JSON format, or simply too long, or valid JSON but something wrong with the content
    UPTANE_ERR_INVALID_SIG, ///< Invalid signature detected
    UPTANE_ERR_NOT_ENOUGH_SIGS, ///< Not enough signatures to verify a metadata
    UPTANE_ERR_FREEZE_ATTACK, ///< freeze attack
    UPTANE_ERR_ROLLBACK_ATTACK, ///< rollback attack
    UPTANE_ERR_MIX_AND_MATCH_ATTACK, ///< mixn-and-match attack
    UPTANE_ERR_INVALID_IMAGE, ///< image verification failure
    UPTANE_ERR_IMAGE_DECRYPT, ///< image decryption failure
    UPTANE_MAX_ERR_NUM
} uptaneErr_t;

#define MAX_ECU_INFO_SIZE_IN_REQ (1024U + sizeof(ecuInfo_t))
#define MAX_CFG_MATCH_REQ_LEN ((1024U * 20U) + (MAX_ECU_INFO_SIZE_IN_REQ * MAX_TOTAL_ECUS)) // 20kB probably big enough for most time, but it will be big when ECU number gets big
#define MAX_CFG_MATCH_RESP_LEN (MAX_METADATA_SZ * 4U + 256U) // timestamp/snapshot/targets/augmented + some overhead

/**
 * Initialize Uptane context
 * @param uptane_ctx [out] Uptane context allocated by Uptane library
 * @param primary_serial [in] primary serial
 * @param mem [in] memory allocated by T1 for JSON parsing to use. Will be passed through to Utility
 * @param qty [in] quantity of util_json_t in mem. Will be passed through to Utility
 * @return UPTANE_NO_ERROR, or UPTANE_ERR_RESOURCE
 */
uptaneErr_t uptane_start(void **uptane_ctx, cstr_t primary_serial, util_json_t *mem, uint16_t qty);

/**
 * Cleanup Uptane context
 * @param uptane_ctx [in] Uptane context allocated by uptane_start
 */
void uptane_cleanup(void *uptane_ctx);

/**
 * Load cached metadata for director repository
 * @param uptane_ctx [in] Uptane context allocated by uptane_start
 * @return UPTANE_NO_ERROR,
 *         UPTANE_ERR_RESOURCE, UPTANE_ERR_INVALID_JSON, UPTANE_ERR_INVALID_METADATA, UPTANE_ERR_INVALID_SIG, or UPTANE_ERR_NOT_ENOUGH_SIGS
 */
uptaneErr_t load_director_repo_cached_meta(void *ctx);
/**
 * Load cached metadata for image repository
 * @param uptane_ctx [in] Uptane context allocated by uptane_start
 * @return UPTANE_NO_ERROR,
 *         UPTANE_ERR_RESOURCE, UPTANE_ERR_INVALID_JSON, UPTANE_ERR_INVALID_METADATA, UPTANE_ERR_INVALID_SIG, or UPTANE_ERR_NOT_ENOUGH_SIGS
 */
uptaneErr_t load_image_repo_cached_meta(void *ctx);

/**
 * Create Uptane style signature list in JSON format
 * @param input [in] string to be the 'signed' part of the request, NUL terminated
 * @param output [out] buffer for the generated signature block. The resulting signature block starts with '[' and ends with ']' then followed by terminating NUL
 * @param outbuf_len [in] size of the output buffer
 * @return UPTANE_NO_ERROR, or UPTANE_ERR_RESOURCE
 */
uptaneErr_t uptane_create_signature_block(cstr_t input, byte_t *output, size_t outbuf_len);

/**
 * Callback function for Uptane library to determine if sync check should be done before full config
 * match, or if the config match should be skipped at all.
 * @param skip_config_match [output] config match should be skipped
 * @param sync_first [output] should do sync check first. Only meaningful if *skip_config_match was false
 * @param cfgMatchInfo [input] Config match request prepared to be sent.
 *              This callback should make the decision based on information in this structure, along with previous sent digest
 */
typedef void (*config_match_seq_cb_t)(bool_t *skip_config_match, bool_t *sync_first, const cfgMatch_t *cfgMatchInfo);

/**
 * Do config match
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param result [out] result of config match. Only valid when this function returns UPTANE_NO_ERROR
 * @param upd [in] OTA config match reason, NUL-terminated
 * @param ecusInfo [in] ECU information array
 * @param numEcuInfos [in] number of ECUs in ecusInfo
 * @param lastCmpId [in] last campaign ID, NUL-terminated
 * @param rxswinInfo [in] rxswin, NUL-terminated
 * @param campaign_vin [in] campaign VIN, NUL-terminated
 * @param cfg_match_seq_func [in] caller provided callback to determine config match sequence
 * @param sdp_url [in] SDP URL, NUL-terminated
 * @param sdp_port [in] SDP PORT, NUL-terminated
 * @param cdn_url [in] CDN URL, NUL-terminated
 * @param cdn_port [in] CDN PORT, NUL-terminated
 * @return UPTANE_NO_ERROR, or UPTANE_ERR_RESOURCE
 */
uptaneErr_t uptane_do_config_match(void *uptctx, augMetaBody_t **result, cstr_t rootca,
        const byte_t *upd, const ecuInfo_t *ecusInfo, const uint8_t numEcuInfos,
        const byte_t *lastCmpId, const byte_t *rxswinInfo, cstr_t campaign_vin,
        config_match_seq_cb_t cfg_match_seq_func,
        cstr_t sdp_url, cstr_t sdp_port, cstr_t cdn_url, cstr_t cdn_port);

/**
 * Download image specified by the change_event
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param change_event [in] specifies which image to download
 * @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE, UPTANE_ERR_NERTOWK
 */
uptaneErr_t uptane_download_change_event_metadata(void *uptctx, changeEvent_t *change_event);


/**
 * Download image specified by the change_event
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param change_event [in] specifies which image to download
 * @param ntf_event [in] notification event
 * @param rootca [in] root certificate
 * @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE, UPTANE_ERR_NERTOWK
 */
uptaneErr_t uptane_download_change_event(void *uptctx, changeEvent_t *change_event, event_t *ntf_event, cstr_t rootca);

/**
 * Download Download Metadata specified by the change_event
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param change_event [in] specifies which image to download
 * @param ntf_event [in] notification event
 * @param rootca [in] root certificate
 * @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE, UPTANE_ERR_NERTOWK
 */
uptaneErr_t uptane_download_download_metadata(void *uptctx, changeEvent_t *change_event, event_t *ntf_event, cstr_t rootca);
/**
 * Download Repro Metadata specified by the change_event
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param change_event [in] specifies which image to download
 * @param ntf_event [in] notification event
 * @param rootca [in] root certificate
 * @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE, UPTANE_ERR_NERTOWK
 */
uptaneErr_t uptane_download_repro_metadata(void *uptctx, changeEvent_t *change_event, event_t *ntf_event, cstr_t rootca);

/**
 * Download HMI messages specified by campaign
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param campaign [in] specifies the hmi messages to download
 * @param ntf_event [in] notification event
 * @param rootca [in] root certificate
* @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE, UPTANE_ERR_NERTOWK
 */
uptaneErr_t uptane_download_and_verify_hmi_msg(void *uptctx, campaign_t *campaign);


/**
 * Verify image repro metadata specified by the change_event. Decrypt and verify the decrypted image if the
 * downloaded image is encrypted.
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param change_event [in] specifies which image to download
 * @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE,
 *         UPTANE_ERR_INVALID_IMAGE, UPTANE_ERR_IMAGE_DECRYPT
 */
uptaneErr_t uptane_vrfy_and_decrypt_image_repro_metadata(void *uptctx, changeEvent_t *change_event);

/**
 * Verify image download metadata specified by the change_event. Decrypt and verify the decrypted image if the
 * downloaded image is encrypted.
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param change_event [in] specifies which image to download
 * @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE,
 *         UPTANE_ERR_INVALID_IMAGE, UPTANE_ERR_IMAGE_DECRYPT
 */
uptaneErr_t uptane_vrfy_and_decrypt_image_dl_metadata(void *uptctx, changeEvent_t *change_event);


/**
 * Verify image specified by the change_event. Decrypt and verify the decrypted image if the
 * downloaded image is encrypted.
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param change_event [in] specifies which image to download
 * @return UPTANE_NO_ERROR, UPTANE_ERRRO_INVALID_METADATA, UPTANE_ERR_RESOURCE,
 *         UPTANE_ERR_INVALID_IMAGE, UPTANE_ERR_IMAGE_DECRYPT
 */
uptaneErr_t uptane_vrfy_and_decrypt_image(void *uptctx, changeEvent_t *change_event);

uptaneErr_t uptane_download_hmi_msg(void *uptctx, campaign_t *campaign, event_t *ntf_event, cstr_t rootca);
uptaneErr_t uptane_verify_hmi_msg(void *uptctx, campaign_t *campaign);

/**
 * Commit change of release counter, after successfully install updates
 * @param uptctx [in] Uptane context allocated by uptane_start
 */
void uptane_commit(void *ctx);

/**
 * verify and parse ota command response
 * @param uptctx  [in] Uptane context allocated by uptane_start
 * @param response [in] full response string from SDP
 * @param rsp_size [in] length of full reponse string from SDP
 * @param otaCmdRes [out] allocated structure to hold parsed results
 * @return UPTANE_NO_ERROR, or UPTANE_ERR_RESOURCE
 */
uptaneErr_t check_OtaCmd_response(void *uptctx, byte_t *response, size_t rsp_size, getOtaCmdRes_t *otaCmdRes);

/**
 * Try to download new Director Root Metadata
 * @param uptctx  [in] Uptane context allocated by uptane_start
 * @param cdn_url  [in] CDN URL
 * @param cdn_port  [in] CDN PORT
 * @return UPTANE_NO_ERROR, or UPTANE_ERR_RESOURCE
 */
uptaneErr_t upt_get_dir_root_up_to_data(void *uptctx, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca);

#endif /* SDK_UPTANE_H_ */
