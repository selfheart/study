//#line 2 "util/byte_buffer.c"
/****************************************************************************
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
 ****************************************************************************/
/**
 * @file ontrac/util/byte_buffer.c
 * @date Mar 20, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 */

#include <ontrac/util/byte_buffer.h>
#include <ontrac/ontrac/abq_str.h>
#include <ontrac/ontrac/mem_usage.h>

#ifndef MAX_BYTE_BUFFERS
#define MAX_BYTE_BUFFERS (48U)
#endif /* MAX_BYTE_BUFFERS */

static void buf_delete(cvar_t old_buf);
DEFINE_CLASS(byte_buffer_class, byte_buffer_t, MAX_BYTE_BUFFERS,
        NULL, NULL, NULL, NULL, buf_delete);
DEFINE_LIST_OF_CLASS(list_of_byte_buffer_class, byte_buffer_class);

byte_buffer_t *buf_wrap(byte_t *byte_array,
        size_t capacity, size_t limit,
        bool_t free_byte_array_on_delete) {
    byte_buffer_t *rvalue = NULL;
    if (NULL == byte_array) {
        (void) abq_status_set(EFAULT, false);
    } else if (limit > capacity) {
        (void) abq_status_set(ERANGE, false);
        if (free_byte_array_on_delete) {
            abq_free(byte_array);
        }
    } else {
        rvalue = CREATE_BASE_INSTANCE(byte_buffer_class, byte_buffer_t);
        if (NULL == rvalue) {
            (void) abq_status_set(ENOMEM, false);
            if (free_byte_array_on_delete) {
                abq_free(byte_array);
            }
        } else {
            // initializes data
            *rvalue = (byte_buffer_t) {
                .byte_array = byte_array,
                .capacity = capacity,
                .limit = limit,
                .position = 0U,
                .mark = 0U,
                .free_byte_array_on_delete = free_byte_array_on_delete
            };
        }
    }
    return rvalue;
}

byte_buffer_t *buf_allocate(buf_size_t capacity) {
    byte_buffer_t *rvalue = NULL;
    // Used in nio_xfer to represent EOS
    if (capacity > 0U) {
        // buf_wrap should always call abq_free on error
        rvalue = buf_wrap(abq_malloc_ex(capacity, NULL), capacity, 0U, true);
    } else {
        static byte_t empty_byte_array[] = "";
        rvalue = buf_wrap(&empty_byte_array[0], capacity, 0U, false);
    }
    return rvalue;
}

byte_buffer_t *buf_resolve(cvar_t item) {
    byte_buffer_t *retval = NULL;
    if (NULL != item) {
        CLASS_RESOLVE(byte_buffer_class, byte_buffer_t, retval, item);
    }
    return retval;
}

static void buf_delete(cvar_t old_buf) {
    if (NULL != old_buf) {
        byte_buffer_t *buf = buf_resolve(old_buf);
        VITAL_NOT_NULL(buf);
        if (buf->free_byte_array_on_delete) {
            abq_free(buf->byte_array);
        }
    }
}

err_t buf_upgrade(byte_buffer_t *buf, byte_t *byte_array,
        size_t capacity, bool_t free_byte_array_on_delete){
    err_t retval = EXIT_SUCCESS;
    if ((NULL == buf) || (NULL == byte_array)) {
        retval = EFAULT;
    } else if (capacity < buf_remaining(buf)) {
        // buffer too small to hold current data
        retval = EOVERFLOW;
    } else if(abq_memory_match(buf->byte_array,
            (uintptr_t) buf->capacity, byte_array)){
        retval = EALREADY;
    } else {
        size_t limiter = buf->capacity - buf->position;
        bytes_copy(byte_array, &buf->byte_array[buf->position], limiter);
        if(buf->free_byte_array_on_delete) {
            abq_free(buf->byte_array);
        }
        limiter = buf->limit - buf->position;
        // reinitializes data with new byte_array
        *buf = (byte_buffer_t) {
                .byte_array = byte_array,
                .capacity = capacity,
                .limit = limiter,
                .position = 0U,
                .mark = 0U,
                .free_byte_array_on_delete = free_byte_array_on_delete
        };
    }
    if((EXIT_SUCCESS != retval) && (EALREADY != retval)
            && (free_byte_array_on_delete)){
        abq_free(byte_array);
    }
    return retval;
}

