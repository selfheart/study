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
 * @file util/byte_buffer.h
 * @date Mar 20, 2017
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief modeled after java.nio.ByteBuffer\n
 *   except that it writes to and moves to 'limit' instead of 'position' when writing\n
 *   note that specific functionality from java.nio.ByteBuffer is imported on an as needed bases\n
 */

#ifndef SPIL_IO_BYTE_BUFFER_H
#define SPIL_IO_BYTE_BUFFER_H

/** typedef for byte_buffer_t */
typedef struct for_byte_buffer byte_buffer_t;

#include <ontrac/ontrac/abq_class.h>
#include <ontrac/unicode/utf8_utils.h>

/** class of byte_buffer_t instances */
extern const class_t byte_buffer_class;
extern const class_t list_of_byte_buffer_class;
/** the struct of a byte_buffer_t */
struct for_byte_buffer {
    /** A buffer's capacity is the number of bytes it contains. The capacity of a buffer is never negative and never changes. */
    size_t capacity;
    /** Used in abq_nio as the "Read" position (where to read next byte of data to),
     * A buffer's position is the index of the next byte to be read.
     * A buffer's position is never negative and is never greater than its limit. */
    size_t position;
    /** Used in abq_nio as the "Write" position (where to write the next byte of data to),
     * A buffer's limit is the index of the first byte that should not be read.
     * A buffer's limit is never negative and is never greater than its capacity */
    size_t limit;
    /** Unlike in Java, the mark here is used to mark any given thing that needs to be tracked in the current context
      from the amount to read to in the and xfer, to the location of a matching string after scanning witht buf_mark_* functions.
      A buffer's mark is typically the index to which its position will be reset when the reset method is invoked. */
    size_t mark;
    /** Flag used to indicate whether or not this byte_buffer_t owns the underlying byte-array and should free it when done */
    bool_t free_byte_array_on_delete;
    /** pointer to the underlying byte-array */
    byte_t *byte_array;
};
/**
 * buffer sizes that may be allocated with 'buf_allocate' function
 * @todo these sizes should correspond with file-system block-sizes or memory page-sizes
 */
typedef uint32_t buf_size_t;
    /** An Empty byte-buffer with zero capacity */
#define BUFFER_EMPTY (0U)
    /** smallest byte-buffer, often used for simple string manipulation */
#define BUFFER_SMALL ((buf_size_t)B128_SIZE)
    /** medium sized byte-buffer, often used for complex string manipulation */
#define BUFFER_MEDIUM ((buf_size_t)B512_SIZE)
    /** larget byte-buffer, often used for file & data I/O.  */
#define BUFFER_LARGE ((buf_size_t)B1024_SIZE)
    /** largest byte-buffer, often used for file & data I/O. !MUST be at lease 4096 to support PEM encoded certificates in the trusted-time response! */
#define BUFFER_X_LARGE ((buf_size_t)B8192_SIZE)

#define POINTER_IN_RANGE_OF_BYTE_BUFFER(pointer, byte_buffer) (((uintptr_t)pointer >= &byte_buffer->byte_array[0]) && ((uintptr_t)pointer <= &byte_buffer->byte_array[byte_buffer->capacity])

static inline bool_t buf_has_data(byte_buffer_t * buf) {
    return ((NULL == buf) || (NULL == buf->byte_array)) ? false
            : (bool_t) (buf->limit > buf->position);
}

static inline byte_t* buf_data(byte_buffer_t * buf){
    return ((NULL == buf) || (NULL == buf->byte_array)) ? NULL : &buf->byte_array[buf->position];
}

static inline abq_encoder_t BUF_ENCODER_INIT(abq_codec_t *codec, byte_buffer_t* buf){
    return (abq_encoder_t) {
        .codec=(NULL == codec) ? &raw_codec : codec,
        .dest=buf->byte_array,
        .pos=buf->limit,
        .max=buf->capacity
    };
}
#define BUF_ENCODER(name, coder, buf) abq_encoder_t name = BUF_ENCODER_INIT(coder, buf);

static inline abq_decoder_t BUF_DECODER_INIT(abq_codec_t *codec, byte_buffer_t* buf) {
    return (abq_decoder_t) {
        .codec=(NULL == codec) ? &raw_codec : codec,
        .source=buf->byte_array,
        .pos=buf->position,
        .max=buf->limit
    };
}
#define BUF_DECODER(name, coder, buf) abq_decoder_t name = BUF_DECODER_INIT(coder, buf);

/**
 * @brief wraps a byte array in a byte_buffer_t
 *
 * @param byte_array: the preallocated data to wrap
 * @param capacity: total number of bytes in the byte array
 * @param limit: number of bytes in the byte array with meaningful data
 * @param free_byte_array_on_delete: true if the byte_buffer_t should take exclusive ownership of the byte-array and free it when done
 * @return pointer to a byte_buffer_t wrapping the byte array, or NULL on failure
 */
extern byte_buffer_t *buf_wrap(byte_t *byte_array,
        size_t capacity, size_t limit,
        bool_t free_byte_array_on_delete);
