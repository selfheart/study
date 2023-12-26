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
#ifndef SPIL_FILE_H
#define SPIL_FILE_H

#include "platform/platformconfig.h"


/**
 * @file spil_file.h
 *
 * Abstraction for file system access.  Functions described here will be implemented
 * to map the underlying system's file I/O capabilities to these interfaces.
 *
 * ## Error Codes
 *
 * Error Code | Description
 * ---|---
 * SF_ERR_INVAL_PARAM | An provided input parameter is invalid or out of range.
 * SF_ERR_TOO_MANY_OPEN | System can't **CURRENTLY** fulfill request because all SPIL file handles are exhausted.
 * SF_ERR_CONTENTION | System can't **CURRENTLY** fulfill request due to resource contention. NOTE: the resource may be single file, and/or an entire file system. The reason for contention may be implementation specific. This should be a temporary error should clear once the resource becomes available again.
 * SF_ERR_OTHER | Some other error occurred performing the requested operation.
 */

// @cond
#define SF_ERR_INVAL_PARAMETER (-1)
#define SF_ERR_AGAIN (-2)                               // read/write operation would block on a file opened in non-blocking mode
#define SF_ERR_TOO_MANY_OPEN (-3)
#define SF_ERR_OTHER (-99)
// @endcond

#define SF_MAX_FILE_HANDLES        16U        // TODO move this into a common config header
#define SF_MAX_DIR_HANDLES        16U        // TODO move this into a common config header


#if defined(_WIN32) && !defined(__SYMBIAN32__)
#define AQ_PATH_SEPARATOR '\\'
#define AQ_PATH_SEPARATOR_STR "\\"
#define SF_INVALID_FILENAME_CHARS "\\/*?\"<>|"
#else
#define AQ_PATH_SEPARATOR '/'
#define AQ_PATH_SEPARATOR_STR "/"
#define SF_INVALID_FILENAME_CHARS "\\/:*?\"<>|"
#endif

/**
 * File access modes that are supported by the spil_file module.  The SPIL file model
 * supports files that are either opened for reading or opened for writing.  It doesn't
 * support a file that is opened for both reading and writing.
 *
 * File access modes can be multiplexed (i.e. (SF_MODE_READ | SF_MODE_CREATE) or (SF_MODE_WRITE | SF_MODE_APPEND) )
 * Caution: (SF_MODE_READ | SF_MODE_WRITE) opens a file for reading only !!
 * Use of (SF_MODE_TRUNCATE) is identical to use of (SF_MODE_WRITE | SF_MODE_TRUNCATE)
 * Use of (SF_MODE_WRITE) is identical to use of (SF_MODE_WRITE | SF_MODE_APPEND)
 */
typedef enum {
    /// The file's access mode is unknown (it isn't opened for read or write access).
            SF_MODE_UNKNOWN = 0x0,

    /// The file is opened or being requested to be opened for read access.
            SF_MODE_READ = 0x1,

    /// The file is opened or being requested to be opened for write access.
            SF_MODE_WRITE = 0x2,

    /// If the file already exists it will be truncated to length 0. This mode implies SF_MODE_WRITE
            SF_MODE_TRUNCATE = 0x4,

    /// If the file does not exist, it will be created. This mode implies SF_MODE_TRUNCATE
            SF_MODE_CREATE = 0x8,

    /// The file is opened in append mode
    /// If the O_APPEND file status flag is set on the open file description,
    /// then a write(2) always moves the file offset to the end of the file,
    /// regardless of the use of lseek().
            SF_MODE_APPEND = 0x10,

} sf_mode_t;


/// File handle type for SPIL file module.
typedef struct sf_file sf_file_t;

/// Directory handle type for SPIL module.
typedef struct sf_dir sf_dir_t;

/// Datatype to hold information specific to a SPIL module file or directory.
typedef struct sf_info sf_info_t;

/**
 * Function type definition for mutual exclusion lock callback.
 */
typedef int32_t (*sf_lock_mutex_t)(cvar_t mutex);

/**
 * Function type definition for mutual exclusion unlock callback.
 */
