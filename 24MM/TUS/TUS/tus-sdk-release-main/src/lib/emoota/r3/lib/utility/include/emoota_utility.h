
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

#ifndef SDK_UTIL_H
#define SDK_UTIL_H

#include "emoota_utility_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the head of each role of Uptane Metadata from the bundled Uptane Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20320">USDM-20320</a>
 *
 * The SDK Utility SHALL provide an API to get the head of each role of Uptane Metadata
 * from the bundled Uptane Metadata
 *
 * @param jsonStr [in] Head pointer of JSON string.
 * @param strLen [in] Length of JSON string.
 * @param metaInfo [in,out] Head address and length of each role of Uptane Metadata are put
 * if #UTIL_NO_ERROR was returned. Undefined values are put if #UTIL_ERR_INVALID_PARAMETER was returned.
 * @return #UTIL_NO_ERROR The process was successfully.
 * @return #UTIL_ERR_INVALID_PARAMETER Some metadata is missing on JSON object.
 */
utilErr_t util_FindBundleSections(cstr_t jsonStr,
                                  size_t strLen,
                                  metaInfo_t *metaInfo);

/**
 * @brief Construct an OTAmatic SDP request signature section.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20319">USDM-20319</a>
 *
 * The SDK Utility SHALL provide an API to construct an OTAmatic SDP request signature section
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param sigInfo [in] Array of a data structure for constructing JSON string.
 * @param numSig [in] Number of valid signature in an array of a populated data structure.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateSignatureBlock(const sigInfo_t sigInfo[], uint16_t numSig, str_t buf,
                                    size_t bufLen);


/**
 * @brief Parse Trust Root Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20307">USDM-20307</a>
 *
 * The SDK Utility SHALL provide an API to parse Trust Root Metadata received from OTAmatic SDP
 *
 * @param signedStr [in] String pointer of JSON string for Trust Root Metadata. String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseTrustRootMetadata(str_t signedStr, size_t length,
                                      util_json_t mem[],
                                      uint16_t qty,
                                      rootMeta_t *out);

/**
 * @brief Construct a JSON string from Trust Root Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20308">USDM-20308</a>
 *
 * The SDK Utility SHALL provide an API to construct a JSON string from Trust Root Metadata
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param trustRootMeta [in] Data struct which has Trust Root Metadata.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateFromTrustRootMeta(const rootMeta_t *const trustRootMeta, str_t buf,
                                       size_t bufLen);

/**
 * @brief Get the head of the Signed Section and Signature Section of Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20314">USDM-20314</a>
 *
 * The SDK Utility SHALL provide an API to get the head of the Signed Section and Signature Section of Metadata
 *
 * @param jsonStr [in] Head pointer of JSON string.
 * @param strLen [in] Length of JSON string.
 * @param secInfo [in,out] Head address and length of the Signed Section and Signature Section are put
 *  if #UTIL_NO_ERROR was returned. Undefined values are put if #UTIL_ERR_INVALID_PARAMETER was returned.
 * @return #UTIL_NO_ERROR The process was successfully.
 * @return #UTIL_ERR_INVALID_PARAMETER JSON string does not have Signed Section and/or Signature Section.
 */
utilErr_t util_FindSignatureSignedBlock(cstr_t jsonStr,
                                        size_t strLen,
                                        secInfo_t *secInfo);

