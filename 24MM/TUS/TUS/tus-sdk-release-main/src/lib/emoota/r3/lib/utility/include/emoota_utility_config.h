
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

#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#ifndef MAX_VERSION_SZ
#define MAX_VERSION_SZ (16U)
#endif

#ifndef MAX_UPLOAD_REASON_SZ
#define MAX_UPLOAD_REASON_SZ (16U)
#endif

#ifndef MAX_SW_DETAILS_NUM
#define MAX_SW_DETAILS_NUM (32U)
#endif

#ifndef MAX_EVENT_OEM_ERR_NUM
#define MAX_EVENT_OEM_ERR_NUM (1U)
#endif

#ifndef MAX_EVENT_ERR_NUM
#define MAX_EVENT_ERR_NUM (1U)
#endif

#ifndef MAX_EVENT_DTC_NUM
#define MAX_EVENT_DTC_NUM (3U)
#endif

#ifndef MAX_EVENT_GPS_SYSTEM_NAME_SZ
#define MAX_EVENT_GPS_SYSTEM_NAME_SZ (5U)
#endif

#ifndef MAX_EVENT_FILE_ID_SZ
#define MAX_EVENT_FILE_ID_SZ (1024U)
#endif

#ifndef MAX_EVENT_ERR_DESC_SZ
#define MAX_EVENT_ERR_DESC_SZ (100U)
#endif

#ifndef MAX_BASE64_MD5_SZ
#define MAX_BASE64_MD5_SZ (24U)
#endif

#ifndef MAX_DESC_SZ
#define MAX_DESC_SZ (100U)
#endif

#ifndef MAX_UUID_NUM
#define MAX_UUID_NUM (3U)
#endif

#ifndef MAX_UPLOAD_CONTEXT_SZ
#define MAX_UPLOAD_CONTEXT_SZ (1600U)
#endif

#ifndef MAX_ETAG_SZ
#define MAX_ETAG_SZ (32U)
#endif

#ifndef MAX_PART_ETAG_NUM
#define MAX_PART_ETAG_NUM (2U)
#endif

#ifndef MAX_HASH_NUM
#define MAX_HASH_NUM (2U)
#endif

#ifndef MAX_SNAPSHOT_META_FILES_NUM
#define MAX_SNAPSHOT_META_FILES_NUM (2U)
#endif

#ifndef MAX_TARGET_AND_CUSTOM_NUM
#define MAX_TARGET_AND_CUSTOM_NUM (512U)
#endif

#ifndef MAX_ECRYPTD_SYMKEY_VALUE_SZ
#define MAX_ECRYPTD_SYMKEY_VALUE_SZ (1024U)
#endif

#ifndef MAX_BASE64_IV_SZ
#define MAX_BASE64_IV_SZ (24U)
#endif

/** The maximum number of octets that can be allocated for the
 *  Vehicle Identification Number (VIN)
 */
#ifndef MAX_VIN_SZ
#define MAX_VIN_SZ (17U)
#endif

/** The maximum number of octets that need to be allocated for each
 *  Serial number
 */
#ifndef MAX_SERIAL_NUM_SZ
#define MAX_SERIAL_NUM_SZ (24U)
#endif

/** The maximum number of bytes that need to be allocated for each
 *  ECU part number
 */
#ifndef MAX_PART_NUM_SZ
#define MAX_PART_NUM_SZ (16U)
#endif

/** The maximum number of bytes that need to be allocated
 *  for each ECU's Software Id
 */
#ifndef MAX_ECU_SW_ID_SZ
#define MAX_ECU_SW_ID_SZ (16U)
#endif

/** The maximum number of octets needed to encode
 *  the campaign Id.
 */
#ifndef MAX_CAMPAIGN_ID_SZ
#define MAX_CAMPAIGN_ID_SZ (16U)
#endif

#ifndef MAX_CAMPAIGN_NAME_SZ
#define MAX_CAMPAIGN_NAME_SZ (16U)
#endif

#ifndef MAX_DATA_GROUP_SZ
#define MAX_DATA_GROUP_SZ (16U)
#endif

#ifndef MAX_DWNLD_URL_SZ
#define MAX_DWNLD_URL_SZ (1024U)
#endif

