/*
 * files.c
 *
 *  Created on: Apr 22, 2019
 *      Author: mvogel
 */

#include <spil_file/ontrac/files.h>
#include <ontrac/util/byte_buffer.h>
#include <ontrac/rest/item_readers.h>
#include <ontrac/rest/item_writers.h>


static int64_t abq_persist_dir_max = -1L;

err_t abq_files_init( const char* config_dir,
        const char *persist_dir, uint64_t persist_dir_max_bytes,
        sf_lock_mutex_t lock, sf_unlock_mutex_t unlock, cvar_t mutex) {
    err_t retval = EXIT_SUCCESS;
    byte_t fullpath[PLATFORM_MAX_FILE_PATH_LEN];
    if((NULL == config_dir) || (NULL == persist_dir)
            || (NULL == lock) || (NULL == unlock) ) {
        retval = EFAULT;
    } else {
        sf_init(lock, unlock, mutex);
        int32_t persist_dir_len = sf_path_resolve(persist_dir,
                fullpath, sizeof(fullpath));
        if((0 >= persist_dir_len) || (persist_dir_len >= (int32_t)sizeof(fullpath))) {
            ABQ_ERROR_MSG_Z("Invalid persist_dir:", persist_dir);
            retval = EINVAL;
        } else if(1 != sf_is_dir(persist_dir)){
            ABQ_DUMP_ERROR(abq_status_pop(), fullpath);
            retval = ENOTDIR;
        } else { // TODO upgrade spil-file to check for read/write permissions
            bytes_copy(OTAMATIC_RESOURCES_DIR, fullpath, sizeof(fullpath));
            ABQ_INFO_MSG_Z("Persist-dir: ", OTAMATIC_RESOURCES_DIR);
            int32_t config_dir_len = sf_path_resolve(config_dir,
                    fullpath, sizeof(fullpath));
            if((0 >= config_dir_len) || (config_dir_len >= (int32_t)sizeof(fullpath))) {
                ABQ_ERROR_MSG_Z("Invalid config_dir:", config_dir);
                retval = EINVAL;
            } else if(1 != sf_is_dir(config_dir)){
                ABQ_DUMP_ERROR(abq_status_pop(), fullpath);
                retval = ENOTDIR;
            } else { // TODO upgrade spil-file to check for read/write permissions
                bytes_copy(OTAMATIC_CONFIG_DIR, fullpath, sizeof(fullpath));
                ABQ_INFO_MSG_Z("Config-dir: ", OTAMATIC_CONFIG_DIR);
                if(persist_dir_max_bytes >= (uint64_t) INT64_MAX) {
                    // Truncate at INT64_MAX if needed
                    abq_persist_dir_max = INT64_MAX;
                } else {
                    abq_persist_dir_max = (int64_t) persist_dir_max_bytes;
                }
            }
        }
    }
    return retval;
}

