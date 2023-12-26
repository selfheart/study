
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

#ifndef SDK_TYPES_H
#define SDK_TYPES_H

#if (__STDC_VERSION__ >= 199901L) || (__cplusplus >= 201103L)

/* C99 or C++11 */
// We are supporting standard booleans.
#include <stdbool.h>
// We want to use the standard int types everywhere.
#include <stdint.h>
// We want to use standard definitions everywhere.
#include <stddef.h>
// We want to use the standard floating point types everywhere.
#include <float.h>
#include "tiny-json.h"

#else
#ifndef __cplusplus
// We are supporting standard booleans.
typedef enum { false = 0, true = 1 } bool;
#else
// We are supporting standard booleans.
#include <stdbool.h>
#endif                  /* __cplusplus */
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef float float32_t;
typedef double float64_t;
typedef long double float128_t;

typedef unsigned int size_t;
#endif                      /* else of (__STDC_VERSION__ >= 199901L) || (__cplusplus >= 201103L) */

#include "emoota_utility_config.h"

typedef json_t util_json_t;
typedef char byte_t;
// const byte type-definition
typedef const byte_t cbyte_t;
// string type-definition, it should be const, but then we can't cast it to a var_t
typedef byte_t *str_t;
// a const byte_t pointer for when we don't need to cast to var_t
typedef cbyte_t *cstr_t;
// number type-definition
typedef double number_t;
typedef number_t *number_ptr;
typedef const double cnumber_t;
typedef void* var_t;
typedef const void* cvar_t;
typedef bool bool_t;

#define MAX_TOTAL_KEY_NUM (MAX_KEY_NUM_PER_ROLE * (uint8_t) NUM_ROLES)
#define UINT_ZERO (0U)
#define UINT_ONE (1U)
#define UINT_TWO (2U)
#define UINT_THREE (3U)
#define UINT_FOUR (4U)
#define UINT_SIX (6U)
#define UINT_0xFFU (0xFFU)
#define SINT_MINUS_ONE (-1)
#define SINT_ZERO (0)
#define SINT_ONE (1)
#define SINT_TWO (2)
#define SINT_THREE (3)
#define SINT_FOUR (4)
#define SINT_SIX (6)
#define INT_0xFF (0xFF)

/** See Supported Public Key Types defined at Server Light Client API.*/
typedef enum {
    KT_RSA,
    KT_ECDSA,
    KT_UNKNOWN
} keyType_t;

/** See Role Types defined at Server Light Client API.*/
typedef enum {
    RT_ROOT = 0,
    RT_TARGETS = 1,
    RT_SNAPSHOT = 2,
    RT_TIMESTAMP = 3,
    NUM_ROLES = 4,
    RT_UNKNOWN
} roleType_t;

/** See Supported Hash Functions defined at Server Light Client API.*/
typedef enum {
    FT_UNKNOWN,
    FT_SHA256
} functionType_t;

/** See Supported Signature Methods defined at Server Light Client API.*/
typedef enum {
    MT_SHA256withRSA,
    MT_NONEwithRSA,
    MT_SHA256withECDSA,
    MT_NONEwithECDSA,
    MT_UNKNOWN
} methodType_t;

typedef struct {
    /** See publicKeyId property of PublicKey defined at Server Light Client API.*/
    uint8_t id[SHA256_LENGTH];
    /** defines the length of value */
    uint64_t value_len;
    /** See publicKeyValue property of PublicKey defined at Server Light Client API.*/
    uint8_t value[MAX_KEY_LENGTH];
    /** See publicKeyType property of PublicKey defined at Server Light Client API.*/
    keyType_t key_type;
} keyInfo_t;

typedef struct {
    /** defines the number of valid entries in ids array */
    uint16_t idsNum;
    /** See keyids property of MultiRole defined at Server Light Client API.*/
    uint8_t ids[MAX_KEY_NUM_PER_ROLE][SHA256_LENGTH];
    /** See rolename property of MultiRole defined at Server Light Client API.*/
    roleType_t role;
    /** See threshold property of MultiRole defined at Server Light Client API.*/
    uint32_t threshold;
} roleInfo_t;

typedef struct {
    /** See keyids property of MultiRole defined at Server Light Client API.*/
    keyInfo_t *key_ids[MAX_KEY_NUM_PER_ROLE];
    /** See threshold property of MultiRole defined at Server Light Client API.*/
    uint32_t threshold;
} roleKeyInfo_t;

typedef struct {
    /** See function property of Hash defined at Server Light Client API.*/
    functionType_t funcType;
    /** See digest property of Hash defined at Server Light Client API.*/
    uint8_t digest[SHA256_LENGTH];
} hash_t;

typedef struct {
    /** See keyid property of Signature defined at Server Light Client API.*/
    uint8_t keyid[SHA256_LENGTH];
    /** See method property of Signature defined at Server Light Client API.*/
    methodType_t methType;
    /** See hash property of Signature defined at Server Light Client API.*/
    hash_t hash;
    /** defines the length of value */
    uint64_t value_len;
    /** See keyid property of Signature defined at Server Light Client API.*/
    uint8_t value[MAX_SIG_LENGTH];
} sigInfo_t;

typedef struct {
    /** Head pointer of JSON string of signed section. */
    byte_t *headSigned;
    /** Head pointer of JSON string of signature section. */
    byte_t *headSignatures;
    /** Length of JSON string of signed section. */
    uint64_t signedLength;
    /** Length of JSON string of signature section. */
    uint64_t signaturesLength;
} secInfo_t;

typedef struct {
    /** Head pointer of JSON string of Targets Metadata.
    * JSON string of Targets Metadata is NOT Null-terminated string.
    * Then, End pointer should be calculated with length of
    * JSON string of Targets Metadata.
    */
    byte_t *headTargets;
    /** Head pointer of JSON string of Snapshot Metadata.
    * JSON string of Snapshot Metadata is NOT Null-terminated string.
    * Then, End pointer should be calculated with length of
    * JSON string of Snapshot Metadata.
    */
    byte_t *headSnapshot;
    /** Head pointer of JSON string of Timestamp Metadata.
    * JSON string of Timestamp Metadata is NOT Null-terminated string.
    * Then, End pointer should be calculated with length of
    * JSON string of Timestamp Metadata.
    */
    byte_t *headTimestamp;
    /** Head pointer of JSON string of Augmented Metadata.
    * JSON string of Augmented Metadata is NOT Null-terminated string.
    * Then, End pointer should be calculated with length of
    * JSON string of Augmented Metadata.
    */
    byte_t *headAugment;
    /** Length of JSON string of Targets Metadata. */
    uint64_t TargetsLength;
    /** Length of JSON string of Snapshot Metadata. */
    uint64_t SnapshotLength;
    /** Length of JSON string of Timestamp Metadata. */
    uint64_t TimestampLength;
    /** Length of JSON string of Augmented Metadata. */
    uint64_t AugmentLength;
} metaInfo_t;

