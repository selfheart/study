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

#ifndef SDK_UPTANE_MANIFEST_H
#define SDK_UPTANE_MANIFEST_H

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Prepare config match request data structure and sync check request data structure
 * @param uptctx [in] Uptane context allocated by uptane_start
 * @param cfgMatchInfo [out] buffer to config match request data structure
 * @param syncCheckInfo [out] buffer to sync check request data structure
 * @param upd [in] OTA config match reason, NUL-terminated
 * @param ecusInfo [in] ECU information array
 * @param numEcuInfos [in] number of ECUs in ecusInfo
 * @param lastCmpId [in] last campaign ID, NUL-terminated
 * @param rxswinInfo [in] rxswin, NUL-terminated
 * @param campaign_vin [in] campaign VIN, NUL-terminated
 * @return UPTANE_NO_ERROR, or UPTANE_ERR_RESOURCE
 */
uptaneErr_t uptane_create_cfg_match_req(void *uptctx,
        cfgMatch_t *cfgMatchInfo, syncCheck_t *syncCheckInfo,
        const byte_t *upd, const ecuInfo_t *ecusInfo, const uint8_t numEcuInfos,
        const byte_t *lastCmpId, const byte_t *rxswinInfo, cstr_t campaign_vin);

#ifdef __cplusplus
}
#endif

#endif /* SDK_UPTANE_MANIFEST_H */