size_t buf_remaining(const byte_buffer_t *buf) {
    size_t retval = 0U;
    if (NULL != buf) {
#if !defined(NDEBUG)
        ABQ_VITAL(0 <= buf->position);
        ABQ_VITAL(buf->position <= buf->limit);
        ABQ_VITAL(buf->limit <= buf->capacity);
#endif /* !defined(NDEBUG) */
        retval = buf->limit - buf->position;
    }
    return retval;
}

size_t buf_unused_bytes(const byte_buffer_t *buf) {
    size_t retval = 0U;
    if (NULL != buf) {
        retval = buf->capacity - buf_remaining(buf);
    }
    return retval;
}

err_t buf_compact(byte_buffer_t * buf){
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(0U == buf->position) {
        // retval = EALREADY already compact
    } else if (buf->limit ==  buf->position) {
        // No data in the buffer, return to 0 position
        buf->limit =0U;
        buf->position = 0U;
    } else { // Must move data to the start of the buffer
#if !defined(NDEBUG)
        ABQ_VITAL(0U <= buf->position);
        ABQ_VITAL(buf->position < buf->limit);
        ABQ_VITAL(buf->limit <= buf->capacity);
#endif /* !defined(NDEBUG) */

        size_t offset = buf->position;
        size_t count = buf->capacity - offset;
        bytes_copy(buf->byte_array, &buf->byte_array[offset], count);
        if(buf->mark >= offset){
            buf->mark -= offset;
        }else{
            buf->mark = 0U;
        }
        buf->limit -= offset;
        buf->position = 0U;
    }
    return retval;
}

err_t buf_write_bytes(byte_buffer_t *buf, cvar_t source, size_t length) {
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else {
        if ((NULL == source) || (0U == length)) {
            retval = EINVAL;
        } else {
            if (length > (buf->capacity - buf->limit)) {
                retval = buf_compact(buf);
            }
            if (EXIT_SUCCESS != retval) {
                // Return error code as is
            } else if (length > (buf->capacity - buf->limit)) {
                retval = EOVERFLOW;
            } else {
                // copy raw bytes from source to destination
                bytes_copy(&buf->byte_array[buf->limit], source, length);
                buf->limit += length;
                // and terminate strings if space is available
                if (buf->limit < buf->capacity) {
                    buf->byte_array[buf->limit] = '\0';
                }
            }
        }
    }
    return retval;
}

err_t buf_write_octet(byte_buffer_t *buf, const byte_t octet) {
    err_t rvalue = EXIT_SUCCESS;
    if (buf->limit >= buf->capacity) {
        if (0U != buf->position) {
            rvalue = buf_compact(buf);
        } else {
            rvalue = EOVERFLOW;
        }
    }
    if (EXIT_SUCCESS == rvalue) {
        buf->byte_array[buf->limit] = octet;
        buf->limit += 1U;
        if (buf->limit < buf->capacity) {
            buf->byte_array[buf->limit] = '\0';
        }
    }
    return rvalue;
}

err_t buf_write_char(byte_buffer_t *buf, const int32_t codepoint) {
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else {
        BUF_ENCODER(encoder, &utf8_codec, buf);
        retval = abq_encode_cp(&encoder, codepoint);
        if((EOVERFLOW == retval) && (0U != buf->position)) {
            retval = buf_compact(buf);
            if(EXIT_SUCCESS == retval) {
                // Reset buffer and try again
                encoder.pos = buf->limit;
                retval = abq_encode_cp(&encoder, codepoint);
            }
        }
        if (EXIT_SUCCESS == retval) {
            buf->limit = encoder.pos;
        }
    }
    return retval;
}

err_t buf_write_int(byte_buffer_t *buf, int64_t value, abq_radix_t radix) {
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else {
        byte_t *dest = &buf->byte_array[buf->limit];
        size_t free_space = buf->capacity - buf->limit;
        int32_t length = utf8_write_int(dest, free_space, value, radix);
        if (0 > length) {
            retval = abq_status_take(EIO);
            if ((EOVERFLOW == retval)
                    && (0U != buf->position)) {
                // move data to beginning of buffer to make space
                retval = buf_compact(buf);
                if (EXIT_SUCCESS == retval) {
                    dest = &buf->byte_array[buf->limit];
                    free_space = buf->capacity - buf->limit;
                    length = utf8_write_int(dest, free_space, value, radix);
                    if (0 > length) {
                        retval = abq_status_take(EIO);
                    }
                }
            }
        }
        if (EXIT_SUCCESS == retval) {
            buf->limit += (size_t) length;
            if (buf->limit < buf->capacity) {
                buf->byte_array[buf->limit] = '\0';
            }
        }
    }
    return retval;
}