/**
 * @brief Construct the signed section of the OTAmatic SDP Initiate Multi Part Upload-V2 request.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20072">USDM-20072</a>
 *
 * The SDK Utility SHALL provide an API to construct the signed section of
 * the OTAmatic SDP Initiate Multi Part Upload-V2 request
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param mpuInit [in] A data structure for constructing JSON string.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateMpuInitBody(const mpuInit_t *const mpuInit, str_t buf,
                                 size_t bufLen);

/**
 * @brief Parse the OTAmatic SDP Initiate Multi Part Upload-V2 response.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20073">USDM-20073</a>
 *
 * The SDK Utility SHALL provide an API to parse the OTAmatic SDP Initiate Multi Part Upload-V2 response
 *
 * @param str [in,out] String pointer of JSON string for the OTAmatic SDP Initiate Multi Part Upload-V2 response.
 * String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * if #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseMpuInitResponse(str_t str, size_t length,
                                    util_json_t mem[],
                                    uint16_t qty,
                                    mpuInitResponse_t *out);

/**
 * @brief Construct the signed section of the OTAmatic SDP Complete Multi Part Upload-V2 request.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20074">USDM-20074</a>
 *
 * The SDK Utility SHALL provide an API to construct the signed section of
 *  the OTAmatic SDP Complete Multi Part Upload-V2 request
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param mpuComp [in] A data structure for constructing JSON string.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateMpuCompBody(const mpuComp_t *const mpuComp, str_t buf,
                                 size_t bufLen);

/**
 * @brief Parse the OTAmatic SDP Get OTA Command response.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20076">USDM-20076</a>
 *
 * The SDK Utility SHALL provide an API to parse the OTAmatic SDP Get OTA Command response
 *

 * @param signedStr [in,out] String pointer with signed section of the OTAmatic SDP Get OTA Command response.
 * String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * if #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseGetOtaCmdRes(str_t signedStr,
                                 size_t length,
                                 util_json_t mem[], uint16_t qty,
                                 getOtaCmdRes_t *out);

/**
 * @brief Parse the signature section of a response from OTAmatic SDP.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20315">USDM-20315</a>
 *
 * The SDK Utility SHALL provide an API to parse the signature section of a response from OTAmatic SDP
 *
 * @param sigStr [in,out] String pointer of signature section of JSON string . String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param sigs [in,out] Array for storing parsed signature data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @param maxSigsNum [in] Size of array for storing parsed signature data.
 * @param validSigNum [out] Number of elements populated by the parsing API
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t
util_ParseSignature(str_t sigStr, size_t length, util_json_t mem[], uint16_t qty, sigInfo_t sigs[], uint16_t maxSigsNum,
                    uint16_t *validSigNum);

/**
 * @brief Parse the signed section of Uptane Timestamp Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20310">USDM-20310</a>
 *
 * The SDK Utility SHALL provide an API to parse the signed section of Uptane Timestamp Metadata.
 *
 * @param signedStr [in,out] String pointer of signed section of Uptane Timestamp Metadata.
 * String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseUptTimeMetadata(str_t signedStr,
                                    size_t length,
                                    util_json_t mem[],
                                    uint16_t qty,
                                    timeMeta_t *out);

/**
 * @brief Parse the signed section of Uptane Root Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20309">USDM-20309</a>
 *
 * The SDK Utility SHALL provide an API to parse signed section of Uptane Root Metadata.
 *
 * @param signedStr [in] String pointer with signed section of Uptane Root Metadata.
 * String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseUptRootMetadata(str_t signedStr, size_t length, util_json_t mem[], uint16_t qty, rootMeta_t *out);

/**
 * @brief Parse the signed section of Uptane Snapshot Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20311">USDM-20311</a>
 *
 * The SDK Utility SHALL provide an API to parse the signed section of Uptane Snapshot Metadata
 *
 * @param signedStr [in,out] String pointer with signed section of Uptane Snapshot Metadata.
 * String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseUptSnapshotMetadata(str_t signedStr,
                                        size_t length,
                                        util_json_t mem[],
                                        uint16_t qty,
                                        snapshotMeta_t *out);

/**
 * @brief Parse the signed section of Uptane Targets Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20312">USDM-20312</a>
 *
 * The SDK Utility SHALL provide an API to parse the signed section of Uptane Targets Metadata
 *
 * @param signedStr [in,out] String pointer with signed section of Uptane Targets Metadata.
 * String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseUptTargetsMetadata(str_t signedStr,
                                       size_t length,
                                       util_json_t mem[],
                                       uint16_t qty,
                                       targetsMeta_t *out);

/**
 * @brief Parse the signed section of Uptane Augmented Metadata.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20313">USDM-20313</a>
 *
 * The SDK Utility SHALL provide an API to parse the signed section of
 * Uptane Augmented Metadata
 *
 * @param signedStr [in,out] String pointer with signed section of Uptane Augmented Metadata.
 *  String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseUptAugmentedMetadata(str_t signedStr,
                                         size_t length,
                                         util_json_t mem[],
                                         uint16_t qty,
                                         augMeta_t *out);

/**
 * @brief Construct the signed section of the OTAmatic SDP Config Sync request.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20317">USDM-20317</a>
 *
 * The SDK Utility SHALL provide an API to construct the signed section of
 *  the OTAmatic SDP Config Sync request
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param syncCheckInfo [in] A data structure for constructing JSON string.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateSyncCheckBody(const syncCheck_t *const syncCheckInfo,
                                   str_t buf, size_t bufLen);

/**
 * @brief Construct the signed section of the OTAmatic SDP Config Match request.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20316">USDM-20316</a>
 *
 * The SDK Utility SHALL provide an API to construct the signed section of
 * the OTAmatic SDP Config Match request
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param cfgMatchInfo [in] A data structure for constructing JSON string.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateConfigMatchBody(const cfgMatch_t *const cfgMatchInfo,
                                     str_t buf, size_t bufLen);

/**
 * @brief Construct the signed section of the OTAmatic SDP Get OTA Command request.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20075">USDM-20075</a>
 * 
 * The SDK Utility SHALL provide an API to construct the signed section of
 * the OTAmatic SDP Get OTA Command request
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param getOtaCmdInfo [in] A data structure for constructing JSON string.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateGetOtaCmdBody(const getOtaCmd_t *const getOtaCmdInfo,
                                   str_t buf, size_t bufLen);

/**
 * @brief Construct the signed section of the OTAmatic SDP Notification request.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20318">USDM-20318</a>
 *
 * The SDK Utility SHALL provide an API to construct the signed section of
 * the OTAmatic SDP Notification request
 *
 * The buffer 'buf' will be wiped/set to zero when an internal encoding error occurs in order to avoid the output
 * of partial or invalid JSON.
 *
 * @param ntf [in] A data structure for constructing JSON string.
 * @param buf [in,out] Head address of memory area for storing constructed JSON string.
 * @param bufLen [in] Length of buf.
 * @return #UTIL_NO_ERROR The constructing process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE Length of buf is not enough for storing JSON string or buf is invalid
 * @return #UTIL_ERR_INVALID_PARAMETER if data structure or structure member is invalid
 */
