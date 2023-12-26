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
#ifndef SDK3_0_T1_SAMPLE_STRING_DB_H
#define SDK3_0_T1_SAMPLE_STRING_DB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "emoota_utility_types.h"

#define ICD_LIST_STRING \
X(primaryIdentifier),   \
X(vehicleIdentifier),   \
X(lastCompletedCommandId), \
X(keyid),               \
X(method),              \
X(hash),                \
X(value),               \
X(SHA256withRSA),       \
X(SHA256withECDSA),     \
X(NONEwithRSA),         \
X(NONEwithECDSA),       \
X(RSA),                 \
X(ECDSA),               \
X(SHA256),              \
X(signatures),          \
X(function),            \
X(digest),              \
X(code),                \
X(targetId),            \
X(dtcCodes),            \
X(ecuSoftwareId),       \
X(updateStatus),        \
X(integrityInfo),       \
X(errorCodes),          \
X(updateMethod),        \
X(rewriteBank),         \
X(campaignId),          \
X(campaignName),        \
X(dataGroup),           \
X(eventSource),         \
X(eventDestination),    \
X(eventTransport),      \
X(downloadURL),         \
X(downloadSize),        \
X(bytesDownloaded),     \
X(ecuInfo),             \
X(oemErrors),           \
X(cancelType),          \
X(cancelExecuted),      \
X(description),         \
X(numEcus),             \
X(successEcus),         \
X(failEcus),            \
X(updateTimeUTS),       \
X(totalBytes),          \
X(bytesProcessed),      \
X(completeEcus),        \
X(ecuProgress),         \
X(latitude),            \
X(longitude),           \
X(gpsTime),             \
X(altitude),            \
X(system),              \
X(location),            \
X(phaseCode),           \
X(siteCode),            \
X(oemCode),             \
X(stateScope),          \
X(fileId),              \
X(fileSize),            \
X(dlTime),              \
X(time),                \
X(clocksource),         \
X(vin),                 \
X(events),              \
X(eventUuid),           \
X(eventType),           \
X(eventMode),           \
X(notificationExtras),  \
X(downloadExtras),      \
X(installStartedExtras),\
X(installProgressExtras),  \
X(installCompleteExtras),  \
X(activationExtras),    \
X(cancelExtras),        \
X(errorExtras),         \
X(rmStateExtras),       \
X(trackingExtras),      \
X(timeStarted),         \
X(currentTime),         \
X(postponeTime),        \
X(numRetries),          \
X(attemptId),           \
X(reportId),            \
X(rmState),             \
X(download),            \
X(installation),        \
X(notification),        \
X(activate),            \
X(syncComplete),        \
X(ackinstall),          \
X(oem_error),           \
X(tracking),            \
X(accepted),            \
X(declined),            \
X(postponed),           \
X(extracted),           \
X(extract_fail),        \
X(validated),           \
X(validate_fail),       \
X(started),             \
X(inProgress),          \
X(suspend),             \
X(resume),              \
X(success),             \
X(failure),             \
X(retry),               \
X(skipped),             \
X(ecuVersionManifests), \
X(securityAttack),      \
X(augmentedManifest),   \
X(clientDigest),        \
X(activeTarget),        \
X(inactiveTarget),      \
X(bank),                \
X(softwareDetails),     \
X(hardwareId),          \
X(rewriteCount),        \
X(subTargetId),         \
X(softwareId),          \
X(timestamp),           \
X(licenses),            \
X(dcmInfo),             \
X(packageStorage),      \
X(rxswins),             \
X(clientInfo),          \
X(uploadReason),        \
X(id),                  \
X(mobileNumber),        \
X(used),                \
X(available),           \
X(version),             \
X(codeHash),            \
X(ecuIdentifier),       \
X(expires),             \
X(publicKeyId),         \
X(publicKeyValue),      \
X(publicKeyType),       \
X(threshold),           \
X(type),                \
X(role),                \
X(roles),               \
X(root),                \
X(snapshot),            \
X(targets),             \
X(body),                \
X(keys),                \
X(keyids),              \
X(status),              \
X(cancel),              \
X(synced),              \
X(gps),                 \
X(active),              \
X(passive),             \
X(vehicle),             \
X(ecu),                 \
X(rmVehState),          \
X(rmEcuState),          \
X(waitCampaignUserAcceptance), \
X(waitPackageDlUserAcceptance),\
X(packageDownload),     \
X(waitInstallUserAcceptance),  \
X(reprogReady),         \
X(reprog),              \
X(waitActivateUserAcceptance), \
X(postProgramming),     \
X(systemSyncCheckComplete),\
X(infoCampaignComplete),\
X(ota),                 \
X(wired),               \
X(SDP),                 \
X(phone),               \
X(wifi),                \
X(cellular),            \
X(bluetooth),           \
X(commandId),           \
X(command),             \
X(UpldVehclCnfg),       \
X(UpldVehclLog),        \
X(iv),                  \
X(cipherMode),          \
X(padding),             \
X(category),            \
X(partNumber),          \
X(uploadParts),         \
X(md5CheckSum),         \
X(log),                 \
X(uploadURL),           \
X(uploadContext),       \
X(partETags),           \
X(etag),                \
X(OTA_DISABLED),        \
X(RE_SYNC),             \
X(STALE_REQUEST),       \
X(NON_STD_CONFIG),      \
X(NO_UPDATE),           \
X(UPDATE),              \
X(subscriptionInfo),    \
X(statusCheckInterval), \
X(otaEnabled),          \
X(campaignType),        \
X(OTA),                 \
X(INFORMATIONAL),       \
X(policy),              \
X(campaignApproval),    \
X(downloadApproval),    \
X(installApproval),     \
X(activateApproval),    \
X(completionAck),       \
X(user_preference),     \
X(length),              \
X(url),                 \
X(preCondition),        \
X(displayMode),         \
X(MANDATORY),           \
X(CONDITIONAL),         \
X(rootUrl),             \
X(queryString),         \
X(elementId),           \
X(filename),            \
X(target),              \
X(custom),              \
X(fileDownloadUrl),     \
X(releaseCounter),      \
X(hardwareIdentifier),  \
X(encryptedTarget),     \
X(deltaTarget),         \
X(encryptedSymmetricKey),  \
X(encryptedSymmetricKeyType),  \
X(encryptedSymmetricKeyValue), \
X(encryptedSymmetricKeyAlgorithmMode), \
X(paddingScheme),       \
X(snapshotMetadataFiles),  \




#define X(value)    icd_key_##value
typedef enum {
    ICD_LIST_STRING
} string_db_access_key_t;
#undef X

extern cstr_t getDBstr(string_db_access_key_t db_key);

#ifdef __cplusplus
}
#endif
#endif //SDK3_0_T1_SAMPLE_STRING_DB_H