typedef struct {
    /** See subTargetId property of SoftwareDetail defined at Server Light Client API.*/
    byte_t subTargetId[MAX_SUB_TARGET_ID_SZ];
    /** See softwareId property of SoftwareDetail defined at Server Light Client API.*/
    byte_t softwareId[MAX_ECU_SW_ID_SZ];
} softwareDetail_t;

/** See bank property of TargetBank defined at Server Light Client API.*/
typedef enum {
    BANK_A = 0,
    BANK_B
} bankType_t;

typedef struct {
    /** See bank property of TargetBank defined at Server Light Client API.*/
    bankType_t bank;
    /** defines the number of valid entries in software_details array */
    uint16_t swDetailNum;
    /** See softwareDetails property of TargetBank defined at Server Light Client API.*/
    softwareDetail_t software_details[MAX_SW_DETAILS_NUM];
    /** See hardwareId property of TargetBank defined at Server Light Client API.*/
    byte_t hardware_id[MAX_PART_NUM_SZ];
    /** See rewriteCount property of TargetBank defined at Server Light Client API.*/
    uint32_t rewrite_count;
} targetBank_t;

/** Data structure that is populated by the Tier 1 application prior to send the vehicle
 *  software configuration info to the OTA Center.
 */
typedef struct {

    /** See ecuIdentifier property of ECUVersionManifest defined at Server Light Client API.*/
    byte_t serialNum[MAX_SERIAL_NUM_SZ];
    /** See activeTarget property of ECUVersionManifest defined at Server Light Client API.*/
    targetBank_t active_bank;
    /** See inactiveTarget property of ECUVersionManifest defined at Server Light Client API.*/
    targetBank_t inactive_bank;
    /** See targetId property of ECUVersionManifest defined at Server Light Client API.*/
    byte_t targetId[MAX_TARGET_ID_SZ];

} ecuInfo_t;

typedef struct {
    /** See used property of PackageStorage defined at Server Light Client API.*/
   uint64_t used;
    /** See available property of PackageStorage defined at Server Light Client API.*/
   uint64_t available;
} packageStorage_t;

typedef struct {
    /** See vehicleIdentifier property of CompactVehicleManifestSigned
     *  defined at Server Light Client API.*/
    byte_t vin[MAX_VIN_SZ];
    /** See primaryIdentifier property of CompactVehicleManifestSigned
     *  defined at Server Light Client API.*/
    byte_t primarySerialNum[MAX_SERIAL_NUM_SZ];
    /** See clientDigest property of CompactVehicleManifestSigned
     *  defined at Server Light Client API.*/
   uint8_t cfgDgst[SHA256_LENGTH];
    /** See timestamp property of CompactVehicleManifestSigned
     *  defined at Server Light Client API.*/
   uint64_t timestamp;
    /** See packageStorage property of CompactVehicleManifestSigned
     *  defined at Server Light Client API.*/
   packageStorage_t packageStorage;
    /** See uploadReason property of CompactVehicleManifestSigned
     *  defined at Server Light Client API.*/
   byte_t upd[MAX_UPLOAD_REASON_SZ];
} syncCheck_t;

typedef struct {
    /** See id property of License defined at Server Light Client API.*/
    byte_t id[MAX_LICENSE_ID_SZ];
} licenseInfo_t;

typedef struct {
    /** See mobileNumber property of DCMInfo defined at Server Light Client API.*/
    byte_t mobileNumber[MAX_MOBILE_NUMBER_SZ];
} dcmInfo_t;

typedef struct {
    /** See version property of ClientInfo defined at Server Light Client API.*/
    byte_t version[MAX_VERSION_SZ];
    /** See codeHash property of ClientInfo defined at Server Light Client API.*/
    uint8_t codehash[SHA256_LENGTH];
} clientInfo_t;

typedef struct {
    /** See timestamp property of AugmentedManifest defined at Server Light Client API.*/
    uint64_t timestamp;
    /** defines the number of valid entries in licenseInfo array */
    uint16_t licenseInfoNum;
    /** See licenses property of AugmentedManifest defined at Server Light Client API.*/
    licenseInfo_t licenseInfo[MAX_TOTAL_LICENSE_INFO];
    /** See dcmInfo property of AugmentedManifest defined at Server Light Client API.*/
    dcmInfo_t dcmInfo;
    /** See packageStorage property of AugmentedManifest defined at Server Light Client API.*/
    packageStorage_t packageStorage;
    /** See campaignId property of AugmentedManifest defined at Server Light Client API.*/
    byte_t lastCmpId[MAX_CAMPAIGN_ID_SZ];
    /** See rxswins property of AugmentedManifest defined at Server Light Client API.*/
    byte_t rxswinInfo[MAX_RXSWIN_SZ];
    /** See clientInfo property of AugmentedManifest defined at Server Light Client API.*/
    clientInfo_t clientInfo;
    /** See uploadReason property of AugmentedManifest defined at Server Light Client API.*/
    byte_t upd[MAX_UPLOAD_REASON_SZ];
}augmentedManifest_t;

typedef struct {
    /** See vehicleIdentifier property of LightVehicleVersionManifestSigned defined at Server Light Client API.*/
    byte_t vin[MAX_VIN_SZ];
    /** See primaryIdentifier property of LightVehicleVersionManifestSigned defined at Server Light Client API.*/
    byte_t primarySerialNum[MAX_SERIAL_NUM_SZ];
    /** defines the number of valid entries in ecusInfo array */
    uint8_t ecuInfoNum;
    /** See ecuVersionManifests property of LightVehicleVersionManifestSigned defined at Server Light Client API.*/
    ecuInfo_t ecusInfo[MAX_TOTAL_ECUS];
    /** See securityAttack property of LightVehicleVersionManifestSigned defined at Server Light Client API.*/
    byte_t securityAttack[MAX_ATTACK_SZ];
    /** See augmentedManifest property of LightVehicleVersionManifestSigned defined at Server Light Client API.*/
    augmentedManifest_t augmentedManifest;
    /** See clientDigest property of LightVehicleVersionManifestSigned defined at Server Light Client API.*/
    uint8_t cfgDgst[SHA256_LENGTH];
} cfgMatch_t;