utilErr_t util_CreateNotificationBody(const ntf_t *const ntf, str_t buf,
                                      size_t bufLen);

/**
 * @brief Parse JSON string for DataEncryptionKey.
 *
 * Requirement tracking: <a href="https://jira.airbiquity.com/browse/USDM-20652">USDM-20652</a>
 *
 * The SDK Utility SHALL provide an API to parse a JSON string containing a Data Encryption Key
 *
 * @param str [in,out] String pointer with JSON string for DataEncryptionKey.
 *  String data will be modified.
 * @param length [in] Length of string for parsing.
 * @param mem [in,out] Array of json properties to allocate.
 * @param qty [in] Number of elements of mem
 * @param out [in,out] Pointer to the data struct for storing parsed data.
 * If #UTIL_ERR_INVALID_PARAMETER was returned, contents of this data struct is invalid.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which exceeds expected length.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected type.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing has a value which is unexpected value.
 * @return #UTIL_ERR_INVALID_PARAMETER String for parsing doesn't have mandatory key-value pair.
 * @return #UTIL_NO_ERROR The parser process was successfully.
 * @return #UTIL_ERR_NO_RESOURCE insufficiently sized mem array
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has an array which exceeds expected length.
 * @return #UTIL_ERR_NO_RESOURCE JSON payload has a data size property which exceeds array size for such data.
 */
utilErr_t util_ParseDataEncryptionKey(str_t str,
                                      size_t length,
                                      util_json_t mem[],
                                      uint16_t qty,
                                      dataEncryptionKey_t *out);

#ifdef __cplusplus
}
#endif
#endif /* SDK_UTIL_H */
