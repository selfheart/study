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
 * @file json_writer.c
 * @date Jan 14, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief TODO
 *
 */

#include <ontrac/stream/mem_sink.h>
#include <ontrac/rest/item_writers.h>
#include <ontrac/rest/item_readers.h>
#include <ontrac/ontrac/primitives.h>

static err_t json_writer_write_number(item_writer_t *serializer, number_t number) {
    serializer->encoder
        = BUF_ENCODER_INIT(&utf8_codec, serializer->cache);
    err_t retval = abq_encode_number(&serializer->encoder, number);
    if (EXIT_SUCCESS == retval) {
        // Success! update position on buffer
        serializer->cache->limit = serializer->encoder.pos;
    }
    return retval;
}

static err_t json_writer_encode_indent(abq_encoder_t *encoder,
        const traverser_t *item_state, bool_t endof) {
    err_t retval = abq_encode_char(encoder, '\n');
    for(size_t index = (endof) ? 1UL : 0UL;
    		index < item_state->depth ; index++) {
        // Encode 4 spaced per indent ?
        retval = abq_encode_ascii(encoder, "    ", 4); // parasoft-suppress CERT_C-MSC41-a-1 "c0366. This string does not contain sensitive information."
    }
    return retval;
}


static err_t json_writer_write_endof(item_writer_t *serializer, byte_t ending){
    err_t retval = EXIT_SUCCESS;
    serializer->encoder
        = BUF_ENCODER_INIT(&ascii_codec, serializer->cache);
    // Attempt to encode both the indent & the '}'/']' character as an "atomic" operation
    if (serializer->indent) {
        retval = json_writer_encode_indent(&serializer->encoder,
                serializer->item_state, true);
    }
    if(EXIT_SUCCESS == retval)  {
        retval = abq_encode_char(&serializer->encoder, ending);
    }
    if(EXIT_SUCCESS == retval) {
        // Success! update position on buffer
        serializer->cache->limit = serializer->encoder.pos;
    }
    return retval;
}

static err_t json_writer_write_begin(item_writer_t *serializer) {
    err_t retval = EXIT_SUCCESS;
    serializer->encoder
        = BUF_ENCODER_INIT(&ascii_codec, serializer->cache);
    // Attempt to encode both the ',' delimiter & the indent as a "atomic" operation
    if (0 < traverser_leaf(serializer->item_state)->child_counter) {
        retval = abq_encode_char(&serializer->encoder, ',');
    }
    if ((EXIT_SUCCESS == retval) && (serializer->indent)) {
        retval = json_writer_encode_indent(&serializer->encoder,
        		serializer->item_state, false);
    }
    if(EXIT_SUCCESS == retval) {
        // Success! update position on buffer
        serializer->cache->limit = serializer->encoder.pos;
    }
    return retval;
}

static err_t json_writer_write_utf8(item_writer_t *serializer, cstr_t utf8, bool_t new_field) {
    // Check if we are continuing from a previous position
    err_t retval = EXIT_SUCCESS;
    if (utf8 != serializer->decoder.source) {
        size_t orig_limit = serializer->cache->limit;
        if (new_field) {
            retval = json_writer_write_begin(serializer);
        }
        if (EXIT_SUCCESS == retval) {
            // New UTF8 string, begin serialization
            serializer->encoder
                = BUF_ENCODER_INIT(&json_codec, serializer->cache);
            // json_codec only writes out initial quotation at 0 position
            //  if it is not at zero postion, write one manually
            retval = raw_codec.encode(&serializer->encoder, '\"');
        }
        if (EXIT_SUCCESS == retval) {
            // Serialization has begun, don't write the '\"' character again
            serializer->decoder
                = abq_decoder_init(&utf8_codec, utf8, -1);
        } else {
            // Starting JSON needs to be a 'atomic' operation
            //  on failure reset to previous position
            serializer->cache->limit = orig_limit;
        }
    } else {
        // Continuation of a partially serialized string
        ABQ_VITAL(&json_codec == serializer->encoder.codec);
        // Reinitialize encoder with current cache conditions
        serializer->encoder = BUF_ENCODER_INIT(&json_codec, serializer->cache);
    }

    int32_t codepoint = -1;
    while ((serializer->decoder.max > serializer->decoder.pos)
            && (EXIT_SUCCESS == retval)) {
        codepoint = abq_decode_cp(&serializer->decoder);
        if (0 > codepoint) {
            retval = abq_status_take(EIO);
        } else {
            // Write out the codepoint we just read in
            retval = abq_encode_cp(&serializer->encoder, codepoint);
        }
    }

    if (utf8 != serializer->decoder.source) {
        // Failed to write indent & '\"' character
        ABQ_VITAL(EXIT_SUCCESS != retval);
        ABQ_VITAL(0 > codepoint);
    } else if (EXIT_SUCCESS == retval) {
        // Update buffer position to reflect the new state
        serializer->cache->limit = serializer->encoder.pos;
        // prefix and stringify are completed, postfix is all that is left
        if (new_field) {
            retval = buf_write_octet(serializer->cache, ':');
        }
        if (EXIT_SUCCESS == retval) {
            // Success! do not continue with current string
            serializer->decoder.source = NULL;
        }
    } else if (EOVERFLOW == retval) {
        ABQ_VITAL(utf8 == serializer->decoder.source);
        // Update buffer position to reflect the new state
        serializer->cache->limit = serializer->encoder.pos;
        if (0 == codepoint) {
            // Reset max so that '\0' terminator can be read another time
            serializer->decoder.max = BUFFER_UNKNOWN_SIZE;
        } else {
            ABQ_VITAL(0 < codepoint); // code-point should only be less then zero on failure to start
            // Rewind decoder so that we can retry the failed character next time
            ABQ_DECODER_REWIND(&serializer->decoder, codepoint);
        }
    } else {
        // Completed serialization with an error
        ABQ_WARN_STATUS(retval, utf8);
    }
    return retval;
}