typedef struct {
    /** See vehicleIdentifier property of GetCommandSigned defined at Server Light Client API.*/
    byte_t vin[MAX_VIN_SZ];
    /** See primaryIdentifier property of GetCommandSigned defined at Server Light Client API.*/
    byte_t primarySerialNum[MAX_SERIAL_NUM_SZ];
    /** See lastCompletedCommandId property of GetCommandSigned defined at Server Light Client API.*/
    uint64_t lastUid;
} getOtaCmd_t;

/** See eventType filed of VsmEvents defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_RMSTATE,
    EVENT_NOTIFICATION,
    EVENT_DOWNLOAD,
    EVENT_INSTALLATION,
    EVENT_ACTIVATE,
    EVENT_SYNC_COMPLETE,
    EVENT_ACK_INSTALL,
    EVENT_OEM_ERROR,
    EVENT_TRACKING
} eventType_t;

/** See eventMode filed of VsmEvents defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_MODE_ACTIVE,
    EVENT_MODE_PASSIVE
} eventMode_t;

/** See eventSource filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_SRC_VEHICLE,
    EVENT_SRC_PHONE
} eventSrc_t;

/** See eventDestination filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_DST_SDP,
    EVENT_DST_PHONE
} eventDst_t;

/** See eventTransport filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
typedef enum {
    TRSPRT_WIFI,
    TRSPRT_CELLULAR,
    TRSPRT_BLUETOOTH
} trsprt_t;

/** See updateStatus filed of EcuInformation and ActivationExtras defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_UPDATE_STATUS_SUCCESS,
    EVENT_UPDATE_STATUS_FAILURE
} updateStatus_t;

/** See updateMethod filed of EcuInformation defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_UPDATE_METHOD_OTA,
    EVENT_UPDATE_METHOD_WIRED
} updateMethod_t;

/** See rmVehState filed of rmStateExtras defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_VEHICLE_STATE_WAIT_CMP_USER_ACCPT,
    EVENT_VEHICLE_STATE_WAIT_PKG_DL_USER_ACCPT,
    EVENT_VEHICLE_STATE_PKG_DL,
    EVENT_VEHICLE_STATE_WAIT_INSTLL_USER_ACCPT,
    EVENT_VEHICLE_STATE_REPROG_READY,
    EVENT_VEHICLE_STATE_REPROG,
    EVENT_VEHICLE_STATE_WAIT_ACTIV_USER_ACCPT,
    EVENT_VEHICLE_STATE_POST_PROG,
    EVENT_VEHICLE_STATE_SYSTEM_SYNC_CHK_COMP,
    EVENT_VEHICLE_STATE_INFO_CMP_COMP
} rmVehState_t;

/** See stateScope filed of rmStateExtras defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_STATE_SCOPE_VEHICLE,
    EVENT_STATE_SCOPE_ECU
} stateScope_t;

/** See cancelType filed of CancelExtras defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_CANCEL_TYPE_OTA,
    EVENT_CANCEL_TYPE_WIRED
} cancelType_t;

/** See clocksource filed of TimeMs defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_CLKSRC_SYNCED,
    EVENT_CLKSRC_DEFAULT,
    EVENT_CLKSRC_GPS
} clockSource_t;

/** See category property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
typedef enum {
    CATEGORY_LOG
} category_t;

typedef struct {
/** See code filed of ErrorCode defined at Vehicle Event Structure Definition.*/
    uint8_t code;
/** See description filed of ErrorCode defined at Vehicle Event Structure Definition.*/
    byte_t description[MAX_EVENT_ERR_DESC_SZ];
} errorCode_t;

typedef struct {
/** See code filed of DtcCode defined at Vehicle Event Structure Definition.*/
    uint64_t code;
} dtcCode_t;

typedef struct {
    /** See targetId filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    byte_t targetId[MAX_TARGET_ID_SZ];
    /** defines the number of valid entries in dtcCodes array */
    uint16_t dtcCodeNum;
    /** See dtcCodes filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    dtcCode_t dtcCodes[MAX_EVENT_DTC_NUM];
    /** See ecuSoftwareId filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    byte_t ecuSoftwareId[MAX_ECU_SW_ID_SZ];
    /** See updateStatus filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    updateStatus_t updateStatus;
    /** See integrityInfo filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    uint8_t integrityInfo[SHA256_LENGTH];
    /** defines the number of valid entries in errorCodes array */
    uint16_t errorCodeNum;
    /** See errorCodes filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    errorCode_t errorCodes[MAX_EVENT_ERR_NUM];
    /** See updateMethod filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    updateMethod_t updateMethod;
    /** See rewriteBank filed of EcuInformation defined at Vehicle Event Structure Definition.*/
    bankType_t rewriteBank;
} eventEcuInfo_t;

typedef struct {
    /** See campaignId filed of NotificationExtras defined at Vehicle Event Structure Definition.*/
    byte_t cpId[MAX_CAMPAIGN_ID_SZ];
    /** See campaignName filed of NotificationExtras defined at Vehicle Event Structure Definition.*/
    byte_t cpName[MAX_CAMPAIGN_NAME_SZ];
    /** See dataGroup filed of NotificationExtras defined at Vehicle Event Structure Definition.*/
    byte_t dataGroup[MAX_DATA_GROUP_SZ];
} ntfExtras_t;

typedef struct {
    /** See phaseCode filed of oemError defined at Vehicle Event Structure Definition.*/
    uint8_t phaseCode;
    /** See siteCode filed of oemError defined at Vehicle Event Structure Definition.*/
    uint8_t siteCode;
    /** See oemCode filed of oemError defined at Vehicle Event Structure Definition.*/
    uint8_t oemCode;
    /** See targetId filed of oemError defined at Vehicle Event Structure Definition.*/
    byte_t targetId[MAX_TARGET_ID_SZ];
} oemError_t;