err_t buf_write_number(byte_buffer_t *buf, number_t value) {
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else {
        byte_t *dest = &buf->byte_array[buf->limit];
        size_t free_space = buf->capacity - buf->limit;
        int32_t length = utf8_write_number(dest, free_space, value);
        if (0 > length) {
            retval = abq_status_take(EIO);
            if ((EOVERFLOW == retval)
                    && (0U != buf->position)) {
                // move data to beginning of buffer to make space
                err_t err = buf_compact(buf);
                abq_status_set(err, true);
                if(abq_status_is_ok()){
                    dest = &buf->byte_array[buf->limit];
                    free_space = buf->capacity - buf->limit;
                    length = utf8_write_number(dest, free_space, value);
                    if (0 > length) {
                        retval = abq_status_take(EIO);
                    }
                }
            }
        }
        if (EXIT_SUCCESS == retval) {
            buf->limit += (size_t)length;
            if(buf->limit < buf->capacity) {
                buf->byte_array[buf->limit] = '\0';
            }
        }
    }
    return retval;
}

err_t buf_write_str(byte_buffer_t *buf, cstr_t str) {
	err_t retval = CHECK_NULL(buf);
    cstr_t content = (NULL == str) ? abq_null_str : str;
    BUF_ENCODER(encoder, &utf8_codec, buf);
    ABQ_DECODER(decoder, &utf8_codec, content, -1);
    if (EXIT_SUCCESS == retval) {
    	retval = abq_decode_encode(&decoder, &encoder);
    	if((EOVERFLOW == retval) && (0U != buf->position)){
    		VITAL_IS_OK(buf_compact(buf));
    		encoder =  BUF_ENCODER_INIT(&utf8_codec, buf);
        	retval = abq_decode_encode(&decoder, &encoder);
    	}
    	if (EXIT_SUCCESS == retval) {
    	    buf->limit = encoder.pos;
    	}
    }
    return retval;
}

int32_t buf_write_buffer(byte_buffer_t *dest, byte_buffer_t *source, int32_t n) {
    int32_t rvalue = -1;
    if ((NULL == dest) || (NULL == source)) {
        abq_status_set(EFAULT, false);
    } else {
        size_t amount = buf_remaining(source);
        size_t unused = buf_unused_bytes(dest);
        if (unused < amount) {
            amount = unused;
        }
        if ((0 <= n) && (((size_t) n) < amount)) {
            amount = (size_t) n;
        }
        byte_t *data = &source->byte_array[source->position];

        err_t err = buf_write_bytes(dest, data, amount);
        if (EXIT_SUCCESS != err) {
            abq_status_set(err, false);
            rvalue = -1;
        } else {
            source->position += amount;
            rvalue = (int32_t) amount;
        }
    }
    return rvalue;
}

err_t buf_mark_char(byte_buffer_t *buf,
        const int32_t codepoint,
        bool_t from_prior_mark) {
    err_t retval = CHECK_NULL(buf);
    int32_t chsize = utf8_codec.bytesize(codepoint);
    if (EXIT_SUCCESS != retval) {
        // Return error as is
    } else if(0 > chsize) {
        retval = EINVAL;
    } else {
        BUF_DECODER(decoder, &utf8_codec, buf);
        int32_t cp = -1;
        if ((from_prior_mark)
                && (buf->position <= buf->mark)
                && (buf->mark < buf->limit)) {
            decoder.pos = buf->mark;
            cp = abq_decode_cp(&decoder);
            if (0 > cp) {
                buf->mark = 0U;
                retval = EILSEQ;
            }
        }
        if (EXIT_SUCCESS == retval) {
            cp = abq_decode_cp(&decoder);
            while ((0 <= cp) && (cp != codepoint)) {
                if(0 == cp) {
                    if(decoder.max > decoder.pos){
                        /* skip over terminators */
                        decoder.pos += 1U;
                    }else{
                        break;
                    }
                }
                cp = abq_decode_cp(&decoder);
            }
            if (cp != codepoint) {
                if (buf->limit >= buf->capacity) {
                    // default to ENODATA if buffer is FULL
                    retval = abq_status_take(ENODATA);
                } else{
                    // else default to EAGAIN if codepoint wasn't found
                    retval = abq_status_take(EAGAIN);
                }
            } else {
                buf->mark = decoder.pos - (size_t)chsize;
                retval = EXIT_SUCCESS;
            }
        }
    }
    return retval;
}

