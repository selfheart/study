/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* system header */
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

/* user header */
#include "logger.h"
#include "tup_compress.h"
#include "tup_lz4.h"
#include "tup_parser.h"
#include "tup_zlib.h"

/* object macro */

/* function definition */

/* typedef definition */
typedef int (*decompress)(
  uint8_t *, uint64_t, const uint8_t *, uint64_t, const uint8_t *, uint64_t);

/* enum definitoin */

/* struct/union definition */
struct decompress_reg {
    uint64_t algorithm;
    decompress func;
};

/* static variable */
static struct decompress_reg decomp[] = { { TUP_LZ4_32, tup_lz4_decompress },
    { TUP_LZ4_64K, tup_lz4_decompress }, { TUP_ZLIB, tup_zlib_decompress } };

/* static function */

/* variable */

/* function */
bool
tup_check_compress_algorithm(uint64_t algorithm)
{
    for (size_t i = 0; i < sizeof(decomp) / sizeof(decomp[0]); ++i)
        if (decomp[i].algorithm == algorithm)
            return true;
    return false;
}

int
tup_do_decompress(uint64_t algorithm, uint8_t *decompress_buffer,
  uint64_t decompress_size, const uint8_t *compress_buffer,
  uint64_t compress_size, const uint8_t *attribution, uint64_t attribution_size)
{
    if (NULL == decompress_buffer) {
        LOG_ERROR("Invalid argument: decompress_buffer=%p", decompress_buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    if (NULL == compress_buffer) {
        LOG_ERROR("Invalid argument: compress_buffer=%p", compress_buffer);
        return TUP_PARSER_ERROR_INVALID_ARGUMENT;
    }

    /* attribution is optional */

    for (size_t i = 0; i < sizeof(decomp) / sizeof(decomp[0]); ++i) {
        if (decomp[i].algorithm == algorithm) {
            return decomp[i].func(decompress_buffer, decompress_size,
              compress_buffer, compress_size, attribution, attribution_size);
        }
    }
    LOG_ERROR("Nof found: algorithm=0x%016" PRIx64, algorithm);
    return TUP_PARSER_ERROR_INVALID_ALGORITHM;
}
