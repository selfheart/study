/*
 * files.h
 *
 *  Created on: Apr 22, 2019
 *      Author: mvogel
 */

#ifndef LIB_SPIL_FILE_INCLUDE_SPIL_FILE_ONTRAC_FILES_H_
#define LIB_SPIL_FILE_INCLUDE_SPIL_FILE_ONTRAC_FILES_H_

#include <spil_file/abq_files.h>
#include <ontrac/ontrac/abq_str.h>
#include <ontrac/stream/abq_io.h>

#ifndef ABQ_PAGESIZE
#include <unistd.h>
#define ABQ_PAGESIZE ((size_t)getpagesize())
#endif /* ABQ_PAGESIZE */

#ifndef ABQ_MIN_PAGES
#define ABQ_MIN_PAGES (32U)
#endif /* ABQ_MIN_PAGES */
/**
 *
 * @param config_dir: Folder holding configuration files for OTAmatic components
 * @param persist_dir: Folder used for persistent data for use by OTAmatic application
 * @param persist_dir_max_bytes: maximum number of bytes OTAmatic is allowed to consume in persist_dir
 * @param lock: function used to lock the mutex
 * @param unlock: function used to unlock the mutex
 * @param mutex: mutex implementation used for synchronizing threads
 * @return EXIT_SUCCESS on success, else an error code
 */
extern err_t abq_files_init( const char* config_dir,
        const char *persist_dir, uint64_t persist_dir_max_bytes,
        sf_lock_mutex_t lock, sf_unlock_mutex_t unlock, cvar_t mutex);

/**
 * Returns the storage space used by files and directories (no hidden files/directories!) for the provided directory path.
 * @param dirName *IN* the directory path to be investiated
 * @param total_bytes *OUT* pointer where sum will be written to
 * @return EXIT_SUCCESS when successful, else an error code
 */
extern int32_t abq_du(cstr_t dirName, uint64_t *total_bytes);
/**
 * @brief remove local dir along with all content under it. Just "rm -rf"
 * @param path root path
 * @param repo repository name
 * @param dirname folder name
 *
 * @return EXIT_SUCCESS for success, EIO/EINVAL for errors
 */
extern err_t abq_rmrf(const char *filename);

/**
 * @brief localizes a path to by replacing slashes & backslashes with system specific folder separators
 * located in spil_nio instead of spil_file so that it can have access to ontrac/abq_str.h
 *
 * @param path: the path to be localized for the current system
 * @param type: distinguishes between a file-system path and a network path
 * @return a localized version of path with a self-reference or NULL on failure
 */
extern cstr_t abq_path_localize(cstr_t path, abq_path_type_t type);
/**
 * @brief joins two path segments together as a unified file system path for local file system
 * located in spil_nio instead of spil_file so that it can have access to ontrac/abq_str.h
 *
 * @param parent: The parent folder in which child is to reside
 * @param child: a subpath within the parent folder path specifiec by parent
 * @param type: distinguishes between a file-system path and a network path
 * @return: The combined path of the two segments, or NULL on failure
 */
extern cstr_t abq_path_join(cstr_t parent, cstr_t child, abq_path_type_t type);
/**
 * @brief serializes a resource into persistent storage using a formatter for the given content-type
 *
 * @param resource_id: relative path of the resource within OTAmatic's storage cache
 * @param content_type: identifier of the formatter to be used to serialize the data
 * @param content: data we wish to persist in long-term storage
 * @return EXIT_SUCCESS if completed
 * @return EINPROGRESS if serialization is being completed asynchronously
 * @return other: error code indicating failure
 *
 */
extern err_t abq_persist(cstr_t resource_id,
        cstr_t content_type, cvar_t content);
/**
 * @brief extracts a previously persisted resource from persistent storage
 *
 * @param resource_id: relative path of the resource within OTAmatic's storage cache
 * @param content_type: identifier for the data-parsed used to de-serialize the data
 * @param class_of_content: target class used to structure any resulting data
 * @return EXIT_SUCCESS if completed
 * @return EINPROGRESS if serialization is being completed asynchronously
 * @return other: error code indicating failure
 */
extern cvar_t abq_restore(cstr_t resource_id,
        cstr_t content_type, class_ptr class_of_content);
/**
 * @brief extracts a configuration file from aqconfig storage
 *
 * @param config_id: relative path of the resource within OTAmatic's config folder
 * @param content_type: identifier for the data-parsed used to de-serialize the data
 * @param class_of_content: target class used to structure any resulting data
 * @return EXIT_SUCCESS if completed
 * @return EINPROGRESS if serialization is being completed asynchronously
 * @return other: error code indicating failure
 */
extern cvar_t abq_config_load(cstr_t config_id,
        cstr_t content_type, class_ptr class_of_content);
/**
 * @brief check against both partition space and configured persist store space for a minimum amount of disk space in both
 *
 * @param min_free_space: The minimum amount of available storage space in both the persist folder's partition & configured max persist dir size
 * @return true if free space is greater or equal to minimal amount, false otherwise
 */
bool_t abq_check_persist_space(size_t min_free_space);
/**
 * @brief Returns the minimum of either the available partition space, or the configured maximum usage space for the persist directory path (OTAMATIC_RESOURCES_DIR).
 *
 * @param available_bytes **[OUT]** Indicates the estimated number of bytes available when this function returns successfully.
 * @param total_bytes **[OUT]** Indicates the estimated number of total storage bytes when this function returns successfully.
 * @return EXIT_SUCCESS on success, else an error code
 */
err_t abq_get_persist_space(uint64_t *available_bytes, uint64_t *total_bytes);

#endif /* LIB_SPIL_FILE_INCLUDE_SPIL_FILE_ONTRAC_FILES_H_ */
