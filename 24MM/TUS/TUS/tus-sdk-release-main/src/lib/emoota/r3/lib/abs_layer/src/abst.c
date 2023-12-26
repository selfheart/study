/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifdef __cplusplus
extern "C" {
#endif


#include "abst.h"
#include "../include/abst_config.h"
#include "emoota_utility.h"
#include "spil_crypt/crypt.h"
#include "spil_file/abq_files.h"
#include "spil_file/spil_file.h"

#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int32_t abst_ParseDownloadMetadataFile(char *filepath, char out[][ABST_MAX_RELATIVE_PATH_SZ], bool isOutOfSeq) {
    int32_t linenumber = 0U;
    FILE *fp;
    char *line = NULL;
    size_t len = 0U;
    size_t cp_len = 0U;
    ssize_t read;

if( false == isOutOfSeq ){
    //printf("[ RUN      ] abq_dwnld_meta_file_002\n");
    //printf("[ RUN      ] abq_dwnld_meta_file_003\n");
    }

if( false == isOutOfSeq ){
    //printf("[ RUN      ] abq_dwnld_meta_file_004\n");
    }

    fp = fopen(filepath, "r");
    if (fp != NULL) {
        while ((read = getline(&line, &len, fp)) != -1) {
            if('\n' == line[strlen(line)-1]){
                cp_len = strlen(line) - 1;
            } else {
                cp_len = strlen(line);
            }
            abst_base64_encode(line,cp_len,out[linenumber],ABST_MAX_RELATIVE_PATH_SZ);
            linenumber++;
        }
        fclose(fp);
    }
    if (line) {
        free(line);
    }
    return linenumber;
}

int32_t abst_LoadTrustRootDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/root.json", UPTANE_BASE, UPT_TRUST_ROOT, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveTrustRootDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/root.json", UPTANE_BASE, UPT_TRUST_ROOT, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadCachedRootDirMetadata(char metadata[MAX_METADATA_SZ], uint32_t version) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/%i.root.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO, version);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }

    return ret;
}

int32_t abst_SaveCachedRootDirMetadata(char metadata[], uint32_t version) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/%i.root.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO, version);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }

    return ret;
}

void abst_DiscardObsoleteRootDir(uint32_t version) {
    char path[512] = {};
    // delete all root files with <= version
    for (uint32_t i = 1; i <= version; i++) {
        snprintf(path, 511, "%s%s%s/%i.root.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO, i);
        sf_delete_file(path);
    }
}

int32_t abst_LoadLatestTargetsDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/targets.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveLatestTargetsDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/targets.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadLatestSnapshotDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/snapshot.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveLatestSnapshotDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/snapshot.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadLatestTimestampDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/timestamp.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveLatestTimestampDirMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/timestamp.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadLatestAugmentedMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/augmented.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveLatestAugmentedMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/augmented.json", UPTANE_BASE, UPT_CACHED, UPT_DIRECTOR_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadTrustRootImgMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/root.json", UPTANE_BASE, UPT_TRUST_ROOT, UPT_IMAGE_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveTrustRootImgMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/root.json", UPTANE_BASE, UPT_TRUST_ROOT, UPT_IMAGE_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadCachedRootImgMetadata(char metadata[MAX_METADATA_SZ], uint32_t version) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/%i.root.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO, version);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveCachedRootImgMetadata(char metadata[], uint32_t version) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/%i.root.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO, version);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }

    return ret;
}

