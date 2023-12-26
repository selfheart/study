/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* system header */
#include <zlib.h>

/* user header */
#include "logger.h"
#include "tup_parser_error_code.h"
#include "tup_zlib.h"

/* object macro */

/* function definition */

/* typedef definition */

/* enum definitoin */

/* struct/union definition */

/* static variable */

/* static function */

/* variable */

/* function */
int
tup_zlib_decompress(uint8_t *decompress_buffer, uint64_t decompress_size,
  const uint8_t *compress_buffer, uint64_t compress_size,
  const uint8_t *attribution, uint64_t attribution_size)
{
    (void)attribution;
    (void)attribution_size;

    int ret = uncompress(
      decompress_buffer, &decompress_size, compress_buffer, compress_size);
    if (Z_OK != ret) {
        LOG_ERROR("uncompress(): 0x%08x", ret);
    }
    return Z_OK != ret ? TUP_PARSER_ERROR_DECOMPRESS : 0;
}