int32_t abq_du(cstr_t dirName, uint64_t *total_bytes){
    int32_t rvalue = EXIT_SUCCESS;
    uint64_t sum = 0;
    sf_info_t info = {0};

    vlist_t *dir_list = NULL;

    byte_t name[PLATFORM_MAX_FILE_NAME_LEN];
    bytes_set(name, '\0', sizeof(name));
    if ((NULL != dirName) && (NULL != total_bytes)) {
        if (sf_is_dir(dirName) == 1) {
            dir_list = vlist_create(&string_class);
            EXPECT_IS_OK(vlist_add(dir_list, (str_t)dirName));

            cstr_t dir = NULL;
            // loop list of directories. list is initialized with parameter 'dirName'
            // more entries are added to the list as they are encountered
            while(false == vlist_is_empty(dir_list)) {
                dir = str_resolve(vlist_pop(dir_list, NULL));
                VITAL_NOT_NULL(dir);
                sf_dir_t fd = {0};
                if (0 == sf_opendir(dir, &fd)) {
                    // iterate over all items in 'dir'
                    while (0 == sf_next_dir_item(&fd, &info, name, sizeof(name))) {
                        cstr_t cur_item = abq_path_join(dir, name, abq_file_sys_path);
                        if ((NULL != cur_item) &&
                            (name[0] != '.')) {   // ignore hidden files, current and parent directory
                            /* get item properties */
                            if (0 == sf_get_file_info(cur_item, &info)) {
                                if (info.isDir != 0) {
                                    // if 'cur_item' is dir => add to dir_list
                                    EXPECT_IS_OK(vlist_add(dir_list, cur_item));
                                }
                                sum += info.size;
                                //printf("%i\t%s\n", info.size, cur_item)
                            }
                        }
                        (void) obj_release_self(cur_item);
                        cur_item = NULL;
                    }
                }
                (void) sf_close_dir(&fd);
                (void) obj_release_self(dir);
            }
        } else {
            rvalue = EINVAL;
        }
        *total_bytes = sum;
    } else {
        rvalue = EINVAL;
    }

    (void)obj_release_self(dir_list);
    return rvalue;
}

static cstr_t pathof_first_child(cstr_t folder) {
    cstr_t retval = NULL;
    sf_dir_t fd = {0};
    sf_info_t info = {0};
    byte_t name[PLATFORM_MAX_FILE_NAME_LEN];
    if (0==sf_opendir(folder, &fd)) {
        while ((NULL == retval) &&
                (0 == sf_next_dir_item(&fd, &info, name, sizeof(name)))) {
            if(name[0] == '.') {
                // ignore hidden files, current and parent directory
            } else {
                retval = abq_path_join(folder, name, abq_file_sys_path);
            }
        }
        (void) sf_close_dir(&fd);
    }
    return retval;
}

err_t abq_rmrf(const char *filename) {
    err_t retval = EXIT_SUCCESS;
    sf_info_t info = {0};
    (void) abq_status_pop(); // clear status if any
    vlist_t *dir_list = vlist_create(&string_class);
    if (0 != sf_get_file_info(filename, &info)) {
        retval = abq_status_take(retval);
        if (ENOENT == retval) {
            retval = EALREADY;
        }
    } else if (false == info.isDir) {
        retval = sf_delete_file(filename);
    } else {
        cstr_t parent = str_coerce(filename, NULL);
        do {
            cstr_t child = pathof_first_child(parent);
            if (NULL == child) {
                // All children in the current folder have been removed,
                //  attempt to delete the folder itself and walk backwards
                retval = sf_delete_dir(parent);
                (void) obj_release_self(parent);
                parent = str_resolve(vlist_pop(dir_list, NULL));
            } else if (1 == sf_is_dir(child)) {
                // Drill down into directories as they are encountered
                EXPECT_IS_OK(vlist_insert(dir_list, 0U, parent));
                (void) obj_release_self(parent);
                parent = child;
            } else {
                retval = sf_delete_file(child);
                (void) obj_release_self(child);
            }
        } while ((EXIT_SUCCESS == retval)
                && (NULL != parent));
        (void) obj_release_self(parent);
    }

    if (status_code_is_error(retval)) { // parasoft-suppress MISRAC2012-DIR_4_7-a-2 "Return value of sf_delete_file() is checked by status_code_is_error()."
        // sf_* returns code is often '-1' w/ errno set
        retval = abq_status_take(retval);
        // Dir 4.7 If a function returns error information, then that error information shall be tested
        if (EXIT_SUCCESS == retval) {
            ABQ_FATAL_STATUS(retval);
        } else {
            ABQ_DUMP_ERROR(retval, filename);
        }
    }

    (void) obj_release_self(dir_list);
    return retval;
}

