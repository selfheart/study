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
 *  Copyright (c) 2017 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */

#include <ontrac/util/abq_item_pool.h>
#include <spil_file/spil_file.h>
#include <spil_file/ontrac/files.h>

typedef struct {
    uint32_t mode;
    uint64_t size;
    uint8_t type;
    uint8_t name[100];
} tar_header_t;

/* internal tar structure */
typedef struct {
    sf_file_t sf_tar_handle;
    uint64_t pos;
    uint64_t remaining_data;
    uint64_t last_header;
} sf_tar_t;

/* posix tar header structure */
typedef struct posix_header {
    /* byte offset */
    byte_t name[100];               /*   0 */
    byte_t mode[8];                 /* 100 */
    byte_t uid[8];                  /* 108 */
    byte_t gid[8];                  /* 116 */
    byte_t size[12];                /* 124 */
    byte_t mtime[12];               /* 136 */
    byte_t chksum[8];               /* 148 */
    byte_t typeflag;                /* 156 */
    byte_t linkname[100];           /* 157 */
    byte_t magic[6];                /* 257 */
    byte_t version[2];              /* 263 */
    byte_t uname[32];               /* 265 */
    byte_t gname[32];               /* 297 */
    byte_t devmajor[8];             /* 329 */
    byte_t devminor[8];             /* 337 */
    byte_t prefix[155];             /* 345 */
    byte_t padding[12];             /* 500 */
    /* 512 */
} tar_raw_header_t;

/**
 * Helper function to copy data from raw tar header to minimal tar header
 * @param h
 * @param rh
 */
static void raw_to_header(tar_header_t *h, const tar_raw_header_t *rh) {
    int64_t value = 0;
    if ((NULL != h) && (NULL != rh)) {
        (void) utf8_read_int(rh->size, sizeof(rh->size), &value, OCTAL_RADIX);
        h->size = (uint64_t) value;
        (void) utf8_read_int(rh->mode, sizeof(rh->mode), &value, DECIMAL_RADIX);
        h->mode = (uint32_t) value;
        h->type = (uint8_t) rh->typeflag;

        (void) bytes_copy((str_t) h->name, (cstr_t) rh->name, sizeof(rh->name));
    }
}

/**
 * Helper function which reads the next file header within a tar-file
 * @param sf_tar structure holding tar-file properties
 * @param h parsed header structure holding file properties
 * @retval 0 if successful
 * @retval <0 if an error occurred

 */
