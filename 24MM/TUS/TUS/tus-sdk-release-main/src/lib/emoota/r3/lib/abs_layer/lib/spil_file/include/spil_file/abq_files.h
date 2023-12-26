/****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file abq_files.h
 * @date Sep 12, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 */

#ifndef ABQ_FILES_H_
#define ABQ_FILES_H_

#include <spil_file/spil_file.h>

extern byte_t OTAMATIC_RESOURCES_DIR[PLATFORM_MAX_FILE_PATH_LEN];
extern byte_t OTAMATIC_CONFIG_DIR[PLATFORM_MAX_FILE_PATH_LEN];

extern err_t abq_file_get_size(cstr_t root_dir, cstr_t file, size_t *size);
extern err_t abq_file_write(cstr_t root_dir, cstr_t file,
        const uint8_t *buf, size_t size);
extern err_t abq_file_read(cstr_t root_dir, cstr_t file,
        uint8_t *buf, size_t size, size_t *readlen);

static inline err_t abq_config_filesize(cstr_t filename, size_t *size) {
    return abq_file_get_size(OTAMATIC_CONFIG_DIR, filename, size);
}

static inline  err_t abq_config_write(cstr_t filename, const uint8_t *buf, size_t size) {
    return abq_file_write(OTAMATIC_CONFIG_DIR, filename, buf, size);
}

static inline  err_t abq_config_read(cstr_t filename, uint8_t *buf, size_t size, size_t *readlen){
    return abq_file_read(OTAMATIC_CONFIG_DIR, filename, buf, size, readlen);
}

static inline err_t abq_resource_filesize(cstr_t filename, size_t *size) {
    return abq_file_get_size(OTAMATIC_RESOURCES_DIR, filename, size);
}

static inline  err_t abq_resource_write(cstr_t filename, const uint8_t *buf, size_t size) {
    return abq_file_write(OTAMATIC_RESOURCES_DIR, filename, buf, size);
}

static inline  err_t abq_resource_read(cstr_t filename, uint8_t *buf, size_t size, size_t *readlen){
    return abq_file_read(OTAMATIC_RESOURCES_DIR, filename, buf, size, readlen);
}

#endif /* ABQ_FILES_H_ */