/**
 * @brief allocate a byte_buffer_t using one of the defined buf_size_t options
 *
 * @param buf_size: size of the byte array to be allocated
 * @return pointer to a byte_buffer_t with a byte array of the given size, or NULL on failure
 */
extern byte_buffer_t *buf_allocate(buf_size_t buf_size);
/**
 * @brief resolves an instance of byte_buffer_class to the byte_buffer_t*
 *
 * @param pointer to item to be resolved
 * @return resolved byte_buffer_t* or NULL on failure
 */
extern byte_buffer_t *buf_resolve(cvar_t item);
/**
 * @brief upgrade buffer to replace the active byte_array with the one provided which will be overwritten
 *
 * @param buf: pointer to a byte_buffer_t to be upgraded
 * @param byte_array: byte array to use as the new backing
 * @param capacity: total number of bytes in the new byte array
 * @param free_byte_array_on_delete: is the byte_buffer_t given exclusive ownership of the byte array (and should free it when finished?)
 * @return 0 on success, else an error code
 */
extern err_t buf_upgrade(byte_buffer_t *buf, byte_t *byte_array, size_t capacity, bool_t free_byte_array_on_delete);
/**
 * @brief calculates the number of bytes available that can be read immediately
 * (buf->limit - buf->position)
 *
 * @param buf: pointer to a byte_buffer_t which may contain data to be read out
 * @return total number of bytes that can currently be read out of the byte_buffer_t
 */
extern size_t buf_remaining(const byte_buffer_t *buf);
/**
 * @brief inverse of buf_remaining, computed via buf->capacity - buf_remaining(buf)
 *
 * @param buf: pointer to a byte_buffer_t we wish to investigate
 * @return: total number of unused or free bytes within the byte array
 */
extern size_t buf_unused_bytes(const byte_buffer_t *buf);
/**
 * @brief moves data to beginning of the byte array
 * data between position and limit is written to the beginning of the byte_array
 *  then limit is set to remaining() and position to 0
 *
 * @param buf: pointer to a byte_buffer_t we wish to compact
 * @return 0 on success, else an error code
 */
extern err_t buf_compact(byte_buffer_t * buf);
/**
 * @brief write data to byte_buffer_t
 *  reads length bytes from source into the byte_buffer
 *   unlike Java in that it writes to and moves 'limit'
 *   instead of position, eliminating the need for flip()
 *   when switching between writes and reads
 *
 * @param buf: pointer to a byte_buffer_t we wish to write to
 * @param source: a byte array to be written to buffer
 * @param length: number of bytes in array to be written
 * @return 0 on success, else an error code
 */
extern err_t buf_write_bytes(byte_buffer_t *buf, cvar_t source, size_t length);
/**
 * @brief write a singular octet to the byte_buffer_t
 *
 * @param buf: pointer to a byte_buffer_t we wish to write to
 * @param octet: one byte of data to be written to the byte_buffer_t
 * @return 0 on success, else an error code
 */
extern err_t buf_write_octet(byte_buffer_t *buf, const byte_t octet);
/**
 * @brief writes a single codepoint to byte_buffer_t with UTF-8 encoding
 *
 * @param buf: pointer to a byte_buffer_t we wish to write to
 * @param codepoint: one character to be encoded onto the byte_buffer_t
 * @return 0 on success, else an error code
 */
extern err_t buf_write_char(byte_buffer_t *buf, const int32_t codepoint);
/**
 * @brief converts int to string value and writes it to byte_array, max radix of 16 for now
 *
 * @param buf: pointer to a byte_buffer_t we wish to write to
 * @param value: a integer value to be converted to string and written to byte_buffer_t
 * @param radix: integer base used for string to number conversion, traditionally 10
 * @return 0 on success, else an error code
 */
extern err_t buf_write_int(byte_buffer_t *buf, int64_t value, abq_radix_t radix);
/**
 * @brief converts a number_t to a string value and writes it to byte_array
 *
 * @param buf: pointer to a byte_buffer_t we wish to write to
 * @param value: a numeric value to be converted to string and written to byte_buffer_t (only supports base 10)
 * @return 0 on success, else an error code
 */
extern err_t buf_write_number(byte_buffer_t *buf, number_t value);
/**
 * @brief writes utf8 characters from str until a null terminator is reached or error occurs
 *
 * @param buf: pointer to a byte_buffer_t we wish to write to
 * @param source: a utf8 encoded string we wish to write into the byte_buffer_t
 * @return 0 on success, else an error code
 */
extern err_t buf_write_str(byte_buffer_t *buf, cstr_t source);
/**
 * @brief merge data from source buffer into a destination buffer
 *
 * @param dest: the consuming byte_buffer_t (dest is short for destination)
 * @param source: the providing byte_buffer_t
 * @param n: max number of bytes to transfer from first buffer to second, -1 for unlimited
 * @return  number of bytes written to dest, or -1 on error
 */
