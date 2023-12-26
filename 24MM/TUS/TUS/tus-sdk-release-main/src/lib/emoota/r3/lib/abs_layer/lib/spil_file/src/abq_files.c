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
 * @file abq_files.c
 * @date Sep 12, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 */

/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <spil_file/abq_files.h>
#include <ontrac/ontrac/mem_usage.h>
#include <ontrac/unicode/utf8_utils.h>

#if defined(INSTALL_BASE)

byte_t OTAMATIC_RESOURCES_DIR[PLATFORM_MAX_FILE_PATH_LEN] = INSTALL_BASE; // "/data/otamatic"
byte_t OTAMATIC_CONFIG_DIR[PLATFORM_MAX_FILE_PATH_LEN] = INSTALL_BASE "/aqconfig"; //"/data/aqconfig"

#else /* defined(INSTALL_BASE)/!defined(INSTALL_BASE) */

#if defined(_WIN32) && !defined(__SYMBIAN32__)
byte_t OTAMATIC_RESOURCES_DIR[PLATFORM_MAX_FILE_PATH_LEN] = "resources";
byte_t OTAMATIC_CONFIG_DIR[PLATFORM_MAX_FILE_PATH_LEN] = "conf";
#else
byte_t OTAMATIC_RESOURCES_DIR[PLATFORM_MAX_FILE_PATH_LEN] = "./"; // "/data/otamatic"
byte_t OTAMATIC_CONFIG_DIR[PLATFORM_MAX_FILE_PATH_LEN] = "./aqconfig"; //"/data/aqconfig"
#endif

#endif /* !defined(INSTALL_BASE) */

err_t abq_file_get_size(cstr_t root_dir, cstr_t file, size_t *size) {
    sf_info_t info = {0};
    err_t retval = EXIT_SUCCESS;
    byte_t filepath[PLATFORM_MAX_FILE_PATH_LEN];
    int32_t pathlen = sf_path_join(root_dir, file,
            filepath, sizeof(filepath), abq_file_sys_path);
    if (0 >= pathlen) {
        retval = (err_t) SF_ERR_INVAL_PARAMETER;
    } else {
        retval = sf_get_file_info(filepath, &info);
        if (EXIT_SUCCESS != retval) {
            // Try to resolve errno into retval
            retval = abq_status_take(retval);
//            (void)sf_path_resolve(filepath, filepath, sizeof(filepath))
//            ABQ_ERROR_MSG(filepath)
        } else {
            if(NULL != size) {
                *size = info.size;
            }
        }
    }
    return retval;
}

err_t abq_file_write(cstr_t root_dir, cstr_t file,
        const uint8_t *buf, size_t size) {
    size_t written = 0;
    sf_file_t fp = {0};
    err_t retval = EXIT_SUCCESS;
    byte_t filepath[PLATFORM_MAX_FILE_PATH_LEN];
    int32_t start=0, toklen=-1, pathlen = sf_path_localize(root_dir,
            filepath, sizeof(filepath), abq_file_sys_path);
    if (0 >= pathlen) {
        retval = (err_t) SF_ERR_INVAL_PARAMETER;
    } else {
        toklen = utf8_end_of_token(&file[start], -1,  "/\\", -1);
        while((0 <= toklen) && (EXIT_SUCCESS == retval)) {
            if (0 != toklen) {
                // check that parent folder exists
                if(0 == sf_exists(filepath)) {
                    retval = sf_mkdir(filepath);
                }
                // Check that there is enough space in buffer for token, delimiter, and a terminator
                if(((int32_t)sizeof(filepath)-pathlen) <= (2+toklen)) {
                    retval = EOVERFLOW;
                } else {
                    // Delimit each path components ...
                    filepath[pathlen] = AQ_PATH_SEPARATOR;
                    pathlen++;
                    // ... and append the token to the filepath
                    bytes_copy(&filepath[pathlen], &file[start], (size_t)toklen);
                    pathlen+=toklen;
                    // finally, terminate it
                    filepath[pathlen] = '\0';
                }
            }
            // skip over
            start += toklen;
            if('\0' == file[start]) {
                break;
            } else {
                start += 1; // skip over delimiter character
                toklen = utf8_end_of_token(&file[start], -1,  "/\\", -1);
            }
        }
        if (EXIT_SUCCESS != retval) {
            // return error as is
        } else if(0 > toklen){
            retval = abq_status_take(EIO);
        } else {
            retval = sf_open_file(filepath, SF_MODE_CREATE, &fp); // & SF_MODE_TRUNCATE ?
            if (EXIT_SUCCESS != retval) {
                // Return error as is
//              (void)sf_path_resolve(filepath, filepath, sizeof(filepath))
//              ABQ_ERROR_MSG("could not open file for writing\n")
//              ABQ_ERROR_MSG(filepath)
            } else {
                retval = sf_writeBuffer(&fp, buf, size, &written);
            }
            (void) sf_close(&fp);
        }
    }
    return retval;
}

err_t abq_file_read(cstr_t root_dir, cstr_t file,
        uint8_t *buf, size_t size, size_t *readlen) {
    sf_file_t fp = {0};
    err_t retval = EXIT_SUCCESS;
    byte_t filepath[PLATFORM_MAX_FILE_PATH_LEN];
    int32_t pathlen = sf_path_join(root_dir, file,
            filepath, sizeof(filepath), abq_file_sys_path);
    if (0 >= pathlen) {
        retval = (err_t) SF_ERR_INVAL_PARAMETER;
    } else {
        retval = sf_open_file(filepath, SF_MODE_READ, &fp);
        if (EXIT_SUCCESS != retval) {
            // Return error as is
//            byte_t fullpath[PLATFORM_MAX_FILE_PATH_LEN]
//            (void)sf_path_resolve(filepath, fullpath, sizeof(fullpath))
//            ABQ_ERROR_MSG("could not open file for reading\n")
//            ABQ_ERROR_MSG(fullpath)
        } else {
            retval = sf_read_buffer(&fp, buf, size, (size_t *)readlen);
        }
        (void)sf_close(&fp);
    }
    return retval;
}