cstr_t abq_path_localize(cstr_t path, abq_path_type_t type) {
    cstr_t rvalue = NULL;
    if (NULL != path) {
        byte_t buffer[PLATFORM_MAX_FILE_PATH_LEN];
        bytes_set(buffer, '\0', sizeof(buffer));
        int32_t path_len = sf_path_localize(path, buffer, sizeof(buffer), type);
        if (0 > path_len) {
            (void) abq_status_set((err_t) path_len, false);
        } else if(sizeof(buffer) <= (size_t)path_len) {
            (void) abq_status_set(EOVERFLOW, false);
        } else {
            rvalue = str_create(buffer, (int32_t) sizeof(buffer), false);
        }
    }
    return rvalue;
}

cstr_t abq_path_join(cstr_t parent, cstr_t child, abq_path_type_t type) {
    cstr_t rvalue = NULL;
    if ((NULL != parent) && (NULL != child)) {
        byte_t buffer[PLATFORM_MAX_FILE_PATH_LEN];
        bytes_set(buffer, '\0', sizeof(buffer));
        err_t err = sf_path_join(parent, child, buffer, sizeof(buffer), type);
        if (err < 0) {
            (void) abq_status_set(err, false);
        } else {
            rvalue = str_create(buffer, (int32_t) sizeof(buffer), false);
        }
    }
    return rvalue;
}

err_t abq_persist(cstr_t resource_id,
        cstr_t content_type, cvar_t content) {
    err_t retval = EXIT_SUCCESS;
    format_data_func_t formatter = abq_lookup_formatter(content_type);
    if (NULL == resource_id) {
        retval = EFAULT;
    } else if(NULL == formatter) {
        retval = abq_status_take(UNSUPPORTED_MEDIA_TYPE);
    } else {
        abq_context_lock();
        byte_buffer_t *buf = buf_wrap( abq_ctx.buffer,
                (size_t)abq_ctx.buf_size, 0U, false);
#if !defined(NDEBUG)
        item_writer_t* writer = item_writer_create(formatter,
                content, buf, true, NULL, NULL);
#else /* NDEBUG */
        item_writer_t* writer = item_writer_create(formatter,
                content, buf, false, NULL, NULL);
#endif /* NDEBUG */
        retval = json_writer_format_data(writer, buf);
        if (ECANCELED != retval) {
            // Return error as is
        } else {
            size_t amount = buf_remaining(buf);
            ABQ_VITAL(0U != amount);
            // Overwrite existing data with the new object
            //  TODO, write to a temp-file and then move the temp-file to given location only once all data has been written out
            //   the will make the write an atomic operation (on posix systems) & help prevent half-written data / file corruption
            retval = abq_resource_write(resource_id, ptr2uint8(buf_data(buf)), amount);
        }
        (void)obj_release_self(writer);
        (void)obj_release_self(buf);
        abq_context_unlock();
    }
    return retval;
}

static cvar_t abq_extract(cstr_t root_dir, cstr_t resource_id,
        cstr_t content_type, class_ptr class_of_content) {
    cvar_t retval = NULL;
    size_t byte_count = 0;
    parse_data_func_t parser = abq_lookup_parser(content_type);
    if (NULL == resource_id) {
        abq_status_set(EFAULT, false);
    } else if(NULL == parser) {
        abq_status_set(UNSUPPORTED_MEDIA_TYPE, false);
    } else {
        abq_context_lock();
        err_t status = abq_file_read(root_dir, resource_id, ptr2uint8(abq_ctx.buffer),
                (size_t)abq_ctx.buf_size, &byte_count);
        if (EXIT_SUCCESS != status) {
            abq_status_set(status, false);
            ABQ_WARN_STATUS(abq_status_peek(), resource_id);
        } else {
            var_result_t* vessel = var_result_create(NULL, EXIT_SUCCESS, NULL, NULL);
            byte_buffer_t *buf = buf_wrap(abq_ctx.buffer,
                    abq_ctx.buf_size, byte_count, false);
            item_reader_t* reader = item_reader_create(parser,
                    class_of_content, NULL, NULL, var_result_on_item_parsed, vessel);
            if ((NULL == vessel) || (NULL == buf) || (NULL == reader)) {
                abq_status_set(ENOMEM, false);
                ABQ_DUMP_ERROR(abq_status_peek(), resource_id);
            } else {
                status = parser(reader, buf);
                if ((EXIT_SUCCESS == vessel->status) && (NULL == vessel->item)) {
                    // parser hasn't to completed parsing, status is likely ENODATA
                    status = item_reader_on_item(reader, status, NULL);
                    if (EXIT_SUCCESS == vessel->status) {
                        vessel->status = status;
                    }
                }
                // Check the results
                retval = vessel->item;
                if (NULL != retval) {
                    // Reserve self-reference to the result so that it is not garbage collected
                    (void) obj_reserve_self(retval);
                } else {
                    abq_status_set(vessel->status, false);
                }
            }
            (void) obj_release_self(reader);
            (void) obj_release_self(buf);
            (void) obj_release_self(vessel);
        }
        abq_context_unlock();
    }
    return retval;
}

