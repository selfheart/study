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

/** used to deliminate between folders on the system drive */
#if defined(_WIN32) && !defined(__SYMBIAN32__)
static const byte_t sf_file_system_dir_separator = '\\'; // should match AQ_PATH_SEPARATOR
#else
static const byte_t sf_file_system_dir_separator = '/'; // should match AQ_PATH_SEPARATOR
#endif

/** used to deliminate between folders in a network (URL) path */
static const byte_t sf_network_dir_separator = '/'; // should match AQ_PATH_SEPARATOR

int32_t sf_path_resolve(cstr_t relative, byte_t *dest, size_t dest_len) {
    int32_t retval = EUNSPECIFIED;
    if((NULL == dest) || (2U >= dest_len)) {
        // Error with dest, return -1
    } else if((NULL == relative) || (relative[0] == '\0')) {  // The utf8_end_of_token() below does not work well with zero length string
        // Error, but initialize dest anyway
        bytes_set(dest, '\0', dest_len);
    } else {
#if defined(_WIN32) && !defined(__SYMBIAN32__)
        dest = _fullpath(dest, relative, (size_t)dest_len);
        retval = (int32_t) strnlen(dest, dest_len);
        // Need to trim off the trailing '/' on directory paths on windows
        while ((1 < retval) && (sf_file_system_dir_separator == dest[retval - 1])) {
            retval -= 1;
            dest[retval] = '\0';
        }
#else
        ABQ_ENCODER(encoder, &utf8_codec, dest, dest_len);
        byte_t linked_path[PLATFORM_MAX_FILE_PATH_LEN];
        int32_t start_of_token = 0, linked_path_len = -1,
                token_size = utf8_end_of_token(relative, -1,  "/\\", -1);
        if (0 >= token_size) {
            /* Absolute path, begin @ root */
            retval = EXIT_SUCCESS;
        } else {
            /* Relative path, begin @ current-working-directory */
            ABQ_ERROR_MSG_Z("Relative path not allowed:", relative);
            retval = EUNSPECIFIED;
        }
        while(((EXIT_SUCCESS == retval) && (0 <= token_size))){
            if (0 != token_size) {
                // token exists, parse it
                if((1 == token_size)
                        && ('.' == relative[start_of_token])) {
                    /* Skip over isolated '.' nodes */
                } else if((2 == token_size)
                        && ('.' == relative[start_of_token])
                        && ('.' == relative[start_of_token+1])) {
                    /* On '..' back up to parent directory */
                    linked_path_len = sf_parent_dir(dest,
                            linked_path, sizeof(linked_path) - 1U);
                    if (0 >= linked_path_len) {
                        // Expect that it was not a symbolic link
                        retval = abq_status_take(linked_path_len);
                    } else if((size_t)linked_path_len >= dest_len){
                        retval = EOVERFLOW;
                    } else {
                        // Update the result with resolved path (minus delimiter)
                        encoder.pos = ((size_t)linked_path_len) - 1U;
                        bytes_copy(linked_path, dest, encoder.pos);

                    }
                } else {
                    // Delimit each path components
                    retval = abq_encode_char(&encoder,
                            sf_file_system_dir_separator);
                    if(EXIT_SUCCESS == retval) {
                        retval = abq_encode_ascii(&encoder,
                                &relative[start_of_token], token_size);
                    }
                }
            }
            start_of_token += token_size;
            if('\0' == relative[start_of_token]) {
                break;
            } else {
                start_of_token += 1; // skip over delimiter character
                token_size = utf8_end_of_token(&relative[start_of_token], -1,  "/\\", -1);
            }
        }
        // Finished resolving 'relative' path, check results are sound and return appropriate value
        if ((EXIT_SUCCESS == retval) && (0U == encoder.pos)) {
            // If no pathname components where encountered, add root delimiter
            retval = abq_encode_char(&encoder,
                    sf_file_system_dir_separator);
        }
        if (EXIT_SUCCESS != retval) {
            abq_status_set(retval, false);
            retval = EUNSPECIFIED;
        } else {
            retval = (int32_t) encoder.pos;
            if (encoder.pos < dest_len) {
                dest[retval] = '\0';
            }
        }
#endif
    }
    return retval;
}

int32_t sf_path_localize(cstr_t path, byte_t *dest, size_t dest_len, abq_path_type_t type) {
    int32_t retval = EUNSPECIFIED;
    byte_t delimiter = sf_file_system_dir_separator;
    cstr_t invalid_path_chars = SF_INVALID_FILENAME_CHARS;
    if(abq_network_path == type) {
        delimiter = sf_network_dir_separator;
        invalid_path_chars = "";
    }

    if ((NULL == dest) || (2 >= (int32_t)dest_len)) {
        // Error with dest, return -1
    } else if(NULL == path) {
        // Assume dest was uninitialized
        bytes_set(dest, '\0', dest_len);
    } else {
        retval = -1;
        do {
            retval++;
            /*
            if ((0 == retval) &&
                    (('/' != path[0]) && ('\\' != path[0]))) {
                ABQ_EXPECT(cwd_len == utf8_write_bytes(dest, default_cwd, cwd_len));
                retval = cwd_len;
            }
            */
            if (dest_len == (size_t)retval) {
                // Reached end of dest buffer, still have not find the end of path
                // abq_status_set(EOVERFLOW, false)
                retval =  EUNSPECIFIED;
                if (0U != dest_len) {
                    dest[0U] = '\0';
                }
            } else if(('/' == path[retval]) || ('\\' == path[retval])) {
                dest[retval] = delimiter;
            } else if(0 <= utf8_index_of_char(invalid_path_chars, -1, path[retval])) {
                // SF_INVALID_FILENAME_CHARS are not allowed
                //abq_status_set(EILSEQ, false)
                dest[retval] = '\0';
                retval = EUNSPECIFIED;
            } else if((path[retval] < ' ')
                    && (path[retval] != '\0')) {
                // No control-characters either
                //abq_status_set(EILSEQ, false)
                dest[retval] = '\0';
                retval = EUNSPECIFIED;
            } else {
                dest[retval] = path[retval];
            }
        } while ((0 <= retval) && ('\0' != path[retval]));
    }
    return retval;
}

