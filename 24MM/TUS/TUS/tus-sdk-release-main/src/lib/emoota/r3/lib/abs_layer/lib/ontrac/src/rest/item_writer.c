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
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 * ****************************************************************************
 */
/**
 * @file item_writer.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */


#include <ontrac/rest/item_writers.h>


#ifndef MAX_ITEM_WRITER
#define MAX_ITEM_WRITER (8)
#endif /* MAX_ITEM_WRITER */

static void item_writer_delete(cvar_t old_serializer);
DEFINE_CLASS(item_writer_class, item_writer_t, MAX_ITEM_WRITER,
        NULL, NULL, NULL, NULL, item_writer_delete, static);

format_data_func_t abq_lookup_formatter(cstr_t mime_type) {
    format_data_func_t retval = NULL;
    byte_t lower_value[BUFFER_SMALL] = {0};
    if (NULL == mime_type) {
        // Default to JSON
        retval = json_writer_format_data;
    } else if (0 < utf8_to_lower_case(mime_type,
            -1, lower_value, (int32_t)BUFFER_SMALL)) {
        if((0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "application/json", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0559. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "text/json", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0560. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "application/text", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0561. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "text/plain", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0562. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "application/octet-stream", -1))) { // parasoft-suppress CERT_C-MSC41-a-1 "c0563. This string does not contain sensitive information."
            retval = json_writer_format_data;
        } else {
            // abq_status_set(UNSUPPORTED_MEDIA_TYPE, false)
            ABQ_WARN_MSG_Z("Unsupported Content-Type:", mime_type);
        }
    } else {
        ABQ_DUMP_ERROR(abq_status_pop(), mime_type);
    }
    return retval;
}

item_writer_t* item_writer_resolve(cvar_t item) {
    item_writer_t* retval = NULL;
    CLASS_RESOLVE(item_writer_class, item_writer_t, retval, item);
    return retval;
}

static void item_writer_delete(cvar_t old_serializer) {
    item_writer_t* serializer
        = item_writer_resolve(old_serializer);
    VITAL_NOT_NULL(serializer);
    SET_RESERVED_FIELD(serializer, item_state, NULL);
    SET_RESERVED_FIELD(serializer, cache, NULL);
    SET_RESERVED_FIELD(serializer, format_ctx, NULL);
}

static void item_writer_datawriter(datasink_t * datasink, cvar_t cbdata) {
    err_t status_code = EXIT_SUCCESS;
    VITAL_NOT_NULL(datasink);
    item_writer_t* serializer
        = item_writer_resolve(cbdata);
    VITAL_NOT_NULL(serializer);
    do {
        status_code = serializer->format_into(serializer, serializer->cache);
        if ((EOVERFLOW == status_code) || (ECANCELED == status_code)) {
            // Attempt to make room in the cache for more data
            status_code = buf_push_to_sink(serializer->cache, datasink);
        }
    } while (EXIT_SUCCESS == status_code);

    if (CHECK_WOULDBLOCK(status_code)) {
        // Unable to ship current state, try again later
    } else {
        // All data was written into datasink_t, iterate to the next data-listener
        VITAL_IS_OK(datasink_remove_listener(datasink,
                    item_writer_datawriter, cbdata));
        SET_RESERVED_FIELD(serializer, format_ctx, NULL);
        if (ENODATA != status_code) {
            // Close sink on error
            DATASINK_ERROR(datasink, status_code);
        }
    }
}

item_writer_t* item_writer_create(format_data_func_t formatter, cvar_t item,
        byte_buffer_t* cache, bool_t indent, on_data_formatted_t on_format, cvar_t format_ctx) {
    item_writer_t* retval= NULL;
    if((NULL == formatter) || (NULL == cache)) {
        abq_status_set(EINVAL, false);
    } else {
        retval = CREATE_BASE_INSTANCE(item_writer_class, item_writer_t);
        if (NULL != retval) {
            (void)obj_reserve(cache, retval);
            (void)obj_reserve(format_ctx, retval);
            *retval = (item_writer_t) {
                .item_state = traverser_create(item, NULL, true, retval),
                .decoder = {0},
                .encoder ={0},
                .cache = cache,
                .format_into = formatter,
                .on_format = on_format,
                .format_ctx = format_ctx,
                .indent = indent
            };

        	// Expect the traverser_t to be in initial state
        	err_t status = traverser_step(retval->item_state);
        	if (EXIT_SUCCESS != status) {
                (void) abq_status_set(status, false);
                (void) obj_release_self(retval);
                retval = NULL;
        	}
        }
    }
    return retval;
}

err_t item_writer_set_sink(const item_writer_t* serializer, datasink_t* dest) {
    err_t retval = datasink_set_combolistener(dest,
            item_writer_datawriter, serializer);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if (abq_io_is_flushed(dest->flags)) {
        // Already completed sinking data
    } else {
        // If serialization hasn't completed, change return code to EINPROGRESS
        retval = EINPROGRESS;
    }
    return retval;
}

