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
 * @file json_reader.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ctype.h>
#include <ontrac/stream/mem_tap.h>

#include <ontrac/rest/item_readers.h>
#include <ontrac/ontrac/primitives.h>

static err_t json_reader_got_child(item_reader_t *reader, cvar_t item) {
	err_t retval = traverser_set_child(reader->item_state,
	        traverser_leaf(reader->item_state), item);
	if (traverser_is_completed(reader->item_state)) {
		if (ECANCELED == retval) {
			retval = EXIT_SUCCESS;
		}
        // parsed a primitive as the top-level-item
        retval = item_reader_on_item(reader, retval, item);
	} else {
		// Continue iterating as normal
	}
    return retval;
}

static err_t json_reader_begin_object(item_reader_t *reader) {
    return traverser_begin_node(reader->item_state, NULL, &ptree_class);
}

static err_t json_reader_finish_object(item_reader_t *reader) {
    err_t status = traverser_finalize_node(reader->item_state);
    if (ECANCELED == status) {
    	ABQ_VITAL(traverser_is_completed(reader->item_state));
		abq_node_t *root = traverser_root(reader->item_state);
    	if (0 >= root->child_counter) {
    		// Translate empty objects at the top-level to NULL
      		status = item_reader_on_item(reader, EXIT_SUCCESS, NULL);
    	} else {
            cvar_t item = abq_coerce(root->item, root->meta, reader);
			status = item_reader_on_item(reader, EXIT_SUCCESS, item);
			(void) obj_release(item, reader);
    	}
    }
    return status;
}

static err_t json_reader_begin_list(item_reader_t *reader) {
    return traverser_begin_node(reader->item_state, NULL, &vlist_class);
}

static err_t json_reader_finish_list(item_reader_t *reader) {
    err_t status = traverser_finalize_node(reader->item_state);
    if(ECANCELED == status) {
    	ABQ_VITAL(traverser_is_completed(reader->item_state));
		abq_node_t *root = traverser_root(reader->item_state);
        cvar_t item = abq_coerce(root->item, root->meta, reader);
		status = item_reader_on_item(reader, EXIT_SUCCESS, item);
		(void) obj_release(item, reader);
    }
    return status;
}

static err_t json_reader_parse_string(item_reader_t *reader) {
    byte_buffer_t *decoded = reader->utf8;
    (void) buf_compact(decoded);

    // Reinitialize the UTF8 encoder with current status of the cache
    reader->encoder = BUF_ENCODER_INIT(&utf8_codec, decoded);

    err_t retval = EXIT_SUCCESS;
    size_t decoder_max = reader->decoder.max;
    if (&json_codec != reader->decoder.codec) {
        // JSON strings must start with a '\"' (quotation)
        if ('\"' != abq_decode_char(&reader->decoder)) {
            retval = abq_status_take(EILSEQ);
        } else {
            // Have now started JSON decoding
            reader->decoder.codec = &json_codec;
            // Already Skipped over the initial quotation
        }
    }
    if(EXIT_SUCCESS == retval){
        do {
            int32_t codepoint = abq_decode_cp(&reader->decoder);
            if (0 > codepoint) {
                retval = abq_status_take(EIO);
            } else {
                // Write out the codepoint we just read in
                retval = abq_encode_cp(&reader->encoder, codepoint);
                if((EOVERFLOW == retval) && (decoded->capacity < BUFFER_X_LARGE)){
                    // Update caches to reflect current amounts of data
                    decoded->limit = reader->encoder.pos; // While Dest used a absolute offet
                    //Attempt to upgrade decoded buffer to able to handle a x large string

                    retval = buf_upgrade(decoded,
                            abq_malloc_ex(BUFFER_X_LARGE, NULL),
                            BUFFER_X_LARGE, true);
                    if(EXIT_SUCCESS == retval) {
                        // Reinitialize the encoder with the new size
                        reader->encoder = BUF_ENCODER_INIT(&utf8_codec, decoded);
                        // And re-attempt to write the byte of interest
                        retval = abq_encode_cp(&reader->encoder, codepoint);
                    }
                }
                if (EXIT_SUCCESS != retval) {
                    // Unread the character we were not able to write
                    ABQ_DECODER_REWIND(&reader->decoder, codepoint);
                }
            }
        } while ((reader->decoder.max > reader->decoder.pos)
                && (EXIT_SUCCESS == retval));
    }

    // Update caches to reflect current amounts of data
    decoded->limit = reader->encoder.pos; // While Dest used a absolute offet

    if (EXIT_SUCCESS != retval) {
        // Return error as is, but update caches based on data (above)
    } else {
        cstr_t parsed = str_create(&decoded->byte_array[decoded->position],
                (int32_t) decoded->limit, false);
        if (NULL == parsed) {
            retval = abq_status_take(ENOMEM);
        } else {
            // Success, read out all of the data, don't want to read it again
            decoded->limit = decoded->position;
            // Revert the decoder back to ascii for non-string data
            reader->decoder.codec = &ascii_codec;
            reader->decoder.max = decoder_max;
            // Update the item_state
            retval = json_reader_got_child(reader, (cvar_t) parsed);
            // Release out temporary reference to the parsed item
            (void)obj_release_self((cvar_t)parsed);
        }
    }
    return retval;
}