typedef struct {
    /** See eventSource filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    eventSrc_t eventSrc;
    /** See eventDestination filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    eventDst_t eventDst;
    /** See eventTransport filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    trsprt_t Trsprt;
    /** See downloadURL filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    byte_t downloadURL[MAX_DWNLD_URL_SZ];
    /** See downloadSize filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    uint64_t downloadSize;
    /** See bytesDownloaded filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    uint64_t bytesDownloaded;
    /** defines the number of valid entries in eventEcuInfos array */
    uint16_t ecuInfoNum;
    /** See ecuInfo filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    eventEcuInfo_t eventEcuInfos[MAX_TOTAL_ECUS];
    /** defines the number of valid entries in oemErrors array */
    uint16_t oemErrorNum;
    /** See oemErrors filed of DownloadExtras defined at Vehicle Event Structure Definition.*/
    oemError_t oemErrors[MAX_EVENT_OEM_ERR_NUM];
} dwnldExtras_t;

typedef struct {
    /** See ecuInfo filed of InstallStartedExtras defined at Vehicle Event Structure Definition.*/
    eventEcuInfo_t eventEcuInfo;
} instllStartedExtras_t;

typedef struct {
    /** See totalBytes filed of EcuProgress defined at Vehicle Event Structure Definition.*/
    uint64_t totalBytes;
    /** See bytesProcessed filed of EcuProgress defined at Vehicle Event Structure Definition.*/
    uint64_t bytesProcessed;
    /** See targetId filed of EcuProgress defined at Vehicle Event Structure Definition.*/
    byte_t targetId[MAX_TARGET_ID_SZ];
} ecuProgress_t;

typedef struct {
    /** See numEcus filed of InstallProgressExtras defined at Vehicle Event Structure Definition.*/
    uint16_t numEcus;
    /** See completeEcus filed of InstallProgressExtras defined at Vehicle Event Structure Definition.*/
    uint16_t completeEcus;
    /** defines the number of valid entries in ecuProgress array */
    uint16_t ecuProgressNum;
    /** See ecuProgress filed of InstallProgressExtras defined at Vehicle Event Structure Definition.*/
    ecuProgress_t ecuProgress[MAX_TOTAL_ECUS];
} instllProgressExtras_t;

typedef struct {
    /** See numEcus filed of InstallCompleteExtras defined at Vehicle Event Structure Definition.*/
    uint16_t numEcus;
    /** See successEcus filed of InstallCompleteExtras defined at Vehicle Event Structure Definition.*/
    uint16_t successEcus;
    /** See failEcus filed of InstallCompleteExtras defined at Vehicle Event Structure Definition.*/
    uint16_t failEcus;
    /** defines the number of valid entries in eventEcuInfo array */
    uint16_t ecuInfoNum;
    /** See ecuInfo filed of InstallCompleteExtras defined at Vehicle Event Structure Definition.*/
    eventEcuInfo_t eventEcuInfo[MAX_TOTAL_ECUS];
    /** defines the number of valid entries in oemErrors array */
    uint16_t oemErrorNum;
    /** See oemErrors filed of InstallCompleteExtras defined at Vehicle Event Structure Definition.*/
    oemError_t oemErrors[MAX_EVENT_OEM_ERR_NUM];
    /** See updateTimeUTS filed of InstallCompleteExtras defined at Vehicle Event Structure Definition.*/
    uint64_t updateTimeUTS;
} instllCompleteExtras_t;

typedef struct {
    /** See latitude filed of Location defined at Vehicle Event Structure Definition.*/
    /** millisecond format: e.g. N35°30'36'' = (35*3600 + 30*60 + 36) * 1000 */
    int32_t latitude;
    /** See longitude filed of Location defined at Vehicle Event Structure Definition.*/
    /** millisecond format: e.g. N35°30'36'' = (35*3600 + 30*60 + 36) * 1000 */
    int32_t longitude;
    /** See gpsTime filed of Location defined at Vehicle Event Structure Definition.*/
    uint64_t gpsTime;
    /** See altitude filed of Location defined at Vehicle Event Structure Definition.*/
    int32_t altitude;
    /** See system filed of Location defined at Vehicle Event Structure Definition.*/
    byte_t system[MAX_EVENT_GPS_SYSTEM_NAME_SZ];
} location_t;

typedef struct {
    /** See location filed of ActivationExtras defined at Vehicle Event Structure Definition.*/
    location_t location;
    /** See updateTimeUTS filed of ActivationExtras defined at Vehicle Event Structure Definition.*/
    uint64_t updateTimeUTS;
    /** See updateStatus filed of ActivationExtras defined at Vehicle Event Structure Definition.*/
    updateStatus_t updateStatus;
    /** defines the number of valid entries in eventEcuInfo array */
    uint16_t ecuInfoNum;
    /** See ecuInfo filed of ActivationExtras defined at Vehicle Event Structure Definition.*/
    eventEcuInfo_t eventEcuInfo[MAX_TOTAL_ECUS];
    /** defines the number of valid entries in oemErrors array */
    uint16_t oemErrorNum;
    /** See oemErrors filed of ActivationExtras defined at Vehicle Event Structure Definition.*/
    oemError_t oemErrors[MAX_EVENT_OEM_ERR_NUM];
} actvtExtras_t;

typedef struct {
    /** See cancelType filed of CancelExtras defined at Vehicle Event Structure Definition.*/
    cancelType_t cancelType;
    /** See cancelExecuted filed of CancelExtras defined at Vehicle Event Structure Definition.*/
    bool cancelExecuted;
} cancelExtras_t;

typedef struct {
    /** See rmVehState filed of rmStateExtras defined at Vehicle Event Structure Definition.*/
    rmVehState_t rmVehState;
    /** See rmEcuState filed of rmStateExtras defined at Vehicle Event Structure Definition.*/
    uint8_t rmEcuState;
    /** See targetId filed of rmStateExtras defined at Vehicle Event Structure Definition.*/
    byte_t targetId[MAX_TARGET_ID_SZ];
    /** See stateScope filed of rmStateExtras defined at Vehicle Event Structure Definition.*/
    stateScope_t stateScope;
} rmStateExtras_t;

typedef struct {
    /** See time filed of TimeMs defined at Vehicle Event Structure Definition.*/
    uint64_t time;
    /** See clocksource filed of TimeMs defined at Vehicle Event Structure Definition.*/
    clockSource_t clockSource;
} timeMs_t;