void abst_DiscardObsoleteRootImg(uint32_t version) {
    char path[512] = {};
    // delete all root files with <= version
    for (uint32_t i = 1; i <= version; i++) {
        snprintf(path, 511, "%s%s%s/%i.root.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO, i);
        sf_delete_file(path);
    }
}

int32_t abst_LoadLatestTargetsImgMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/targets.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadLatestSnapshotImgMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 512, "%s%s%s/snapshot.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveLatestSnapshotImgMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/snapshot.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_LoadLatestTimestampImgMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/timestamp.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO);
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        size_t read = fread(metadata, MAX_METADATA_SZ, 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

int32_t abst_SaveLatestTimestampImgMetadata(char metadata[MAX_METADATA_SZ]) {
    int32_t ret = -1;
    char path[512] = {};

    snprintf(path, 511, "%s%s%s/timestamp.json", UPTANE_BASE, UPT_CACHED, UPT_IMAGE_REPO);
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fwrite(metadata, strnlen(metadata, MAX_METADATA_SZ), 1U, fp);
        fclose(fp);
        ret = 0U;
    }
    return ret;
}

abst_err_t abst_get_packageStorage(packageStorage_t *ps) {
    //hardcoded
    ps->used = 1000U;
    ps->available = 82914560U;
    return ABST_NO_ERROR;
}

abst_err_t abst_get_vin(byte_t *vin, str_t campaign_vin) {
    strncpy(vin, campaign_vin, MAX_VIN_SZ);
    return ABST_NO_ERROR;
}

abst_err_t abst_get_primary_serialno(byte_t *primarySerialNum, cstr_t primary_serial) {
    strncpy(primarySerialNum, primary_serial, MAX_SERIAL_NUM_SZ);
    return ABST_NO_ERROR;
}

abst_err_t abst_get_dcm_info(dcmInfo_t *dcmInfo) {
    strncpy(dcmInfo->mobileNumber, "15551230728", MAX_MOBILE_NUMBER_SZ);
    return ABST_NO_ERROR;
}

abst_err_t abst_get_client_info(clientInfo_t *clientInfo) {
    strcpy(clientInfo->version, "v0.1");
    strcpy((char *) clientInfo->codehash, "hash");
    return ABST_NO_ERROR;
}

abst_err_t abst_get_license_info(licenseInfo_t *licenseInfo) {
    strcpy(licenseInfo[0].id, "license");
    return ABST_NO_ERROR;
}

abst_err_t abst_get_filename_of(cstr_t path, byte_t *dest, size_t dest_len) {
    if (0 > sf_filename_of(path, dest, dest_len)) {
        return ABST_NO_ERROR;
    } else {
        return ABST_ERR_INVALID_PARAMETER;
    }
}

static byte_t s_securityAttack[MAX_ATTACK_SZ] = {0};

abst_err_t abst_set_security_attack(cstr_t securityAttack) {
    memcpy(s_securityAttack, securityAttack, MAX_ATTACK_SZ);
    // Optional: persist it across power cycle
    return ABST_NO_ERROR;
}

abst_err_t abst_get_security_attack(byte_t *securityAttack) {
    memcpy(securityAttack, s_securityAttack, MAX_ATTACK_SZ);
    return ABST_NO_ERROR;
}

abst_err_t abst_do_sync_check(cstr_t sig_block, cstr_t signed_block, byte_t *response, size_t rsp_size,
                                cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca) {
    // internal call, assume parameters already checked
    abst_err_t err = ABST_NO_ERROR;
    size_t full_req_len = strlen(sig_block) + strlen(signed_block) + 100U; // plenty of extra space

    byte_t *request_buf = (byte_t *) calloc(full_req_len, 1);
    uint32_t http_status_code = 0U;

    if (NULL == request_buf) {
        err = ABST_ERR_NO_RESOURCE;
    } else {
        // Optional, if underline networking lib supports sending it in blocks, then no need for this
        snprintf(request_buf, full_req_len, "{\"signatures\":%s, \"signed\":%s}", sig_block, signed_block);
    }
    if (ABST_NO_ERROR == err) {
        err = abst_SLC_transaction(sdp_api_syncCheck, request_buf, full_req_len, response, &rsp_size, NULL,
                                   &http_status_code, sdp_url, sdp_port, rootca);
        if ((ABST_NO_ERROR == err) && (200U != http_status_code)) {
            err = ABST_ERR_TRANSACTION_FAILED;
        }
    }
    free(request_buf);
    return err;
}

abst_err_t abst_do_cfg_match(cstr_t sig_block, cstr_t signed_block, byte_t *response, size_t rsp_size,
                            cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca) {
    // internal call, assume parameters already checked
    abst_err_t err = ABST_NO_ERROR;
    size_t full_req_len = strlen(sig_block) + strlen(signed_block) + 100U; // plenty of extra space

    byte_t *request_buf = (byte_t *) calloc(full_req_len, 1);
    uint32_t http_status_code = 0U;

    if (NULL == request_buf) {
        err = ABST_ERR_NO_RESOURCE;
    } else {
        // Optional, if underline networking lib supports sending it in blocks, then no need for this
        snprintf(request_buf, full_req_len, "{\"signatures\":%s, \"signed\":%s}", sig_block, signed_block);
    }
    if (ABST_NO_ERROR == err) {
        err = abst_SLC_transaction(sdp_api_configMatch, request_buf, full_req_len, response, &rsp_size, NULL,
                                   &http_status_code, sdp_url, sdp_port, rootca);
        if ((ABST_NO_ERROR == err) && (200U != http_status_code)) {
            err = ABST_ERR_TRANSACTION_FAILED;
        }
    }
    free(request_buf);
    return err;
}

abst_err_t abst_DownloadRootDirMetadata(char metadata[], size_t maxlen, size_t *outlen, uint32_t version,
                                        uint32_t *http_status_code, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca) {

    abst_err_t err = ABST_NO_ERROR;
    byte_t *url = (byte_t *) calloc(512U, 1);

    if (NULL == url) {
        err = ABST_ERR_NO_RESOURCE;
    } else {
        // Optional, if underline networking lib supports sending it in blocks, then no need for this
        snprintf(url, 511U, "%s:%s/uptane/director/%i.root.json", cdn_url, cdn_port, version);
    }
    if (ABST_NO_ERROR == err) {
        err = abst_GETUptMetafromCDN(url, metadata, &maxlen, http_status_code, rootca);
        if ((ABST_NO_ERROR == err) && (200U == *http_status_code)) {
            *outlen = maxlen;
        } else if ((ABST_NO_ERROR == err) && (404U == *http_status_code)) {
            err = ABST_ERR_NOT_AVAILABLE;
        } else {
            err = ABST_ERR_TRANSACTION_FAILED;
        }
    }
    free(url);
    return err;
}

abst_err_t abst_DownloadRootImgMetadata(char metadata[], size_t maxlen, size_t *outlen, uint32_t version,
                                        uint32_t *http_status_code, cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca) {
    abst_err_t err = ABST_NO_ERROR;
    byte_t *url = (byte_t *) calloc(512U, 1);

    if (NULL == url) {
        err = ABST_ERR_NO_RESOURCE;
    } else {
        // Optional, if underline networking lib supports sending it in blocks, then no need for this
        snprintf(url, 511U, "%s:%s/uptane/image/%i.root.json", cdn_url, cdn_port, version);
    }
    if (ABST_NO_ERROR == err) {
        err = abst_GETUptMetafromCDN(url, metadata, &maxlen, http_status_code, rootca);
        if ((ABST_NO_ERROR == err) && (200U == *http_status_code)) {
            *outlen = maxlen;
        } else if ((ABST_NO_ERROR == err) && (404U == *http_status_code)) {
            err = ABST_ERR_NOT_AVAILABLE;
        } else {
            err = ABST_ERR_TRANSACTION_FAILED;
        }
    }
    free(url);
    return err;
}

abst_err_t abst_DownloadTimestampImgMetadata(char metadata[], size_t maxlen, size_t *outlen,
                                            cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca) {
    abst_err_t err = ABST_NO_ERROR;
    byte_t *url = (byte_t *) calloc(512U, 1);
    uint32_t http_status_code = 0U;

    if (NULL == url) {
        err = ABST_ERR_NO_RESOURCE;
    } else {
        // Optional, if underline networking lib supports sending it in blocks, then no need for this
        snprintf(url, 511U, "%s:%s/uptane/image/timestamp.json", cdn_url, cdn_port);
    }
    if (ABST_NO_ERROR == err) {
        //printf("[ RUN      ] abq_dwnld_img_meta_002\n");
        err = abst_GETUptMetafromCDN(url, metadata, &maxlen, &http_status_code, rootca);
        if ((ABST_NO_ERROR == err) && (200U == http_status_code)) {
            *outlen = maxlen;
        } else {
            err = ABST_ERR_TRANSACTION_FAILED;
        }
        //printf("[ RUN      ] abq_dwnld_img_meta_003\n");
    }
    free(url);
    return err;
}

abst_err_t abst_DownloadSnapshotImgMetadata(char metadata[], size_t maxlen, size_t *outlen, cstr_t filename,
                                            cstr_t cdn_url, cstr_t cdn_port, cstr_t rootca) {
    abst_err_t err = ABST_NO_ERROR;
    byte_t *url = (byte_t *) calloc(512U, 1);
    uint32_t http_status_code = 0U;

    if (NULL == url) {
        err = ABST_ERR_NO_RESOURCE;
    } else {
        // Optional, if underline networking lib supports sending it in blocks, then no need for this
        snprintf(url, 511U, "%s:%s/uptane/image/%s", cdn_url, cdn_port, filename);
    }
    if (ABST_NO_ERROR == err) {
        //printf("[ RUN      ] abq_dwnld_img_meta_010\n");
        err = abst_GETUptMetafromCDN(url, metadata, &maxlen, &http_status_code, rootca);
        if ((ABST_NO_ERROR == err) && (200U == http_status_code)) {
            *outlen = maxlen;
        } else {
            err = ABST_ERR_TRANSACTION_FAILED;
        }
        //printf("[ RUN      ] abq_dwnld_img_meta_011\n");
    }
    free(url);
    return err;
}

static uint32_t stored_rc = 1U;
uint32_t abst_get_curr_release_counter(void) {
    return stored_rc;
}

void abst_set_curr_release_counter(uint32_t rc) {
    stored_rc = rc;
}

#ifdef __cplusplus
}
#endif