err_t buf_mark_str(byte_buffer_t *buf,
        const cstr_t substring, bool_t from_prior_mark) {
    err_t rvalue = CHECK_NULL(buf);
    if (EXIT_SUCCESS != rvalue) {
        // Return error code as is
    } else {
        size_t pos = buf->position;
        if ((from_prior_mark)
                && (buf->position <= buf->mark)
                && (buf->mark < buf->limit)) {
            int32_t count = utf8_char_length(&buf->byte_array[buf->mark]);
            if (0 > count) {
                buf->mark = 0;
                rvalue = EILSEQ;
            } else if ((buf->mark + (size_t)count) >= buf->limit) {
                // default to EAGAIN if substring wasn't found
                rvalue = EAGAIN;
            } else {
                pos = buf->mark + (size_t)count;
            }
        }
        if (EXIT_SUCCESS != rvalue) {
            // Return error code as is
        } else {
            byte_t *source = &buf->byte_array[pos];
            size_t source_len = buf->limit - pos;
            int32_t rel_loc = utf8_index_of(source,
                    (int32_t) source_len, substring, -1);
            if (rel_loc >= 0) {
                buf->mark = pos + (size_t)rel_loc;
                rvalue = EXIT_SUCCESS;
            } else {
                // default to EAGAIN if substring wasn't found
                rvalue = EAGAIN;
            }
        }
    }
    return rvalue;
}

int32_t buf_marked_length(const byte_buffer_t *buf) {
	int32_t rvalue = -1;
	if(NULL == buf) {
	    abq_status_set(EFAULT, false);
	} else {
		if((buf->position <= buf->mark)
				&& (buf->mark < buf->limit)){
		    // marked length to include the marked character
		    // so that if mark is at position, length is still 1
			rvalue = (1 + (int32_t)buf->mark) - ((int32_t)buf->position);
		}
	}
	return rvalue;
}

int32_t buf_read_bytes(byte_buffer_t *buf, var_t dest, size_t amount) {
    int32_t retval = -1;
    if((NULL == buf) || (NULL == dest)) {
        abq_status_set(EFAULT, false);
    } else if(0U == amount) {
        retval = 0;
    } else {
        size_t count = buf_remaining(buf);
        if(count > amount) {
            count = amount;
        }
        bytes_copy(dest, &buf->byte_array[buf->position], count);
        buf->position += count;
        retval = (int32_t) count;
    }
    return retval;
}

int32_t buf_read_octet(byte_buffer_t *buf){
    int32_t rvalue = -1;
    if(NULL == buf) {
        abq_status_set(EFAULT, false);
    } else {
        if(false == buf_has_data(buf)){
            (void)abq_status_set(ENODATA, true);
        }else{
            rvalue = (int32_t) ((uint8_t) buf->byte_array[buf->position]);
            buf->position += 1U;
        }
    }
    return rvalue;
}

int32_t buf_read_char(byte_buffer_t *buf) {
    int32_t retval = -1;
    if (NULL == buf) {
        abq_status_set(EFAULT, false);
    } else {
        BUF_DECODER(decoder, &utf8_codec, buf);
        retval = abq_decode_cp(&decoder);
        if (0 > retval) {
            // return error as is
        } else {
            buf->position = decoder.pos;
        }
    }
    return retval;
}

err_t buf_read_int(byte_buffer_t *buf, int64_t *dest, abq_radix_t radix) {
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else {
        BUF_DECODER(decoder, &utf8_codec, buf);
        retval = abq_decode_int(&decoder, dest, radix);
        if (EXIT_SUCCESS == retval) {
            buf->position = decoder.pos;
        }
    }
    return retval;
}

err_t buf_read_number(byte_buffer_t *buf, number_ptr dest) {
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else {
        BUF_DECODER(decoder, &utf8_codec, buf);
        retval = abq_decode_number(&decoder, dest);
        if (EXIT_SUCCESS == retval) {
            buf->position = decoder.pos;
        }
    }
    return retval;
}

err_t buf_read_str(byte_buffer_t *buf, cstr_t *str_ptr, int32_t length, bool_t trim) {
    err_t retval = CHECK_NULL(buf);
    if (EXIT_SUCCESS != retval) {
        // Return error code as is
    } else if(NULL == str_ptr) {
        retval = EFAULT;
    } else {
        BUF_DECODER(decoder, &utf8_codec, buf);
        size_t byte_count = decoder.max - decoder.pos;
        if((0 <= length)
                && (length < (int32_t)byte_count)) {
            byte_count = (size_t)length;
            decoder.max = decoder.pos + (size_t)byte_count;
        }
        if ( trim ) {
            retval = abq_decode_skip_ws(&decoder);
            if(EXIT_SUCCESS == retval) {
                int32_t trim_len = utf8_trim_length(&decoder.source[decoder.pos],
                            (int32_t)decoder.max - (int32_t)decoder.pos);
                if (0 > trim_len) {
                    retval = abq_status_take(EIO);
                } else {
                    decoder.max = decoder.pos + (size_t)trim_len;
                }
            }
        }
        if (EXIT_SUCCESS == retval) {
            *str_ptr = abq_decode_str(&decoder);
            if (NULL == *str_ptr) {
                retval = abq_status_take(EIO);
            } else {
                buf->position += byte_count;
            }
        }
    }
    return retval;
}