#ifndef MAX_QUERY_STRING_SZ
#define MAX_QUERY_STRING_SZ (128U * 10U)
#endif

#ifndef MAX_UPLD_URL_SZ
#define MAX_UPLD_URL_SZ (2000U)
#endif

#ifndef FORMATED_TIME_SZ
#define FORMATED_TIME_SZ (20U)
#endif

#ifndef MAX_UPLOAD_PARTS_REQ_NUM
#define MAX_UPLOAD_PARTS_REQ_NUM (2U)
#endif

#ifndef MAX_UPLOAD_PARTS_RES_NUM
#define MAX_UPLOAD_PARTS_RES_NUM (2U)
#endif

#ifndef MAX_CLIENT_UPLOAD_ID_SZ
#define MAX_CLIENT_UPLOAD_ID_SZ (32U)
#endif

#ifndef MAX_REPORT_ID_SZ
#define MAX_REPORT_ID_SZ (32U)
#endif

#ifndef MAX_FILE_NAME_SZ
#define MAX_FILE_NAME_SZ (128U)
#endif

#ifndef MAX_PRECONDITION_NUM
#define MAX_PRECONDITION_NUM (16U)
#endif

#ifndef MAX_PRECONDITION_STR_SZ
#define MAX_PRECONDITION_STR_SZ (16U)
#endif

#ifndef MAX_BASE64_ENCRYPTED_DEK_SZ
#define MAX_BASE64_ENCRYPTED_DEK_SZ (700U)
#endif

#ifndef MAX_DEVICE_ID_SZ
#define MAX_DEVICE_ID_SZ (32U)
#endif

#ifndef MAX_CLIENT_TYPE_SZ
#define MAX_CLIENT_TYPE_SZ (32U)
#endif

/**
 * The maximum number of campaigns which can be tracked by the SDK, a value provided to an API function or structure for
 * num_campaign_infos, or num_campaigns with value larger then this definition are considered invalid
 */
#ifndef MAX_CAMPAIGNS
#define MAX_CAMPAIGNS (5U)
#endif

/**
 * The maximum number of change events contained within a single campaign */
#ifndef MAX_CHANGE_EVENTS
#define MAX_CHANGE_EVENTS (16U)
#endif

/** The maximum total number of ECUs which are maintained
 *  and/or reported by reprogramming master.
 */
#ifndef MAX_TOTAL_ECUS
#define MAX_TOTAL_ECUS (16U)
#endif

/** The maximum number of notifications to be included in ntf_t */
#ifndef MAX_NOTIFICATIONS
#define MAX_NOTIFICATIONS (5U)
#endif

#ifndef MAX_TARGET_ID_SZ
#define MAX_TARGET_ID_SZ (32U)
#endif

#ifndef MAX_SUB_TARGET_ID_SZ
#define MAX_SUB_TARGET_ID_SZ (5U)
#endif

#ifndef MAX_RXSWIN_SZ
#define MAX_RXSWIN_SZ (32U)
#endif

#ifndef MAX_TOTAL_LICENSE_INFO
#define MAX_TOTAL_LICENSE_INFO (3U)
#endif

#ifndef MAX_LICENSE_ID_SZ
#define MAX_LICENSE_ID_SZ (32U)
#endif

#ifndef MAX_MOBILE_NUMBER_SZ
#define MAX_MOBILE_NUMBER_SZ (16U)
#endif

#ifndef MAX_ATTACK_SZ
#define MAX_ATTACK_SZ (16U)
#endif

#ifndef SHA256_LENGTH
#define SHA256_LENGTH (32U)
#endif

#ifndef MAX_KEY_LENGTH
#define MAX_KEY_LENGTH (1024U)
#endif

#ifndef MAX_SIG_LENGTH
#define MAX_SIG_LENGTH (512U)
#endif

#ifndef UUID_LEN
#define UUID_LEN (37U)
#endif

#ifndef MAX_ATTEMPT_ID_SZ
#define MAX_ATTEMPT_ID_SZ (UUID_LEN)
#endif

#ifndef MAX_METADATA_SZ
#define MAX_METADATA_SZ (20 * 1024)
#endif

#ifndef MAX_KEY_NUM_PER_ROLE
#define MAX_KEY_NUM_PER_ROLE (3U)
#endif


#endif                          /* SDK_CONFIG_H */