static int32_t tar_read_header(sf_tar_t *sf_tar, tar_header_t *h) {
    int32_t rvalue = EUNSPECIFIED;
    tar_raw_header_t rh;
    bytes_set(&rh, '\0', sizeof(tar_raw_header_t));
    size_t byt_read = 0;
    if (sf_tar != NULL) {
        if (h != NULL) {
            sf_tar->last_header = sf_tar->pos;

            /* Read raw header */
            if (0 == sf_read_buffer(&sf_tar->sf_tar_handle, ptr2uint8(&rh), sizeof(tar_raw_header_t), &byt_read)) {
                sf_tar->pos += byt_read;
                if ((byt_read == sizeof(tar_raw_header_t))
                        && ('\0' != rh.name[0])) {
                    raw_to_header(h, &rh);
                    rvalue = 0;
                }
            }
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_tar_extract(cstr_t tarfile, cstr_t target_dir) {
    int32_t rvalue = 0;
    sf_tar_t sf_tar;
    tar_header_t tar_header;
    sf_file_t new_file_handle;
    byte_t new_file_path[PLATFORM_MAX_FILE_PATH_LEN];

    (void) bytes_set(&sf_tar, '\0', sizeof(sf_tar_t));
    (void) bytes_set(&tar_header, '\0', sizeof(tar_header_t));
    (void) bytes_set(&new_file_handle, '\0', sizeof(sf_file_t));
    (void) bytes_set(new_file_path, '\0', sizeof(new_file_path));

    /* Open file */
    if (0 == sf_open_file(tarfile, SF_MODE_READ, &sf_tar.sf_tar_handle)) {
        while ((tar_read_header(&sf_tar, &tar_header)) == 0) {
            if ((0 != utf8_compare_insensitive((cstr_t) tar_header.name, "./", -1)) &&
                    (0 != utf8_compare_insensitive((cstr_t) tar_header.name, "../", -1))) {
                ABQ_INFO_MSG_Z("extracting file: ", (byte_t * ) tar_header.name);
                byte_t *tgtname = (byte_t *) tar_header.name;
                if ((tgtname[0] == '.') && (tgtname[1] == '/')) {
                    tgtname = &tgtname[2]; // skip the leading "./"
                }
                if (NULL == target_dir) {
                    // relative path not allowed
                    rvalue = EUNSPECIFIED;
                } else {
                    if (sf_mkdir(target_dir) == 0) {
                        if (0 > sf_path_join(target_dir, (cstr_t) tgtname,
                                new_file_path, sizeof(new_file_path), abq_file_sys_path)) {
                            // ABQ_ERROR_MSG("TAR File extraction: failed to create sys path")
                            rvalue = EUNSPECIFIED;
                        }
                    }
                }

                if (0 != rvalue) {
                    // error case
                } else if (0 == sf_open_file(new_file_path, SF_MODE_CREATE, &new_file_handle)) {
                    if (1 == sf_exists(new_file_path)) {
                        size_t read_sz = 0;
                        size_t bytes_read = 0, bytes_written = 0;
                        uint8_t buffer[1024];
                        (void) bytes_set(buffer, '\0', sizeof(buffer));
                        sf_tar.remaining_data = tar_header.size;
                        bool_t do_break = false;
                        while (sf_tar.remaining_data > 0U) {
                            // read 1024 bytes or remaining from current tar file
                            read_sz = (sf_tar.remaining_data > sizeof(buffer))
                                    ? sizeof(buffer) : (size_t) sf_tar.remaining_data;

                            /* read data part of current file in tar */
                            if (0 != sf_read_buffer(&sf_tar.sf_tar_handle, buffer, read_sz, &bytes_read)) {
                                //ABQ_ERROR_MSG("Failed to read from file")
                            }
                            sf_tar.remaining_data -= bytes_read;

                            if (bytes_read != read_sz) {
                                // ABQ_ERROR_MSG("TAR File extraction: failed to read data")
                                rvalue = EUNSPECIFIED;
                                do_break = true;
                            }
                            if (false == do_break) {
                                /* write buffer to new file */
                                if (0 != sf_writeBuffer(&new_file_handle, buffer, bytes_read, &bytes_written)) {
                                    // ABQ_ERROR_MSG("Failed to write to file")
                                }

                                if (bytes_read != bytes_written) {
                                    // ABQ_ERROR_MSG("TAR File extraction: mismatch in read/write data")
                                    rvalue = EUNSPECIFIED;
                                    do_break = true;
                                }
                            }
                            if (true == do_break) {
                                break;
                            }
                        }
                    } else {
                        // ABQ_ERROR_MSG("TAR File extraction: failed to create new file")
                    }
                    // close new file
                    (void) sf_close(&new_file_handle);
                } else {
                    // ABQ_ERROR_MSG("TAR File extraction: failed to create new file")
                    rvalue = EUNSPECIFIED;
                }
            } else {
                ABQ_INFO_MSG_Z("skipping file: ", (byte_t * ) tar_header.name);
            }

            if (0 != rvalue) {
                break;
            } else {
                /* seek to next file pos in tar */
                uint8_t padding_block = (((sf_tar.last_header + tar_header.size) % 512U) > 0U) ? (uint8_t) 1U
                                                                                                 :
                                                                                                 (uint8_t) 0U;
                uint64_t offset =
                        ((((sf_tar.last_header + tar_header.size) / 512U) + 1U) + padding_block) * sizeof(tar_raw_header_t);
                size_t seeked = 0;
                if (0 == sf_seek_absolute(&sf_tar.sf_tar_handle, offset, &seeked)) {
                    sf_tar.pos = (uint32_t) seeked;
                }
            }
        }
    }

    /* Close archive */
    (void) sf_close(&sf_tar.sf_tar_handle);

    return rvalue;
}

bool_t sf_get_is_tar(cstr_t fileName) {
    bool_t rvalue = false;
    sf_file_t fd;
    byte_t buf[5] = {0};
    size_t bytes_read = 0;
    size_t seeked = 0;
    (void) bytes_set(buf, '\0', sizeof(buf));
    (void) bytes_set(&fd, '\0', sizeof(sf_file_t));

    if (sf_open_file(fileName, SF_MODE_READ, &fd) == 0) {
        if (sf_seek_absolute(&fd, 0x101U, &seeked) == 0) {
            if (sf_read_buffer(&fd, (uint8_t *) buf, 5U, &bytes_read) == 0) {
                if (utf8_compare_exact((cstr_t) buf, (cstr_t) "ustar", 5) == 0) {
                    rvalue = true;
                }
            }
        }
        (void) sf_close(&fd);
    }
    return rvalue;
}
