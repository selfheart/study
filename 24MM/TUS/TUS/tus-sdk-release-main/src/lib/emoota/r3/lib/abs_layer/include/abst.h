/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifndef SDK_ABST_H
#define SDK_ABST_H

#ifdef __cplusplus
extern "C" {
#endif


#include "emoota_utility_types.h"
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

typedef enum {
     /** No error occurred */
     ABST_NO_ERROR = 0,

     /** One or more of the provided parameters to an API call
      *  are not valid
      */
     ABST_ERR_INVALID_PARAMETER = -2,

     ABST_ERR_NO_RESOURCE = -3,
     ABST_ERR_VERIFICATION_FAILED = -4,
     ABST_ERR_TRANSACTION_FAILED = -5,
     ABST_ERR_NOT_AVAILABLE = -6
} abst_err_t;


typedef enum sdp_api {
    sdp_api_configMatch,
    sdp_api_syncCheck,
    sdp_api_getOtaCmd,
    sdp_api_notification,
    sdp_api_multiInit,
    adp_api_multiComp
} sdp_api_t;


/**
 * Manages a HTTP based transaction between client and backend.
 * @param sdpApi Specifies the SLC API to use
 * @param request_buffer specifies the user request data to send. !Must be NULL terminated!
 * @param request_sz specifies the user request data length. !Currently ignored!
 * @param response_buffer specifies the buffer to copy response data to
 * @param response_sz specifies the response buffer size. Value will be overwritten with the response data size witten to response_buffer
 * @param uuid specifies the eventUUID. mandatory for use with sdpApi == sdp_api_notification
 *
 *  char output[2048] = {0};
    size_t resp_size = sizeof(output);
    char data[] = "{ \"tokens\":[2707936312] }";
    abst_BackendSendReceive(sdp_api_uptTime, data, strlen(data), output, &resp_size, NULL);
 *
 * @return ABST_NO_ERROR when transaction succeeded
 * @return ABST_ERR_TRANSACTION_FAILED when transaction failed
 */
extern abst_err_t abst_SLC_transaction(sdp_api_t sdpApi, char *request_buffer, size_t request_sz, char *response_buffer,
                                       size_t *response_sz, char *uuid, uint32_t *http_status_code,
                                       cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca);


/**
 * Downloads a file from the provided URL
 * @param url file to GET
 * @param file optional handle to store received data
 * @param dir optional directory path. only used if 'file' is not provided
 * @param filename optional file name. only used if 'file' is not provided. if 'filename' is not provided, the url filename is used
 * @return ABST_NO_ERROR when transaction succeeded
 * @return ABST_ERR_TRANSACTION_FAILED when transaction failed
 * @return ABST_ERR_NO_RESOURCE when output file is not provided and can't be created
 */
extern abst_err_t abst_GETfromCDN(const char *url, FILE *file, cstr_t const dir, cstr_t const filename, uint32_t *http_status_code, cstr_t rootca);


/**
 * Upload a file part to CDN
 * @param url upload URL specific to file part
 * @param md5 base64 encoded md5 checksum specific to file part
 * @param request_buffer data payload specific to file part
 * @param http_status_code
 * @return ABST_NO_ERROR when transaction succeeded
 * @return ABST_ERR_TRANSACTION_FAILED when transaction failed
 * @return ABST_ERR_NO_RESOURCE when output file is not provided and can't be created
 */
extern abst_err_t
abst_PUTonCDN(char *url, cstr_t md5, uint8_t *request_buffer, size_t buffer_sz, char *response_buffer, size_t response_sz, uint32_t *http_status_code, cstr_t rootca);

/**
 * Downloads a file from the provided URL
 * @param url file to GET
 * @param out output buffer
 * @param response_sz output buffer size
 * @return ABST_NO_ERROR when transaction succeeded
 * @return ABST_ERR_TRANSACTION_FAILED when transaction failed
 * @return ABST_ERR_NO_RESOURCE when output file is not provided and can't be created
 */
extern abst_err_t
abst_GETUptMetafromCDN(char *url, char *out, size_t *response_sz, uint32_t *http_status_code, cstr_t rootca);


#define ABST_MAX_RELATIVE_PATH_NUM (10U)
#define ABST_MAX_RELATIVE_PATH_SZ       (128U)
/**
 * Opens file 'filepath' and reads content line by line into 'out'
 * @return number of lines read.
 * @return 0 if error or no lines read
 */
extern int32_t abst_ParseDownloadMetadataFile(char* filepath, char out[][ABST_MAX_RELATIVE_PATH_SZ], bool isOutOfSeq);

/**
 *
 * @param metadata
 * @return Number of bytes loaded
 */
int32_t abst_LoadTrustRootDirMetadata(char metadata[]);

/**
 *
 * @param metadata
 * @param version requested version to load from cached set
 * @return Number of bytes loaded
 */
int32_t abst_LoadCachedRootDirMetadata(char metadata[], uint32_t version);
int32_t abst_LoadLatestTargetsDirMetadata(char metadata[]);
int32_t abst_LoadLatestSnapshotDirMetadata(char metadata[]);
int32_t abst_LoadLatestTimestampDirMetadata(char metadata[]);
int32_t abst_LoadLatestAugmentedMetadata(char metadata[]);

int32_t abst_LoadTrustRootImgMetadata(char metadata[]);

/**
 *
 * @param metadata
 * @param version requested version to load from cached set
 * @return Number of bytes loaded
 */
int32_t abst_LoadCachedRootImgMetadata(char metadata[], uint32_t version);
int32_t abst_LoadLatestTargetsImgMetadata(char metadata[]);
int32_t abst_LoadLatestSnapshotImgMetadata(char metadata[]);
int32_t abst_LoadLatestTimestampImgMetadata(char metadata[]);

// Update cached files and trust root
int32_t abst_SaveTrustRootDirMetadata(char metadata[]);
int32_t abst_SaveTrustRootImgMetadata(char metadata[]);

int32_t abst_SaveCachedRootDirMetadata(char metadata[], uint32_t version);
int32_t abst_SaveCachedRootImgMetadata(char metadata[], uint32_t version);

int32_t abst_SaveLatestTargetsDirMetadata(char metadata[]);
int32_t abst_SaveLatestSnapshotDirMetadata(char metadata[]);
int32_t abst_SaveLatestTimestampDirMetadata(char metadata[]);
int32_t abst_SaveLatestAugmentedMetadata(char metadata[]);
int32_t abst_SaveLatestSnapshotImgMetadata(char metadata[]);
int32_t abst_SaveLatestTimestampImgMetadata(char metadata[]);

void abst_DiscardObsoleteRootDir(uint32_t version);
void abst_DiscardObsoleteRootImg(uint32_t version);

#ifndef CRYPT_LOAD_ROOTCA_FROM_FILE
abst_err_t abst_crypt_init(cstr_t root_ca, size_t root_ca_len);
#else
abst_err_t abst_crypt_init(void);
#endif
void abst_GenerateSignatureSHA256(unsigned char *md_value, size_t md_len, sigInfo_t *sig) ;
void abst_CalculateDigestSHA256(unsigned char *md_value, const char *str, const unsigned long str_len );
void abst_SHA256Init(void **ctx);
void abst_SHA256Update(void *ctx, cbyte_t *data, size_t len);
void abst_SHA256Finalize(void *ctx, unsigned char *md_value);
abst_err_t abst_VerifySignature(const keyInfo_t *key, const sigInfo_t *sig, const unsigned char md_value[EVP_MAX_MD_SIZE]);
size_t abst_base64_decode(cstr_t source, size_t source_len, byte_t *dest, size_t dest_len);
size_t abst_base64_encode(cstr_t source, size_t source_len, byte_t *dest, size_t dest_len);
// returns ABST_ERR_TRANSACTION_FAILED if failed
abst_err_t abst_decrypt_dek(const byte_t *encrypted, size_t ilen,
                            uint8_t *output, size_t buflen, size_t *olen, padding_t padding);

// Functions for Uptane to get platform dependent information
abst_err_t abst_get_trusted_time(uint64_t *time_ms);
abst_err_t  abst_get_uuid(char *uuid, size_t buf_size);
abst_err_t abst_get_packageStorage(packageStorage_t *ps);
// buffer size should be MAX_VIN_SZ
abst_err_t abst_get_vin(byte_t *vin, str_t campaign_vin);
// buffer size should be MAX_SERIAL_NUM_SZ
abst_err_t abst_get_primary_serialno(byte_t *primarySerialNum, cstr_t primary_serial);
abst_err_t abst_get_dcm_info(dcmInfo_t *dcmInfo);
abst_err_t abst_get_client_info(clientInfo_t *clientInfo);
// buffer size should be sizeof(licenseInfo_t) * MAX_TOTAL_LICENSE_INFO
abst_err_t abst_get_license_info(licenseInfo_t *licenseInfo);
abst_err_t abst_get_filename_of(cstr_t path, byte_t *dest, size_t dest_len);

abst_err_t abst_set_security_attack(cstr_t securityAttack);
// buffer size should be MAX_ATTACK_SZ
abst_err_t abst_get_security_attack(byte_t *securityAttack);

abst_err_t abst_do_sync_check(cstr_t sig_block, cstr_t signed_block, byte_t *response, size_t rsp_size, cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca);
abst_err_t abst_do_cfg_match(cstr_t sig_block, cstr_t signed_block, byte_t *response, size_t rsp_size, cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca);
abst_err_t abst_DownloadRootDirMetadata(char metadata[], size_t maxlen, size_t *outlen, uint32_t version, uint32_t *http_status_code, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca);
abst_err_t abst_DownloadRootImgMetadata(char metadata[], size_t maxlen, size_t *outlen, uint32_t version, uint32_t *http_status_code, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca);
abst_err_t abst_DownloadTimestampImgMetadata(char metadata[], size_t maxlen, size_t *outlen, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca);
abst_err_t abst_DownloadSnapshotImgMetadata(char metadata[], size_t maxlen, size_t *outlen, cstr_t filename, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca);
uint32_t abst_get_curr_release_counter(void);
void abst_set_curr_release_counter(uint32_t rc);

abst_err_t abst_getImageFileName(char* out, const targetAndCustom_t *target, const changeEvent_t *ce, bool_t encrypted);
abst_err_t
abst_DownloadImage(const char *url, const targetAndCustom_t *target, const changeEvent_t *ce, bool_t encrypted, cstr_t rootca);
// Let abst handle image decryption/verifying since it could be stored in platform dependent way
abst_err_t abst_VerifyFile(const uint8_t *const digest, cstr_t const filepath, const size_t filesize);
abst_err_t abst_VerifyImage(const targetAndCustom_t *target, const changeEvent_t *ce, bool_t encrypted);
abst_err_t abst_DecryptImage(const targetAndCustom_t *target, const changeEvent_t *ce, const byte_t *dek, size_t deklen,
                             const byte_t *iv, size_t ivlen);

#ifdef __cplusplus
}
#endif

#endif /* SDK_ABST_H */