typedef int32_t (*sf_unlock_mutex_t)(cvar_t mutex);


/**
 * Information about a specific SPIL file or directory node.
 */
struct sf_info {

    /// \c true if the node is a directory.  \c false otherwise.
    bool_t isDir;

    /// The time that file or directory was created.
    uint32_t createTime;

    /// The time that the file or directory was last accessed.
    uint32_t modifiedTime;

    /// The number of bytes that consumed by the node.
    size_t size;
};

/// Underlying data structure utilized by SPIL file handle sf_file_t.
struct sf_file {
    /// Indication of whether the file is opened for read or write access.
    sf_mode_t mode;

    /// The underlying system's handle to the file.
    void *handle;
};

/// Underlying data structure utilized by SPIL directory handle sf_dir_t.
struct sf_dir {
    /// The underlying system's handle to the directory.
    void *handle;
};

/**
 * Extracts the contents of a TAR-file
 * @param tarfile The name of the source file to be extracted
 * @param target_dir [optional] set to NULL to have files extracted in the same directory as tarfile
 *          OR The name of the target directory in which tarfile should be extracted
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_tar_extract(cstr_t tarfile, cstr_t target_dir);

/**
 * (Re-) initializes the spil_file internal data structures. Closes all open files and clear data. Should be executed once at application startup.
 *
 */
void sf_init(sf_lock_mutex_t lock_mutex_cb, sf_unlock_mutex_t unlock_mutex_cb, cvar_t mutex);


/**
 * Register a callback function used to lock a mutex semaphore object.
 *
 * @param lock_mutex_cb The callback function to be used to lock a mutex semaphore
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_reg_lock_mutex(sf_lock_mutex_t lock_mutex_cb);

/**
 * Register a callback function used to unlock a mutex semaphore object.
 *
 * @param unlock_mutex_cb The callback function to be used to unlock a mutex semaphore
 * @retval 0 on success
 * @retval <0 on error
 */
int32_t sf_reg_unlock_mutex(sf_unlock_mutex_t unlock_mutex_cb);

/**
 * System mutex semaphore that will be utilized to protect data structures
 * internal to Spil File.  This mutex will be locked/unlocked via
 * the callback functions registered via sf_reg_lock_mutex() and sf_reg_unlock_mutex().
 *
 * If both neither of callbacks are registered and the mutex is not registered, it
 * is assumed that the underlying implementation insures that the SPIL_FILE API is only called
 * within the context of a single thread.
 *
 * @param mutex A pointer to the system mutex semaphore which will be locked/unlocked via
 *              the registered callback functions.
 * @retval 0 on success
 * @retval <0 on error
 */
int32_t sf_reg_mutex(cvar_t mutex);


/**
 * Returns true if a file or directory exists for the passed path, otherwise false. Sets the error
 * response if an error occurs.
 *
 * @param fileName **[IN]** An absolute or relative system path as a null terminated string
 * @retval 0 if a file or directory does not exist for the passed path, otherwise false.
 * @retval 1 if a file or directory exists for the passed path, otherwise false.
 * @retval <0 if an error occurred when processing the request
 */
int32_t sf_exists(cstr_t fileName);

/**
 * Returns true if a directory exists for the passed path, otherwise false. Sets the error
 * response if an error occurs.
 *
 * @param fileName **[IN]** An absolute or relative system path as a null terminated string
 * @retval 0 if a file or directory does not exist for the passed path, otherwise false.
 * @retval 1 if a file or directory exists for the passed path, otherwise false.
 * @retval <0 if an error occurred when processing the request
 */
int32_t sf_is_dir(cstr_t fileName);

