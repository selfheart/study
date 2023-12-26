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
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>

#if defined(_WIN32) && !defined(__SYMBIAN32__)
#include <windows.h>
#include <io.h>
#else

#include <sys/statvfs.h>

#endif

/* define type for 'stat' structure defined in stat.h	*/
typedef struct stat stat_t;

/* internal file structure */
typedef struct sf_int_file {
    fd_type fd;            /* system specific file descriptor type */
    cstr_t name;
} sf_int_file_t;

/* internal directory structure */
typedef struct sf_int_dir {
    DIR *fd;            /* system specific directory descriptor */
    size_t path_len;
    byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
} sf_int_dir_t;

/** used to deliminate between folders in a network (URL) path */
static const byte_t sf_network_dir_separator = '/'; // should match AQ_PATH_SEPARATOR

#ifndef O_BINARY
#define O_BINARY (0)
#endif /* O_BINARY */

#ifndef O_SYNC
#define O_SYNC (0)
#endif /* O_SYNC */

#ifdef FILE_SYNC
#define ABQ_SYNC O_SYNC
#else
#define ABQ_SYNC (0)
#endif /* ABQ_SYNC */

#if defined(_WIN32) && !defined(__SYMBIAN32__)
/** used to deliminate between folders on the system drive */
static const byte_t sf_file_system_dir_separator = '\\'; // should match AQ_PATH_SEPARATOR
enum {
    SF_OPEN_READ=_O_RDONLY,       /* read only */
    SF_OPEN_WRITE=_O_WRONLY,      /* write only */
    /* not used  SF_OPEN_READ_WRITE =_O_RDWR */
    SF_OPEN_CREATE=_O_CREAT,      /* create new file - used implicit */
    SF_OPEN_TRUNCATE=_O_TRUNC,    /* delete existing file content */
    SF_OPEN_EXCLUDE=_O_EXCL,      /* not used */
    SF_OPEN_APPEND=_O_APPEND,     /* start writing at end of file - used implicit */
    SF_OPEN_BINARY=_O_BINARY,
    SF_OPEN_SYNC=ABQ_SYNC,
    SF_OPEN_DUMMY=0x00,          /*  same as READ ONLY !! */
};
#else
/** used to deliminate between folders on the system drive */
static const byte_t sf_file_system_dir_separator = '/';

/* spil file posix internal permissions */

#define    SF_OPEN_IRUSR    ((uint32_t)S_IRUSR)    /* Read by owner.  */
#define    SF_OPEN_IWUSR    ((uint32_t)S_IWUSR)    /* Write by owner.  */
#define    SF_OPEN_IXUSR    ((uint32_t)S_IXUSR)    /* Execute by owner.  */
/* Read, write, and execute by owner.  */
#define    SF_OPEN_IRWXU    ((uint32_t)(SF_OPEN_IRUSR|SF_OPEN_IWUSR|SF_OPEN_IXUSR))

#define    SF_OPEN_IRGRP    ((uint32_t)(SF_OPEN_IRUSR >> 3U))    /* Read by group.  */
#define    SF_OPEN_IWGRP    ((uint32_t)(SF_OPEN_IWUSR >> 3U))    /* Write by group.  */
/* define    SF_OPEN_IXGRP    ((uint32_t)(SF_OPEN_IXUSR >> 3U))    not used, Execute by group.  */
/* Read, write, and execute by group.  */
#define    SF_OPEN_IRWXG    ((uint32_t)(SF_OPEN_IRWXU >> 3U))

/* define    SF_OPEN_IROTH    ((uint32_t)(SF_OPEN_IRGRP >> 3U))    not used, Read by others.  */
/* define    SF_OPEN_IWOTH    ((uint32_t)(SF_OPEN_IWGRP >> 3U))    not used, Write by others.  */
/* define    SF_OPEN_IXOTH    ((uint32_t)(SF_OPEN_IXGRP >> 3U))    not used, Execute by others.  */
/* Read, write, and execute by others.  */
/* define    SF_OPEN_IRWXO    ((uint32_t)(SF_OPEN_IRWXG >> 3U))    not used, */

/* Spil File posix internal file mode flags */

