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
 * @file item_reader.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/rest/item_readers.h>

#ifndef MAX_ITEM_READER
#define MAX_ITEM_READER (8)
#endif /* MAX_ITEM_READER */

static void item_reader_delete(cvar_t old_reader);

DEFINE_CLASS(item_reader_class, item_reader_t, MAX_ITEM_READER,
        NULL, NULL, NULL, NULL, item_reader_delete, static);

parse_data_func_t abq_lookup_parser(cstr_t mime_type) {
    parse_data_func_t retval = NULL;
    byte_t lower_value[BUFFER_SMALL] = {0};
    if (NULL == mime_type) {
        // Default to JSON
        retval = json_reader_parse_data;
    } else if (0 < utf8_to_lower_case(mime_type,
            -1, lower_value, (int32_t)BUFFER_SMALL)) {

        if((0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "application/json", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0361. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "text/json", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0362. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "application/text", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0363. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "text/plain", -1)) // parasoft-suppress CERT_C-MSC41-a-1 "c0364. This string does not contain sensitive information."
             || (0 <= utf8_index_of(lower_value, (int32_t)sizeof(lower_value), "application/octet-stream", -1))) { // parasoft-suppress CERT_C-MSC41-a-1 "c0365. This string does not contain sensitive information."
            // Automatically select JSON parser for generic Content-Types
            retval = json_reader_parse_data;
        } else {
            // abq_status_set(UNSUPPORTED_MEDIA_TYPE, false)
            ABQ_WARN_MSG_Z("Unsupported Content-Type:", mime_type);
        }
    } else {
        ABQ_DUMP_ERROR(abq_status_pop(), mime_type);
    }
    return retval;
}

static item_reader_t* item_reader_resolve(cvar_t item) {
    item_reader_t* retval = NULL;
    CLASS_RESOLVE(item_reader_class, item_reader_t, retval, item);
    return retval;
}

static void item_reader_delete(cvar_t old_reader) {
    item_reader_t* reader = item_reader_resolve(old_reader);
    VITAL_NOT_NULL(reader);
    SET_RESERVED_FIELD(reader, item_state, NULL);
    SET_RESERVED_FIELD(reader, cache, NULL);
    SET_RESERVED_FIELD(reader, utf8, NULL);
    UNEXPECT(NULL != reader->on_consumed);
    SET_RESERVED_FIELD(reader, consumed_ctx, NULL);
    UNEXPECT(NULL != reader->on_completed);
    SET_RESERVED_FIELD(reader, completed_ctx, NULL);
}

err_t item_reader_on_item(item_reader_t *reader, err_t err, cvar_t body) {
    err_t retval = EXIT_SUCCESS;
    if (NULL == reader) {
        ABQ_ERROR_MSG("NULL item_reader_t");
        retval = EFAULT;
    } else if (NULL != reader->on_completed) {
        if (reader->on_completed(err, reader->completed_ctx, body)) {
            retval = ECANCELED;
        } else if (status_code_is_error(err)) {
            retval = err;
        } else {
            // Prepare for the next item as instructed
        	retval = traverser_reinit(reader->item_state,
        			NULL, reader->class_of_items);
            // (void) reader->on_completed(retval, reader->completed_ctx, NULL)
        }
        if (EXIT_SUCCESS != retval) {
            // Disable on consumed listener
            SET_RESERVED_FIELD(reader, consumed_ctx, NULL);
            reader->on_consumed = NULL;
            // Disable on_completed listener
            SET_RESERVED_FIELD(reader, completed_ctx, NULL);
            reader->on_completed = NULL;
        }
    } else {
        // Can't fire event without a callback
        retval = ECANCELED;
    }
    return retval;
}