/**
 * Gets the file info for the file or directory at the passed path and populates the
 * passed itemInfo structure. Returns true on success, otherwise false. Sets the error response
 * if an error occurs.
 *
 * @param fileName **[IN]** a null terminated string representing absolute system path.
 * @param info **[IN]** Address of an sf_info_t object to populate.
 * @retval 0 on success
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_get_file_info(cstr_t fileName, sf_info_t *info);

/**
 * Creates a directory at the passed path, including all sub-directories in the path, if necessary.
 * Returns true on success, otherwise false.
 *
 * @param fileName **[IN]** A null terminated string representing absolute system path.
 * @retval 0 on success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_mkdir(cstr_t dirName);

/**
 * Opens a directory for iterating using sf_next_dir_item(). Populates the address of an sf_dir_t
 * object or NULL if the directory could not be opened.
 *
 * @param dirName **[IN]** A null terminated string representing an absolute system path.
 * @param dir **[OUT]** A pointer to an sf_file_t which will be populated with the underlying system directory structure.
 * @retval 0 on success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_opendir(cstr_t dirName, sf_dir_t *dir);

/**
 * Gets the next (or first) item in the passed directory.
 * On success the passed itemInfo structure will be populated.
 * *
 * If element_name is not NULL, and the directory item's name is too large to be stored in the buffer provided (determined by element_name_sz), then an error will be returned.
 * *
 * @param dir *[IN]* The address of a sf_dir_t object
 * @param info *[OUT]* Address of an sf_info_t object to populate.
 * @param element_name *[OUT]* Buffer which will be populated with the directory item's element name. It is acceptable to pass NULL for this value if the the item's name needed. When this value is populated, the element item's name will be NULL terminated.
 * @param element_name_sz *[IN]* The number of bytes that are allocated to the buffer element_name.
 * @retval 0 on success.
 * @retval 1 if end of directory is reached.
 * @retval <0 on error. see \c SF_ERR_*
 */
int32_t sf_next_dir_item(const sf_dir_t *dir, sf_info_t *info,
        byte_t* element_name, size_t element_name_sz);

/**
 * Closes the passed directory.
 *
 * @param dir **[IN]** The address of a sf_dir_t object.
 * @retval 0 on success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_close_dir(sf_dir_t *dir);

/**
 * Opens a file for either reading or writing based upon the requested \c mode.  If the
 * file is successfully opened the file handle will be copied to the output parameter \c file.
 *
 * NOTE: If the file does not exist at the passed path and the specified mode is \c SF_MODE_WRITE
 * the file is created.
 *
 * @param pathString **[IN]** An absolute system path.
 * @param pathMax **[IN]** The maximum number of characters in the path string.
 * @param mode **[IN]** The mode the file is to be opened.  This should either be \c SF_MODE_READ or \c SF_MODE_WRITE.
 * @param file **[OUT]** Address of an sf_file_t object which will be populated on success
 * @retval 0 on success
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_open_file(cstr_t fileName, sf_mode_t mode, sf_file_t *file);


/**
 * Populate the output parameter \c mode with the access mode (\c SF_MODE_READ or \c SF_MODE_WRITE) of
 * the given \c file object.
 *
 * @param file **[IN]** Address of the sf_file_t to obtain access mode from.
 * @param mode **[IN]** The access mode the file is opened as.
 * @retval 0 on success
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_get_access_mode(const sf_file_t *file, sf_mode_t *mode);


/**
 * Indicates an estimate of the number of bytes available for read or skip operations.
 *
 * @param file **[IN]** Address of the sf_file_t to estimate number of bytes available.
 * @param available **[OUT]** Indicates the estimated number of bytes available when this function returns successfully.
 * @retval 0 on success
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_get_bytes_available(const sf_file_t *file, size_t *available);


/**
 * Returns the available partition space for the provided directory path.
 *
 * @param dirName **[IN]** A null terminated string representing an absolute system path.
 * @param available_bytes **[OUT]** Indicates the estimated number of bytes available when this function returns successfully.
 * @param total_bytes **[OUT]** Indicates the estimated number of total storage bytes when this function returns successfully.
 * @retval 0 on success
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_get_system_space(cstr_t dirName, uint64_t *available_bytes, uint64_t *total_bytes);


/**
 * Reads one 8-bit byte and returns it, or returns < 1 if the byte could not be read.
 *
 * @param file **[IN]** Address of the sf_file_t to read a byte from.
 * @retval >=0 the value byte that was read from the file.  This will be in the range 0 .. 255
 * @retval <0 on error.  see \c SF_ERR_*
 */