#define SF_OPEN_READ         ((uint32_t)O_RDONLY)        /* read only */
#define SF_OPEN_WRITE        ((uint32_t)O_WRONLY)        /* write only */
/* define SF_OPEN_READ_WRITE   ((uint32_t)O_RDWR)          not used */
#define SF_OPEN_CREATE       ((uint32_t)O_CREAT)         /* create new file - used implicit */
#define SF_OPEN_TRUNCATE     ((uint32_t)O_TRUNC)         /* delete existing file content */
/* define SF_OPEN_EXCLUDE      ((uint32_t)O_EXCL)          not used */
#define SF_OPEN_APPEND       ((uint32_t)O_APPEND)        /* start writing at end of file - used implicit */
#define SF_OPEN_BINARY       ((uint32_t)O_BINARY)
#define SF_OPEN_SYNC         ((uint32_t)ABQ_SYNC)
#define SF_OPEN_DUMMY        ((uint32_t)0x0000U)          /*  same as READ ONLY !! */
#endif

/* not used */
#if(0U)
static const uint16_t mode_map[] = {
    SF_OPEN_DUMMY,	  	/* bit 1=0 -- SF_OPEN_MODE_UNKNOWN */
    SF_OPEN_READ,	  	/* bit 1=1 -- SF_OPEN_MODE_READ */
    SF_OPEN_WRITE,	  	/* bit 2 -- SF_OPEN_MODE_WRITE */
    SF_OPEN_TRUNCATE,	/* bit 3 -- SF_OPEN_MODE_TRUNCATE */
};
#endif

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