err_t buf_pull_on_tap(byte_buffer_t *buf, datatap_t *tap) {
    err_t retval = EXIT_SUCCESS;
    if ((NULL == buf) || (NULL == tap)) {
        retval = EFAULT;
    } else {
        int64_t nbyte = ((int64_t) buf->capacity) - ((int64_t) buf->limit);
        if (0L == nbyte) {
            // No space to read into, can we make some ?
            if (0U != buf->position) {
                // Yes, create some space
                VITAL_IS_OK(buf_compact(buf));
                // And recompute the available space
                nbyte = ((int64_t) buf->capacity) - ((int64_t) buf->limit);
            } else {
                retval = datatap_get_status(tap);
                if (EXIT_SUCCESS == retval) {
                    // No, return error if we can't read any additional data
                    retval = EOVERFLOW;
                }
            }
        } else if ((buf->limit == buf->position) && (0U != buf->position)) {
            // No data in the buffer, reset position to zero
            buf->position = 0U;
            buf->limit = 0U;
            nbyte = (int64_t) buf->capacity;
        } else {
            // Write the the write_pos as is
        }
        if (0L != nbyte) {
            byte_t * data = &buf->byte_array[buf->limit];
            nbyte = datatap_read(tap, data, nbyte);
            if (nbyte > 0L) {
                buf->limit += (size_t) nbyte;
                if(buf->limit < buf->capacity) {
                    buf->byte_array[buf->limit] = '\0';
                } else {
                    ABQ_VITAL(buf->limit == buf->capacity);
                }
            } else {
                retval = abq_status_take(ECANCELED);
            }
        }
    }
    return retval;
}

err_t buf_push_to_sink(byte_buffer_t *buf, datasink_t *sink) {
    err_t retval = datasink_get_status(sink);
    if (EXIT_SUCCESS != retval) {
        // Keep error as is
    } else if((NULL == buf) || (NULL == sink)) {
        retval = EFAULT;
    } else if(buf->limit <= buf->position) {
        ABQ_VITAL(buf->position == buf->limit);
        // No more data to be pushed
        retval = ENODATA;
    } else {
#if !defined(NDEBUG)
        ABQ_VITAL(0 <= buf->position);
        ABQ_VITAL(buf->limit <= buf->capacity);
#endif /* !defined(NDEBUG) */
        int64_t amount = ((int64_t)buf->limit) - ((int64_t)buf->position);
        while (0L != amount) {
            byte_t *data = &buf->byte_array[buf->position];
            amount = datasink_write(sink, data, amount);
            if (0L >= amount) {
                retval = abq_status_take(EWOULDBLOCK);
                break;
            } else {
                buf->position += (size_t) amount;
                amount = ((int64_t)buf->limit) - ((int64_t)buf->position);
            }
        }
        if (EXIT_SUCCESS == retval) {
            // No data left in buffer, restart positions
            VITAL_IS_OK(buf_compact(buf));
        }
    }
    return retval;
}

static void buf_datawriter(datasink_t *sink, cvar_t cbdata) {
    if ((NULL != sink) && (NULL != cbdata)) {
        byte_buffer_t *buf = buf_resolve(cbdata);
        VITAL_NOT_NULL(buf);
        err_t status = buf_push_to_sink(buf, sink);
        while (EXIT_SUCCESS == status) {
            status = buf_push_to_sink(buf, sink);
        }
        if (CHECK_WOULDBLOCK(status)) {
            // Wait for datasink_t to become writable
        } else {
            VITAL_IS_OK(datasink_remove_listener(sink, buf_datawriter, cbdata));
            if (ENODATA != status) {
                DATASINK_ERROR(sink, status);
            }
        }
    }
}

err_t buf_setup_sinkwriter(const byte_buffer_t* buf, datasink_t *sink) {
    return datasink_add_datawriter(sink, buf_datawriter, buf);
}
