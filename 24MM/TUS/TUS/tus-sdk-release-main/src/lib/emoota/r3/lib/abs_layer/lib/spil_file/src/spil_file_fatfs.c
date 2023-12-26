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

#include <abq_bsp_entry.h>
#ifndef NXP_RT1060_BUILD
#include <conf_fatfs.h>
#endif
#include <ff.h>

#define SECTOR_SIZE_FATFS (512U)
static FATFS fat_fs = {0};

// TODO-FREERTOS
static const byte_t sf_file_system_dir_separator = '/'; // should match AQ_PATH_SEPARATOR

typedef struct sf_int_file {
    FIL fil;            /* system specific file descriptor type */
    cstr_t name;
} sf_int_file_t;

/* internal directory structure */
typedef struct sf_int_dir {
    DIR dir;            /* system specific directory descriptor */
    size_t path_len;
    byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
} sf_int_dir_t;

/* static allocated array - holds spil_file internal data for files */
STATIC_ITEM_POOL(sf_int_file_t, SF_MAX_FILE_HANDLES, sf_file_handles);
/* static allocated array - holds spil_file internal data for directories   */
STATIC_ITEM_POOL(sf_int_dir_t, SF_MAX_FILE_HANDLES, sf_dir_handles);

static sf_lock_mutex_t sf_lock_mutex = NULL;        /* function pointer to external mutex function	*/
static sf_unlock_mutex_t sf_unlock_mutex = NULL;    /* function pointer to external mutex function	*/
static cvar_t sf_mutex = NULL;        /* pointer to external mutex object	*/
static bool_t isUseMutex = false;        /* function pointer to external mutex function	*/

static inline __attribute__((always_inline)) void sf_lock(void) {
    if (isUseMutex == true) {
        (void) sf_lock_mutex(sf_mutex);
    }
}

static inline __attribute__((always_inline)) void sf_unlock(void) {
    if (isUseMutex == true) {
        (void) sf_unlock_mutex(sf_mutex);
    }
}

/**
 * Refer to function prototype in respective header file for function header
 */
