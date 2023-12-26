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
 * @file item_readers.h
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#ifndef ONTRAC_REST_ITEM_READERS_H_
#define ONTRAC_REST_ITEM_READERS_H_

#include <ontrac/ontrac/traverser.h>
#include <ontrac/stream/datatap.h>
#include <ontrac/util/misc_items.h>

typedef struct for_item_reader item_reader_t;

/**
 * @brief callback used to deliver final parsing results to back to the user when de-serialization has completed
 *
 * @param err: status code indicating final state of parsing, EXIT_SUCCESS for successfully parsed item
 * @param ctx: the callback context passed in alongside the callback
 * @param body: the fully parsed item created from the de-serialized data
 * @return true to continue parsing items from the input-data, else false
 */
typedef bool_t (*on_item_parsed_t) (err_t err, cvar_t ctx, cvar_t body);

/**
 * @brief callback used to notify user each time some raw data was successfully parsed by the deserializer_t
 *
 * @param ctx: the callback context passed in alongside the callback
 * @param item_state: current state of the de-serialized item AFTER parsing the given data
 * @param rawdata: unmodified data read from the input-data which has been parsed by the deserializer_t
 * @param amount: number of bytes in rawdata that where consumed by the parser
 * @return EXIT_SUCCESS to continue parsing, else an error code to indicate parsing has failed.
 */
typedef err_t (*on_data_consumed_t) (cvar_t ctx,
        traverser_t *item_state, const byte_t *rawdata, size_t amount);
/**
 * @brief typedef for a function used to process input-data with an item-reader
 *  The datatap_t or equivalent will feed sequencial segments of data into this function for parsing
 *
 * @param reader: used to cache the item state of the deserialized item
 * @param input_data: additional data used to represent the item being deserialized
 * @return ENODATA if it expects more input data to finish current item
 * @return ECANCELED when completed
 */
typedef err_t (*parse_data_func_t) (item_reader_t *reader, byte_buffer_t *input_data);

struct for_item_reader {
    /** traverser_create(NULL, class_of_items, true, item_reader), */
    traverser_t *item_state;
    /** type of item to expect from the data stream */
    class_ptr class_of_items;
    /** decodes formatted/escaped character from the given language into unicode characters*/
    abq_decoder_t decoder;
    /** encodes unicode into internal UTF8 strings */
    abq_encoder_t encoder;
    /** Potentially cached input data from last call to parse_from function*/
    byte_buffer_t *cache;
    /** Additional cache can be used to store partially decoded utf8 data which does not make a full string */
    byte_buffer_t *utf8;
    /** function pointer to function used to process raw data into item(s) */
    parse_data_func_t parse_from;
    /** Optional callback to be invoked each time some data is parsed from buffer */
    on_data_consumed_t on_consumed;
    cvar_t consumed_ctx;
    on_item_parsed_t on_completed;
    cvar_t completed_ctx;
};

extern parse_data_func_t abq_lookup_parser(cstr_t mime_type);

extern item_reader_t* item_reader_create(parse_data_func_t data_parser, class_ptr class_of_items,
        on_data_consumed_t on_consumed, cvar_t consumed_ctx,
        on_item_parsed_t on_completed, cvar_t completed_ctx);

extern err_t datatap_item_reader(datatap_t* source,
        parse_data_func_t data_parser, class_ptr class_of_items,
        on_data_consumed_t on_consumed, cvar_t consumed_ctx,
        on_item_parsed_t on_completed, cvar_t completed_ctx);

extern err_t item_reader_on_item(item_reader_t *reader, err_t err, cvar_t body);

/**
 * @brief decodes formatted data from the datatap_t and invokes the callback once the top-level item is fully parsed or error occurs
 *
 * @param source: pointer to an datatap_t from which the formatted data will be received
 * @param class_of_items: the target class used for identify a structure and map data to the fields within
 * @param on_consumed: callback to be invoked in sequence each time data is parsed by the parser
 * @param consumed_ctx:  a context to be passed back to the on_consumed callback each time it is invoked
 * @param on_completed: callback to be called once instance is parsed or error occurs
 * @param completed_ctx: a context to be passed back to the on_completed callback when it is parsed
 * @return EXIT_SUCCESS to indicate that item has been fully parsed
 * @return EINPROGRESS to indicate that deserializer_t is listening for additional data from the datatap_t
 * @return else different error code for failure
 */
extern err_t datatap_read_json(datatap_t* source, class_ptr class_of_items,
        on_data_consumed_t on_consumed, cvar_t consumed_ctx,
         on_item_parsed_t on_completed, cvar_t completed_ctx);

/**
 * @brief attempts to parse a JSON serialized item with a given class from the source data
 *
 * @param source: data containing JSON content
 * @param source_len: maximum number of bytes to read from the source buffer
 * @param class_of_content: type of class used to deserialize the JSON content
 * @return pointer to a self-referencing deserialized instance of the given class, or NULL on failure
 */
extern cvar_t str_parse_json(cstr_t source, int32_t source_len, class_ptr class_of_items);
/**
 * @brief attempts to parse a JSON serialized item with a given class from the source data
 *
 * @param source: data containing JSON content
 * @param source_len: maximum number of bytes to read from the source buffer
 * @param class_of_items: type of class used to deserialize the JSON content
 * @param on_consumed: A callback to be invoked each time the serializer consumes data
 * @param consumed_ctx: A context to be passed back to on_consumed callback when invoked
 * @return pointer to a self-referencing deserialized instance of the given class, or NULL on failure
 */
extern cvar_t str_parse_json_ext(cstr_t source, int32_t source_len,
        class_ptr class_of_items, on_data_consumed_t on_consumed, cvar_t consumed_ctx);
/**
 * @brief A JSON parser implementation matching parse_data_func_t
 *
 * @param reader: initialized instance of a data-reader used for storing item state as it is being deserialized
 * @param input_data: a buffer filled with (partial) JSON formatted data
 * @return ENODATA: if more data is required in order to finish the current item
 * @return ECANCELED: current item was fully parsed
 */
extern err_t json_reader_parse_data(item_reader_t *reader, byte_buffer_t *input_data);

#endif /* ONTRAC_REST_ITEM_READERS_H_ */
