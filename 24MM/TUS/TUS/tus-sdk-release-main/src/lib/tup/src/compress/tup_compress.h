/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
#ifndef TUP_COMPRESS_H
#define TUP_COMPRESS_H

/* system header */
#include <stdbool.h>
#include <stdint.h>

/* user header */

/* object macro */

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */

/* extern variable */

/* function prototype */
bool tup_check_compress_algorithm(uint64_t);
int tup_do_decompress(uint64_t algorithm, uint8_t *decompress_buffer,
  uint64_t decompress_size, const uint8_t *compress_buffer,
  uint64_t compress_size, const uint8_t *attribution,
  uint64_t attribution_size);

/* inline function */

#endif /* !TUP_COMPRESS_H */