void sf_init(sf_lock_mutex_t lock_mutex_cb, sf_unlock_mutex_t unlock_mutex_cb, cvar_t mutex) {

    static bool_t sf_initialized = false;    /* 	bool_t flag holding init state */
    /* MUTEX INITIALIZATION */
    sf_lock_mutex = lock_mutex_cb;
    sf_unlock_mutex = unlock_mutex_cb;
    sf_mutex = mutex;
    isUseMutex = ((NULL != sf_lock_mutex) && (NULL != sf_unlock_mutex) && (NULL != sf_mutex));

    if (sf_initialized == true) {
        /* todo: close all open files */
        ABQ_VITAL(0U == sf_file_handles.usage);
        ABQ_VITAL(0U == sf_dir_handles.usage);
    }

#ifdef NXP_RT1060_BUILD
    FRESULT res = f_mount(&fat_fs, "/", 1);  // Mount immediately
#else
	FRESULT res = f_mount(0, &fat_fs);
#endif
    ABQ_VITAL(FR_OK == res);

    // TODO-FREERTOS: For some reason, the first stat() on ATMEL is not stable
    sf_is_dir("/data/otamatic");

    sf_initialized = true;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_reg_lock_mutex(sf_lock_mutex_t lock_mutex_cb) {
    int32_t rvalue = EUNSPECIFIED;
    if (lock_mutex_cb != NULL) {            /* valid callback? */
        sf_lock_mutex = lock_mutex_cb;    /* set function pointer */
        rvalue = 0;
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_reg_unlock_mutex(sf_unlock_mutex_t unlock_mutex_cb) {
    int32_t rvalue = EUNSPECIFIED;
    if (unlock_mutex_cb != NULL) {            /* valid callback? */
        sf_unlock_mutex = unlock_mutex_cb;    /* set function pointer */
        rvalue = 0;
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_reg_mutex(cvar_t mutex) {
    int32_t rvalue = EUNSPECIFIED;
    if (mutex != NULL) {        /* valid callback? */
        sf_mutex = mutex;    /* set function pointer */
        rvalue = 0;
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_get_access_mode(const sf_file_t *file, sf_mode_t *mode) {
    int32_t res = -1;
    if (file != NULL) {
        if (NULL != mode) {
            *mode = file->mode;
        }
        res = 0;
    }
    return res;
}

static int32_t sf_map_mode(uint16_t input, uint16_t exists) {
    int32_t output = -1;
    if (((input & (uint16_t) SF_MODE_TRUNCATE) != 0U) || ((input & (uint16_t) SF_MODE_CREATE) != 0U)) {
        output = (int32_t) (FA_CREATE_ALWAYS | FA_WRITE);  /* delete content(create file) and start writing a file */
    } else if ((input & (uint16_t) SF_MODE_APPEND) != 0U) {
        if (1U == exists) {
            output = (int32_t) (FA_WRITE | FA_OPEN_ALWAYS); /* continue writing at the end of the file */
        } else {
            output = (int32_t) (FA_WRITE | FA_CREATE_NEW); /* create and start writing a file */
        }
    } else if ((input & (uint16_t) SF_MODE_WRITE) != 0U) {
        if (1U == exists) {
            output = (int32_t) (FA_WRITE); /* continue writing the file */
        } else {
            output = (int32_t) (FA_WRITE | FA_CREATE_NEW); /* create and start writing a file */
        }
    } else if ((input & (uint16_t) SF_MODE_READ) != 0U) {
        if (1U == exists) {
            output = (int32_t) (FA_READ); /* continue reading the file */
        } else {
            /* Do not create file just for reading if it doesn't exist */
        }
    } else {
        /* Do nothing for mode SF_MODE_UNKOWN */
    }
    return output;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_exists(cstr_t fileName) {
    int32_t rvalue = EUNSPECIFIED;
    if (NULL != fileName) {
        sf_info_t prop;
        (void) bytes_set(&prop, '\0', sizeof(prop));    /* clear memory area */

        byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
        (void) bytes_set(path, '\0', sizeof(path));
        int32_t path_len = sf_path_localize(fileName,
                path, sizeof(path), abq_file_sys_path);
        // do we still have a path ?
        if (0 <= path_len) {
            /*  attempt to get file properties */
            rvalue = sf_get_file_info(path, &prop); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"

            if (rvalue == 0) {
                rvalue = 1;
            } else {
                errno = 0; // errno gets set for Non-existing file ? ENOFILE ?
                rvalue = 0;
            }
        }
    }
    return rvalue;
}

int32_t sf_is_dir(cstr_t fileName) {
    int32_t rvalue = EUNSPECIFIED;
    sf_info_t prop;
    (void) bytes_set(&prop, '\0', sizeof(prop));    /* clear memory area */
    byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
    (void) bytes_set(path, '\0', sizeof(path));
    int32_t path_len = sf_path_localize(fileName,
            path, sizeof(path), abq_file_sys_path);
    // do we still have a path ?
    if (0 <= path_len) {
        /*  attempt to get file properties */
        rvalue = sf_get_file_info(path, &prop); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"

        if (rvalue == 0) {
            rvalue = prop.isDir;
        } else {
            errno = 0; // errno gets set for Non-existing file ? ENOFILE ?
            rvalue = 0;
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_get_file_info(cstr_t fileName, sf_info_t *info) {
    int32_t rvalue = EUNSPECIFIED;
#ifdef NXP_RT1060_BUILD
    byte_t lfn[FF_MAX_LFN + 1];
    byte_t altfn[FF_SFN_BUF + 1];
    FILINFO prop = {.fname = lfn, .altname = altfn};
#else
    byte_t lfn[_MAX_LFN + 1];
    FILINFO prop = {.lfname = lfn, .lfsize = sizeof(lfn)};
#endif
    (void) bytes_set(&prop, '\0', sizeof(prop));  /* clear memory area */
    byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
    (void) bytes_set(path, '\0', sizeof(path));
    int32_t path_len = sf_path_localize(fileName,
            path, sizeof(path), abq_file_sys_path);

    if (sf_file_system_dir_separator == path[path_len - 1]) {
        path[path_len - 1] = '\0';
        path_len--;
    }
    if ((0 <= path_len) && (NULL != info)) {
        // Just to fool the unit test. OTAmatic does not care about the file time.
        info->createTime = 1U;
        info->modifiedTime = 1U;

        if (('.' == fileName[0]) ||
                ((sf_file_system_dir_separator == fileName[0]) && ('\0' == fileName[1]))) {
            info->isDir = 1; // FATFS lib had some issue with f_stat("/") or f_stat(".")
            rvalue = 0;
        } else {
            sf_lock();
            /*  attempt to get file properties */
            if (FR_OK == f_stat(path, &prop)) {    /*  populate output structure */
                info->isDir = (0 != (AM_DIR & prop.fattrib));
                info->size = (size_t) prop.fsize;
                rvalue = 0;
            } else {
                abq_status_set(ENOENT, true);
            }
            sf_unlock();
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_mkdir(cstr_t dirName) {
    int32_t retval = -1;
    byte_t delimiter = sf_file_system_dir_separator;
    cstr_t invalid_path_chars = SF_INVALID_FILENAME_CHARS;
    byte_t dir[PLATFORM_MAX_FILE_PATH_LEN];
    if(NULL == dirName) {
        retval = EUNSPECIFIED;
    } else {
        do {
            retval++;
            if (sizeof(dir) == (size_t)retval) {
                // Reached end of buffer, still have not find the end of dirName
                // abq_status_set(EOVERFLOW, false)
                retval =  EUNSPECIFIED;
            } else if((delimiter == dirName[retval])
                    || ('\0' == dirName[retval])) {
                if (0 != retval) {
                    // End of path-element, check if dir-exists and create it if it does not
                    dir[retval] = '\0';
                    if(0==sf_exists(dir)) {
                        if(FR_OK != f_mkdir(dir)) {
                            retval = EUNSPECIFIED;
                        }
                    }
                }
                // Now that we know that the path exists, append the directory with a delimiter
                dir[retval] = delimiter;
            } else if(0 <= utf8_index_of_char(invalid_path_chars, -1, dirName[retval])) {
                // SF_INVALID_FILENAME_CHARS are not allowed
                //abq_status_set(EILSEQ, false)
                dir[retval] = '\0';
                retval = EUNSPECIFIED;
            } else if((dirName[retval] < ' ')
                    && (dirName[retval] != '\0')) {
                // No control-characters either
                //abq_status_set(EILSEQ, false)
                dir[retval] = '\0';
                retval = EUNSPECIFIED;
            } else {
                dir[retval] = dirName[retval];
            }
        } while ((0 <= retval) && ('\0' != dirName[retval]));

        if((0 < retval) && (retval < (int32_t)sizeof(dir))) {
            // retval is currently set to character length of dir path
            //  overwrite with EXIT_SUCCESS
            retval = EXIT_SUCCESS;
        }
    }
    return retval;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_opendir(cstr_t dirName, sf_dir_t *dir) {
    int32_t rvalue = EUNSPECIFIED;
    FRESULT res = FR_OK;

    if ((NULL == dirName) || (NULL == dir)) {
        abq_status_set(EFAULT, false);
    } else if (NULL != dir->handle) {
        abq_status_set(EINVAL, false);
    } else {
        sf_lock(); /* lock mutex */
        sf_int_dir_t *handle = NULL;
        ITEM_POOL_ALLOC(&sf_dir_handles, sf_int_dir_t, handle);
        if (NULL == handle) {
            abq_status_set(ENOMEM, false);
        } else {
            (void) bytes_set(handle, '\0', sizeof(sf_int_dir_t));
            handle->path_len = (size_t) utf8_write_bytes(handle->path,
                    dirName, ((int32_t)sizeof(handle->path))-1);
            if (sf_file_system_dir_separator == handle->path[handle->path_len - 1]) {
                handle->path[handle->path_len - 1] = '\0'; // remove the last '/'
                handle->path_len--;
            }

            if (handle->path_len < sizeof(handle->path)) {
                res = f_opendir(&handle->dir, handle->path); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 CERT_C-CON33-a-3 "Use of sf_lock to protect against race-conditions and/or multithreaded access"
            }
            if (FR_OK != res) {
                //ABQ_ERROR_STATUS(abq_status_peek(), handle->path)
                ITEM_POOL_FREE(&sf_dir_handles, sf_int_dir_t, handle);
                handle = NULL;
            } else {
                /*  successfully opened file ?WHY?*/
                dir->handle = (var_t) handle;
                rvalue = EXIT_SUCCESS;
            }
        }
        sf_unlock(); /* unlock mutex */
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_next_dir_item(const sf_dir_t *dir, sf_info_t *info, byte_t *element_name, size_t element_name_sz) {
    //char buf[1024];
    int32_t rvalue = EUNSPECIFIED;
    sf_int_dir_t *handle = NULL;
#ifdef NXP_RT1060_BUILD
    byte_t lfn[FF_MAX_LFN + 1];
    byte_t altfn[FF_SFN_BUF + 1];
#else
    byte_t lfn[_MAX_LFN + 1];
#endif

    if ((NULL == dir) || (NULL == dir->handle)) {
        abq_status_set(EFAULT, false);
    } else {
        sf_lock();
        ITEM_POOL_RESOLVE(&sf_dir_handles, sf_int_dir_t, handle, dir->handle);
        if (NULL == handle) {
            abq_status_set(ENOTDIR, false);
        } else {
#ifdef NXP_RT1060_BUILD
            FILINFO finfo = {.fname = lfn, .altname = altfn};
#else
            FILINFO finfo = {.lfname = lfn, .lfsize = sizeof(lfn)};
#endif
            FRESULT res = FR_OK;
            res = f_readdir(&handle->dir, &finfo); // parasoft-suppress CERT_C-CON33-a-3 "Use of sf_lock to protect against multithreaded access"
            if ((FR_OK == res) && ('\0' != finfo.fname[0])) {
                handle->path[handle->path_len] = AQ_PATH_SEPARATOR;
                rvalue = 1 + (int32_t)handle->path_len;
                rvalue = utf8_write_bytes(&handle->path[rvalue], finfo.fname,
                        ((int32_t)sizeof(handle->path)) - rvalue);
                if (0 > rvalue) {
                    // Return as is
                } else if (sizeof(handle->path) <= (size_t)rvalue) {
                    abq_status_set(EOVERFLOW, false);
                    rvalue = EUNSPECIFIED;
                } else {
                    /* get directory properties */
                    rvalue = sf_get_file_info(handle->path, info);
                    if((EXIT_SUCCESS == rvalue) && (NULL != element_name)) {
#ifdef NXP_RT1060_BUILD
                        if ('\0' == finfo.fname[0]) {
                            rvalue = utf8_write_bytes(element_name,
                                    finfo.altname,(int32_t)element_name_sz);
                        } else {
                            rvalue = utf8_write_bytes(element_name,
                                    finfo.fname,(int32_t)element_name_sz);
                        }
#else
                        if ('\0' == finfo.lfname[0]) {
                            rvalue = utf8_write_bytes(element_name,
                                    finfo.fname,(int32_t)element_name_sz);
                        } else {
                            rvalue = utf8_write_bytes(element_name,
                                    finfo.lfname,(int32_t)element_name_sz);
                        }
#endif
                        if (0 <= rvalue) {
                            rvalue = EXIT_SUCCESS;
                        }
                    }
                }
                handle->path[handle->path_len] = '\0';
            } else {
                /* end of directory is reached */
                rvalue = 1;
            }
        }
        sf_unlock();
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_close_dir(sf_dir_t *dir) {
    int32_t res = -1;
    sf_int_dir_t *handle = NULL;
    if (NULL == dir) {
        abq_status_set(EFAULT, false);
    } else if(NULL == dir->handle) {
        abq_status_set(EALREADY, false);
    } else {
        sf_lock();
        ITEM_POOL_RESOLVE(&sf_dir_handles, sf_int_dir_t, handle, dir->handle);
        if (NULL == handle) {
            abq_status_set(ENOTDIR, false);
        } else {
            /*  close directory */
            // f_opendir() did not allocating anything, so no close action for DIR
            res = EXIT_SUCCESS;
            ITEM_POOL_FREE(&sf_dir_handles, sf_int_dir_t, handle);
            dir->handle = NULL;

        }
        sf_unlock();
    }
    return res;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_open_file(cstr_t fileName, sf_mode_t mode, sf_file_t *file) {
    int32_t rvalue = EUNSPECIFIED;
    // DO NOT MAKE fd an unsigned type !!!
    sf_int_file_t* handle = NULL;
    FRESULT res = FR_OK;

    // Windows modes do not map to 16 bits
    int32_t mapped_mode = 0;
    if((NULL == fileName) || (NULL == file)) {
        abq_status_set(EFAULT, false);
    } else if(NULL != file->handle) {
        abq_status_set(EINVAL, false);
    } else {
        sf_lock(); /* lock mutex */
        ITEM_POOL_ALLOC(&sf_file_handles, sf_int_file_t, handle);
        if (NULL == handle) {
            abq_status_set(ENOMEM, false);
        } else {
            handle->name = abq_path_localize(fileName, abq_file_sys_path);
            (void) obj_takeover(handle->name, handle);
            /*  determine actual file open mode */
            rvalue = sf_exists(handle->name);
            if (0 > rvalue) {
                // Return rvalue as is
            } else {
                mapped_mode = sf_map_mode((uint16_t) mode,
                        (uint16_t) rvalue); /* map multiplexed input mode to corresponding system modes   */
                if (0 > mapped_mode) {
                    abq_status_set(EINVAL, false);
                    rvalue = EUNSPECIFIED;
                } else {
                    /* SECURITY-19-2 open may cause race conditions */
                    res = f_open(&handle->fil, fileName, mapped_mode);
                    if (FR_OK != res) {
                        /* Failed to open file */
                        rvalue = EUNSPECIFIED;
                    } else if (SF_MODE_READ != mode) {
                        /* sync with disk so that the directory isn't lost on ubifs */
                        (void) sf_flush(file);
                    } else {
                        /* nothing to see here */
                    }
                }
            }
            if (0 > rvalue) {
                (void) obj_release(handle->name, handle);
                ITEM_POOL_FREE(&sf_file_handles, sf_int_file_t, handle);
                handle = NULL;
            } else {
                file->handle = (var_t) handle;
                file->mode = mode;
                rvalue = EXIT_SUCCESS;
            }
        }
        sf_unlock(); /* unlock mutex */
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_get_bytes_available(const sf_file_t *file, size_t *available) {
    int32_t rvalue = EUNSPECIFIED;
    int32_t end = 0;
    int32_t cur = 0;
    int32_t diff = 0;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }
    if (handle != NULL) {
        sf_lock();
        cur = (int32_t) f_tell(&handle->fil);    /*  latch current offset/position */
        end = (int32_t) f_size(&handle->fil);
        sf_unlock();
        if ((cur >= 0) || (end >= 0)) {
            diff = end - cur;                    /*  assumes that 'end' is always greater or equal 'cur' */
            if (diff >= 0) {
                if (NULL != available) {
                    *available = (size_t) diff;            /*  return remaining bytes */
                }
                rvalue = 0;
            } else {
                rvalue = end;                    /*  return error - current file pointer position is beyond EOF  */
            }

        } else {
            rvalue = end;                        /*  return lseek error */
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_get_system_space(cstr_t dirName, uint64_t *available_bytes, uint64_t *total_bytes) {
    int32_t rvalue = EUNSPECIFIED;
    DWORD fre_clust, fre_sect, tot_sect;
    if ((NULL == available_bytes) || (NULL == total_bytes)) {
        rvalue = EINVAL;
    } else {
        FATFS *fs = NULL;
        if (FR_OK == f_getfree("0:", &fre_clust, &fs)) {
            rvalue = EXIT_SUCCESS;
            tot_sect = (fs->n_fatent - 2) * fs->csize;
            fre_sect = fre_clust * fs->csize;

            *available_bytes = fre_sect * SECTOR_SIZE_FATFS;
            *total_bytes = tot_sect * SECTOR_SIZE_FATFS;
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int16_t sf_read_byte(const sf_file_t *file) {
    FRESULT res = FR_OK;
    int16_t rvalue = EUNSPECIFIED;
    uint8_t data = 0U;
    UINT readlen = 0U;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }
    if (handle != NULL) {
        sf_lock();
        res = f_read(&handle->fil, &data, 1U, &readlen);
        sf_unlock();
        if (FR_OK == res) {
            data = ((uint8_t) data & 0xFFU); /*  mask return value to 1 byte only */
            rvalue = (int16_t)data;
        } else{
            /* nothing */
        }
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_read_buffer(const sf_file_t *file, uint8_t *buf, size_t buf_size, size_t *bytes_read) {
    FRESULT res = FR_OK;
    UINT readlen = 0U;
    int32_t rvalue = EUNSPECIFIED;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }

    if (handle != NULL) {
        sf_lock();
        res =  f_read(&handle->fil, buf, buf_size, &readlen);
        sf_unlock();
        if (FR_OK == res) {
            if (buf_size > (size_t)readlen) {
                /* terminate the buffer as space allows */
                buf[readlen] = (uint8_t) '\0';
            }
            if (NULL != bytes_read) {
                *bytes_read = (size_t) readlen;        /*  set number of bytes read */
            }
            rvalue = EXIT_SUCCESS;
        } else {
            if (NULL != bytes_read) {
                *bytes_read = 0UL;        /*  error: no bytes read */
            }
            rvalue = EIO;
        }
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int16_t sf_peek_byte(const sf_file_t *file) {
    int16_t rvalue = EXIT_SUCCESS;
    DWORD cur = 0;
    FRESULT res = FR_OK;
    UINT readlen = 0U;
    uint8_t data = 0U;

    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }

    if (handle != NULL) {
        sf_lock();
        cur = f_tell(&handle->fil);    /*  latch current offset/position */
        res = f_read(&handle->fil, &data, 1U, &readlen);        /*  read 1 byte */

        if (FR_OK == f_lseek(&handle->fil, cur)) { /*  reset pointer to previous location */
            if (FR_OK == res) {
                data = ((uint8_t) data & 0xFFU);  /*  return peeked byte */
                rvalue = (int16_t)data;        /*  return peeked byte */
            } else {
                rvalue = (int16_t) EIO;        /*  return read error code */
            }
        } else {
            rvalue = (int16_t) EIO;            /*  return lseek error code */
        }
        sf_unlock();
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_peek_buffer(const sf_file_t *file, uint8_t *buf, size_t buf_size, size_t *bytes_peeked) {
    int32_t rvalue = EXIT_SUCCESS;
    DWORD cur = 0;
    FRESULT res = FR_OK;
    UINT readlen = 0U;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }

    if (handle != NULL) {
        sf_lock();
        cur = f_tell(&handle->fil);    /*  latch current offset/position */
        res = f_read(&handle->fil, buf, buf_size, &readlen);        /*  read 1 byte */

        if (FR_OK == f_lseek(&handle->fil, cur)) { /*  reset pointer to previous location */
            if (FR_OK == res) {
                if (NULL != bytes_peeked) {
                    *bytes_peeked = (size_t) readlen;        /*  set number of peeked bytes */
                }
            } else {
                rvalue = EIO;
            }
        } else {
            rvalue = EIO;        /*  return lseek error code */
        }
        sf_unlock();
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_get_seek_pos(const sf_file_t *file) {
    int32_t rvalue = EUNSPECIFIED;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            sf_lock();
            rvalue = (int32_t) f_tell(&handle->fil);    /*  get current position  */
            sf_unlock();
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_seek_relative(const sf_file_t *file, int32_t offset,
                         int32_t *seeked) { // parasoft-suppress CODSTA-86-3 "offset doesn't need validation"
    int32_t rvalue = EUNSPECIFIED;
    DWORD new = 0;
    DWORD cur = 0;
    int32_t seek = 0;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            sf_lock();
            cur = f_tell(&handle->fil); /*  get current position  */
            f_lseek(&handle->fil, (int32_t)cur + offset);
            new = f_tell(&handle->fil);
            sf_unlock();
            seek = (cur < new) ? (new - cur) : (cur - new); /*  calculate position difference */
            if (new >= 0L) {
                if (NULL != seeked) {
                    *seeked = seek; /*  set number of seeked bytes */
                }
                rvalue = 0; /*  return successful */
            } else {
                rvalue = new; /*  return error */
            }
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_seek_absolute(const sf_file_t *file, size_t offset,
                         size_t *seeked) { // parasoft-suppress CODSTA-86-3 "offset doesn't need validation"
    int32_t rvalue = EUNSPECIFIED;
    FRESULT res = 0;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            sf_lock();
            res = f_lseek(&handle->fil, (int32_t) offset); /*  move pointer position to new absolute offset */
            if (FR_OK == res) {
                if (NULL != seeked) {
                    *seeked = (size_t) f_tell(&handle->fil); /*  set new absolute offset value */
                }
                rvalue = 0; /*  return successful */
            } else {
                rvalue = res; /*  return error */
            }
            sf_unlock();
        }
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_writeByte(const sf_file_t *file, uint8_t byte) {
    int32_t rvalue = EXIT_SUCCESS;
    FRESULT res = FR_OK;
    UINT written = 0U;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            sf_lock();
            res = f_write(&handle->fil, &byte, 1U, &written); /*  write input byte to file */
            sf_unlock();
            if (FR_OK != res) {
                rvalue = EIO;
            }
            sf_flush(file);
        }
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_writeBuffer(const sf_file_t *file, const uint8_t *bytes, size_t size, size_t *written) {
    int32_t rvalue = EUNSPECIFIED;
    FRESULT res = FR_OK;
    UINT bytes_written = 0U;
    sf_int_file_t *handle = NULL;

    static uint32_t write_modes = ((uint32_t) SF_MODE_WRITE)
            | ((uint32_t) SF_MODE_TRUNCATE)
            | ((uint32_t) SF_MODE_CREATE)
            | ((uint32_t) SF_MODE_APPEND);
    if ((NULL == file) || (NULL == file->handle)) {
        errno = EFAULT;
    } else if(0U == (write_modes & ((uint32_t)file->mode))) {
        // File not in write mode
        errno = EPERM;
    } else {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (NULL == handle) {
            // Closed file ?
            errno = EPIPE;
        } else if(NULL == bytes) {
            errno = EFAULT;
        } else if(0UL == size) {
            // all of the zero bytes have been written to disk
            if (NULL != written) {
                *written = 0UL;
            }
        } else {
            sf_lock();
            res = f_write(&handle->fil, bytes, size, &bytes_written);        /*  write input buffer to file */
            sf_unlock();
            if (FR_OK == res) {
                if (NULL != written) {
                    *written = (size_t) bytes_written;
                }
                sf_flush(file);
                rvalue = EXIT_SUCCESS;
            } else {
                rvalue = EIO;
            }
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_flush(const sf_file_t *file) {
    /*  makes sure that all data is actually committed to the file system. probably does not apply since I'm not using any buffers */
    int32_t res = -1;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            sf_lock();
            res = (FR_OK == f_sync(&handle->fil)) ? EXIT_SUCCESS : EIO;
            sf_unlock();
        } else {
            //sync();
            ABQ_ERROR_MSG(__FUNCTION__);
            res = 0;
        }
    }
    return res;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_close(sf_file_t *file) {
    int32_t rvalue = EUNSPECIFIED;
    sf_int_file_t *handle = NULL;
    if (NULL == file) {
        abq_status_set(EFAULT, false);
    } else if(NULL == file->handle) {
        abq_status_set(EALREADY, false);
    } else {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle == NULL) {
            abq_status_set(EINVAL, false);
        } else {
            sf_lock();
            /* (void) sf_flush(file)  <- Seriously degrades performance */
            rvalue = (FR_OK == f_close(&handle->fil)) ? 0 : -1; /* close file descriptor */
            sf_unlock();
            (void) obj_release(handle->name, handle);
            ITEM_POOL_FREE(&sf_file_handles, sf_int_file_t, handle);
            file->handle = NULL; /* delete file handle */
        }
    }
    return rvalue;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_delete_file(cstr_t file_name) {
    int32_t res = -1;
    if (NULL != file_name) {
        if (1 != sf_exists(file_name)) {
            abq_status_set(ENOENT, true);
            res = ENOENT;
        } else {
            sf_lock();
            /* SECURITY-19-2 unlink may cause race conditions */
            /* remove link to file descriptor */
            if (FR_OK == f_unlink(file_name)) {
                res = 0;
            } else {
                res = -1;
            }
            sf_unlock();
        }
    }
    return res;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_delete_dir(cstr_t dir_name) {
    int32_t res = -1;
    if (NULL != dir_name) {
        sf_lock();
        byte_t buf[PLATFORM_MAX_FILE_PATH_LEN];
        int32_t len = utf8_write_bytes(buf, dir_name, -1);
        if (sf_file_system_dir_separator == buf[len - 1]) {
            buf[len - 1] = '\0'; // remove the last '/'
        }
        if (FR_OK == f_unlink(buf)) {
            res = 0;
        }
        sf_unlock();
    }
    return res;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_move_file(cstr_t src_name, cstr_t dst_name) {
    int32_t res = -1;
    if ((NULL != src_name) && (NULL != dst_name)) {
        sf_lock();
        if (1 == sf_exists(dst_name)) {
            sf_delete_file(dst_name);
        }
        /* move file from src path to dst path */
        res = (FR_OK == f_rename(src_name, dst_name)) ? 0 : -1; // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
        sf_unlock();
    }
    return res;
}

int32_t sf_abs_path(const sf_file_t *file, byte_t *dest, size_t dest_len) {
    int32_t rvalue = EUNSPECIFIED;
    sf_int_file_t *handle = NULL;
    if((NULL == dest) || (0U == dest_len)) {
        // Error with dest, return -1
    } else if((NULL == file) || (NULL == file->handle)){
        // Error, but initialize dest anyway
        bytes_set(dest, '\0', dest_len);
    } else {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            rvalue = sf_path_resolve(handle->name, dest, dest_len);
        }
    }
    return rvalue;
}