extern int32_t buf_write_buffer(byte_buffer_t *dest, byte_buffer_t *source, int32_t n);
/**
 * @brief marks the first instance of utf8 codepoint found between position and limit
 * if from_prior_mark is true, will continue the search from previously marked position (if mark is >= position)\n
 * otherwise it will start searching from current position and increment until 'codepoint' is found\n
 * returns 0 if 'codepoint' was found and mark was set, EAGAIN if it failed to find codepoint, or errno on other error\n
 *
 * @param buf: pointer to a byte_buffer_t in which we should search for a codepoint
 * @param codepoint: the character to scan for within the given byte_buffer_T
 * @param from_prior_mark: true if we wish to continue scan from a prior mark, false if we wish to scan all data
 * @return 0 if codepoint was found and marked, EAGAIN if codepoint wasn't found, else an error code
 */
extern err_t buf_mark_char(byte_buffer_t *buf, const int32_t codepoint, bool_t from_prior_mark);
/**
 * @brief marks the first instance of utf8 encoded string if found between position and limit
 *
 * @param buf: pointer to a byte_buffer_t in which we should search for a substring
 * @param substring: the utf8 character sequence to search for within the byte_buffer_t data
 * @param from_prior_mark: true if we wish to continue scan from a prior mark, false if we wish to scan all data
 * @return 0 if substring was found and marked, EAGAIN if substring wasn't found, else an error code
 */
extern err_t buf_mark_str(byte_buffer_t *buf, const cstr_t substring, bool_t from_prior_mark);
/**
 * @brief if mark >= to position and < limit, returns (1 + mark - position), otherwise -1
 *
 * @param buf: pointer to a byte_buffer_t with potentially marked data
 * @return (1 + mark - position) if mark >= to position and < limit, else -1
 */
extern int32_t buf_marked_length(const byte_buffer_t *buf);
/**
 * @brief reads bytes into dest up to amount, returns actual number of bytes read
 *
 * @param buf: pointer to a byte_buffer_t from which to read
 * @param dest: a byte array used to store data read from the byte_buffer_t
 * @param amount: maximum amount of data to read into dest
 * @return actual amount of data read from byte_buffer_t, or -1 on error
 */
extern int32_t buf_read_bytes(byte_buffer_t *buf, var_t dest, size_t amount);
/**
 * @brief attempts to read the next byte of data from the byte_buffer_t
 *
 * @param buf: pointer to a byte_buffer_t from which to read
 * @return an unsigned byte value of the byte read out, or -1 or error
 */
extern int32_t buf_read_octet(byte_buffer_t *buf);
/**
 * @brief reads out the next utf8 encoded character from the byte_buffer_t
 *
 * @param buf: pointer to a byte_buffer_t from which to read
 * @return a codepoint value of a character that was read out, or -1 or error
 */
extern int32_t buf_read_char(byte_buffer_t *buf);
/**
 * @brief reads data up to a previously marked position
 * @todo not yet implemented
 *
 * @param buf: pointer to a byte_buffer_t from which to read
 * @param dest: a byte array used to store data read from the byte_buffer_t
 * @param amount: maximum amount of data to read into dest
 * @return actual amount of data read from byte_buffer_t, or -1 on error
 */
extern int32_t buf_read_to_mark(byte_buffer_t *buf, byte_t *dest, size_t amount);
/**
 * @brief reads out a sequence of digit characters from byte_array into a integer
 *
 * @param buf: pointer to a byte_buffer_t from which to read
 * @param dest: a int64_t used to store the integer data that was read out
 * @param radix: number base used from representing the integer value, traditionally 10
 * @return 0 on success, else an error code
 */
extern err_t buf_read_int(byte_buffer_t *buf, int64_t *dest, abq_radix_t radix);
/**
 * @brief attempts to read out a decimal number from the beginning of the byte_buffer_t
 *
 * @param buf: pointer to a byte_buffer_t from which to read
 * @param dest: a number_t pointer used to store the result
 * @return 0 on success, else an error code
 */
extern err_t buf_read_number(byte_buffer_t *buf, number_ptr dest);
/**
 * @brief creates a cstr_t object from data at the beginning of the byte_buffer_t with a self-reference
 *
 * @param buf: pointer to a byte_buffer_t from which to read
 * @param str_ptr: a cstr_t pointer used to store the result
 * @param length: max number of bytes to read into the new string
 * @param trim: should the new string be trimmed on creation?
 * @return 0 on success, else an error code
 */
extern err_t buf_read_str(byte_buffer_t *buf, cstr_t *str_ptr, int32_t length, bool_t trim);

#include <ontrac/stream/datatap.h>
extern err_t buf_pull_on_tap(byte_buffer_t *buf, datatap_t *tap);

#include <ontrac/stream/datasink.h>
/**
 * @brief a one-time attempt to write current data into a datasink_t
 *
 * @param buf
 * @param sink
 * @return
 */
extern err_t buf_push_to_sink(byte_buffer_t *buf, datasink_t *sink);
/**
 * @brief setup a datawriter to continue to write data into sink until byte_buffer_t has been emptied
 *
 * @param buf
 * @param sink
 * @return
 */
extern err_t buf_setup_sinkwriter(const byte_buffer_t* buf, datasink_t *sink);

#endif //SPIL_IO_BYTE_BUFFER_H