typedef struct {
    /** See fileId filed of trackingExtras defined at Vehicle Event Structure Definition.*/
    byte_t fileId[MAX_EVENT_FILE_ID_SZ];
    /** See fileSize filed of trackingExtras defined at Vehicle Event Structure Definition.*/
    uint64_t fileSize;
    /** See dlTime filed of trackingExtras defined at Vehicle Event Structure Definition.*/
    timeMs_t dlTime;
} trackingExtras_t;

typedef enum{
    /** For a case where there is no extra information for sending event.*/
    UNION_FLAG_NO_EXTRAS,
    /** For a case where notificationExtras field will be sent.*/
    UNION_FLAG_NTF,
    /** For a case where downloadExtras field will be sent.*/
    UNION_FLAG_DWNLD,
    /** For a case where installStartedExtras field will be sent.*/
    UNION_FLAG_INSTLLSTARTED,
    /** For a case where installProgressExtras field will be sent.*/
    UNION_FLAG_INSTLLPROGRESS,
    /** For a case where installCompleteExtras field will be sent.*/
    UNION_FLAG_INSTLLCOMPLETE,
    /** For a case where activationExtras field will be sent.*/
    UNION_FLAG_ACTVT,
    /** For a case where errorExtras field will be sent.*/
    UNION_FLAG_ERROR,
    /** For a case where rmStateExtras field will be sent.*/
    UNION_FLAG_RMSTATE,
    /** For a case where trackingExtras field will be sent.*/
    UNION_FLAG_TRACKING,
} unionFlag_t;