cvar_t abq_restore(cstr_t resource_id,
        cstr_t content_type, class_ptr class_of_content) {
    return abq_extract(OTAMATIC_RESOURCES_DIR, resource_id, content_type, class_of_content);
}

cvar_t abq_config_load(cstr_t config_id,
        cstr_t content_type, class_ptr class_of_content) {
    return abq_extract(OTAMATIC_CONFIG_DIR, config_id, content_type, class_of_content);
}

bool_t abq_check_persist_space(size_t min_free_space) {
    bool_t retval = false; // Default to false (insufficient space)
    uint64_t used_bytes = 0UL, free_space = 0UL, total_space=0UL;
    err_t status = sf_get_system_space(OTAMATIC_RESOURCES_DIR,
            &free_space, &total_space);
    if (EXIT_SUCCESS != status) {
        ABQ_DUMP_ERROR(status, OTAMATIC_RESOURCES_DIR);
        // Use default retval
    } else if(min_free_space > free_space) {
        // Insufficient free-space
        retval = false;
    } else if(abq_persist_dir_max < 0L) {
        // Sufficient free-space
        retval = true;
    } else {
        status = abq_du(OTAMATIC_RESOURCES_DIR, &used_bytes);
        free_space = ((uint64_t)abq_persist_dir_max) - used_bytes;
        if (EXIT_SUCCESS != status) {
            ABQ_DUMP_ERROR(status, OTAMATIC_RESOURCES_DIR);
            // Use default retval
        } else if ((used_bytes > (uint64_t)abq_persist_dir_max)
                || (min_free_space > free_space)){
            // Insufficient free-space
            retval = false;
        } else {
            // Sufficient free-space
            retval = true;
        }
    }
    return retval;
}

err_t abq_get_persist_space(uint64_t *available_bytes, uint64_t *total_bytes) {
    uint64_t used_bytes = 0UL, free_space = 0UL, partition_size = 0UL;
    err_t retval = sf_get_system_space(OTAMATIC_RESOURCES_DIR,
                                       available_bytes, &partition_size);
    if (EXIT_SUCCESS == retval) {
        if (NULL != total_bytes) {
            *total_bytes = partition_size;
        }
        if (abq_persist_dir_max > 0L) {
            if(partition_size > (uint64_t)abq_persist_dir_max) {
                partition_size = (uint64_t)abq_persist_dir_max;
            }
            if (NULL != available_bytes) {
                retval = abq_du(OTAMATIC_RESOURCES_DIR, &used_bytes);
                free_space = partition_size - used_bytes;
                if ((EXIT_SUCCESS == retval) && (used_bytes <= partition_size)
                        && (free_space < *available_bytes)) {
                    *available_bytes = free_space;
                }
            }
        }
    }
    return retval;
}
