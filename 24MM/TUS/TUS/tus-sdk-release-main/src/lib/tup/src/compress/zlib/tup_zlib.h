/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
#ifndef TUP_ZLIB_H
#define TUP_ZLIB_H

/* system header */
#include <stdint.h>

/* user header */

/* object macro */
#define TUP_ZLIB (0x42494c5a00100100UL)

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */

/* extern variable */

/* function prototype */
int tup_zlib_decompress(uint8_t *decompress_buffer, uint64_t decompress_size,
  const uint8_t *compress, uint64_t compress_size, const uint8_t *attribution,
  uint64_t attribution_size);

/* inline function */

#endif