int16_t sf_read_byte(const sf_file_t *file);


/**
 * Reads data from the file into the passed buffer, up to the passed buffer size. Indicates
 * the number of bytes read in the output parameter \c bytes_read.
 *
 * @param file **[IN]** Address of the sf_file_t to read from.
 * @param buf **[OUT]** Buffer to place the data read from the file.
 * @param buf_size **[IN]** The size allocated to \c buf (Don't read more bytes than allocated to \c buf).
 * @param bytes_read **[OUT]** The number of bytes that were read from \c file and placed into \c buf.
 * @retval 0 on success
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_read_buffer(const sf_file_t *file, uint8_t *buf, size_t buf_size, size_t *bytes_read);


/**
 * Peeks one 8-bit byte and returns it, or returns < 1 if the byte could not be peeked.
 *
 * @param file **[IN]** Address of the sf_file_t to peek a byte from.
 * @retval >=0 the value byte that was peeked from the file.  This will be in the range 0 .. 255
 * @retval <0 on error.  see \c SF_ERR_*
 */
int16_t sf_peek_byte(const sf_file_t *file);

/**
 * Reads data peeked the file into the passed buffer, up to the passed buffer size. Indicates
 * the number of bytes peeked in the output parameter \c bytes_read.  The bytes that are peeked
 * are still available for subsequent operations such as peek or read.
 *
 * @param file **[IN]** Address of the sf_file_t to peek from.
 * @param buf **[OUT]** Buffer to place the data peeked from the file.
 * @param buf_size **[IN]** The size allocated to \c buf (Don't peek more bytes than allocated to \c buf).
 * @param bytes_peeked **[OUT]** The number of bytes that were peeked from \c file and placed into \c buf.
 * @retval 0 on success
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_peek_buffer(const sf_file_t *file, uint8_t *buf, size_t buf_size, size_t *bytes_peeked);

/**
 * Query the current byte offset from the beginning of an opened file
 * @param file **[IN]** Address of the sf_file_t object to perform the seek operation.
 * @retval Offset (>=0) in bytes from start of file.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_get_seek_pos(const sf_file_t *file);

/**
 * Changes the current file position relative to the current position, seeking backwards if the
 * passed offset is negative and forwards if the passed offset is positive. Places the number of
 * bytes actually 'seeked' into the output parameter \c seeked.
 *
 * @param file **[IN]** Address of the sf_file_t object to perform the seek operation.
 * @param offset **[IN]** The number of bytes to seek backwards (negative) or forward (positive).
 * @param seeked **[OUT]** The number of bytes seeked.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_seek_relative(const sf_file_t *file, int32_t offset, int32_t *seeked);

/**
 * Changes the current file position relative to the beginning of the file. Returns the number
 * of bytes actually 'seeked'.
 *
 * @param file **[IN]** The address of a sf_file_t object
 * @param offset **[IN]** The number of bytes to seek to, relative to the beginning of the file.
 * @param seeked **[OUT]** The number of bytes seeked.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_seek_absolute(const sf_file_t *file, size_t offset, size_t *seeked);


/**
 * Writes one 8-bit byte. Returns true on success, otherwise false. Sets the error response if
 * an error occurs.
 *
 * @param file **[IN]** The address of an sf_file_t structure.
 * @param byte **[IN]** Byte to write.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_writeByte(const sf_file_t *file, uint8_t byte);


/**
 * Writes data to the file from the passed buffer.
 * Sets the error response if an error occurs.
 *
 * @param file **[IN]** The address of an sf_file_t structure.
 * @param bytes **[OUT]** Buffer to write from.
 * @param size **[OUT]** Number of bytes to write
 * @param written **[OUT]** The number of bytes that were actually written (this may be fewer than the number
 *                          of bytes specified by \c size.
 * @return The number of bytes actually written.
 */
int32_t sf_writeBuffer(const sf_file_t *file, const uint8_t *bytes, size_t size, size_t *written);