static int32_t sf_map_mode(uint16_t input, uint16_t exists) {
    int32_t output = -1;
    if (((input & (uint16_t) SF_MODE_TRUNCATE) != 0U) || ((input & (uint16_t) SF_MODE_CREATE) != 0U)) {
        output = (int32_t) (SF_OPEN_WRITE | SF_OPEN_TRUNCATE | SF_OPEN_CREATE | SF_OPEN_BINARY |
                            SF_OPEN_SYNC);  /* delete content(create file) and start writing a file */
    } else if ((input & (uint16_t) SF_MODE_APPEND) != 0U) {
        if (1U == exists) {
            output = (int32_t) (SF_OPEN_WRITE | SF_OPEN_APPEND | SF_OPEN_BINARY |
                                SF_OPEN_SYNC); /* continue writing at the end of the file */
        } else {
            output = (int32_t) (SF_OPEN_WRITE | SF_OPEN_APPEND | SF_OPEN_CREATE | SF_OPEN_BINARY |
                                SF_OPEN_SYNC); /* create and start writing a file */
        }
    } else if ((input & (uint16_t) SF_MODE_WRITE) != 0U) {
        if (1U == exists) {
            output = (int32_t) (SF_OPEN_WRITE | SF_OPEN_BINARY |
                                SF_OPEN_SYNC); /* continue writing the file */
        } else {
            output = (int32_t) (SF_OPEN_WRITE | SF_OPEN_CREATE | SF_OPEN_BINARY | SF_OPEN_SYNC); /* create and start writing a file */
        }
    } else if ((input & (uint16_t) SF_MODE_READ) != 0U) {
        if (1U == exists) {
            output = (int32_t) (SF_OPEN_READ | SF_OPEN_BINARY); /* continue reading the file */
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
        stat_t prop;
        (void) bytes_set(&prop, '\0', sizeof(prop));    /* clear memory area */

        byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
        (void) bytes_set(path, '\0', sizeof(path));
        int32_t path_len = sf_path_localize(fileName,
                path, sizeof(path), abq_file_sys_path);
        // do we still have a path ?
        if (0 <= path_len) {
            /*  attempt to get file properties */
            rvalue = stat(path, &prop); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"

            if (rvalue >= 0) {
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
    stat_t prop;
    (void) bytes_set(&prop, '\0', sizeof(prop));  /* clear memory area */
    byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
    (void) bytes_set(path, '\0', sizeof(path));
    int32_t path_len = sf_path_localize(fileName,
            path, sizeof(path), abq_file_sys_path);
    // do we still have a path ?
    if (0 <= path_len) {
        /*  attempt to get file properties */
        rvalue = stat(path, &prop); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"

        if (rvalue >= 0) {
            rvalue = S_ISDIR(prop.st_mode);
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
    stat_t prop;
    (void) bytes_set(&prop, '\0', sizeof(prop));  /* clear memory area */
    byte_t path[PLATFORM_MAX_FILE_PATH_LEN];
    (void) bytes_set(path, '\0', sizeof(path));
    int32_t path_len = sf_path_localize(fileName,
            path, sizeof(path), abq_file_sys_path);
    if ((0 <= path_len) && (NULL != info)) {
        /*  attempt to get file properties */
        rvalue = stat(path, &prop);  // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
        if (rvalue >= 0) {    /*  populate output structure */

#if defined(_WIN32) && !defined(__SYMBIAN32__)
            info->createTime = prop.st_ctime;
            info->modifiedTime = prop.st_mtime;
#elif defined(__APPLE__)
            info->createTime = prop.st_ctimespec.tv_sec;
            info->modifiedTime = prop.st_mtimespec.tv_sec;
#else
            info->createTime = (uint32_t) prop.st_ctim.tv_sec;
            info->modifiedTime = (uint32_t) prop.st_mtim.tv_sec;
#endif
            info->isDir = S_ISDIR(prop.st_mode);
            info->size = (size_t) prop.st_size;
            rvalue = 0;
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
            } else if(('/' == dirName[retval])
                    || ('\\' == dirName[retval])
                    || ('\0' == dirName[retval])) {
                if (0 != retval) {
                    // End of path-element, check if dir-exists and create it if it does not
                    dir[retval] = '\0';
                    if(0==sf_exists(dir)) {
#if defined(_WIN32) && !defined(__SYMBIAN32__)
                        if(0 != mkdir(dir))
#else
                        /*  660 should grant limit permissions (drw-rw----)
                         * HOWEVER, on ubuntu it produced (d-w--w-r-T)
                         * and then we could not create files with same level
                         * of permissions within the directory
                         * So permissions were upgraded to 0770 (mvogel)
                         */
                        if(0 == mkdir(dir, (SF_OPEN_IRWXU | SF_OPEN_IRWXG))) {
                            sync();
                        } else
#endif
                        {
                            if (EEXIST == errno) {
                                retval = EXIT_SUCCESS;
                                errno = 0;
                            } else {
                                retval = -1 * errno;
                                errno = 0;
                                break;
                            }
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
            if (handle->path_len < sizeof(handle->path)) {
                handle->fd = opendir(handle->path); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 CERT_C-CON33-a-3 "Use of sf_lock to protect against race-conditions and/or multithreaded access"
            }
            if (NULL == handle->fd) {
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
    int32_t rvalue = EUNSPECIFIED;
    sf_int_dir_t *handle = NULL;
    struct dirent *ds = NULL;

    if ((NULL == dir) || (NULL == dir->handle)) {
        abq_status_set(EFAULT, false);
    } else {
        sf_lock();
        ITEM_POOL_RESOLVE(&sf_dir_handles, sf_int_dir_t, handle, dir->handle);
        if (NULL == handle) {
            abq_status_set(ENOTDIR, false);
        } else if(0L > telldir(handle->fd)) { // parasoft-suppress CERT_C-CON33-a-3 "Use of sf_lock to protect against multithreaded access"
            // Invalid directory stream, errno was set
        } else {
            ds = readdir(handle->fd); // parasoft-suppress CERT_C-CON33-a-3 "Use of sf_lock to protect against multithreaded access"
            if (ds != NULL) {
                handle->path[handle->path_len] = AQ_PATH_SEPARATOR;
                rvalue = 1 + (int32_t)handle->path_len;
                rvalue = utf8_write_bytes(&handle->path[rvalue], ds->d_name,
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
                        rvalue = utf8_write_bytes(element_name,
                                ds->d_name,(int32_t)element_name_sz);
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
            res = closedir(handle->fd);  // parasoft-suppress CERT_C-CON33-a-3 "Use of sf_lock to protect against multithreaded access"
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
#if defined(_WIN32) && !defined(__SYMBIAN32__)
                    int32_t pmode = (SF_OPEN_READ & mapped_mode) ? _S_IREAD : _S_IWRITE;
                    handle->fd = _open(fileName, mapped_mode, pmode);
#else
                    /*  if file is created, limited access rights are granted (-rw-rw----) */
                    mode_t permissions = ((SF_OPEN_IRUSR | SF_OPEN_IWUSR) | (SF_OPEN_IRGRP | SF_OPEN_IWGRP));
                    /* SECURITY-19-2 open may cause race conditions */
                    handle->fd = open(fileName, mapped_mode, permissions);  // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
#endif
                    if (0 >= handle->fd) {
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
        cur = (int32_t) lseek((int32_t) handle->fd, 0, SEEK_CUR);    /*  latch current offset/position */
        end = (int32_t) lseek((int32_t) handle->fd, 0,
                              SEEK_END);    /*  set position to end of file to get EOF offset */
        (void) lseek((int32_t) handle->fd, cur, SEEK_SET);    /*  reset position to the initial offset */
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
    if(NULL == dirName){

    } else {
#if defined(_WIN32) && !defined(__SYMBIAN32__)
#ifdef  UNICODE
    // LPCTSTR is of type wide-byte_t
   int32_t count=-1, index = 0;
   LPCTSTR pszDrive[PLATFORM_MAX_FILE_PATH_LEN];
   MultiByteToWideChar(CP_ACP, 0, dirName, -1, buf, sizeof(pszDrive));
#else
   // LPCTSTR is of normal single-byte byte_t
   LPCTSTR pszDrive = dirName;
#endif
   __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes;
   BOOL fResult = GetDiskFreeSpaceEx( pszDrive,
   (PULARGE_INTEGER)&lpFreeBytesAvailable,
   (PULARGE_INTEGER)&lpTotalNumberOfBytes,
   (PULARGE_INTEGER)&lpTotalNumberOfFreeBytes
   );
   if(fResult){
        if (NULL != available_bytes) {
            *available_bytes = (uint64_t)lpTotalNumberOfFreeBytes;
        }
        if (NULL != total_bytes) {
            *total_bytes = (uint64_t)lpTotalNumberOfBytes;
        }
       rvalue = 0;
   }else{
       errno = GetLastError();
   }
#else
        struct statvfs statvfs1;
        int32_t stat_rs = statvfs(dirName, &statvfs1);
        if (0 == stat_rs) {
            // the available size is f_bsize * f_bavail
            if (NULL != available_bytes) {
                *available_bytes = (uint64_t) statvfs1.f_bsize * (uint64_t) statvfs1.f_bavail;
            }
            if (NULL != total_bytes) {
                *total_bytes = (uint64_t) statvfs1.f_blocks * (uint64_t) statvfs1.f_frsize;
            }
            rvalue = 0;
        } else {
            if (NULL != available_bytes) {
                *available_bytes = 0;
            }
            if (NULL != total_bytes) {
                *total_bytes = 0;
            }
        }
#endif
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int16_t sf_read_byte(const sf_file_t *file) {
    int32_t res = 0;
    int16_t rvalue = EUNSPECIFIED;
    uint8_t data = 0U;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }
    if (handle != NULL) {
        res = (int32_t) read((int32_t) handle->fd, &data, 1U);
        if (res > 0) {
            data = ((uint8_t) data & 0xFFU); /*  mask return value to 1 byte only */
            rvalue = (int16_t)data;
        } else if(0 == res) {
            rvalue = (int16_t)res;
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
    int32_t res = 0;
    int32_t rvalue = EUNSPECIFIED;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }

    if (handle != NULL) {
        res = (int32_t) read((int32_t) handle->fd, buf, buf_size);
        if (res > 0) {
            if (buf_size > (size_t)res) {
                /* terminate the buffer as space allows */
                buf[res] = (uint8_t) '\0';
            }
            if (NULL != bytes_read) {
                *bytes_read = (size_t) res;        /*  set number of bytes read */
            }
            rvalue = 0;
        } else {
            if (NULL != bytes_read) {
                *bytes_read = 0UL;        /*  error: no bytes read */
            }
            rvalue = res;
        }
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int16_t sf_peek_byte(const sf_file_t *file) {
    int16_t rvalue = EXIT_SUCCESS;
    int32_t cur = 0;
    int32_t res = 0;
    uint8_t data = 0U;

    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }

    if (handle != NULL) {
        cur = (int32_t) lseek((int32_t) handle->fd, 0, SEEK_SET);    /*  latch current offset/position */
        res = (int32_t) read((int32_t) handle->fd, &data, 1U);        /*  read 1 byte */
        cur = (int32_t) lseek((int32_t) handle->fd, cur, SEEK_SET);   /*  reset pointer to previous location */

        if (cur >= 0) {
            if (res > 0) {
                data = ((uint8_t) data & 0xFFU);  /*  return peeked byte */
                rvalue = (int16_t)data;        /*  return peeked byte */
            } else {
                rvalue = (int16_t) res;        /*  return read error code */
            }
        } else {
            rvalue = (int16_t) cur;            /*  return lseek error code */
        }
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_peek_buffer(const sf_file_t *file, uint8_t *buf, size_t buf_size, size_t *bytes_peeked) {
    int32_t cur = 0;
    int32_t res = -1;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
    }

    if (handle != NULL) {
        cur = (int32_t) lseek((int32_t) handle->fd, 0, SEEK_SET);    /*  latch current (absolute)offset/position */
        res = (int32_t) read((int32_t) handle->fd, buf, buf_size);    /*  read x bytes */
        cur = (int32_t) lseek((int32_t) handle->fd, cur,
                              SEEK_SET);    /*  reset pointer to previous (absolute)location */

        if (cur >= 0) {
            if (res > 0) {
                if (NULL != bytes_peeked) {
                    *bytes_peeked = (size_t) res;        /*  set number of peeked bytes */
                }
                res = 0;                    /*  return positive response */
            }
        } else {
            res = cur;        /*  return lseek error code */
        }
    }
    return res;
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
            rvalue = (int32_t) lseek((int32_t) handle->fd, 0, SEEK_CUR);    /*  get current position  */
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
    int32_t new = 0;
    int32_t cur = 0;
    int32_t seek = 0;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            cur = (int32_t) lseek((int32_t) handle->fd, 0, SEEK_CUR); /*  get current position  */
            new = (int32_t) lseek((int32_t) handle->fd, offset,
            SEEK_CUR); /*  move position by offset bytes and get new position */
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
    int32_t res = 0;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            res = (int32_t) lseek((int32_t) handle->fd, (int32_t) offset,
            SEEK_SET); /*  move pointer position to new absolute offset */

            if (res >= 0) {
                if (NULL != seeked) {
                    *seeked = (size_t) res; /*  set new absolute offset value */
                }
                rvalue = 0; /*  return successful */
            } else {
                rvalue = res; /*  return error */
            }
        }
    }
    return rvalue;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_writeByte(const sf_file_t *file, uint8_t byte) {
    int32_t res = -1;
    sf_int_file_t *handle = NULL;
    if ((NULL != file) && (NULL != file->handle)) {
        ITEM_POOL_RESOLVE(&sf_file_handles, sf_int_file_t, handle, file->handle);
        if (handle != NULL) {
            res = (int32_t) write((int32_t) handle->fd, &byte, 1U); /*  write input byte to file */
            if (res > 0) {
                res = 0;
            }
        }
    }
    return res;
}


/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_writeBuffer(const sf_file_t *file, const uint8_t *bytes, size_t size, size_t *written) {
    int32_t res = -1;
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
            res = (int32_t) write((int32_t) handle->fd, bytes, size);        /*  write input buffer to file */
            if (res > 0) {
                if (NULL != written) {
                    *written = (size_t) res;
                }
                res = 0;
            } else {
                if ((-1 == res) && (EAGAIN == errno)) {
                    res = SF_ERR_AGAIN;
                    errno = 0;
                }
            }
        }
    }
    return res;
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
#if defined(_WIN32) && !defined(__SYMBIAN32__)
            FlushFileBuffers((HANDLE)handle->fd);
            res = 0;
#elif defined(__APPLE__)
            res = 0;
#else
            res = fsync((int32_t) handle->fd);
#endif
        } else {
#if defined(_WIN32) && !defined(__SYMBIAN32__)
#else
            sync();
#endif
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
            /* (void) sf_flush(file)  <- Seriously degrades performance */
            rvalue = close((int32_t) handle->fd); /* close file descriptor */
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
        sf_lock();
        /* SECURITY-19-2 unlink may cause race conditions */
        /* remove link to file descriptor */
        res = unlink(file_name);  // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
        if (res >= 0) {
            res = 0;
        } else {
            res = -1 * errno;
            errno = 0;
        }
        sf_unlock();
    }
    return res;
}

/**
 * Refer to function prototype in respective header file for function header
 */
int32_t sf_delete_dir(cstr_t dir_name) {
    int32_t res = -1;
    if (NULL != dir_name) {
        res = rmdir(dir_name);        /* remove directory*/
        if (res >= 0) {
            res = 0;
        }
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
#if defined(_WIN32) && !defined(__SYMBIAN32__)
        // res = rename(src_name, dst_name) // also fails to move a file with open file-handle
        if(MoveFile(src_name, dst_name)) {
            res = 0;
        } else {
            errno = GetLastError();
            if(ERROR_ALREADY_EXISTS == errno) {
                errno = EEXIST;
            }
        }
#else /* !Windows */
        /* move file from src path to dst path */
        res = link(src_name, dst_name); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
        if((0 > res) && (EEXIST == errno)) {
            /* Potential race condition, required for 1003.1 conformance. try again */
            /* SECURITY-19-2 unlink may cause race conditions */
            res = unlink(dst_name); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
            if(0 <= res) {
                res = link(src_name, dst_name); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
            }
        }
        if (0 <= res) {
            /* Once dest link ha been created, delete the source link */
            /* SECURITY-19-2 unlink may cause race conditions */
            res = unlink(src_name); // parasoft-suppress SECURITY-19-2 CERT_C-POS35-a-1 CERT_C-CON43-a-3 "Use of sf_lock to protect against race-conditions"
        }
#endif /* !Windows */
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