static err_t json_reader_got_number(item_reader_t *reader, number_t numeric_value) {
	err_t retval = EXIT_SUCCESS;
    number_ptr number = number_create(numeric_value);
    if(NULL == number){
        retval = abq_status_take(EIO);
    }else{
        retval = json_reader_got_child(reader, (cvar_t) number);
        (void)obj_release_self((cvar_t)number);
    }
    return retval;
}
static err_t json_reader_parse_number(item_reader_t *reader) {
    number_t numeric_value = abq_nan;
    size_t orig_pos = reader->decoder.pos;
    err_t retval = abq_decode_number(&reader->decoder, &numeric_value);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(reader->decoder.max > reader->decoder.pos) {
    	retval = json_reader_got_number(reader, numeric_value);
    }else{
        // assume partial content until some character
        //  is available that is beyond the number
        retval = ENODATA;
    }
    if (EXIT_SUCCESS != retval) {
        // Rewind on error
        reader->decoder.pos = orig_pos;
    }
    return retval;
}

static err_t json_reader_decode(item_reader_t *reader) {
    byte_t ascii = '\0';
    err_t retval = EXIT_SUCCESS;
    size_t prev_pos = reader->decoder.pos;
    if (&json_codec == reader->decoder.codec) {
        // Continue to parse JSON string with latest data
        retval = json_reader_parse_string(reader);
        // Check for consumed data to be reported to listener
        if ((prev_pos != reader->decoder.pos) && (NULL != reader->on_consumed)) {
            if (ENODATA != retval) {
                EXPECT_IS_OK(retval);
            }
            // Notify the listener of all consumed data
            retval = reader->on_consumed(reader->consumed_ctx, reader->item_state,
                    &reader->decoder.source[prev_pos], (reader->decoder.pos - prev_pos));
        }
    }
    while (EXIT_SUCCESS == retval) {
        prev_pos = reader->decoder.pos;
        ascii = abq_decode_char(&reader->decoder);
        if ('\0' == ascii) {
            retval = ENODATA; // Need more data to continue decoding
        } else if (ascii_is_friendly((int32_t)(uint8_t)ascii)) {
            switch (ascii) {
            case '{':
                retval = json_reader_begin_object(reader);
                break;
            case '}':
                retval = json_reader_finish_object(reader);
                break;
            case '[':
                retval = json_reader_begin_list(reader);
                break;
            case ']':
                retval = json_reader_finish_list(reader);
                break;
            case '"':
                ABQ_DECODER_REWIND(&reader->decoder, ascii);
                retval = json_reader_parse_string(reader);
                break;
            case 'n':
                ABQ_DECODER_REWIND(&reader->decoder, ascii);
                retval = abq_decode_skip_prefix(&reader->decoder, abq_null_str, 4);
                if (EXIT_SUCCESS == retval) {
                    retval = json_reader_got_child(reader, (cvar_t) NULL);
                } else if(ENODATA == retval) {
                	// Not a mismatch, just insufficient source data
                } else {
                    //  check for "nan" on error parsing null
                	retval = abq_decode_skip_prefix(&reader->decoder, abq_nan_str, -1);
                	if(EXIT_SUCCESS == retval) {
                    	retval = json_reader_got_number(reader, abq_nan);
                	}
                }
                break;
            case 't':
                ABQ_DECODER_REWIND(&reader->decoder, ascii);
                retval = abq_decode_skip_prefix(&reader->decoder, abq_true_str, 4);
                if (EXIT_SUCCESS == retval) {
                    retval = json_reader_got_child(reader, bool_var(true));
                }
                break;
            case 'f':
                ABQ_DECODER_REWIND(&reader->decoder, ascii);
                retval = abq_decode_skip_prefix(&reader->decoder, abq_false_str, 5);
                if (EXIT_SUCCESS == retval) {
                    retval = json_reader_got_child(reader, bool_var(false));
                }
                break;
            case ':':
                // check that it is between a property name and value (child_property not NULL)
                if(traverser_is_endof_node(reader->item_state)){
                    retval = EILSEQ;
                    ABQ_WARN_STATUS(retval, "No field was named");
                }
                break;
            case ',':
                // check that it is not between a property name and value (child_property is NULL)
                if(false == traverser_do_break(reader->item_state)) {
                    retval = EILSEQ;
                    ABQ_WARN_STATUS(retval, abq_node_child_name(traverser_leaf(reader->item_state)));
                }
                break;
            default:
                // check for a JSON number:
                if (('-' == ascii) || ('+' == ascii) || ('.' == ascii)
                        || (('0' <= ascii) && (ascii <= '9'))) {
                    ABQ_DECODER_REWIND(&reader->decoder, ascii);
                    retval = json_reader_parse_number(reader);
                } else if(ascii_is_space((int32_t)(uint8_t)ascii)) {
                    // Skip over whitespace
                } else {
                    retval = EILSEQ;
                }
                break;
            }
            if((ENODATA != retval) && (ECANCELED!= retval)
                    && (status_code_is_error(retval))) {
                ABQ_DUMP_ERROR(retval, "");
                // rewind on unexpected error, don't invoke on_consumed
                reader->decoder.pos = prev_pos;
            } else if ((prev_pos != reader->decoder.pos) && (NULL != reader->on_consumed)) {
                ascii = '\0';// So that it doesn't print a parsing error below if the function returns an error

                // Notify the listener of all consumed data
                retval = reader->on_consumed(reader->consumed_ctx, reader->item_state,
                        &reader->decoder.source[prev_pos], (reader->decoder.pos - prev_pos));
            } else {
                // No need to call on_consumed
            }
            if ((NULL == reader->on_completed)
                    && (EXIT_SUCCESS == retval)) {
                    retval = ECANCELED;
            }
        } else {
            retval = abq_status_take(EINVAL);
        }
    }
    if ((ENODATA == retval) || (ECANCELED == retval) || ('\0' == ascii)) {
        // ENODATA: waiting for more data to process
        // ECANCELED: completed successfully
    } else {
        ABQ_DUMP_ERROR(retval, "Failure to parse JSON as the given class");
        ABQ_INFO_MSG_Y("Input Data: ",
                &reader->decoder.source[prev_pos], (reader->decoder.max - prev_pos));
        ABQ_INFO_MSG_X("Location of failed item: ", (reader->decoder.pos - prev_pos));
        ABQ_INFO_MSG_X("Unicode character value: ", ascii);
        ABQ_INFO_MSG_Y("Remaining Data: ",
                &reader->decoder.source[ reader->decoder.pos], (reader->decoder.max -  reader->decoder.pos));
    }
    return retval;
}