/**
 * Flushes the output stream, if buffered, forcing any buffered data to be written to the
 * underlying stream.
 *
 * @param file **[IN]** The address of an sf_file_t structure.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_flush(const sf_file_t *file);

/**
 * Closes the passed file.
 *
 * @param file **[IN]** The address of an sf_file_t structure.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_close(sf_file_t *file);

/**
 * Delete the specified file.
 *
 * CAUTION!! on posix systems it's possible to delete a file while having an opened file descriptor for that file
 *
 * @param file_name The name of the file to be deleted.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_delete_file(cstr_t file_name);

/**
 * Delete the specified directory.
 *
 * @param dir_name The name of the directory to be deleted.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_delete_dir(cstr_t dir_name);

/**
 * Move the contents of the file named src_name to a file named dst_name.
 * When possible use underlying system calls for an atomic move.  Otherwise
 * Copy from source to destination and then delete source.
 *
 * CAUTION!!
 *  - on posix systems it's possible to move a file while having an opened file descriptor for that file
 *  - when the filename in source is identical to an already existing filename in destination, the existing file will be replaced/overwritten
 *
 * @param src_name The name of the source file.
 * @param dst_name The name of the destination file.
 * @retval 0 on Success.
 * @retval <0 on error.  see \c SF_ERR_*
 */
int32_t sf_move_file(cstr_t src_name, cstr_t dst_name);

/**
 * Lookup the absolute path name of the file provided and
 *
 * @param file: The file to lookup the pathname of
 * @param dest: a buffer used to store the result
 * @param dest_len: maximum number of bytes that can be written to dest
 * @return number of bytes that was written to dest, or -1 on failure
 */
int32_t sf_abs_path(const sf_file_t *file, str_t dest, size_t dest_len);

typedef enum {
    abq_file_sys_path = 1,
    abq_network_path,
} abq_path_type_t;

/**
 * Returns absolute path for passed in relative path in 'dest' buffer
 * @param relative path to be resolved
 * @param dest  output buffer
 * @param dest_len output buffer size
 * @return number (>=0) of bytes copied to dest if successful, or -1 on failure
 */
int32_t sf_path_resolve(cstr_t relative, str_t dest, size_t dest_len);


/**
 * Replaces path delimiters depending on the system(unix/windows) and path type (file/network)
 * @param path the path to be localized
 * @param dest the output buffer destination
 * @param dest_len the output buffer size
 * @param type path type
 * @return bytes in path if successful, or -1 on failure
 */
int32_t sf_path_localize(cstr_t path, str_t dest, size_t dest_len, abq_path_type_t type);

/**
 * Write the joined path of parent and child into the provided buffer
 *
 * @param parent first part of path to be written
 * @param child second part of path to be written
 * @param dest buffer used to store the result
 * @param dest_len maximum number of bytes that can be written to dest
 * @param abq_path_type_t type of path, to check for delimiters, either filesystem type or network_path
 * @return number of bytes that was written to dest, or -1 on failure
 */
int32_t sf_path_join(cstr_t parent, cstr_t child, str_t dest, size_t dest_len, abq_path_type_t type);

/**
 * Write the directory path of the provided file into the provided buffer
 *
 * @param filename complete path of a file
 * @param dest buffer used to store the result
 * @param dest_len maximum number of bytes that can be written to dest
 * @return number of bytes that was written to dest, or -1 on failure,
 *         or -1*EOVERFLOW
 */
int32_t sf_filename_of(cstr_t path, str_t dest, size_t dest_len);

/**
 * Write the directory path of the provided file into the provided buffer
 *
 * @param filepath complete path of a file
 * @param dest buffer used to store the result
 * @param dest_len maximum number of bytes that can be written to dest
 * @return number of bytes that was written to dest, or -1 on failure
 */
int32_t sf_parent_dir(cstr_t filepath,
        byte_t* dest, size_t dest_len);

/* for backwards compatibility */
static inline int32_t sf_path_of(cstr_t filepath,
                                byte_t* dest, size_t dest_len) {
    return sf_parent_dir(filepath, dest, dest_len);
}

#endif /* SPIL_FILE_H */