static err_t json_writer_write_state(item_writer_t *serializer) {
    err_t status = EXIT_SUCCESS;
    traverser_t * traverser = serializer->item_state;
    ABQ_VITAL(0UL != traverser->depth); // Don't write initial & completed states
    abq_node_t *node = traverser_leaf(traverser);
    VITAL_NOT_NULL(node->meta);
    size_t original_amount = buf_remaining(serializer->cache);
    if (&null_class == node->meta) {
        if (traverser_root(traverser) != node) {
            status = buf_write_bytes(serializer->cache, abq_null_str, 4);
        } else {
            // special exception, if it is top level object and an NULL value,
            //  then ship an empty object so remote json parsers don't blow up
            status = buf_write_bytes(serializer->cache, "{}", 2); // parasoft-suppress CERT_C-MSC41-a-1 "c0367. This string does not contain sensitive information."
        }
    } else if (&bool_class == node->meta) {
        if ((cvar_t) node->item == (cvar_t) true_ptr) {
            status = buf_write_bytes(serializer->cache, abq_true_str, 4);
        } else {
            status = buf_write_bytes(serializer->cache, abq_false_str, 5);
        }
    } else if (&number_class == node->meta) {
        status = json_writer_write_number(serializer, number_resolve(node->item));
    } else if (&byte_buffer_class == node->meta) {
        byte_buffer_t *buf = buf_resolve(node->item);
        VITAL_NOT_NULL(buf);
        // Not just any binary is JSON compatible,
        //  Verify that the buffer contains valid JSON content
        if (false == buf_has_data(buf)) {
            status = abq_status_take(ENODATA);
        } else if (buf->byte_array == serializer->decoder.source) {
            // Continuation of prior serialization
        } else {
            // TODO more lightweight validation
            cvar_t item = str_parse_json(buf_data(buf), (int32_t) buf_remaining(buf), NULL);
            if (NULL == item) {
                // TODO: base64 encoded string ?
                ABQ_ERROR_MSG_Y("Not JSON compatible: ", buf_data(buf), (int32_t) buf_remaining(buf));
                status = abq_status_take(EINVAL);
            } else {
                serializer->decoder
                = BUF_DECODER_INIT(&utf8_codec, buf);
                (void) obj_release_self(item);
            }
        }
        if (EXIT_SUCCESS == status) {
            int32_t codepoint = -1;
            // Re-initialize the encoder with current conditions
            serializer->encoder
            = BUF_ENCODER_INIT(&utf8_codec, serializer->cache);
            while ((serializer->decoder.max > serializer->decoder.pos)
                    && (EXIT_SUCCESS == status)) {
                codepoint = abq_decode_cp(&serializer->decoder);
                if (0 > codepoint) {
                    status = abq_status_take(EIO);
                } else {
                    // Write out the codepoint we just read in
                    status = abq_encode_cp(&serializer->encoder, codepoint);
                }
            }
            if (EXIT_SUCCESS != status) {
                // last character wasn't written, rewind so that it can be included in the next pass
                ABQ_DECODER_REWIND(&serializer->decoder, codepoint);
            }
            // Update buffer position to reflect the new state
            serializer->cache->limit = serializer->encoder.pos;
        }
    } else if (&string_class == node->meta) {
        cstr_t strval = str_resolve(node->item);
        VITAL_NOT_NULL(strval);
        if ((traverser_root(traverser) == node) && (utf8_is_whitespace(strval, -1))) {
            // special exception, if it is top level object and an empty string value,
            //  then ship an empty object so remote json parsers don't blow up
            status = buf_write_bytes(serializer->cache, "{}", 2); // parasoft-suppress CERT_C-MSC41-a-1 "c0368. This string does not contain sensitive information."
        } else {
            status = json_writer_write_utf8(serializer, strval, false);
        }
    } else if (ABQ_NODE_VARLIST == node->nodetype) {
        if (traverser_is_endof_node(traverser)) {
            FATAL_IF(traverser_is_new_node(traverser));
            if (1 >= node->child_counter) {
                // Special exception for list with one item, no indents
                status = buf_write_octet(serializer->cache, ']');
            } else {
                status = json_writer_write_endof(serializer, ']');
            }
        } else if (traverser_is_new_node(traverser)) {
            // beginning of list event
            status = buf_write_octet(serializer->cache, '[');
        } else if (1 >= vlist_size(vlist_resolve(node->item))) {
            // Lists with 1 or less items don't get newline + indent
        } else {
            status = json_writer_write_begin(serializer);
        }
    } else if (ABQ_NODE_OBJECT == node->nodetype) {
        // ptree_class & misc. object serializables
        if (traverser_is_endof_node(traverser)) {
            FATAL_IF(traverser_is_new_node(traverser));
            if (0 >= node->child_counter) {
                // Special exception for object with no children, no indents
                status = buf_write_octet(serializer->cache, '}');
            } else {
                status = json_writer_write_endof(serializer, '}');
            }
        } else {
            if (traverser_is_new_node(traverser)) {
                FATAL_IF(traverser_has_child(traverser));
                // beginning of object event
                status = buf_write_octet(serializer->cache, '{');
            } else {
                // Expect that we have skipped empty children
                status = json_writer_write_utf8(serializer, abq_node_child_name(node), true);
            }
        }
    } else {
        abq_fatal(__FILENAME__, __LINE__, // parasoft-suppress CERT_C-MSC41-a-1 "c0369. This string does not contain sensitive information."
                  ENOSYS, "?"); // parasoft-suppress CERT_C-MSC41-a-1 "c0370. This string does not contain sensitive information."
    }

    size_t final_amount = buf_remaining(serializer->cache);
    if((NULL != serializer->on_format) && (final_amount != original_amount)) {
        ABQ_VITAL(final_amount > original_amount);
        err_t tmp = serializer->on_format(serializer->format_ctx, serializer->item_state,
                &serializer->cache->byte_array[serializer->cache->position + original_amount],
                (final_amount - original_amount));
        if(EXIT_SUCCESS != tmp) {
            status = tmp; // Overwrite status on error
        }
    }
    return status;
}

