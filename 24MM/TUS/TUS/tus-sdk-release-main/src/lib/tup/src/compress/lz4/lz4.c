/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* system header */
#include <lz4.h>

/* user header */
#include "logger.h"
#include "tup_parser_error_code.h"
#include "tup_lz4.h"

/* object macro */

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */

/* static variable */

/* static function */

/* variable */

/* function */
int
tup_lz4_decompress(uint8_t *decompress_buffer, uint64_t decompress_size,
  const uint8_t *compress_buffer, uint64_t compress_size,
  const uint8_t *attribution, uint64_t attribution_size)
{
    (void)attribution;
    (void)attribution_size;

    int ret = LZ4_decompress_safe((const char *)compress_buffer,
      (char *)decompress_buffer, compress_size, decompress_size);
    if (0 > ret) {
        LOG_ERROR("LZ4_decompress_safe(): 0x%08x", ret);
    }
    return ret < 0 ? TUP_PARSER_ERROR_DECOMPRESS : 0;
}