static void item_reader_datareader(datatap_t * datatap, cvar_t cbdata) {
    item_reader_t* parser
        = item_reader_resolve(cbdata);
    VITAL_NOT_NULL(parser);
    VITAL_NOT_NULL(datatap);
    err_t status = EXIT_SUCCESS;
    do {
        status = parser->parse_from(parser, datatap->cache);
        if ((EXIT_SUCCESS == status) || (ENODATA == status)) {
            status = buf_pull_on_tap(datatap->cache, datatap);
        }
    } while (EXIT_SUCCESS == status);

    if (CHECK_WOULDBLOCK(status)) {
        // Retry again later on next event
    } else {
        (void) item_reader_on_item(parser, status, NULL);
        SET_RESERVED_FIELD(parser, consumed_ctx, NULL);
        VITAL_IS_OK(datatap_remove_listener(datatap,
                item_reader_datareader, cbdata));
        if (ECANCELED == status) {
            // Dump remaining data (if any) to debug
            EXPECT_IS_OK(datatap_dump_debug(datatap));
        } else {
            // Complete serialization with an error
            DATATAP_ERROR(datatap, status);
        }
    }
}

item_reader_t * item_reader_create(parse_data_func_t data_parser, class_ptr class_of_items,
        on_data_consumed_t on_consumed, cvar_t consumed_ctx,
        on_item_parsed_t on_completed, cvar_t completed_ctx) {
     item_reader_t *item_reader
         = CREATE_BASE_INSTANCE(item_reader_class, item_reader_t);
     if (NULL != item_reader) {
         (void)obj_reserve(consumed_ctx, (cvar_t)item_reader);
         (void)obj_reserve(completed_ctx, (cvar_t)item_reader);
         *item_reader = (item_reader_t) {
             .item_state = traverser_create(NULL, class_of_items, true, item_reader),
             .class_of_items = class_of_items,
             .decoder = {0},
             .encoder ={0},
             .utf8= buf_allocate(BUFFER_LARGE),
             .cache = NULL,
             .parse_from = data_parser,
             .on_consumed = on_consumed,
             .consumed_ctx = consumed_ctx,
             .on_completed = on_completed,
             .completed_ctx = completed_ctx
         };
         (void) obj_takeover(item_reader->utf8, item_reader);
         // Check that the cache & item_state for item_reader were successfully allocated
         if ((NULL == item_reader->utf8) || (NULL == item_reader->item_state)) {
             VITAL_IS_OK(obj_release_self(item_reader));
             item_reader = NULL;
         }
     }
     return item_reader;
 }

err_t datatap_item_reader(datatap_t* source,
        parse_data_func_t data_parser, class_ptr class_of_items,
        on_data_consumed_t on_consumed, cvar_t consumed_ctx,
        on_item_parsed_t on_completed, cvar_t completed_ctx) {

    err_t retval = CHECK_NULL(source);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if ((NULL == data_parser) || (NULL == on_completed)){
        retval = EFAULT;
    } else {
        item_reader_t *item_reader
                = item_reader_create(data_parser, class_of_items,
                        on_consumed, consumed_ctx, on_completed, completed_ctx);
        if (NULL == item_reader) {
            retval = abq_status_take(ENOMEM);
        } else {
            // Check that the cache for source (formatted) data is available
            if (NULL == source->cache) {
                source->cache = buf_allocate(BUFFER_SMALL);
                if (NULL == source->cache) {
                    retval = abq_status_take(ENOMEM);
                } else {
                    EXPECT_IS_OK(obj_takeover(source->cache, source));
                }
            }
            if (EXIT_SUCCESS != retval) {
                    // Return error as is
            } else {
                retval = datatap_set_combolistener(source,
                        item_reader_datareader, item_reader);
                if (EXIT_SUCCESS != retval) {
                    // Return error code as is
                } else if(abq_io_is_closed(source->flags)) {
                    // Already completed reading all data
                } else {
                    // If serialization hasn't completed, change return code to EINPROGRESS
                    retval = EINPROGRESS;
                }
            }
            VITAL_IS_OK(obj_release_self(item_reader));
        }
    }
    return retval;
}