err_t json_writer_format_data(item_writer_t *writer, byte_buffer_t *output_buf) {
    err_t retval = EXIT_SUCCESS;
    if((NULL == writer) || (NULL == output_buf)){
        retval = EFAULT;
    }else if(traverser_is_completed(writer->item_state)) {
        // Item has been fully serialized to cache
        //  and that we no only need to drain the cache
        retval = ECANCELED;
    }else if (output_buf != writer->cache) {
        if(NULL != writer->cache) {
            // Transfer existing data into new buffer
            retval = buf_write_buffer(output_buf, writer->cache, -1);
        }
        if (EXIT_SUCCESS == retval) {
            if (buf_has_data(writer->cache)) {
                retval = EIO; // Failed to transfer existing data
            } else {
                // Overwrite cache with new output_buf
                SET_RESERVED_FIELD(writer, cache, output_buf);
            }
        }
    } else {
        // cache is already updated to output_buf
    }
    while (EXIT_SUCCESS == retval) {
        retval = json_writer_write_state(writer);
        if (EXIT_SUCCESS == retval) {
            // Success! continue formatting
            retval = traverser_step(writer->item_state);
        	// Check for completed
            if (ECANCELED == retval) {
            	ABQ_VITAL(traverser_is_completed(writer->item_state));
            }
        }
    }
    return retval;
}

err_t datasink_write_json(datasink_t* dest, cvar_t item, bool_t indent) {
        err_t retval = CHECK_NULL(dest);
        if (EXIT_SUCCESS != retval) {
            // Return error as is
        } else {
            byte_buffer_t *cache = buf_allocate(BUFFER_SMALL);
            item_writer_t *serializer = item_writer_create(json_writer_format_data,
                    item, cache, indent, NULL, NULL);
            if (NULL == serializer) {
                retval = abq_status_take(ENOMEM);
            } else {
                retval = item_writer_set_sink(serializer, dest);
            }
            (void) obj_release_self(serializer);
            (void) obj_release_self(cache);
        }
        return retval;
}

int32_t json_format_data(cvar_t item,
        byte_t *dest, int32_t max_bytes, bool_t indent) {
    int32_t retval = -1;
    if (NULL == dest) {
        (void) abq_status_set(EFAULT, false);
    } else if (0 >= max_bytes) {
        (void) abq_status_set(ERANGE, false);
    } else {
        byte_buffer_t *buf = buf_wrap(dest, (size_t)max_bytes, 0U, false);
        item_writer_t* writer = item_writer_create(json_writer_format_data,
                item, buf, indent, NULL, NULL);
        err_t status = json_writer_format_data(writer, buf);
        if (ECANCELED != status) {
            ABQ_VITAL(status_code_is_error(status));
            (void) abq_status_set(status, false);
        } else {
            retval = (int32_t) buf->limit;
        }
        (void)obj_release_self(writer);
        (void)obj_release_self(buf);
    }
    return retval;
}