err_t json_reader_parse_data(item_reader_t *reader, byte_buffer_t *input_data) {
    err_t retval = EXIT_SUCCESS;
    if((NULL == reader) || (NULL == input_data)) {
        retval = EFAULT;
    } else if (traverser_is_completed(reader->item_state)) {
        retval = EALREADY;
    } else {
        if (input_data != reader->cache) {
            if(NULL != reader->cache) {
                // Transfer existing data into new buffer
                retval = buf_write_buffer(input_data, reader->cache, -1);
            }
            if (EXIT_SUCCESS == retval) {
                if (buf_has_data(reader->cache)) {
                    retval = EIO; // Failed to transfer existing data
                } else {
                    // Overwrite cache with new output_buf
                    SET_RESERVED_FIELD(reader, cache, input_data);
                }
            }
        }
        if (EXIT_SUCCESS == retval) {
            if (&json_codec == reader->decoder.codec) {
                // Update decoder with current data positions
                reader->decoder = BUF_DECODER_INIT(&json_codec, input_data);
            } else {
                reader->decoder = BUF_DECODER_INIT(&ascii_codec, input_data);
            }
            retval = json_reader_decode(reader);
            // Update cache data based on what was parsed
            input_data->position = reader->decoder.pos;
        }
    }
    return retval;
}

err_t datatap_read_json(datatap_t* source, class_ptr class_of_items,
        on_data_consumed_t on_consumed, cvar_t consumed_ctx,
         on_item_parsed_t on_completed, cvar_t completed_ctx) {
    return datatap_item_reader(source, json_reader_parse_data, class_of_items,
            on_consumed, consumed_ctx, on_completed, completed_ctx);
}

cvar_t str_parse_json_ext(cstr_t source, int32_t source_len,
        class_ptr class_of_items, on_data_consumed_t on_consumed, cvar_t consumed_ctx) {
    cvar_t retval = NULL;
    int32_t input_data = utf8_byte_length(source, source_len);
    if (NULL  == source) {
        abq_status_set(EFAULT, false);
    } else if(0 >= input_data) {
        abq_status_set(ENODATA, false);
    } else {
        // Used to collect the parsing results within a callback
        var_result_t* vessel = var_result_create(NULL,
                EXIT_SUCCESS, NULL, NULL);
        item_reader_t* reader = item_reader_create(json_reader_parse_data,
                class_of_items, on_consumed, consumed_ctx, var_result_on_item_parsed, vessel);
        if ((NULL == vessel) || (NULL == reader)) {
            abq_status_set(ENOMEM, false);
        } else {
            reader->decoder = abq_decoder_init(&ascii_codec, source, (int64_t) input_data);
            err_t status = json_reader_decode(reader);
            if (ECANCELED != status) {
            	status =  item_reader_on_item(reader, status, NULL);
            	if(EXIT_SUCCESS == vessel->status) {
            		vessel->status = status;
            	}
            }
			// Check the results
			retval = vessel->item;
			if(NULL != retval) {
				// Reserve self-reference to the result so that it is not garbage collected
				(void) obj_reserve_self(retval);
			} else {
				abq_status_set(vessel->status, false);
			}
        }
        (void) obj_release_self(reader);
        (void) obj_release_self(vessel);
    }
    return retval;
}

cvar_t str_parse_json(cstr_t source,
        int32_t source_len, class_ptr class_of_items){
    return str_parse_json_ext(source,
            source_len, class_of_items, NULL, NULL);
}