typedef union {
    /** See notificationExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    ntfExtras_t ntfExtras;
    /** See downloadExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    dwnldExtras_t dwnldExtras;
    /** See installStartedExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    instllStartedExtras_t instllStartedExtras;
    /** See installProgressExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    instllProgressExtras_t instllProgressExtras;
    /** See installCompleteExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    instllCompleteExtras_t instllCompleteExtras;
    /** See activationExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    actvtExtras_t actvtExtras;
    /** See errorExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    oemError_t errorExtras[MAX_EVENT_OEM_ERR_NUM];
    /** See rmStateExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    rmStateExtras_t rmStateExtras;
    /** See trackingExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/
    trackingExtras_t trackingExtras;
} unionEventExtras_t;

/** See status filed of VsmEvents defined at Vehicle Event Structure Definition.*/
typedef enum {
    EVENT_STATUS_ACCEPTED,
    EVENT_STATUS_DECLINED,
    EVENT_STATUS_POSTPONED,
    EVENT_STATUS_EXTRACTED,
    EVENT_STATUS_EXTRACT_FAIL,
    EVENT_STATUS_VALIDATED,
    EVENT_STATUS_VALIDATE_FAIL,
    EVENT_STATUS_STARTED,
    EVENT_STATUS_INPROGRESS,
    EVENT_STATUS_SUSPEND,
    EVENT_STATUS_RESUME,
    EVENT_STATUS_SUCCESS,
    EVENT_STATUS_FAILURE,
    EVENT_STATUS_CANCEL,
    EVENT_STATUS_SKIPPED,
    EVENT_STATUS_RETRY
} eventStatus_t;

typedef struct {
    /** See eventUuid filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    byte_t eventUuid[UUID_LEN];
    /** See eventType filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    eventType_t eventType;
    /** See eventMode filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    eventMode_t eventMode;
    /** See totalBytes filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    uint64_t totalBytes;
    /** See timeStarted filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    timeMs_t timeStarted;
    /** See currentTime filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    timeMs_t currentTime;
    /** See postponeTime filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    timeMs_t postponeTime;
    /** See status filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    eventStatus_t status;
    /** defines the number of valid entries in errorCodes array */
    uint16_t errorCodeNum;
    /** See errorCodes filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    errorCode_t errorCodes[MAX_EVENT_ERR_NUM];
    /** See numRetries filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    uint16_t numRetries;
    /** See attemptId filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    byte_t attemptId[MAX_ATTEMPT_ID_SZ];
    /** See reportId filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    byte_t reportId[MAX_REPORT_ID_SZ];
    /** See campaignId filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    byte_t campaignId[MAX_CAMPAIGN_ID_SZ];
    /** defines the number of valid entries in eventExtras.oemErrors array */
    uint16_t oemErrorNum;
    /** the type of extra information for event*/
    unionFlag_t unionFlag;
    /** a union for extra information*/
    unionEventExtras_t eventExtras;
    /** See cancelExtras filed of VsmEvents defined at Vehicle Event Structure Definition.*/    
    cancelExtras_t cancelExtras;
} event_t;

typedef struct {
    /** See vin filed of VsmDeviceEventsSigned defined at Vehicle Event Structure Definition.*/    
    byte_t vin[MAX_VIN_SZ];
    /** See primaryIdentifier filed of VsmDeviceEventsSigned defined at Vehicle Event Structure Definition.*/    
    byte_t primarySerialNum[MAX_SERIAL_NUM_SZ];
    /** defines the number of valid entries in events array */
    uint16_t eventsNum;
    /** See events filed of VsmDeviceEventsSigned defined at Vehicle Event Structure Definition.*/    
    event_t events[MAX_NOTIFICATIONS];
} ntf_t;

typedef struct {
    /** See partNumber property of UploadPart for MultiPartInitiateUploadRequest
     *  defined at Server Light Client API.*/
    uint16_t partNumber;
    /** See md5CheckSum property of UploadPart for MultiPartInitiateUploadRequest
     *  defined at Server Light Client API.*/
    byte_t md5CheckSum[MAX_BASE64_MD5_SZ];
} uploadPartReq_t;

typedef struct {
    /** See partNumber property of UploadPart of Response for MultiPartInitiateUploadRequest
     *  defined at Server Light Client API.*/
    uint16_t partNumber;
    /** See md5CheckSum property of UploadPart of Response for MultiPartInitiateUploadRequest
     *  defined at Server Light Client API.*/
    byte_t md5CheckSum[MAX_BASE64_MD5_SZ];
    /** See uploadURL property of UploadPart of Response for MultiPartInitiateUploadRequest
     *  defined at Server Light Client API.*/
    byte_t uploadURL[MAX_UPLD_URL_SZ];
    /** See etag property of PartETag defined at Server Light Client API.*/
    byte_t etag[MAX_ETAG_SZ];
} uploadPartRes_t;

typedef struct {
    /** See vehicleIdentifier property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t vin[MAX_VIN_SZ];
    /** See primaryIdentifier property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t primarySerialNum[MAX_SERIAL_NUM_SZ];
    /** See currentTime property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    uint64_t currentTime;
    /** defines the number of valid entries in uplPartReqNum array */
    uint16_t uplPartReqNum;
    /** See uploadParts property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    uploadPartReq_t uploadPartReq[MAX_UPLOAD_PARTS_REQ_NUM];
    /** See device-id property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t deviceId[MAX_DEVICE_ID_SZ];
    /** See client-type property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t clientType[MAX_CLIENT_TYPE_SZ];
    /** See description property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t description[MAX_DESC_SZ];
    /** See begin-time property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t beginTime[FORMATED_TIME_SZ];
    /** See end-time property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t endTime[FORMATED_TIME_SZ];
    /** See category property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    category_t category;
    /** See transport-type property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    trsprt_t trsprtType;
    /** See client-upload-id property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t clientUploadId[MAX_CLIENT_UPLOAD_ID_SZ];
    /** See report-id property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t reportId[MAX_REPORT_ID_SZ];
    /** See file-name property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t fileName[MAX_FILE_NAME_SZ];
    /** defines the number of valid entries in eventUuids array */
    uint16_t eventUuidNum;
    /** See eventUuid property of MultiPartInitiateUploadRequest defined at Server Light Client API.*/
    byte_t eventUuids[MAX_UUID_NUM][UUID_LEN];
} mpuInit_t;

typedef struct {
    /** See partNumber property of PartETag defined at Server Light Client API.*/
    uint16_t partNumber;
    /** See etag property of PartETag defined at Server Light Client API.*/
    byte_t etag[MAX_ETAG_SZ];
} partETag_t;

typedef struct {
    /** See vehicleIdentifier property of MultiPartCompleteUploadRequest defined at Server Light Client API.*/
    byte_t vin[MAX_VIN_SZ];
    /** See primaryIdentifier property of MultiPartCompleteUploadRequest defined at Server Light Client API.*/
    byte_t primarySerialNum[MAX_SERIAL_NUM_SZ];
    /** See currentTime property of MultiPartCompleteUploadRequest defined at Server Light Client API.*/
    uint64_t currentTime;
    /** See uploadContext property of MultiPartCompleteUploadRequest defined at Server Light Client API.*/
    byte_t uploadContext[MAX_UPLOAD_CONTEXT_SZ];
    /** defines the number of valid entries in partETags array */
    uint16_t partETagNum;
    /** See partETags property of MultiPartCompleteUploadRequest defined at Server Light Client API.*/
    partETag_t partETags[MAX_PART_ETAG_NUM];
} mpuComp_t;

typedef struct {
    /** defines the number of valid entries in the keys array */
    uint8_t keysNum; 
    /** See keys property of RootMetadata defined at Server Light Client API.*/
    keyInfo_t keys[MAX_TOTAL_KEY_NUM];
    /** defines the number of valid entries in the roles array */
    uint8_t rolesNum; 
    /** See roles property of RootMetadata defined at Server Light Client API.*/
    roleInfo_t roles[NUM_ROLES];
} rootMetaBody_t;

typedef struct {
    /** See RootMetadata property defined at Server Light Client API.*/
    rootMetaBody_t rootMetaBody;
    /** See expires property of SignedMetadata defined at Server Light Client API.*/
    uint64_t expires;
    /** See type property of SignedMetadata defined at Server Light Client API.*/
    roleType_t type;
    /** See version property of SignedMetadata defined at Server Light Client API.*/
    uint32_t version;
} rootMeta_t;


typedef struct {
    /** See uploadContext property of UploadPart of Response for MultiPartInitiateUploadRequest
     *  defined at Server Light Client API.*/
    byte_t uploadContext[MAX_UPLOAD_CONTEXT_SZ];
    /** defines the number of valid entries in uploadPartRes array */
    uint16_t upPartResNum;
    /** See uploadParts property of UploadPart of Response for MultiPartInitiateUploadRequest
     *  defined at Server Light Client API.*/
    uploadPartRes_t uploadPartRes[MAX_UPLOAD_PARTS_RES_NUM];
} mpuInitResponse_t;

/** See command property of GetCommandResponseBody defined at Server Light Client API.*/
typedef enum {
    OTA_CMD_TYPE_NOT_APPLICABLE = 0,
    OTA_CMD_TYPE_CONFIG_MATCH,
    OTA_CMD_TYPE_UPLD_LOG,
    NUM_OF_OTA_CMD_TYPE
} getOtaCmdType_t;

typedef struct {
    /** See vehicleIdentifier property of GetCommandResponseBody defined at Server Light Client API.*/
    byte_t vin[MAX_VIN_SZ];
    /** See commandId property of GetCommandResponseBody defined at Server Light Client API.*/
    uint64_t cmdId;
    /** See command property of GetCommandResponseBody defined at Server Light Client API.*/
    getOtaCmdType_t getOtaCmdType;
} getOtaCmdResBody_t;

typedef struct {
    /** See body property of GetCommandResponseSigned defined at Server Light Client API.*/
    getOtaCmdResBody_t getOtaCmdResBody;
    /** See expires property of SignedMetadata defined at Server Light Client API.*/
    uint64_t expires;
    /** See type property of SignedMetadata defined at Server Light Client API.*/
    roleType_t type;
    /** See version property of SignedMetadata defined at Server Light Client API.*/
    uint32_t version;
} getOtaCmdRes_t;

typedef struct {
    /** See filename property of TimestampMetadata defined at Server Light Client API.*/
    byte_t fileName[MAX_FILE_NAME_SZ];
    /** See version property of TimestampMetadata defined at Server Light Client API.*/
    uint32_t version;
    /** See length property of TimestampMetadata defined at Server Light Client API.*/
    uint32_t length;
    /** defines the number of valid entries in hashes array */
    uint16_t hashesNum;
    /** See hashes property of TimestampMetadata defined at Server Light Client API.*/
    hash_t hashes[MAX_HASH_NUM];
} timeMetaBody_t;

typedef struct {
    /** See TimestampMetadata property defined at Server Light Client API.*/
    timeMetaBody_t timeMetaBody;
    /** See expires property of SignedMetadata defined at Server Light Client API.*/
    uint64_t expires;
    /** See type property of SignedMetadata defined at Server Light Client API.*/
    roleType_t type;
    /** See version property of SignedMetadata defined at Server Light Client API.*/
    uint32_t version;
} timeMeta_t;

typedef struct {
    /** See filename property of SnapshotMetadataFile defined at Server Light Client API.*/
    byte_t fileName[MAX_FILE_NAME_SZ];
    /** See version property of SnapshotMetadataFile defined at Server Light Client API.*/
    uint32_t version;
} snapshotMetaFile_t;

typedef struct {
    /** defines the number of valid entries in snapshotMetaFiles array */
    uint16_t snapshotNum;
    /** See snapshotMetadataFiles of SnapshotMetadata property defined at Server Light Client API.*/
    snapshotMetaFile_t snapshotMetaFiles[MAX_SNAPSHOT_META_FILES_NUM];
} snapshotMetaBody_t;

typedef struct {
    /** See SnapshotMetadata property defined at Server Light Client API.*/
    snapshotMetaBody_t snapshotMetaBody;
    /** See expires property of SignedMetadata defined at Server Light Client API.*/
    uint64_t expires;
    /** See type property of SignedMetadata defined at Server Light Client API.*/
    roleType_t type;
    /** See version property of SignedMetadata defined at Server Light Client API.*/
    uint32_t version;
} snapshotMeta_t;

typedef struct {
    /** See filename property of Target defined at Server Light Client API.*/
    byte_t fileName[MAX_FILE_NAME_SZ];
    /** See length property of Target defined at Server Light Client API.*/
    uint32_t length;
    /** defines the number of valid entries in hashes array */
    uint16_t hashesNum;
    /** See hashes property of Target defined at Server Light Client API.*/
    hash_t hashes[MAX_HASH_NUM];
    /** See fileDownloadUrl property of Target defined at Server Light Client API.*/
    byte_t fileDownloadUrl[MAX_DWNLD_URL_SZ];
} target_t;

/** See encryptedSymmetricKeyType property of EncryptedSymmetricKey
 *  defined at Server Light Client API.*/
typedef enum {
    ECRYPTD_SYMKEY_TYPE_AES,
    ECRYPTD_SYMKEY_TYPE_DES,
    ECRYPTD_SYMKEY_TYPE_UNKNOWN
} ecryptdSymKeyType_t;

/** See encryptedSymmetricKeyAlgorithmMode property of EncryptedSymmetricKey
 *  defined at Server Light Client API.*/
typedef enum {
    ECRYPTD_SYMKEY_ALG_MODE_CBC,
    ECRYPTD_SYMKEY_ALG_MODE_UNKNOWN
} ecryptdSymKeyAlgMode_t;

/** See paddingScheme property of EncryptedSymmetricKey
 *  defined at Server Light Client API.*/
typedef enum {
    PADDING_SCHEME_UNKNOWN,
    PADDING_SCHEME_PKCS5
} paddingScheme_t;

typedef struct {
    /** See encryptedSymmetricKeyType property of EncryptedSymmetricKey
     *  defined at Server Light Client API.*/
    ecryptdSymKeyType_t ecryptdSymKeyType;
    /** See encryptedSymmetricKeyValue property of EncryptedSymmetricKey
     *  defined at Server Light Client API.*/
    byte_t ecryptdSymKeyValue[MAX_ECRYPTD_SYMKEY_VALUE_SZ];
    /** See encryptedSymmetricKeyAlgorithmMode property of EncryptedSymmetricKey
     *  defined at Server Light Client API.*/
    ecryptdSymKeyAlgMode_t ecryptdSymKeyAlgMode;
    /** See paddingScheme property of EncryptedSymmetricKey
     *  defined at Server Light Client API.*/
    paddingScheme_t paddingScheme;
    /** See iv property of EncryptedSymmetricKey defined at Server Light Client API.*/
    byte_t iv[MAX_BASE64_IV_SZ];
} ecryptdSymKey_t;

typedef struct {
    /** See releaseCounter property of Custom defined at Server Light Client API.*/
    uint32_t releaseCounter;
    /** See hardwareIdentifier property of Custom defined at Server Light Client API.*/
    byte_t hardwareIdentifier[MAX_PART_NUM_SZ];
    /** See ecuIdentifier property of Custom defined at Server Light Client API.*/
    byte_t ecuIdentifier[MAX_SERIAL_NUM_SZ];
    /** See encryptedTarget property of Custom defined at Server Light Client API.*/
    target_t encryptedTarget;
    /** See encryptedSymmetricKey property of Custom defined at Server Light Client API.*/
    ecryptdSymKey_t ecryptdSymKey;
    /** See deltaTarget property of Custom defined at Server Light Client API.*/
    target_t deltaTarget;
    /** See reportId property of Custom defined at Server Light Client API.*/
    byte_t reportId[MAX_REPORT_ID_SZ];
} custom_t;

typedef struct {
    /** See target property of TargetAndCustom defined at Server Light Client API.*/
    target_t target;
    /** See custom property of TargetAndCustom defined at Server Light Client API.*/
    custom_t custom;
} targetAndCustom_t;

typedef struct {
    /** defines the number of valid entries in targets array */
    uint16_t targetsNum;
    /** See targets property of TargetsMetadata defined at Server Light Client API.*/
    targetAndCustom_t targets[MAX_TARGET_AND_CUSTOM_NUM];
} targetsMetaBody_t;

typedef struct {
    /** See TargetsMetadata property defined at Server Light Client API.*/
    targetsMetaBody_t targetsMetaBody;
    /** See expires property of SignedMetadata defined at Server Light Client API.*/
    uint64_t expires;
    /** See type property of SignedMetadata defined at Server Light Client API.*/
    roleType_t type;
    /** See version property of SignedMetadata defined at Server Light Client API.*/
    uint32_t version;
} targetsMeta_t;

/** See campaignType property of Campaign defined at Server Light Client API.*/
typedef enum {
    CMP_TYPE_OTA,
    CMP_TYPE_INFO,
    CMP_TYPE_UNKNOWN,
} campaignType_t;

/** See ApprovalMode defined at Server Light Client API.*/
typedef enum {
    APPROVAL_MODE_ACTIVE,
    APPROVAL_MODE_PASSIVE,
    APPROVAL_MODE_USER_PREF,
    APPROVAL_MODE_UNKNOWN
} approvalMode_t;

typedef struct {
    /** See campaignApproval property of UpdatePolicy defined at Server Light Client API.*/
    approvalMode_t campaignApproval;
    /** See downloadApproval property of UpdatePolicy defined at Server Light Client API.*/
    approvalMode_t downloadApproval;
    /** See installApproval property of UpdatePolicy defined at Server Light Client API.*/
    approvalMode_t installApproval;
    /** See activateApproval property of UpdatePolicy defined at Server Light Client API.*/
    approvalMode_t activateApproval;
    /** See completionAck property of UpdatePolicy defined at Server Light Client API.*/
    approvalMode_t completionAck;
} updatePolicy_t;

typedef struct {
    /** See reportId property of ChangeEvent defined at Server Light Client API.*/
    byte_t reportId[MAX_REPORT_ID_SZ];
    /** See elementId property of ChangeEvent defined at Server Light Client API.*/
    byte_t elementId[MAX_SERIAL_NUM_SZ];
    /** See rootUrl property of ChangeEvent defined at Server Light Client API.*/
    byte_t rootUrl[MAX_DWNLD_URL_SZ];
    /** See queryString property of ChangeEvent defined at Server Light Client API.*/
    byte_t queryString[MAX_QUERY_STRING_SZ];
} changeEvent_t;

typedef struct {
    /** See reportId property of HmiMessage defined at Server Light Client API.*/
    byte_t reportId[MAX_REPORT_ID_SZ];
    /** See url property of HmiMessage defined at Server Light Client API.*/
    byte_t url[MAX_DWNLD_URL_SZ];
    /** See length property of HmiMessage defined at Server Light Client API.*/
    uint32_t length;
    /** See digest property of HmiMessage defined at Server Light Client API.*/
    uint8_t digest[SHA256_LENGTH];
} hmiMessage_t;

/** See displayMode property of PreCondition defined at Server Light Client API.*/
typedef enum {
    DISPLAY_MODE_MANDATORY,
    DISPLAY_MODE_CONDITIONAL,
    DISPLAY_MODE_UNKNOWN
} displayMode_t;

typedef struct {
    /** See preCondition property of PreCondition defined at Server Light Client API.*/
    byte_t preCondition[MAX_PRECONDITION_STR_SZ];
    /** See displayMode property of PreCondition defined at Server Light Client API.*/
    displayMode_t displayMode;
    /** See value property of PreCondition defined at Server Light Client API.*/
    int32_t value;
} preCondition_t;

typedef struct {
    /** See campaignId property of Campaign defined at Server Light Client API.*/
    byte_t campaignId[MAX_CAMPAIGN_ID_SZ];
    /** See campaignType property of Campaign defined at Server Light Client API.*/
    campaignType_t campaignType;
    /** See policy property of Campaign defined at Server Light Client API.*/
    updatePolicy_t policy;
    /** defines the number of valid entries in changeEvents array */
    uint16_t ceNum;
    /** See changeEvents property of Campaign defined at Server Light Client API.*/
    changeEvent_t changeEvents[MAX_CHANGE_EVENTS];
    /** See hmiMessages property of Campaign defined at Server Light Client API.*/
    hmiMessage_t hmiMessages;
    /** defines the number of valid entries in preConditions array */
    uint16_t preCondNum;
    /** See preConditions property of Campaign defined at Server Light Client API.*/
    preCondition_t preConditions[MAX_PRECONDITION_NUM];
} campaign_t;

typedef struct {
    /** See otaEnabled property of SubscriptionInfo defined at Server Light Client API.*/
    bool otaEnabled;
    /** See statusCheckInterval property of SubscriptionInfo defined at Server Light Client API.*/
    uint32_t statusCheckInterval;
} subscInfo_t;

/** See status property of AugmentedMetadata defined at Server Light Client API.*/
typedef enum {
    CONFIG_CHECK_STATUS_OTA_DISABLED,
    CONFIG_CHECK_STATUS_RE_SYNC,
    CONFIG_CHECK_STATUS_STALE_REQUEST,
    CONFIG_CHECK_STATUS_NON_STD_CONFIG,
    CONFIG_CHECK_STATUS_NO_UPDATE,
    CONFIG_CHECK_STATUS_UPDATE,
    CONFIG_CHECK_STATUS_UNKNOWN
} configCheckStatus_t;

typedef struct {
    /** defines the number of valid entries in campaigns array */
    uint16_t campaignsNum;
    /** See campaigns property of AugmentedMetadata defined at Server Light Client API.*/
    campaign_t campaigns[MAX_CAMPAIGNS];
    /** See subscriptionInfo property of AugmentedMetadata defined at Server Light Client API.*/
    subscInfo_t subscInfo;
    /** See status property of AugmentedMetadata defined at Server Light Client API.*/
    configCheckStatus_t status;
} augMetaBody_t;

typedef struct {
    /** See AugmentedMetadata property defined at Server Light Client API.*/
    augMetaBody_t augMetaBody;
    /** See expires property of SignedMetadata defined at Server Light Client API.*/
    uint64_t expires;
    /** See type property of SignedMetadata defined at Server Light Client API.*/
    roleType_t type;
    /** See version property of SignedMetadata defined at Server Light Client API.*/
    uint32_t version;
} augMeta_t;

/** See padding property of DataEncryptionKey defined at Server Light Client API.*/
typedef enum {
    OAEP_SHA1_MGF1,
    OAEP_SHA256_MGF1,
    OAEP_SHA384_MGF1,
    PADDING_UNKNOWN
} padding_t;

/** See cipherMode property of DataEncryptionKey defined at Server Light Client API.*/
typedef enum {
    CIPHER_MODE_ECB,
    CIPHER_MODE_UNKNOWN
} cipherMode_t;

typedef struct {
    /** See value property of DataEncryptionKey defined at Server Light Client API.*/
    byte_t value[MAX_BASE64_ENCRYPTED_DEK_SZ];
    /** See cipherMode property of DataEncryptionKey defined at Server Light Client API.*/
    cipherMode_t cipherMode;
    /** See padding property of DataEncryptionKey defined at Server Light Client API.*/
    padding_t padding;
    /** See iv property of DataEncryptionKey defined at Server Light Client API.*/
    byte_t iv[MAX_BASE64_IV_SZ];
} dataEncryptionKey_t;

typedef enum {

    /** No error occurred */
    UTIL_NO_ERROR = 0,

    /** One or more of the provided parameters to an API call
     *  are not valid
     */
    UTIL_ERR_INVALID_PARAMETER = -2,

    /** Resource is not enough.*/
    UTIL_ERR_NO_RESOURCE = -3,
} utilErr_t;
#endif                          /* SDK_TYPES_H */