int32_t sf_path_join(cstr_t parent, cstr_t child, byte_t *dest, size_t dest_len, abq_path_type_t type) {
    int32_t retval = 0, offset = 0;
    const byte_t delimiter = (abq_file_sys_path == type)
             ? sf_file_system_dir_separator : sf_network_dir_separator;
    if ((NULL == dest) || (dest_len == 0UL)) {
        retval = -1;
    } else {
        if (NULL != parent) {
            retval = sf_path_localize(parent, dest, dest_len, type);
        } else {
            // Assume dest is uninitialized
            bytes_set(dest, '\0', dest_len);
        }
        if(0 > retval) {
            // Return error as is
        } else if(retval >= (int32_t)dest_len) {
            // No space to write child data
            errno = EOVERFLOW;
            retval = -1;
        } else if((NULL == child) || ('\0' == child[0])) {
            // Localization of parent was completed
            //  check for tailing delimiter
            if((0 != retval) && (delimiter != dest[retval-1])) {
                if(retval >= (int32_t)dest_len) {
                    // No space to write a delimiter
                    errno = EOVERFLOW;
                    retval = -1;
                } else {
                    // Append a tailing delimiter after parent
                    dest[retval] = delimiter;
                    retval++;
                    if(retval < (int32_t)dest_len) {
                        dest[retval] = '\0';
                    }
                }
            }
        } else {
            if((0 != retval) && (delimiter == dest[retval-1])) {
                // Already ends with a delimiter
                if(('/' == child[0]) || ('\\' == child[0])) {
                    // Skip over repeat delimiter
                    offset = sf_path_localize(&child[1],
                            &dest[retval], (dest_len - (size_t)retval), type);
                } else {
                    // Child doesn't start with a delimiter, write all
                    offset = sf_path_localize(child,
                            &dest[retval], (dest_len - (size_t)retval), type);
                }
            } else {
                // Parent didn't end with a delimiter
                if ((NULL != parent) && ('/' != child[0]) && ('\\' != child[0])) {
                    // And child doesn't start with a delimiter
                    dest[retval] = delimiter;
                    retval++;
                }
                // Now that it is delimited, write out child
                offset = sf_path_localize(child,
                        &dest[retval], (dest_len - (size_t)retval), type);
            }
            if (0 >= offset) {
                // Failed to write child data
                retval = -1;
            } else {
                retval += offset;
            }
        }
    }
    return retval;
}

int32_t sf_filename_of(cstr_t path, byte_t *dest, size_t dest_len) {
    int32_t index = -1, offset = -1;
    if((NULL == dest) || (0U == dest_len)) {
        // Error with dest, return -1
    } else if(NULL == path) {
        // Error, but initialize dest anyway
        bytes_set(dest, '\0', dest_len);
    } else {
        byte_t ch = '?';
        size_t in_len = (size_t)utf8_byte_length(path, -1);
        for (size_t i = 0; i < in_len; i++) {
            ch = path[i];
            if ('\0' == ch) {
                break;
            }
            if (('/' == ch) || ('\\' == ch)) {
                // overwrite previous file separator
                // since we want last index of
                offset = (int32_t) i;
            }
        }
        // we do not with to include the separator in the filename
        //  or if no separator was found we need to increment to 0
        offset++;
        for (index = 0; index < (int32_t) dest_len; index++) {
            dest[index] = path[index + offset];
            if ('\0' == dest[index]) {
                break;
            }
        }
        if (index == (int32_t) dest_len) {
            index = -1 * EOVERFLOW;
        }
    }
    return index;
}

int32_t sf_parent_dir(cstr_t filepath,
        byte_t *dest, size_t dest_len) {
    int32_t rvalue = EUNSPECIFIED;
    size_t offset = 0UL;
    if((NULL == dest) || (0U == dest_len)) {
        // Error with dest, return -1
    } else if(NULL == filepath) {
        // Error, but initialize dest anyway
        bytes_set(dest, '\0', dest_len);
    } else {
        offset = (size_t)utf8_byte_length(filepath, -1);
        if (0U != offset) {
            offset--; // Skip over end character (may be a folder delimiter)
        }
        while (offset > 0U) {
            offset--;
            byte_t ch = filepath[offset];
            if (('/' == ch) || ('\\' == ch)) {
                // Do include the delimiter in the final sequence
                offset++;
                // Found a folder delimiter
                if(offset >= dest_len) {
                    errno = EOVERFLOW;
                } else {
                    rvalue = (int32_t) offset;
                    dest[rvalue] = '\0';
                    if (filepath != dest) {
                        (void) bytes_copy(dest, filepath, rvalue);
                    }
                }
                break;
            }
        }
        if ((0 > rvalue) && (0U == offset)) {
            if(offset >= dest_len) {
                errno = EOVERFLOW;
            } else {
                rvalue = 0;
                dest[rvalue] = '\0';
            }
        }
    }
    return rvalue;
}
